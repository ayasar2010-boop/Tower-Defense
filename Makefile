SRC      = $(wildcard src/*.c)
CFLAGS   = -std=c99 -Wall -O2 -pipe
RELEASE  = release

ifeq ($(OS),Windows_NT)
CC       = C:/msys64/mingw64/bin/gcc.exe
INCS     = -IC:/msys64/mingw64/include
LIBDIR   = -LC:/msys64/mingw64/lib
LIBS     = -lraylib -lopengl32 -lgdi32 -lwinmm
TARGET   = $(RELEASE)/game.exe
MSYS_BIN = C:/msys64/mingw64/bin
else
CC       = gcc
INCS     =
LIBDIR   =
LIBS     = -lraylib -lm -lpthread -ldl -lrt
TARGET   = $(RELEASE)/game
endif

all: $(RELEASE) $(TARGET) dlls assets

$(RELEASE):
	@mkdir -p $(RELEASE)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCS) $(SRC) $(LIBDIR) $(LIBS) -o $(TARGET)

dlls:
ifeq ($(OS),Windows_NT)
	@[ -f $(RELEASE)/libraylib.dll ] || cp "$(MSYS_BIN)/libraylib.dll" $(RELEASE)/
	@[ -f $(RELEASE)/glfw3.dll ]     || cp "$(MSYS_BIN)/glfw3.dll"     $(RELEASE)/
endif

assets:
	@if [ -d assets ] && [ "$$(ls -A assets 2>/dev/null)" ]; then \
		mkdir -p $(RELEASE)/assets && cp -r assets/. $(RELEASE)/assets/; \
	fi

clean:
	rm -rf $(RELEASE)

run: all
	./$(TARGET)

.PHONY: all dlls assets clean run
