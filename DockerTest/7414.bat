@echo off

REM 設定變數
set IMAGE_NAME=myapp
set TAG=latest

echo build Docker image...
docker build -t %IMAGE_NAME%:%TAG% .

if errorlevel 1 (
    echo Build ERROR!
    pause
    exit /b 1
)

echo Build Success
REM docker run --rm -it %IMAGE_NAME%:%TAG%
docker run -d -p 49738:80 %IMAGE_NAME%:%TAG%

echo RUN END.
pause
