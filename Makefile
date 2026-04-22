SRC    = src/main.c
CFLAGS = -std=c99 -Wall -O2

# Windows (MSYS2 MinGW64)
ifeq ($(OS),Windows_NT)
CC       = C:/msys64/mingw64/bin/gcc.exe
INCS     = -IC:/msys64/mingw64/include
LIBDIR   = -LC:/msys64/mingw64/lib
LIBS     = -lraylib -lopengl32 -lgdi32 -lwinmm
TARGET   = game.exe
MSYS_BIN = C:/msys64/mingw64/bin
else
# Linux / macOS
CC      = gcc
INCS    =
LIBDIR  =
LIBS    = -lraylib -lm -lpthread -ldl -lrt
TARGET  = game
endif

all: $(TARGET) dlls

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCS) $(SRC) $(LIBDIR) $(LIBS) -o $(TARGET)

# Windows: gerekli DLL'leri exe yanına kopyala
dlls:
ifeq ($(OS),Windows_NT)
	@if not exist libraylib.dll copy "$(MSYS_BIN)\libraylib.dll" . >NUL
	@if not exist glfw3.dll     copy "$(MSYS_BIN)\glfw3.dll"     . >NUL
endif

clean:
ifeq ($(OS),Windows_NT)
	-del /f $(TARGET) libraylib.dll glfw3.dll 2>NUL
else
	rm -f $(TARGET)
endif

run: all
	./$(TARGET)

.PHONY: all dlls clean run
