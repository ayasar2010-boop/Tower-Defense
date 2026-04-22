SRC    = src/main.c
CFLAGS = -std=c99 -Wall -O2

# Windows (MSYS2 MinGW64)
ifeq ($(OS),Windows_NT)
CC      = C:/msys64/mingw64/bin/gcc.exe
INCS    = -IC:/msys64/mingw64/include
LIBDIR  = -LC:/msys64/mingw64/lib
LIBS    = -lraylib -lopengl32 -lgdi32 -lwinmm
TARGET  = game.exe
else
# Linux / macOS
CC      = gcc
INCS    =
LIBDIR  =
LIBS    = -lraylib -lm -lpthread -ldl -lrt
TARGET  = game
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCS) $(SRC) $(LIBDIR) $(LIBS) -o $(TARGET)

clean:
ifeq ($(OS),Windows_NT)
	del /f $(TARGET) 2>NUL || true
else
	rm -f $(TARGET)
endif

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
