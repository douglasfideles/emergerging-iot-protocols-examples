FROM python:3.11-alpine

WORKDIR /app

# Install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy all attack scripts
COPY attack-coap-*.py /app/

# Default target address (can be overridden)
ENV TARGET_HOST=localhost
ENV TARGET_PORT=5683

# Entry point allows running any attack script
ENTRYPOINT ["python"]
#CMD ["attack-coap-resource-exhaustion_attack.py"]
