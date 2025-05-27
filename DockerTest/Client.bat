@echo off

docker build -t client ./client

docker run --rm -it -v "C:\Users\andy.chen\Pictures\Screenshots:/img" client

echo RUN END.
pause

