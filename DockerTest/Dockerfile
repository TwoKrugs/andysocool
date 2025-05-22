# Dockerfile
FROM gcc:latest

WORKDIR /app
COPY . /app

RUN gcc -o myapp test.c

CMD ["./myapp"]