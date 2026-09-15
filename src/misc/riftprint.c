#include "riftprint.h"

void riftprint(const char *fmt, ...)
{
    va_list args;

    printf("[RIFTNEST CONTAINERIZATION PLATFORM] ");

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    putchar('\n');
}