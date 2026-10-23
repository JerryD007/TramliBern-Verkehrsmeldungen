FROM python:3.11-slim
WORKDIR /app
RUN pip install --no-cache-dir gunicorn==23.0.0
COPY service /app/service
USER 65534:65534
EXPOSE 8080
CMD ["gunicorn", "--workers", "1", "--threads", "4", "--bind", "0.0.0.0:8080", "service.siri.acquire:application"]
