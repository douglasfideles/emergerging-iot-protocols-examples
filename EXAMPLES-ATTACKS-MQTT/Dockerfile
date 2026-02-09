FROM python:3.11-alpine

WORKDIR /app

# Install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy all attack scripts
COPY *.py /app/

# Default broker address (can be overridden)
ENV BROKER_HOST=localhost

# Entry point allows running any attack script
ENTRYPOINT ["python"]
CMD ["attack-mqtt-topic-injection.py"]
