#pragma once

#include "errorcodes.h"
// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
typedef struct rn_backend {
    const char *name;

    rn_err_t (*probe)(void);

    rn_err_t (*pull)(const char *name, const char *url);
    rn_err_t (*list_images)(void);
    rn_err_t (*list_instances)(void);

    rn_err_t (*run)(const char *image, int argc, char **argv);
    rn_err_t (*create)(const char *instance, const char *image);
    rn_err_t (*exec)(const char *instance, int argc, char **argv);
    rn_err_t (*destroy)(const char *instance);

    rn_err_t (*net_setup)(const char *instance, const char *host_fwd, const char *guest_port);
    rn_err_t (*net_teardown)(const char *instance);
} rn_backend_t;

extern rn_backend_t proot_backend;
extern rn_backend_t qemu_backend;

rn_backend_t *backend_probe_select(void);
