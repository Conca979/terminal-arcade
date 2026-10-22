#ifndef LIFE_PLATFORM_H
#define LIFE_PLATFORM_H

#include <stdbool.h>

void platform_init(void);
void platform_cleanup(void);
double platform_get_time(void);
void platform_sleep_ms(int ms);

#endif /* LIFE_PLATFORM_H */
