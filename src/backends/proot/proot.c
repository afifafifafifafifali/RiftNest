// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
#define _POSIX_C_SOURCE 200809L
#include "../../backend.h"
#include "../../paths.h"
#include "../../image.h"
#include "../../instance.h"
#include "../../misc/riftprint.h"
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

static const char *proot_bin_path(void)
{
    static char buf[4096];

    if (access("./target/proot", X_OK) == 0)
        return "./target/proot";

    snprintf(buf, sizeof(buf), "%s/%s", rn_home(), RN_PROOT_BIN);
    if (access(buf, X_OK) == 0)
        return buf;

    if (access("proot", X_OK) == 0)
        return "proot";

    return buf;
}

static rn_err_t proot_probe(void)
{
    const char *bin = proot_bin_path();
    if (access(bin, X_OK) != 0) {
        riftprint("proot not found");
        return RN_ERR_PROOT_NOT_FOUND;
    }
    return RN_OK;
}

static rn_err_t proot_pull(const char *name, const char *url)
{
    return image_pull(name, url);
}

static rn_err_t proot_list_images(void)
{
    return image_list();
}

static rn_err_t proot_list_instances(void)
{
    return instance_list();
}

static void rmrf(const char *path)
{
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
    system(cmd);
}

static int run_proot(const char *rootfs, int argc, char **argv)
{
    const char *bin = proot_bin_path();

    char tmpbase[4096];
    snprintf(tmpbase, sizeof(tmpbase), "%s/tmp", rn_home());
    mkdir(tmpbase, 0700);

    char tmpdir[4096];
    snprintf(tmpdir, sizeof(tmpdir), "%s/tmp/rn_XXXXXXXX", rn_home());
    if (!mkdtemp(tmpdir)) {
        riftprint("ERROR: failed to create temp directory");
        return -1;
    }

    char copy_cmd[8192];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp -a '%s/.' '%s'/.", rootfs, tmpdir);
    if (system(copy_cmd) != 0) {
        riftprint("ERROR: failed to copy rootfs to temp directory");
        rmrf(tmpdir);
        return -1;
    }

    int proot_argc = 10 + argc;
    char **proot_argv = calloc(proot_argc + 1, sizeof(char *));
    if (!proot_argv) {
        rmrf(tmpdir);
        return -1;
    }

    int i = 0;
    proot_argv[i++] = (char *)bin;
    proot_argv[i++] = "-r";
    proot_argv[i++] = tmpdir;
    proot_argv[i++] = "-b";
    proot_argv[i++] = "/proc";
    proot_argv[i++] = "-b";
    proot_argv[i++] = "/sys";
    proot_argv[i++] = "-b";
    proot_argv[i++] = "/dev";
    proot_argv[i++] = "-b";
    proot_argv[i++] = "/etc/resolv.conf";

    if (argc > 0) {
        for (int j = 0; j < argc; j++)
            proot_argv[i++] = argv[j];
    } else {
        proot_argv[i++] = "/bin/sh";
    }

    proot_argv[i] = NULL;

    pid_t pid = fork();
    if (pid < 0) {
        free(proot_argv);
        rmrf(tmpdir);
        return -1;
    }

    if (pid == 0) {
        execvp(bin, proot_argv);
        _exit(127);
    }

    free(proot_argv);

    int status;
    waitpid(pid, &status, 0);

    rmrf(tmpdir);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return -1;
}

static int run_proot_in_ns(const char *instance_name, const char *rootfs,
                           int argc, char **argv)
{
    (void)instance_name;

    riftprint("Running inside network namespace (net enabled for '%s')", instance_name);

    const char *bin = proot_bin_path();

    char proot_cmd[8192];
    int pos = 0;
    pos += snprintf(proot_cmd + pos, sizeof(proot_cmd) - pos,
        "%s -r %s -b /proc -b /sys -b /dev -b /etc/resolv.conf", bin, rootfs);

    for (int j = 0; j < argc; j++)
        pos += snprintf(proot_cmd + pos, sizeof(proot_cmd) - pos, " %s", argv[j]);

    if (argc == 0)
        pos += snprintf(proot_cmd + pos, sizeof(proot_cmd) - pos, " /bin/sh");

    char script[16384];
    snprintf(script, sizeof(script),
        "exec unshare --user --map-root-user --net -- sh -c '"
        "sleep infinity & "
        "NSPID=$!; "
        "sleep 0.5; "
        "slirp4netns --configure --mtu=65520 --disable-host-loopback $NSPID tap0 & "
        "sleep 1.5; "
        "kill $NSPID 2>/dev/null; "
        "exec %s'"
        "",
        proot_cmd);

    pid_t pid = fork();
    if (pid < 0)
        return -1;

    if (pid == 0) {
        setsid();
        execlp("sh", "sh", "-c", script, (char *)NULL);
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return -1;
}

static rn_err_t proot_run(const char *image, int argc, char **argv)
{
    if (!image || !image[0])
        return RN_ERR_BADARG;

    char img_path[4096];
    snprintf(img_path, sizeof(img_path), "%s/%s", rn_images_dir(), image);

    struct stat st;
    if (stat(img_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        riftprint("ERROR: no image named '%s'", image);
        return RN_ERR_NOTFOUND;
    }

    riftprint("Running in image '%s' (proot)", image);

    int rc = run_proot(img_path, argc, argv);
    return rc == 0 ? RN_OK : RN_ERR_INSTANCE_RUN;
}

static rn_err_t proot_create(const char *instance, const char *image)
{
    return instance_create(instance, image);
}

static rn_err_t proot_exec(const char *instance, int argc, char **argv)
{
    if (!instance || !instance[0])
        return RN_ERR_BADARG;

    char inst_path[4096];
    snprintf(inst_path, sizeof(inst_path), "%s/%s", rn_instances_dir(), instance);

    struct stat st;
    if (stat(inst_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        riftprint("ERROR: no instance named '%s'", instance);
        return RN_ERR_NOTFOUND;
    }

    riftprint("Executing in instance '%s' (proot)", instance);

    int rc;
    if (proot_net_is_active(instance)) {
        rc = run_proot_in_ns(instance, inst_path, argc, argv);
    } else {
        rc = run_proot(inst_path, argc, argv);
    }
    return rc == 0 ? RN_OK : RN_ERR_INSTANCE_EXEC;
}

static rn_err_t proot_destroy(const char *instance)
{
    if (proot_net_is_active(instance))
        proot_net_teardown(instance);
    return instance_destroy(instance);
}

static rn_err_t proot_net_setup_wrap(const char *instance, const char *host_fwd, const char *guest_port)
{
    return proot_net_setup(instance, host_fwd, guest_port);
}

static rn_err_t proot_net_teardown_wrap(const char *instance)
{
    return proot_net_teardown(instance);
}

rn_backend_t proot_backend = {
    .name         = "proot",
    .probe        = proot_probe,
    .pull         = proot_pull,
    .list_images  = proot_list_images,
    .list_instances = proot_list_instances,
    .run          = proot_run,
    .create       = proot_create,
    .exec         = proot_exec,
    .destroy      = proot_destroy,
    .net_setup    = proot_net_setup_wrap,
    .net_teardown = proot_net_teardown_wrap,
};
