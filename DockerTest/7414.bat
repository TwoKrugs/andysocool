@echo off

docker run -it --rm --name client --network chatnet --ip 172.31.0.20 client

echo RUN END.
pause