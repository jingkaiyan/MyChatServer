#!/usr/bin/env python3
import os
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

METRICS_FILE = os.environ.get("MYCHAT_METRICS_FILE", "./exports/runtime_metrics.prom")
REFRESH_INTERVAL = int(os.environ.get("MYCHAT_METRICS_REFRESH_SECONDS", "5"))
LISTEN_HOST = os.environ.get("MYCHAT_METRICS_HOST", "0.0.0.0")
LISTEN_PORT = int(os.environ.get("MYCHAT_METRICS_PORT", "9108"))
SNAPSHOT_SCRIPT = os.environ.get("MYCHAT_METRICS_SNAPSHOT", "./scripts/metrics_snapshot.sh")


def refresh_loop() -> None:
    while True:
        try:
            subprocess.run([SNAPSHOT_SCRIPT, METRICS_FILE], check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except Exception:
            pass
        time.sleep(REFRESH_INTERVAL)


class MetricsHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path not in ["/metrics", "/metrics/"]:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"not found\n")
            return

        if not os.path.exists(METRICS_FILE):
            self.send_response(503)
            self.end_headers()
            self.wfile.write(b"metrics file not ready\n")
            return

        with open(METRICS_FILE, "rb") as f:
            data = f.read()

        self.send_response(200)
        self.send_header("Content-Type", "text/plain; version=0.0.4; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, format, *args):
        return


def main() -> None:
    os.makedirs(os.path.dirname(METRICS_FILE) or ".", exist_ok=True)
    threading.Thread(target=refresh_loop, daemon=True).start()
    server = HTTPServer((LISTEN_HOST, LISTEN_PORT), MetricsHandler)
    print(f"metrics exporter listening on http://{LISTEN_HOST}:{LISTEN_PORT}/metrics")
    server.serve_forever()


if __name__ == "__main__":
    main()
