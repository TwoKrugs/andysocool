@echo off

REM docker run -it --rm --name client --network chatnet --ip 172.31.0.20 client

docker build -t client ./client

docker run --rm -it client

echo RUN END.
pause