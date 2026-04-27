@echo off
set PATH=C:\msys64\mingw64\bin;%PATH%
gcc -std=c99 -Wall -O2 -IC:\msys64\mingw64\include src\*.c -LC:\msys64\mingw64\lib -lraylib -lopengl32 -lgdi32 -lwinmm -o game.exe > build_log.txt 2>&1
echo Exit Code: %errorlevel% >> build_log.txt
if %errorlevel% == 0 copy /Y game.exe release\game.exe >> build_log.txt 2>&1
