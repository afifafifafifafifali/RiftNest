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
// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol

static void net_pid_path(const char *instance_name, char *buf, size_t buflen)
{
    snprintf(buf, buflen, "%s/%s.pid", rn_net_dir(), instance_name);
}

rn_err_t qemu_net_setup(const char *instance_name, const char *host_fwd, const char *guest_port)
{
    (void)host_fwd;
    (void)guest_port;

    if (!instance_name || !instance_name[0])
        return RN_ERR_BADARG;

    riftprint("QEMU networking is handled inline by the VM launch command");
    return RN_OK;
}

rn_err_t qemu_net_teardown(const char *instance_name)
{
    if (!instance_name || !instance_name[0])
        return RN_ERR_BADARG;

    char pidfile[4096];
    net_pid_path(instance_name, pidfile, sizeof(pidfile));

    FILE *f = fopen(pidfile, "r");
    if (!f) {
        riftprint("WARN: no QEMU process found for '%s'", instance_name);
        return RN_OK;
    }

    char buf[64];
    if (fgets(buf, sizeof(buf), f)) {
        pid_t pid = (pid_t)atol(buf);
        if (pid > 0) {
            kill(pid, SIGTERM);
            usleep(500000);
            kill(pid, SIGKILL);
        }
    }
    fclose(f);
    unlink(pidfile);

    riftprint("QEMU VM for '%s' stopped", instance_name);
    return RN_OK;
}
