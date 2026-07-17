#!/usr/bin/env python3
"""临时验证脚本：启动 core，用 TCP 验证 start/stop/status 是否有效。

用法: python tools/verify_tcp.py <core_exe_path> <expected_build_tag> <expected_flow_count>
退出码 0 = 全部通过。
"""
import json
import os
import socket
import subprocess
import sys
import time

exe, want_tag, want_flows = sys.argv[1], sys.argv[2], int(sys.argv[3])
if not os.path.exists(exe) and os.path.exists(exe + ".exe"):
    exe += ".exe"
fails = []


def check(name, cond, detail=""):
    print(("  PASS  " if cond else "  FAIL  ") + name + (("  <- " + detail) if detail else ""))
    if not cond:
        fails.append(name)


class Link:
    def __init__(self, sock):
        self.s, self.buf = sock, b""

    def lines(self, seconds):
        """收集 seconds 秒内到达的所有 JSON 行"""
        out, end = [], time.time() + seconds
        while time.time() < end:
            self.s.settimeout(max(0.05, end - time.time()))
            try:
                chunk = self.s.recv(4096)
            except socket.timeout:
                break
            if not chunk:
                break
            self.buf += chunk
            while b"\n" in self.buf:
                line, self.buf = self.buf.split(b"\n", 1)
                line = line.decode().strip()
                if line:
                    try:
                        out.append(json.loads(line))
                    except ValueError:
                        fails.append("invalid JSON: " + line)
        return out

    def cmd(self, c, wait=1.0):
        self.s.sendall((c + "\n").encode())
        return self.lines(wait)


print("=== 启动 core: %s ===" % exe)
proc = subprocess.Popen([exe], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
check("core 进程存活", proc.poll() is None)

link = Link(socket.create_connection(("127.0.0.1", 9500), timeout=5))

# 1) 默认自动跑：应收到结果流
msgs = link.lines(1.5)
results = [m for m in msgs if m.get("type") == "result"]
check("独立运行时持续产生结果", len(results) >= 2, "收到 %d 条" % len(results))
flows_seen = {m["flow"] for m in results}
check("编入的流程数 = %d" % want_flows, len(flows_seen) == want_flows, str(sorted(flows_seen)))
check("结果字段完整", all({"frame", "score", "verdict"} <= m.keys() for m in results))
check("verdict 只有 OK/NG", all(m["verdict"] in ("OK", "NG") for m in results))

# 2) status
st = [m for m in link.cmd("status") if m.get("type") == "status"]
check("status 有回复", len(st) == 1)
if st:
    check("status.build = %s" % want_tag, st[0]["build"] == want_tag, str(st[0].get("build")))
    check("status.flow_count = %d" % want_flows, st[0]["flow_count"] == want_flows)
    check("status.running = true", st[0]["running"] is True)

# 3) stop -> 结果流必须停
link.cmd("stop")
after = [m for m in link.lines(1.5) if m.get("type") == "result"]
check("stop 后不再产生结果", len(after) == 0, "仍收到 %d 条" % len(after))
st = [m for m in link.cmd("status") if m.get("type") == "status"]
check("stop 后 status.running = false", st and st[0]["running"] is False)

# 4) start -> 结果流恢复
link.cmd("start")
again = [m for m in link.lines(1.5) if m.get("type") == "result"]
check("start 后结果恢复", len(again) >= 1, "收到 %d 条" % len(again))

# 5) UI 断开不影响核心
link.s.close()
time.sleep(1.0)
check("UI 断开后 core 仍存活", proc.poll() is None)
link2 = Link(socket.create_connection(("127.0.0.1", 9500), timeout=5))
check("UI 可重新接入并继续收数据",
      len([m for m in link2.lines(1.5) if m.get("type") == "result"]) >= 1)
link2.s.close()

proc.terminate()
try:
    proc.wait(timeout=5)
except subprocess.TimeoutExpired:
    proc.kill()

print("\n=== %s ===" % ("全部通过" if not fails else "失败: " + ", ".join(fails)))
sys.exit(1 if fails else 0)
