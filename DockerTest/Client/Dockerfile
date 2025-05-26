FROM gcc:latest
WORKDIR /app
COPY ClientTest.c .
RUN gcc -o client ClientTest.c -pthread
CMD ["./client"]