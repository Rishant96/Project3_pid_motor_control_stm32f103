#!/usr/bin/env python3
"""
P3 PID Motor Control — DataStar SSE Server.

Reads CSV telemetry from Blue Pill over CP2102 UART,
serves DataStar-compatible SSE events on http://localhost:8080.
Supports bidirectional communication (dashboard -> firmware via /cmd).

Usage:
    python3 server.py
    python3 server.py --serial /dev/ttyUSB0 --baud 115200 --port 8080
    python3 server.py --serial /dev/pts/5   (for fake_serial.py testing)
"""

import json
import sys
import threading
import time
import argparse
from http.server import HTTPServer, BaseHTTPRequestHandler
from socketserver import ThreadingMixIn
from pathlib import Path

try:
    import serial
except ImportError:
    print("Install pyserial:")
    print("  pip install pyserial --break-system-packages")
    sys.exit(1)

# ── Shared state ─────────────────────────────────────────────────────────
latest = {
    "tick": "0",
    "setpoint": "0",
    "actual": "0",
    "error": "0",
    "output": "0",
    "edges": "0",
    "rpm": "0",
    "kp": "5",
    "ki": "1",
    "kd": "0",
    "connected": "0",
}

clients = []
clients_lock = threading.Lock()
serial_port_ref = None


# ── SSE Broadcast ────────────────────────────────────────────────────────

def broadcast_signals(data):
    clean = {k: v for k, v in data.items() if not k.startswith("_")}
    payload = json.dumps(clean)
    event = f"event: datastar-patch-signals\ndata: signals {payload}\n\n"
    _send_to_clients(event)


def broadcast_element(html):
    lines = html.strip().split("\n")
    event = "event: datastar-patch-elements\n"
    for line in lines:
        event += f"data: elements {line}\n"
    event += "\n"
    _send_to_clients(event)


def _send_to_clients(event):
    dead = []
    encoded = event.encode()
    with clients_lock:
        for wfile in clients:
            try:
                wfile.write(encoded)
                wfile.flush()
            except (BrokenPipeError, OSError):
                dead.append(wfile)
        for d in dead:
            clients.remove(d)


# ── UART Reader Thread ──────────────────────────────────────────────────

def uart_reader(port, baud):
    global serial_port_ref
    while True:
        try:
            ser = serial.Serial(port, baud, timeout=1)
            serial_port_ref = ser
            print(f"[UART] Connected: {port} @ {baud}")
            latest["connected"] = "1"
            broadcast_signals(latest)

            while True:
                raw = ser.readline().decode("ascii", errors="ignore").strip()
                if not raw:
                    continue

                parts = raw.split(",")

                if len(parts) < 2:
                    broadcast_element(
                        f'<div id="status-msg" class="status-flash">{raw}</div>'
                    )
                    continue

                prefix = parts[0]

                # Prefixed CSV: CSV,tick,setpoint,error,duty,edges,rpm
                if prefix == "CSV" and len(parts) >= 7:
                    telemetry = {
                        "tick": parts[1],
                        "setpoint": parts[2],
                        "error": parts[3],
                        "output": parts[4],
                        "edges": parts[5],
                        "rpm": parts[6],
                    }
                    latest.update(telemetry)
                    broadcast_signals(telemetry)

                # Raw CSV (no prefix): tick,setpoint,error,duty,edges,rpm
                elif len(parts) == 6:
                    try:
                        [int(p) for p in parts]
                        telemetry = {
                            "tick": parts[0],
                            "setpoint": parts[1],
                            "error": parts[2],
                            "output": parts[3],
                            "edges": parts[4],
                            "rpm": parts[5],
                        }
                        latest.update(telemetry)
                        broadcast_signals(telemetry)
                    except ValueError:
                        pass

                # INSPECT response: INS,Kp,5
                elif prefix == "INS" and len(parts) >= 3:
                    latest[parts[1].lower()] = parts[2]
                    broadcast_signals(latest)

                # Gain dump (raw INSPECT): "5,1,0"
                elif len(parts) == 3:
                    try:
                        vals = [int(p) for p in parts]
                        latest["kp"] = str(vals[0])
                        latest["ki"] = str(vals[1])
                        latest["kd"] = str(vals[2])
                        broadcast_signals(latest)
                    except ValueError:
                        pass

        except serial.SerialException as e:
            print(f"[UART] Error: {e} — retrying in 2s...")
            latest["connected"] = "0"
            broadcast_signals(latest)
            serial_port_ref = None
            time.sleep(2)
        except Exception as e:
            print(f"[UART] Unexpected: {e}")
            time.sleep(1)


# ── HTTP Handler ─────────────────────────────────────────────────────────

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True

class Handler(BaseHTTPRequestHandler):

    def do_GET(self):
        if self.path.startswith("/events"):
            self._handle_sse()
        elif self.path == "/" or self.path == "/index.html":
            self._serve_file("index.html", "text/html")
        else:
            self.send_error(404)

    def do_POST(self):
        if self.path == "/cmd":
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length).decode()
            try:
                signals = json.loads(body)
                cmd = signals.get("cmd", "").strip()
                if cmd and serial_port_ref:
                    serial_port_ref.write((cmd + "\r\n").encode())
                    print(f"[CMD] -> {cmd}")

                    # Update latest so gains persist correctly
                    parts = cmd.split()
                    if len(parts) == 2 and parts[0] in ("KP", "KI", "KD"):
                        latest[parts[0].lower()] = parts[1]

                elif not serial_port_ref:
                    print("[CMD] No serial connection")
            except Exception as e:
                print(f"[CMD] Error: {e}")

            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.end_headers()
            self.wfile.write(
                b"event: datastar-patch-signals\ndata: signals {}\n\n"
            )
        else:
            self.send_error(404)

    def _handle_sse(self):
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Connection", "keep-alive")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()

        clean = {k: v for k, v in latest.items() if not k.startswith("_")}
        payload = json.dumps(clean)
        initial = f"event: datastar-patch-signals\ndata: signals {payload}\n\n"
        try:
            self.wfile.write(initial.encode())
            self.wfile.flush()
        except (BrokenPipeError, OSError):
            return

        with clients_lock:
            clients.append(self.wfile)

        try:
            while True:
                time.sleep(1)
        except (BrokenPipeError, OSError):
            pass
        finally:
            with clients_lock:
                if self.wfile in clients:
                    clients.remove(self.wfile)

    def _serve_file(self, filename, content_type):
        path = Path(__file__).parent / filename
        if not path.exists():
            self.send_error(404, f"{filename} not found next to server.py")
            return
        content = path.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", len(content))
        self.end_headers()
        self.wfile.write(content)

    def log_message(self, fmt, *args):
        pass


def main():
    parser = argparse.ArgumentParser(
        description="P3 PID Motor Control — DataStar SSE Server"
    )
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--serial", default="/dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    t = threading.Thread(
        target=uart_reader, args=(args.serial, args.baud), daemon=True
    )
    t.start()

    httpd = ThreadedHTTPServer(("0.0.0.0", args.port), Handler)
    print(f"[HTTP] Dashboard: http://localhost:{args.port}")
    print(f"[UART] Reading from {args.serial} @ {args.baud}")
    print(f"[INFO] Ctrl+C to stop\n")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[INFO] Shutting down.")
        httpd.server_close()


if __name__ == "__main__":
    main()
