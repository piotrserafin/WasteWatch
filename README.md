# WasteWatch

Pebble smartwatch app that displays your waste collection schedule from [kiedywywoz.pl](https://kiedywywoz.pl).

## How It Works

```
┌─────────────┐      POST /schedule        ┌─────────────────┐    POST harmo_img     ┌───────────────┐
│  Pebble JS  │ ───────────────────────→   │  AWS Lambda     │ ──────────────────→   │ kiedywywoz.pl │
│  (phone)    │ ←───────────────────────   │  (Tesseract)    │ ←──────────────────   │  (290KB PNG)  │
└──────┬──────┘       ~1KB JSON            └─────────────────┘                       └───────────────┘
       │ AppMessage
┌──────┴──────┐
│ Pebble Watch│  "06.07 Poniedzialek — Bio"
│             │  "10.07 Piatek — Zielone"
│             │  "15.07 Sroda — Zmieszane"
└─────────────┘
```

The kiedywywoz.pl API only returns schedule data as a large PNG image (~290KB).
A Lambda OCR proxy converts it to lightweight JSON that the watch can display.

## Features

- **Instant launch** — cached schedule shown immediately, refreshed in background
- Color-coded waste types (on color watches)
- Last-fetched timestamp in header
- Street/number resolved by name (no IDs needed)
- ~5s response time (warm Lambda)

## Usage

- **Launch** — shows cached schedule instantly, auto-refreshes from server
- **UP/DOWN** — scroll through entries
- **BACK** — exit app
- To get fresh data, simply exit and reopen the app

## Project Structure

```
src/c/                  Pebble watch app (C)
src/pkjs/               PebbleKit JS (phone-side communication)
infra/                  AWS infrastructure
  lambda/               Lambda handler + Polish OCR data
  layer/                Pre-built Tesseract binary layer
  docker/               Dockerfile for local deployment
  lib/                  CDK stack definition
```

## Setup

### 1. Build & Install the Watch App

```bash
pebble build
pebble install --phone <PHONE_IP>
```

### 2. Configure via Settings

Open the Pebble app on your phone → WasteWatch → Settings:

- **Token** — kiedywywoz.pl API token (see below)
- **Proxy URL** _(optional)_ — leave empty to use the default hosted service
- **Street** — street name (e.g. `Do Przystani`)
- **House Number** — house number (e.g. `12b`)

#### Finding your token

The token determines which city's data you access. The default token is scraped from
[mpo.krakow.pl/harmonogram-odbioru](https://mpo.krakow.pl/harmonogram-odbioru/) and works for **Kraków** only.

If you're in a different city that uses kiedywywoz.pl, you need to find the token yourself —
look in your municipality's waste schedule page source code (search for `token` in the
page's JavaScript or network requests to `kiedywywoz.pl`).

### 3. OCR Service

The app ships with a **default hosted OCR service** — no setup required.

> ⚠️ The default service is provided free of charge for personal use. If usage costs grow beyond what's sustainable, it may be shut down. In that case, deploy your own (see below).

To deploy your own:

#### Local with Docker

```bash
# From project root:
docker build -f infra/docker/Dockerfile -t wastewatch-ocr .
docker run --rm -p 5674:5674 wastewatch-ocr
```

Set Proxy URL in Settings to `http://<your-machine-ip>:5674`

#### Local without Docker

Requires: Python 3.12+, [Tesseract](https://github.com/tesseract-ocr/tesseract) with Polish language pack.

```bash
# macOS
brew install tesseract tesseract-lang

# Ubuntu/Debian
sudo apt-get install tesseract-ocr tesseract-ocr-pol
```

```bash
cd infra/lambda
python3 -c "
import os, json, subprocess, tempfile
from http.server import HTTPServer, BaseHTTPRequestHandler

os.environ['TESSDATA_PREFIX'] = '/opt/homebrew/share/tessdata'  # macOS
os.environ['LAMBDA_TASK_ROOT'] = '.'
import handler

def local_ocr(png_bytes):
    with tempfile.NamedTemporaryFile(suffix='.png', delete=False) as f:
        f.write(png_bytes); path = f.name
    try:
        return subprocess.run(['tesseract', path, 'stdout', '-l', 'pol'],
                             capture_output=True, text=True, timeout=60).stdout
    finally:
        os.unlink(path)
handler._ocr = local_ocr

class H(BaseHTTPRequestHandler):
    def do_POST(self):
        body = self.rfile.read(int(self.headers.get('Content-Length', 0))).decode()
        result = handler.lambda_handler({'httpMethod': 'POST', 'path': self.path, 'body': body,
                                         'headers': {'content-type': 'application/x-www-form-urlencoded'}}, None)
        self.send_response(result['statusCode'])
        for k, v in result['headers'].items(): self.send_header(k, v)
        self.end_headers()
        self.wfile.write(result['body'].encode())
    def do_GET(self):
        result = handler.lambda_handler({'httpMethod': 'GET', 'path': self.path}, None)
        self.send_response(result['statusCode'])
        for k, v in result['headers'].items(): self.send_header(k, v)
        self.end_headers()
        self.wfile.write(result['body'].encode())

print('OCR proxy on http://localhost:5674')
HTTPServer(('0.0.0.0', 5674), H).serve_forever()
"
```

#### AWS Lambda (production)

```bash
cd infra
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
cdk bootstrap  # first time only
cdk deploy --profile <your-profile>
```

Uses a [pre-built Tesseract Lambda layer](https://github.com/bweigel/aws-lambda-tesseract-layer).
Set Proxy URL in Settings to the Function URL from `cdk deploy` output.

**Cost:** $0 under AWS free tier. ~$0.01/month after (2048MB, ~5s per invocation).

## API

### `POST /schedule`

Request (form-encoded or JSON):

```
token=YOUR_TOKEN&street=Do+Przystani&number=12b
```

Response:

```json
{
  "status": "ok",
  "street_id": "39339",
  "number_id": "834465",
  "count": 5,
  "entries": [
    { "date": "06.07", "day": "Poniedziałek", "type": "Bio" },
    { "date": "10.07", "day": "Piątek", "type": "Zielone" },
    { "date": "13.07", "day": "Poniedziałek", "type": "Bio" },
    { "date": "15.07", "day": "Środa", "type": "Zmieszane" },
    { "date": "17.07", "day": "Piątek", "type": "Zielone" }
  ]
}
```

Cached IDs can be passed as `ulica` + `numer` for faster responses (skips street lookup).

### `GET /health`

Returns `{"status": "ok"}`.

## Waste Types & Colors

| Type          | Color      | Description        |
| ------------- | ---------- | ------------------ |
| Bio           | Brown      | Organic/food waste |
| Papier        | Blue       | Paper              |
| Plastik/Metal | Yellow     | Plastics & metals  |
| Szkło         | Mint green | Glass              |
| Zielone       | Dark green | Garden waste       |
| Zmieszane     | Dark gray  | Mixed/general      |

## Security

The kiedywywoz.pl API token is:

- Stored on your phone (PebbleKit JS localStorage + watch persistent storage)
- Sent to the proxy over HTTPS for each request
- **Never stored** by the Lambda — used only for the outbound API call

## Supported Platforms

aplite, basalt, chalk, diorite, emery, flint, gabbro

Color coding active on: basalt, chalk, emery, flint, gabbro.

## License

MIT
