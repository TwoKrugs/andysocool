FROM gcc:latest
WORKDIR /app
COPY . .

RUN gcc -o server server.c && gcc -o client client.c
RUN chmod +x entrypoint.sh

CMD ["./entrypoint.sh"]