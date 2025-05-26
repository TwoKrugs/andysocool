# Dockerfile
FROM gcc:latest

WORKDIR /app
COPY . /app
RUN apt-get update && apt-get install -y build-essential
RUN gcc -o myapp udp.c -lpthread

CMD ["./myapp","172.16.1.49"]