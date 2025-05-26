# Dockerfile
FROM gcc:latest

WORKDIR /app
COPY . /app
RUN apt-get update && apt-get install -y build-essential
RUN gcc -o myapp quic.c

CMD ["./myapp","172.16.1.49"]