#ifndef CHESS_PLATFORM_H
#define CHESS_PLATFORM_H

#include <stdbool.h>

void platform_init(void);
void platform_cleanup(void);
unsigned long long platform_get_time_ms(void);
void platform_sleep_ms(int ms);

#endif /* CHESS_PLATFORM_H */
