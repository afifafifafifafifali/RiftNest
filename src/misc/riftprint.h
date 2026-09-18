#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
void riftprint(const char *fmt, ...) __attribute__((format(printf, 1, 2)));