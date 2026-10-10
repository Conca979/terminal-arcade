CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
TARGET = 2048.exe
TEST_TARGET = test_2048.exe

SRCS = src/main.c src/game.c src/input.c src/platform.c src/render.c
OBJS = $(SRCS:.c=.o)
TEST_SRCS = test_2048.c src/game.c

ifeq ($(OS),Windows_NT)
  CLEAN_FILES = src\main.o src\game.o src\input.o src\platform.o src\render.o $(TARGET) $(TEST_TARGET)
  CLEAN_CMD = -cmd /C "del /Q /F $(CLEAN_FILES) 2>nul" || exit 0
else
  CLEAN_CMD = rm -f $(OBJS) $(TARGET) $(TEST_TARGET)
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(CLEAN_CMD)

.PHONY: all test clean
