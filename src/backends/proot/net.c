// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
#define _POSIX_C_SOURCE 200809L
#include "net.h"
#include "../../paths.h"
#include "../../misc/riftprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

static void net_flag_path(const char *instance_name, char *buf, size_t buflen)
{
    snprintf(buf, buflen, "%s/%s.net", rn_net_dir(), instance_name);
}

static int has_cmd(const char *cmd)
{
    char check[1024];
    snprintf(check, sizeof(check), "command -v %s >/dev/null 2>&1", cmd);
    return system(check) == 0;
}

rn_err_t proot_net_setup(const char *instance_name, const char *host_fwd, const char *guest_port)
{
    (void)guest_port;
    (void)host_fwd;

    if (!instance_name || !instance_name[0])
        return RN_ERR_BADARG;

    char flagfile[4096];
    net_flag_path(instance_name, flagfile, sizeof(flagfile));

    struct stat st;
    if (stat(flagfile, &st) == 0) {
        riftprint("WARN: networking already enabled for '%s'", instance_name);
        return RN_OK;
    }

    if (!has_cmd("slirp4netns")) {
        riftprint("ERROR: slirp4netns not found on PATH");
        riftprint("  Install: apt install slirp4netns  (or)  dnf install slirp4netns");
        return RN_ERR_NET_SETUP;
    }

    if (!has_cmd("unshare")) {
        riftprint("ERROR: unshare not found on PATH");
        return RN_ERR_NET_SETUP;
    }

    int fd = open(flagfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        riftprint("ERROR: cannot create network flag file");
        return RN_ERR_NET_SETUP;
    }
    close(fd);

    riftprint("Networking enabled for '%s'", instance_name);
    riftprint("  Network will be active on next exec (unshare+slirp4netns)");
    return RN_OK;
}

rn_err_t proot_net_teardown(const char *instance_name)
{
    if (!instance_name || !instance_name[0])
        return RN_ERR_BADARG;

    char flagfile[4096];
    net_flag_path(instance_name, flagfile, sizeof(flagfile));

    if (unlink(flagfile) == 0)
        riftprint("Networking disabled for '%s'", instance_name);
    else
        riftprint("WARN: networking was not enabled for '%s'", instance_name);

    return RN_OK;
}

int proot_net_is_active(const char *instance_name)
{
    if (!instance_name || !instance_name[0])
        return 0;

    char flagfile[4096];
    net_flag_path(instance_name, flagfile, sizeof(flagfile));

    struct stat st;
    return stat(flagfile, &st) == 0;
}
