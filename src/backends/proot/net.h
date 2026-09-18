#pragma once

#include "../../errorcodes.h"
// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
rn_err_t proot_net_setup(const char *instance_name, const char *host_fwd, const char *guest_port);
rn_err_t proot_net_teardown(const char *instance_name);
