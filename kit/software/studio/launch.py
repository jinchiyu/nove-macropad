#!/usr/bin/env python3
"""Local static server; no serial bridge, no account, no external bindings."""
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
import sys, subprocess, time, urllib.request, json

ROOT = Path(__file__).resolve().parent
PORT = 9939


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=str(ROOT / "dist"), **kw)

    def do_GET(self):
        if self.path == "/__99bit_health":
            body = b'{"app":"99bit-studio"}'
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(body)
        else:
            super().do_GET()

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        super().end_headers()


def healthy():
    try:
        with urllib.request.urlopen(
            f"http://127.0.0.1:{PORT}/__99bit_health", timeout=1
        ) as r:
            return json.load(r).get("app") == "99bit-studio"
    except Exception:
        return False


if "--serve" in sys.argv:
    ThreadingHTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
else:
    if not (ROOT / "dist/index.html").exists():
        raise SystemExit(
            "Build mancante. Eseguire npm install e npm run build nella cartella studio."
        )
    if not healthy():
        log = open(ROOT / "server.log", "a")
        child = subprocess.Popen(
            [sys.executable, str(Path(__file__).resolve()), "--serve"],
            stdout=log,
            stderr=log,
            start_new_session=True,
        )
        for _ in range(30):
            if healthy():
                break
            if child.poll() is not None:
                raise SystemExit(
                    "Porta 9939 occupata o avvio fallito. Vedere studio/server.log."
                )
            time.sleep(0.1)
    if not healthy():
        raise SystemExit("Server locale non disponibile.")
    subprocess.run(
        ["open", "-a", "Google Chrome", f"http://127.0.0.1:{PORT}"], check=True
    )
