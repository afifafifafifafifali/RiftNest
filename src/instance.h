// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
#pragma once

#include "errorcodes.h"
rn_err_t instance_create(const char *name, const char *image);
rn_err_t instance_destroy(const char *name);
rn_err_t instance_list(void);
