@echo off

REM 設定變數
set IMAGE_NAME=myapp
set TAG=latest

docker network rm chatnet

REM docker network create --subnet=172.31.0.0/16 --gateway=172.31.0.1 --driver bridge chatnet

echo build Docker image...

docker build -t server ./server
docker build -t client ./client

if errorlevel 1 (
    echo Build ERROR!
    pause
    exit /b 1
)

REM docker run -it --rm --name server --network chatnet --ip 172.31.0.10 server
docker run --rm -it -p 65520:12345/tcp server

echo RUN END.
pause
