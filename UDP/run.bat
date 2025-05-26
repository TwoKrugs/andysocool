@echo off
set IMAGE_NAME=my_c_app

echo  Building Docker image...
docker build -t %IMAGE_NAME% .

echo  Running container...
docker run --rm -it -p 7680:8888/udp %IMAGE_NAME%
echo  Removing Docker image...
docker rmi %IMAGE_NAME%
@REM -----------------------------------以下是本機互聯測試
@REM @echo off
@REM setlocal

@REM echo [1] 建立 Docker 映像...
@REM docker build -t udp-chat .

@REM echo [2] 檢查是否已建立 chatnet 網路...
@REM docker network inspect chatnet >nul 2>&1
@REM if errorlevel 1 (
@REM     echo [2] chatnet 網路不存在，正在建立...
@REM     docker network create --subnet=172.25.0.0/16 chatnet
@REM ) else (
@REM     echo [2] chatnet 網路已存在。
@REM )

@REM choice /C AB /M "你要啟動哪一個角色容器？(A 或 B)"

@REM if errorlevel 2 (
@REM     set CONTAINER_NAME=userB
@REM     set MY_IP=172.25.0.3
@REM     set TARGET_IP=172.25.0.2
@REM ) else (
@REM     set CONTAINER_NAME=userA
@REM     set MY_IP=172.25.0.2
@REM     set TARGET_IP=172.25.0.3
@REM )

@REM echo [3] 啟動容器 %CONTAINER_NAME%，並連線到對方 IP %TARGET_IP%...
@REM docker run --rm -it --name %CONTAINER_NAME% --network chatnet --ip %MY_IP% udp-chat %TARGET_IP%

@REM endlocal
pause