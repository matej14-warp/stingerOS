#!/usr/bin/env python3
"""
scorpion OS CDN Backend
Serves packages from ./fetch3/<package_name>/ as .zip files
Runs on port 5000

Usage:
    python3 server.py

Package layout:
    fetch3/
      hello-world/
        main.c
        setup-runtime.mbf

To serve a package, create the folder and files, then
the server will automatically zip on request.
"""

import os
import io
import zipfile
import mimetypes
import json
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from datetime import datetime

BASE_DIR    = Path(__file__).parent
PACKAGE_DIR = BASE_DIR / "fetch3"
PORT        = 5000

# Ensure fetch3/ exists
PACKAGE_DIR.mkdir(exist_ok=True)


def log(level: str, msg: str):
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{ts}] [{level}] {msg}")


def build_zip(package_path: Path) -> bytes:
    """Build an in-memory ZIP of all files under package_path."""
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for root, dirs, files in os.walk(package_path):
            # Skip hidden dirs
            dirs[:] = [d for d in dirs if not d.startswith(".")]
            for fname in files:
                if fname.startswith("."):
                    continue
                fpath = Path(root) / fname
                # Archive name relative to package root
                arcname = fpath.relative_to(package_path)
                zf.write(fpath, arcname)
    buf.seek(0)
    return buf.read()


class ScorpionCDNHandler(BaseHTTPRequestHandler):

    def log_message(self, fmt, *args):
        # Suppress default access log (we use our own)
        pass

    def send_json(self, code: int, obj: dict):
        body = json.dumps(obj, indent=2).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_error_msg(self, code: int, msg: str):
        log("WARN", f"{self.client_address[0]} -> {code} {msg}")
        self.send_json(code, {"error": msg, "code": code})

    def do_GET(self):
        path = self.path.rstrip("/")

        # ── GET /                → server info
        if path == "" or path == "/":
            self.send_json(200, {
                "server":  "scorpion CDN",
                "version": "1.0",
                "packages": self._list_packages(),
                "usage":   "GET /fetch3/<package_name>"
            })
            return

        # ── GET /fetch3          → list packages
        if path == "/fetch3":
            pkgs = self._list_packages()
            self.send_json(200, {
                "packages": pkgs,
                "count":    len(pkgs)
            })
            return

        # ── GET /fetch3/<name>   → download package as .zip
        if path.startswith("/fetch3/"):
            parts = path.split("/")
            # /fetch3/<name> → parts = ['', 'fetch3', '<name>']
            if len(parts) < 3 or not parts[2]:
                self.send_error_msg(400, "Missing package name")
                return

            pkg_name = parts[2]

            # Basic sanitization
            if ".." in pkg_name or "/" in pkg_name or "\\" in pkg_name:
                self.send_error_msg(400, "Invalid package name")
                return

            pkg_path = PACKAGE_DIR / pkg_name
            if not pkg_path.exists() or not pkg_path.is_dir():
                self.send_error_msg(404, f"Package '{pkg_name}' not found")
                log("WARN", f"Package not found: {pkg_name}")
                return

            # Check package has at least one file
            files = list(pkg_path.rglob("*"))
            if not any(f.is_file() for f in files):
                self.send_error_msg(404, f"Package '{pkg_name}' is empty")
                return

            # Warn if no setup-runtime.mbf
            has_setup = (pkg_path / "setup-runtime.mbf").exists()
            if not has_setup:
                log("WARN", f"Package '{pkg_name}' has no setup-runtime.mbf")

            log("INFO", f"{self.client_address[0]} downloading package: {pkg_name}")

            try:
                zip_data = build_zip(pkg_path)
            except Exception as e:
                self.send_error_msg(500, f"Failed to build ZIP: {e}")
                return

            self.send_response(200)
            self.send_header("Content-Type", "application/zip")
            self.send_header("Content-Disposition",
                             f'attachment; filename="{pkg_name}.zip"')
            self.send_header("Content-Length", str(len(zip_data)))
            self.send_header("X-Package-Name", pkg_name)
            self.send_header("X-Has-Setup", str(has_setup).lower())
            self.end_headers()
            self.wfile.write(zip_data)

            log("INFO", f"Served {pkg_name}.zip ({len(zip_data)} bytes)")
            return

        # ── Fallback
        self.send_error_msg(404, f"Not found: {self.path}")

    def _list_packages(self) -> list:
        pkgs = []
        if not PACKAGE_DIR.exists():
            return pkgs
        for entry in sorted(PACKAGE_DIR.iterdir()):
            if entry.is_dir() and not entry.name.startswith("."):
                files    = [f.name for f in entry.rglob("*") if f.is_file()]
                has_mbf  = (entry / "setup-runtime.mbf").exists()
                pkgs.append({
                    "name":      entry.name,
                    "files":     files,
                    "has_setup": has_mbf,
                    "url":       f"/fetch3/{entry.name}"
                })
        return pkgs


def main():
    # Create example package if fetch3/ is empty
    hello_dir = PACKAGE_DIR / "hello-world"
    hello_dir.mkdir(exist_ok=True)

    main_c = hello_dir / "main.c"
    if not main_c.exists():
        main_c.write_text(
            '/* hello-world module for scorpion OS */\n'
            '#include <stdio.h>\n'
            'int main(void) {\n'
            '    /* This C file would be compiled and linked\n'
            '       by a future scorpion runtime compiler */\n'
            '    return 0;\n'
            '}\n'
        )

    setup_mbf = hello_dir / "setup-runtime.mbf"
    if not setup_mbf.exists():
        setup_mbf.write_text(
            '# hello-world setup-runtime.mbf\n'
            '# Runs when user confirms installation via fetch\n'
            '\n'
            'echo Installing hello-world module...\n'
            'mkdir /modules/hello-world\n'
            'copy main.c /modules/hello-world\n'
            '\n'
            'set version 1.0\n'
            'if version == 1.0 {\n'
            '    echo hello-world v1.0 installed successfully!\n'
            '} else {\n'
            '    echo Unknown version, installed anyway.\n'
            '}\n'
            '\n'
            'echo Done. Run: cat /modules/hello-world/main.c\n'
        )

    log("INFO", f"scorpion CDN starting on port {PORT}")
    log("INFO", f"Serving packages from: {PACKAGE_DIR}")
    log("INFO", f"Packages available: {[d.name for d in PACKAGE_DIR.iterdir() if d.is_dir()]}")
    log("INFO", f"http://localhost:{PORT}/fetch3/<package>")

    server = HTTPServer(("0.0.0.0", PORT), ScorpionCDNHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        log("INFO", "Server stopped")


if __name__ == "__main__":
    main()
