# client/Dockerfile
FROM gcc:latest
WORKDIR /app
COPY client.c .
RUN gcc -o client client.c -lpthread
CMD ["./client"]