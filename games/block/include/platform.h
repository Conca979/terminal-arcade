#ifndef BLOCK_PLATFORM_H
#define BLOCK_PLATFORM_H

void platform_init(void);
void platform_cleanup(void);
void platform_sleep_ms(int ms);
unsigned long long platform_get_time_ms(void);

#endif // BLOCK_PLATFORM_H
