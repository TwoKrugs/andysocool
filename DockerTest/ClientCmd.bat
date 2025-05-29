@echo off

echo build Client Cmd Mode...

REM gcc .\clientCmd\ClientCmd.c -o .\clientCmd\ClientCmd.exe -lws2_32 -lpthread
gcc .\clientCmd\ClientCmd.c -o .\clientCmd\ClientCmd.exe -static -lws2_32 -lpthread

if errorlevel 1 (
    echo Build ERROR!

    echo Running a new client in this window...
    .\clientCmd\ClientCmd.exe
    echo RUN END.
    pause
)

.\clientCmd\ClientCmd.exe

echo RUN END.
pause
