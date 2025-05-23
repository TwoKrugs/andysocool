@echo off
set IMAGE_NAME=my_c_app

echo  Building Docker image...
docker build -t %IMAGE_NAME% .

echo  Running container...
docker run -it --rm %IMAGE_NAME%

echo  Removing Docker image...
docker rmi %IMAGE_NAME%

pause