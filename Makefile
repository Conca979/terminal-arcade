CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LAUNCHER = arcade.exe

ifeq ($(OS),Windows_NT)
  CLEAN_CMD = -cmd /C "del /Q /F $(LAUNCHER) src\main.o 2>nul" || exit 0
else
  CLEAN_CMD = rm -f $(LAUNCHER) src/main.o
endif

all: $(LAUNCHER) tetris 2048 minesweeper snake invaders sudoku breaker block pong typing wordle

$(LAUNCHER): src/main.c
	$(CC) $(CFLAGS) -o $@ $<

tetris:
	$(MAKE) -C games/tetris

2048:
	$(MAKE) -C games/2048

minesweeper:
	$(MAKE) -C games/minesweeper

snake:
	$(MAKE) -C games/snake

invaders:
	$(MAKE) -C games/invaders

sudoku:
	$(MAKE) -C games/sudoku

breaker:
	$(MAKE) -C games/breaker

block:
	$(MAKE) -C games/block

pong:
	$(MAKE) -C games/pong

typing:
	$(MAKE) -C games/typing

wordle:
	$(MAKE) -C games/wordle

test:
	$(MAKE) -C games/tetris test
	$(MAKE) -C games/2048 test
	$(MAKE) -C games/minesweeper test
	$(MAKE) -C games/snake test
	$(MAKE) -C games/invaders test
	$(MAKE) -C games/sudoku test
	$(MAKE) -C games/breaker test
	$(MAKE) -C games/block test
	$(MAKE) -C games/pong test
	$(MAKE) -C games/typing test
	$(MAKE) -C games/wordle test

clean:
	$(CLEAN_CMD)
	$(MAKE) -C games/tetris clean
	$(MAKE) -C games/2048 clean
	$(MAKE) -C games/minesweeper clean
	$(MAKE) -C games/snake clean
	$(MAKE) -C games/invaders clean
	$(MAKE) -C games/sudoku clean
	$(MAKE) -C games/breaker clean
	$(MAKE) -C games/block clean
	$(MAKE) -C games/pong clean
	$(MAKE) -C games/typing clean
	$(MAKE) -C games/wordle clean

.PHONY: all tetris 2048 minesweeper snake invaders sudoku breaker block pong typing wordle test clean
