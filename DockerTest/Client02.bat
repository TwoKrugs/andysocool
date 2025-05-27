@echo off

docker build -t client02 ./client

docker run --rm -it client02

echo RUN END.
pause