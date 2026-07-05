"""
WasteWatch OCR Lambda

POST /schedule
  Body (form-encoded or JSON): token, street, number (optional: ulica, numer)

Resolves street/number names to IDs, fetches schedule PNG from kiedywywoz.pl,
OCRs with Tesseract, returns parsed waste collection entries.
"""

import base64
import json
import logging
import os
import re
import subprocess
import tempfile
from datetime import datetime
from urllib.parse import urlencode, parse_qs
from urllib.request import Request, urlopen

logger = logging.getLogger()
logger.setLevel(logging.INFO)

API_URL = "https://kiedywywoz.pl/API/harmo_img/"

MONTHS_PL = {
    "stycznia": 1, "lutego": 2, "marca": 3, "kwietnia": 4,
    "maja": 5, "czerwca": 6, "lipca": 7, "sierpnia": 8,
    "września": 9, "wrzesnia": 9,
    "października": 10, "pazdziernika": 10,
    "listopada": 11, "grudnia": 12,
}

DAY_RE = re.compile(
    r"^(poniedzia[łl]?ek|wtorek|[śs]rod[aą]|czwartek|pi[ąa]tek|sobota|niedziela)"
    r",?\s+(\d+)\s+(\w+)",
    re.IGNORECASE,
)

TYPE_FIXES = {
    "szkto": "Szkło", "Szkto": "Szkło",
    "Tworzywa sztuczne": "Plastik/Metal",
}
TESSDATA = os.environ.get("TESSDATA_PREFIX", "/opt/tesseract/share/tessdata")



def _normalize(s: str) -> str:
    return re.sub(r"[\s\-\.]+", "", s.strip().strip('"\'""').lower())


def _api(token: str, ulica: str = "", numer: str = ""):
    params = {"token": token}
    if ulica:
        params["ulica"] = ulica
    if numer:
        params["numer"] = numer
    req = Request(API_URL, data=urlencode(params).encode(), method="POST",
                  headers={"Content-Type": "application/x-www-form-urlencoded"})
    with urlopen(req, timeout=30) as resp:
        return json.loads(resp.read())


def _resolve_street(token: str, name: str) -> str | None:
    data = _api(token)
    if not isinstance(data, list):
        return None
    target = _normalize(name)
    for item in data:
        if _normalize(item.get("name", "")) == target:
            return item["id"]
    for item in data:
        if target in _normalize(item.get("name", "")):
            return item["id"]
    for item in data:
        n = _normalize(item.get("name", ""))
        if n and n in target:
            return item["id"]
    return None


def _resolve_number(token: str, street_id: str, number: str) -> str | None:
    data = _api(token, ulica=street_id)
    if not isinstance(data, list):
        return None
    target = _normalize(number)
    for item in data:
        if _normalize(item.get("name", "")) == target:
            return item["id"]
    for item in data:
        if target in _normalize(item.get("name", "")):
            return item["id"]
    return None


def _fetch_image(token: str, ulica: str, numer: str) -> bytes | None:
    data = _api(token, ulica=ulica, numer=numer)
    if isinstance(data, dict) and data.get("status") == 1 and data.get("img"):
        return base64.b64decode(data["img"].split(",")[1].strip())
    return None


def _ocr(png_bytes: bytes) -> str:
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False, dir="/tmp") as f:
        f.write(png_bytes)
        path = f.name
    try:
        env = os.environ.copy()
        env["TESSDATA_PREFIX"] = TESSDATA
        r = subprocess.run(
            ["/opt/bin/tesseract", path, "stdout", "-l", "pol"],
            capture_output=True, text=True, timeout=60, env=env,
        )
        if r.returncode != 0:
            raise RuntimeError(f"tesseract: {r.stderr[:100]}")
        return r.stdout
    finally:
        os.unlink(path)


def _clean_type(raw: str) -> str:
    t = raw.lstrip("'\u2018\u2019").split("(")[0].strip()
    for old, new in TYPE_FIXES.items():
        t = t.replace(old, new)
    return t[:30]


def _parse(text: str, address: str = "") -> list[dict]:
    now = datetime.now()
    lines = [l.strip() for l in text.split("\n")]
    addr_norm = _normalize(address) if address else ""
    entries = []

    i = 0
    while i < len(lines):
        m = DAY_RE.match(lines[i])
        if not m:
            i += 1
            continue

        day_name, day_num, month_name = m.group(1), int(m.group(2)), m.group(3).lower()
        month_num = MONTHS_PL.get(month_name)
        if not month_num:
            i += 1
            continue

        try:
            date = datetime(now.year, month_num, day_num)
        except ValueError:
            i += 1
            continue

        waste_type = ""
        for j in range(i + 1, min(i + 4, len(lines))):
            line = lines[j].strip().lstrip("'")
            if not line:
                continue
            ln = _normalize(line)
            if addr_norm and (addr_norm in ln or ln in addr_norm):
                continue
            waste_type = line
            break

        if waste_type and now.date() <= date.date():
            entry = {"date": date.strftime("%d.%m"), "day": day_name.capitalize(), "type": _clean_type(waste_type)}
            if entry not in entries:
                entries.append(entry)

        i += 1

    return entries



def lambda_handler(event, context):
    method = event.get("httpMethod") or event.get("requestContext", {}).get("http", {}).get("method", "")
    path = event.get("path") or event.get("rawPath", "")

    if method == "GET" and "/health" in path:
        return _resp(200, {"status": "ok"})
    if method != "POST":
        return _resp(404, {"error": "Not found"})

    body = event.get("body", "")
    if event.get("isBase64Encoded"):
        body = base64.b64decode(body).decode()

    try:
        ct = (event.get("headers") or {}).get("content-type", "")
        data = json.loads(body) if "json" in ct else {k: v[0] for k, v in parse_qs(body).items()}
    except (json.JSONDecodeError, TypeError, ValueError):
        return _resp(400, {"error": "Invalid body"})

    token = data.get("token", "")
    if not token:
        return _resp(400, {"error": "Required: token"})

    ulica = data.get("ulica", "")
    numer = data.get("numer", "")
    street = data.get("street", "").strip()
    number = data.get("number", "").strip()

    logger.info(f"Request: street='{street}' number='{number}' ulica={ulica} numer={numer}")

    try:
        if not ulica and street:
            ulica = _resolve_street(token, street)
            if not ulica:
                return _resp(404, {"error": f"Street not found: {street}"})

        if not numer and number:
            if not ulica:
                return _resp(400, {"error": "Need street to resolve number"})
            numer = _resolve_number(token, ulica, number)
            if not numer:
                return _resp(404, {"error": f"Number not found: {number}"})

        if not ulica or not numer:
            return _resp(400, {"error": "Required: street+number or ulica+numer"})

        png = _fetch_image(token, ulica, numer)
        if not png:
            return _resp(502, {"error": "No schedule from API"})

        address = f"{street} {number}".strip()
        entries = _parse(_ocr(png), address=address)

        return _resp(200, {"status": "ok", "street_id": ulica, "number_id": numer,
                           "count": len(entries), "entries": entries})
    except Exception as e:
        logger.exception("Handler error")
        return _resp(500, {"error": str(e)[:100]})


def _resp(code: int, body: dict) -> dict:
    return {
        "statusCode": code,
        "headers": {"Content-Type": "application/json; charset=utf-8", "Access-Control-Allow-Origin": "*"},
        "body": json.dumps(body, ensure_ascii=False),
    }
