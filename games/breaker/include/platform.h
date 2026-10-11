#ifndef BREAKER_PLATFORM_H
#define BREAKER_PLATFORM_H

void platform_init(void);
void platform_cleanup(void);
void platform_sleep_ms(int ms);
unsigned long long platform_get_time_ms(void);
void platform_get_window_size(int *width, int *height);

#endif // BREAKER_PLATFORM_H
