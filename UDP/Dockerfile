# Dockerfile
FROM gcc:latest

WORKDIR /app
COPY . /app
RUN apt-get update && apt-get install -y build-essential
RUN gcc -o myapp udp.c -lpthread

CMD ["./myapp","172.31.1.44"]
# ------------------以下是本地測試要使用以下先把cmd註解掉
# ENTRYPOINT ["./myapp"]