#pragma once

#include "errorcodes.h"
// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
rn_err_t image_pull(const char *name, const char *url);
rn_err_t image_list(void);
rn_err_t image_remove(const char *name);
