@echo off

REM 設定變數
set IMAGE_NAME=myapp
set TAG=latest

echo build Docker image...
docker build -t %IMAGE_NAME%:%TAG% .

if errorlevel 1 (
    echo Build ERROR!
    exit /b 1
)

echo Build Success
docker run --rm -it %IMAGE_NAME%:%TAG%

echo RUN END.
pause
