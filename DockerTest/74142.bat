@echo off

REM 設定變數
set IMAGE_NAME=myapp
set TAG=latest

docker network create chatnet

echo build Docker image...

docker build -t server ./server
docker build -t client ./client

if errorlevel 1 (
    echo Build ERROR!
    pause
    exit /b 1
)

docker run -it --rm --name server --network chatnet server

echo RUN END.
pause
