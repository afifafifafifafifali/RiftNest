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
#include <sys/wait.h>
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

    if (access("slirp4netns", X_OK) != 0) {
        riftprint("ERROR: slirp4netns not found on PATH");
        riftprint("  Install: apt install slirp4netns  (or)  dnf install slirp4netns");
        return RN_ERR_NET_SETUP;
    }

    riftprint("Setting up network for '%s'...", instance_name);

    char cmd[8192];

    snprintf(cmd, sizeof(cmd),
        "unshare --net -- sh -c '"
        "  slirp4netns --configure --mtu=65520 --disable-host-loopback $$ &"
        "  sleep 1"
        "  %s%s"
        "  echo $$$$ > '%s'"
        "  wait'"
        "  %s%s %s",
        host_fwd ? "iptables -t nat -A PREROUTING -p tcp --dport " : "",
        host_fwd ? host_fwd : "",
        pidfile,
        "",
        "",
        "");

    int rc = system(cmd);
    if (rc != 0) {
        riftprint("ERROR: failed to set up network namespace");
        unlink(pidfile);
        return RN_ERR_NET_SETUP;
    }

    riftprint("Network setup for '%s'", instance_name);
    riftprint("  Network namespace is active (PID stored in %s)", pidfile);
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

int proot_net_is_active(const char *instance_name)
{
    if (!instance_name || !instance_name[0])
        return 0;

    char pidfile[4096];
    net_pid_path(instance_name, pidfile, sizeof(pidfile));

    struct stat st;
    return stat(pidfile, &st) == 0;
}

int proot_net_nsenter_cmd(const char *instance_name, char *buf, size_t buflen)
{
    char pidfile[4096];
    net_pid_path(instance_name, pidfile, sizeof(pidfile));

    FILE *f = fopen(pidfile, "r");
    if (!f)
        return -1;

    char pidstr[64] = {0};
    if (!fgets(pidstr, sizeof(pidstr), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    size_t len = strlen(pidstr);
    if (len > 0 && pidstr[len - 1] == '\n')
        pidstr[len - 1] = '\0';

    snprintf(buf, buflen, "nsenter --net=/proc/%s/ns/net -- ", pidstr);
    return 0;
}
