#include "riftprint.h"
// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
void riftprint(const char *fmt, ...)
{
    va_list args;

    printf("[RIFTNEST CONTAINERIZATION PLATFORM] ");

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    putchar('\n');
}