FROM python:3.9-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir redis

COPY worker.py .

CMD ["python", "worker.py"]
