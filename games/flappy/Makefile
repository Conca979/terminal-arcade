CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm
TARGET = flappy.exe
TEST_TARGET = test_flappy.exe

SRCS = src/main.c src/game.c src/input.c src/platform.c src/render.c
OBJS = $(SRCS:.c=.o)
TEST_SRCS = test_flappy.c src/game.c src/render.c

ifeq ($(OS),Windows_NT)
  CLEAN_FILES = src\main.o src\game.o src\input.o src\platform.o src\render.o $(TARGET) $(TEST_TARGET) savegame.dat
  CLEAN_CMD = -cmd /C "del /Q /F $(CLEAN_FILES) 2>nul" || exit 0
else
  CLEAN_CMD = rm -f $(OBJS) $(TARGET) $(TEST_TARGET) savegame.dat
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(CLEAN_CMD)

.PHONY: all test clean
