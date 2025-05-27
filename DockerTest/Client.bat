@echo off

docker build -t client ./client

docker run --rm -it client

echo RUN END.
pause