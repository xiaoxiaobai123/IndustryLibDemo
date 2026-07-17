#!/usr/bin/env python3
"""Web UI 桥接：核心 TCP  <->  浏览器 HTTP+SSE。只用标准库，不装任何依赖。

  GET  /            -> index.html
  GET  /events      -> SSE，把核心推的每行 JSON 转成一条 event
  POST /cmd         -> body 为 start/stop/status，转发给核心

浏览器打开 http://127.0.0.1:8080
"""
import json
import os
import queue
import socket
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

CORE = ("127.0.0.1", 9500)
WEB_PORT = 8080
HERE = os.path.dirname(os.path.abspath(__file__))

_subs = []            # 每个浏览器一个队列
_subs_lock = threading.Lock()
_sock = None          # 与核心的连接


def _fanout(msg):
    with _subs_lock:
        for q in list(_subs):
            q.put(msg)


def core_reader():
    """维持与核心的长连接；断了就重连——核心重启不影响页面。"""
    global _sock
    while True:
        try:
            _sock = socket.create_connection(CORE, timeout=3)
            _fanout(json.dumps({"type": "link", "connected": True}))
            buf = b""
            while True:
                chunk = _sock.recv(4096)
                if not chunk:
                    break
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    line = line.decode("utf-8", "replace").strip()
                    if line:
                        _fanout(line)
        except OSError:
            pass
        _fanout(json.dumps({"type": "link", "connected": False}))
        if _sock:
            _sock.close()
            _sock = None
        time.sleep(1)


def send_core(cmd):
    if _sock is None:
        return False
    try:
        _sock.sendall((cmd + "\n").encode())
        return True
    except OSError:
        return False


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *_):
        pass  # 静音访问日志

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            self._serve_index()
        elif self.path == "/events":
            self._serve_sse()
        else:
            self.send_error(404)

    def do_POST(self):
        if self.path != "/cmd":
            self.send_error(404)
            return
        n = int(self.headers.get("Content-Length", 0))
        cmd = self.rfile.read(n).decode().strip()
        ok = cmd in ("start", "stop", "status") and send_core(cmd)
        body = json.dumps({"sent": ok}).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _serve_index(self):
        with open(os.path.join(HERE, "index.html"), "rb") as f:
            body = f.read()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _serve_sse(self):
        q = queue.Queue()
        with _subs_lock:
            _subs.append(q)
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Connection", "keep-alive")
        self.end_headers()
        try:
            self.wfile.write(b": connected\n\n")
            self.wfile.flush()
            while True:
                try:
                    msg = q.get(timeout=15)
                    self.wfile.write(("data: %s\n\n" % msg).encode())
                except queue.Empty:
                    self.wfile.write(b": ping\n\n")  # 保活
                self.wfile.flush()
        except OSError:
            pass
        finally:
            with _subs_lock:
                if q in _subs:
                    _subs.remove(q)


if __name__ == "__main__":
    threading.Thread(target=core_reader, daemon=True).start()
    print("bridge: core %s:%d  ->  http://127.0.0.1:%d" % (CORE[0], CORE[1], WEB_PORT))
    ThreadingHTTPServer(("127.0.0.1", WEB_PORT), Handler).serve_forever()
