#pragma once

#include "../../errorcodes.h"
#include <stddef.h>

rn_err_t proot_net_setup(const char *instance_name, const char *host_fwd, const char *guest_port);
rn_err_t proot_net_teardown(const char *instance_name);
int proot_net_is_active(const char *instance_name);
int proot_net_nsenter_cmd(const char *instance_name, char *buf, size_t buflen);
