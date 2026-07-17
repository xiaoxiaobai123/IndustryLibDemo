#!/usr/bin/env python3
"""tkinter UI —— 只用标准库。UI 是可替换的外壳，核心不依赖它。

协议：TCP 127.0.0.1:9500，每行一个 JSON。
发送 start / stop / status（纯文本行），接收 result / ack / status。
"""
import json
import queue
import socket
import threading
import tkinter as tk
from tkinter import ttk

HOST, PORT = "127.0.0.1", 9500

BG, PANEL = "#14171c", "#1d2129"
OK, NG = "#3ddc84", "#ff5252"
FG, MUTED = "#e6e9ef", "#8b94a3"


class CoreLink:
    """后台线程收行；UI 线程从队列取，不阻塞界面。"""

    def __init__(self, on_line, on_state):
        self.on_line, self.on_state = on_line, on_state
        self.sock = None
        threading.Thread(target=self._loop, daemon=True).start()

    def _loop(self):
        import time

        while True:
            try:
                self.sock = socket.create_connection((HOST, PORT), timeout=3)
                # 超时只该管"连"，不该管"收"：核心 stop 之后本来就没数据，
                # 留着 timeout=3 会让 recv 每 3 秒抛超时 -> 假掉线 -> 重连打转
                self.sock.settimeout(None)
                self.on_state(True)
                buf = b""
                while True:
                    chunk = self.sock.recv(4096)
                    if not chunk:
                        break
                    buf += chunk
                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        self.on_line(line.decode("utf-8", "replace").strip())
            except OSError:
                pass
            self.on_state(False)
            if self.sock:
                self.sock.close()
                self.sock = None
            time.sleep(1)  # 断线自动重连

    def send(self, cmd):
        if self.sock:
            try:
                self.sock.sendall((cmd + "\n").encode())
            except OSError:
                pass


class App:
    def __init__(self, root):
        self.q = queue.Queue()
        self.ok_n = self.ng_n = 0
        root.title("IndustryLibDemo - tkinter UI")
        root.configure(bg=BG)
        root.geometry("760x560")
        self._build(root)
        self.link = CoreLink(self.q.put, lambda up: self.q.put(("conn", up)))
        root.after(100, self._pump)

    def _build(self, root):
        top = tk.Frame(root, bg=PANEL, padx=14, pady=10)
        top.pack(fill="x", padx=12, pady=(12, 6))
        self.conn = tk.Label(top, text="● 未连接", bg=PANEL, fg=NG,
                             font=("Segoe UI", 11, "bold"))
        self.conn.pack(side="left")
        for text, cmd in (("启动", "start"), ("停止", "stop"), ("状态", "status")):
            tk.Button(top, text=text, width=7, bd=0, bg="#2a303c", fg=FG,
                      activebackground="#39414f", activeforeground=FG,
                      command=lambda c=cmd: self.link.send(c)).pack(side="right", padx=4)

        mid = tk.Frame(root, bg=BG)
        mid.pack(fill="x", padx=12, pady=6)

        lamp = tk.Frame(mid, bg=PANEL, width=260, height=150)
        lamp.pack(side="left", fill="both", expand=True, padx=(0, 6))
        lamp.pack_propagate(False)
        self.lamp = tk.Label(lamp, text="--", bg=PANEL, fg=MUTED,
                             font=("Segoe UI", 62, "bold"))
        self.lamp.pack(expand=True)

        stats = tk.Frame(mid, bg=BG)
        stats.pack(side="left", fill="both", expand=True)
        self.ok_lbl = self._stat(stats, "OK 计数", OK)
        self.ng_lbl = self._stat(stats, "NG 计数", NG)

        table = tk.Frame(root, bg=PANEL)
        table.pack(fill="both", expand=True, padx=12, pady=(6, 12))
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("D.Treeview", background=PANEL, fieldbackground=PANEL,
                        foreground=FG, rowheight=24, borderwidth=0)
        style.configure("D.Treeview.Heading", background="#2a303c", foreground=MUTED,
                        borderwidth=0)
        cols = ("frame", "flow", "metric", "verdict")
        self.tree = ttk.Treeview(table, columns=cols, show="headings", style="D.Treeview")
        for c, w in zip(cols, (80, 130, 80, 90)):
            self.tree.heading(c, text={"frame": "帧号", "flow": "流程",
                                       "metric": "主指标", "verdict": "判定"}[c])
            self.tree.column(c, width=w, anchor="center")
        self.tree.tag_configure("OK", foreground=OK)
        self.tree.tag_configure("NG", foreground=NG)
        self.tree.pack(fill="both", expand=True, padx=6, pady=6)

    def _stat(self, parent, title, color):
        box = tk.Frame(parent, bg=PANEL, height=71)
        box.pack(fill="both", expand=True, pady=(0, 4))
        tk.Label(box, text=title, bg=PANEL, fg=MUTED,
                 font=("Segoe UI", 9)).pack(anchor="w", padx=12, pady=(8, 0))
        lbl = tk.Label(box, text="0", bg=PANEL, fg=color, font=("Segoe UI", 22, "bold"))
        lbl.pack(anchor="w", padx=12)
        return lbl

    def _pump(self):
        while not self.q.empty():
            item = self.q.get()
            if isinstance(item, tuple):
                up = item[1]
                self.conn.config(text="● 已连接 127.0.0.1:9500" if up else "● 未连接",
                                 fg=OK if up else NG)
            else:
                self._on_json(item)
        self.tree.after(100, self._pump)

    def _on_json(self, line):
        try:
            m = json.loads(line)
        except ValueError:
            return
        if m.get("type") != "result":
            return
        v = m["verdict"]
        self.lamp.config(text=v, fg=OK if v == "OK" else NG)
        if v == "OK":
            self.ok_n += 1
            self.ok_lbl.config(text=str(self.ok_n))
        else:
            self.ng_n += 1
            self.ng_lbl.config(text=str(self.ng_n))
        # 各项目主指标不同：表面检测看 score，角度测量看 angle
        metric = m["score"] if "score" in m else "%d°" % m["angle"]
        self.tree.insert("", 0, values=(m["frame"], m["flow"], metric, v), tags=(v,))
        kids = self.tree.get_children()
        if len(kids) > 200:
            self.tree.delete(*kids[200:])


if __name__ == "__main__":
    root = tk.Tk()
    App(root)
    root.mainloop()
