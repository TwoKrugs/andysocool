FROM gcc:latest
WORKDIR /app
COPY ServerTest.c .
RUN gcc -o server ServerTest.c
CMD ["./server"]