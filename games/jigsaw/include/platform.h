#ifndef JIGSAW_PLATFORM_H
#define JIGSAW_PLATFORM_H

#include <stdbool.h>

void platform_init(void);
void platform_cleanup(void);
double platform_get_time(void);
void platform_sleep_ms(int ms);

#endif /* JIGSAW_PLATFORM_H */
