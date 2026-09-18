#pragma once

#include "errorcodes.h"
// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
rn_err_t instance_create(const char *name, const char *image);
rn_err_t instance_destroy(const char *name);
rn_err_t instance_list(void);
