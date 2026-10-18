#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static HANDLE h_stdout = INVALID_HANDLE_VALUE;
static HANDLE h_stdin = INVALID_HANDLE_VALUE;
static DWORD original_out_mode = 0;
static DWORD original_in_mode = 0;
static UINT original_cp = 0;
static bool initialized = false;
static LARGE_INTEGER perf_freq;
static LARGE_INTEGER start_time;

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
  (void)ctrl_type;
  platform_cleanup();
  ExitProcess(0);
  return TRUE;
}

void platform_init(void) {
  if (initialized) {
    return;
  }

  h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  h_stdin = GetStdHandle(STD_INPUT_HANDLE);

  if (h_stdout != INVALID_HANDLE_VALUE) {
    GetConsoleMode(h_stdout, &original_out_mode);
    DWORD new_out_mode = original_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
    SetConsoleMode(h_stdout, new_out_mode);
  }

  if (h_stdin != INVALID_HANDLE_VALUE) {
    GetConsoleMode(h_stdin, &original_in_mode);
    DWORD new_in_mode = (original_in_mode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(h_stdin, new_in_mode);
    FlushConsoleInputBuffer(h_stdin);
  }

  original_cp = GetConsoleOutputCP();
  SetConsoleOutputCP(CP_UTF8);

  QueryPerformanceFrequency(&perf_freq);
  QueryPerformanceCounter(&start_time);

  SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
  atexit(platform_cleanup);

  printf("\033[?1049h\033[?25l");
  fflush(stdout);

  initialized = true;
}

void platform_cleanup(void) {
  if (!initialized) {
    return;
  }

  printf("\033[?25h\033[?1049l\033[0m");
  fflush(stdout);

  if (h_stdout != INVALID_HANDLE_VALUE) {
    SetConsoleMode(h_stdout, original_out_mode);
  }
  if (h_stdin != INVALID_HANDLE_VALUE) {
    SetConsoleMode(h_stdin, original_in_mode);
  }
  if (original_cp != 0) {
    SetConsoleOutputCP(original_cp);
  }

  initialized = false;
}

unsigned long long platform_get_time_ms(void) {
  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);
  unsigned long long elapsed_ticks = (unsigned long long)(current.QuadPart - start_time.QuadPart);
  return (elapsed_ticks * 1000ULL) / (unsigned long long)perf_freq.QuadPart;
}

void platform_sleep_ms(int ms) {
  if (ms > 0) {
    Sleep((DWORD)ms);
  }
}
