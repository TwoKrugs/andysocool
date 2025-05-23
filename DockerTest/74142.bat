@echo off

echo build Docker image...
docker-compose build

if errorlevel 1 (
    echo Build ERROR!
    pause
    exit /b 1
)

echo Build Success
docker-compose up

echo RUN END.
pause