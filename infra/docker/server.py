"""Local HTTP server wrapping the Lambda handler with Tesseract."""

import os
import subprocess
import tempfile
from http.server import HTTPServer, BaseHTTPRequestHandler

os.environ.setdefault("TESSDATA_PREFIX", "/usr/share/tesseract-ocr/5/tessdata")
os.environ.setdefault("LAMBDA_TASK_ROOT", "/app")

import handler


def _local_ocr(png_bytes: bytes) -> str:
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as f:
        f.write(png_bytes)
        path = f.name
    try:
        r = subprocess.run(
            ["tesseract", path, "stdout", "-l", "pol"],
            capture_output=True, text=True, timeout=60,
        )
        if r.returncode != 0:
            raise RuntimeError(f"tesseract: {r.stderr[:100]}")
        return r.stdout
    finally:
        os.unlink(path)


handler._ocr = _local_ocr


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0))).decode()
        result = handler.lambda_handler({
            "httpMethod": "POST",
            "path": self.path,
            "body": body,
            "headers": {"content-type": self.headers.get("Content-Type", "")},
        }, None)
        self._send(result)

    def do_GET(self):
        result = handler.lambda_handler({
            "httpMethod": "GET",
            "path": self.path,
        }, None)
        self._send(result)

    def _send(self, result):
        self.send_response(result["statusCode"])
        for k, v in result["headers"].items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(result["body"].encode())

    def log_message(self, fmt, *args):
        print(f"{fmt % args}")


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5674))
    print(f"WasteWatch OCR on :{port}")
    HTTPServer(("0.0.0.0", port), Handler).serve_forever()
