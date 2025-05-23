# Dockerfile
FROM gcc:latest

WORKDIR /app
COPY . /app
RUN apt-get update && apt-get install -y build-essential
RUN gcc -o myapp udp.c -lpthread

CMD ["./myapp","172.29.66.146"]