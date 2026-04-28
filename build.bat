@echo off
set PATH=C:\msys64\mingw64\bin;%PATH%
echo Derleme islemi basladi...
gcc -std=c99 -Wall -O2 -IC:\msys64\mingw64\include src\*.c -LC:\msys64\mingw64\lib -lraylib -lopengl32 -lgdi32 -lwinmm -o game.exe
if %errorlevel% == 0 (
    echo Derleme basarili! Dosya kopyalaniyor...
    if not exist release mkdir release
    copy /Y game.exe release\game.exe
) else (
    echo.
    echo DERLEME BASARISIZ OLDU! Lutfen yukaridaki hatalari kontrol edin.
    echo GCC'nin C:\msys64\mingw64\bin altinda kurulu oldugundan emin olun.
)
pause