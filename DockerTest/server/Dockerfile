# server/Dockerfile
FROM gcc:latest
RUN apt-get update && apt-get install -y jp2a
WORKDIR /app
COPY server.c .
RUN gcc -o server server.c -lpthread
CMD ["./server"]