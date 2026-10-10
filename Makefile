CC = gcc
CFLAGS = -Wall -Wextra -O2
LAUNCHER = arcade.exe

ifeq ($(OS),Windows_NT)
  CLEAN_CMD = -cmd /C "del /Q /F $(LAUNCHER) src\main.o 2>nul" || exit 0
else
  CLEAN_CMD = rm -f $(LAUNCHER) src/main.o
endif

all: $(LAUNCHER) tetris 2048 minesweeper

$(LAUNCHER): src/main.c
	$(CC) $(CFLAGS) -o $@ $<

tetris:
	$(MAKE) -C games/tetris

2048:
	$(MAKE) -C games/2048

minesweeper:
	$(MAKE) -C games/minesweeper

test:
	$(MAKE) -C games/tetris test
	$(MAKE) -C games/2048 test
	$(MAKE) -C games/minesweeper test

clean:
	$(CLEAN_CMD)
	$(MAKE) -C games/tetris clean
	$(MAKE) -C games/2048 clean
	$(MAKE) -C games/minesweeper clean

.PHONY: all tetris 2048 minesweeper test clean
