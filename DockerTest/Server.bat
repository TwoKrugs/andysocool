@echo off

echo build Docker image...

docker build -t server ./server

if errorlevel 1 (
    echo Build ERROR!
    pause
    exit /b 1
)

docker run --rm -it -p 5358:12345/tcp server

echo RUN END.
pause
