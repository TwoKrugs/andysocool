# server/Dockerfile
FROM gcc:latest
WORKDIR /app
COPY server.c .
RUN gcc -o server server.c -lpthread
CMD ["./server"]