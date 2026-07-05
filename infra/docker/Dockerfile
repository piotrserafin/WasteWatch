FROM python:3.14-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends tesseract-ocr tesseract-ocr-pol && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY infra/lambda/handler.py .
COPY infra/docker/server.py .

ENV TESSDATA_PREFIX=/usr/share/tesseract-ocr/5/tessdata
EXPOSE 5674

CMD ["python", "-u", "server.py"]
