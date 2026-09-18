#define _POSIX_C_SOURCE 200809L
#include "backend.h"
#include "misc/riftprint.h"
#include <stddef.h>
// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
static rn_backend_t *backends[] = {
    &proot_backend,
    &qemu_backend,
    NULL,
};

rn_backend_t *backend_probe_select(void)
{
    riftprint("Probing backends...");

    for (int i = 0; backends[i] != NULL; i++) {
        rn_backend_t *b = backends[i];
        riftprint("  Trying %s...", b->name);
        if (b->probe() == RN_OK) {
            riftprint("  -> %s backend selected", b->name);
            return b;
        }
    }

    riftprint("ERROR: no usable backend found");
    return NULL;
}
