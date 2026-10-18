CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude -I../../include
LDFLAGS = -lm
TARGET = chess.exe
TEST_TARGET = test_chess.exe

SRCS = src/main.c src/config.c src/game.c src/input.c src/platform.c src/render.c
OBJS = $(SRCS:.c=.o)
TEST_SRCS = test_chess.c src/config.c src/game.c src/render.c

ifeq ($(OS),Windows_NT)
  CLEAN_FILES = src\main.o src\config.o src\game.o src\input.o src\platform.o src\render.o $(TARGET) $(TEST_TARGET)
  CLEAN_CMD = -cmd /C "del /Q /F $(CLEAN_FILES) 2>nul" || exit 0
else
  CLEAN_CMD = rm -f $(OBJS) $(TARGET) $(TEST_TARGET)
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
