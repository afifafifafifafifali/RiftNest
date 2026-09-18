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

rn_err_t proot_net_setup(const char *instance_name, const char *host_fwd, const char *guest_port)
{
    (void)guest_port;

    if (!instance_name || !instance_name[0])
        return RN_ERR_BADARG;

    char pidfile[4096];
    net_pid_path(instance_name, pidfile, sizeof(pidfile));

    struct stat st;
    if (stat(pidfile, &st) == 0) {
        riftprint("WARN: network already set up for '%s'", instance_name);
        return RN_OK;
    }

    const char *rootlesskit = "rootlesskit";
    if (access(rootlesskit, X_OK) != 0) {
        riftprint("WARN: rootlesskit not found, skipping network setup");
        return RN_OK;
    }

    const char *slirp = "slirp4netns";
    if (access(slirp, X_OK) != 0) {
        riftprint("WARN: slirp4netns not found, skipping network setup");
        return RN_OK;
    }

    char cmd[8192];

    snprintf(cmd, sizeof(cmd),
        "nohup %s "
        "--copy-up=/etc "
        "--net=slirp4netns "
        "--disable-host-loopback "
        "--propagation=rslave "
        "%s%s "
        "> /dev/null 2>&1 & echo $! > '%s'",
        rootlesskit,
        host_fwd ? "-p " : "",
        host_fwd ? host_fwd : "",
        pidfile);

    int rc = system(cmd);
    if (rc != 0) {
        riftprint("ERROR: failed to start rootlesskit");
        return RN_ERR_NET_SETUP;
    }

    riftprint("Network setup for '%s'", instance_name);
    return RN_OK;
}

rn_err_t proot_net_teardown(const char *instance_name)
{
    if (!instance_name || !instance_name[0])
        return RN_ERR_BADARG;

    char pidfile[4096];
    net_pid_path(instance_name, pidfile, sizeof(pidfile));

    FILE *f = fopen(pidfile, "r");
    if (!f) {
        riftprint("WARN: no network process found for '%s'", instance_name);
        return RN_OK;
    }

    char buf[64];
    if (fgets(buf, sizeof(buf), f)) {
        pid_t pid = (pid_t)atol(buf);
        if (pid > 0) {
            kill(pid, SIGTERM);
            usleep(100000);
            kill(pid, SIGKILL);
        }
    }
    fclose(f);
    unlink(pidfile);

    riftprint("Network torn down for '%s'", instance_name);
    return RN_OK;
}
