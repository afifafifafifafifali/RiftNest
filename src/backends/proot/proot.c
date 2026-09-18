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

static int run_proot(const char *rootfs, int argc, char **argv)
{
    const char *bin = proot_bin_path();

    int proot_argc = 10 + argc;
    char **proot_argv = calloc(proot_argc + 1, sizeof(char *));
    if (!proot_argv)
        return -1;

    int i = 0;
    proot_argv[i++] = (char *)bin;
    proot_argv[i++] = "-r";
    proot_argv[i++] = (char *)rootfs;
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
        return -1;
    }

    if (pid == 0) {
        execvp(bin, proot_argv);
        _exit(127);
    }

    free(proot_argv);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return -1;
}

static int run_proot_in_ns(const char *instance_name, const char *rootfs,
                           int argc, char **argv)
{
    char nsenter_cmd[4096];
    if (proot_net_nsenter_cmd(instance_name, nsenter_cmd, sizeof(nsenter_cmd)) != 0) {
        riftprint("WARN: failed to get network namespace, running without network");
        return run_proot(rootfs, argc, argv);
    }

    const char *bin = proot_bin_path();

    char rootfs_arg[4096];
    snprintf(rootfs_arg, sizeof(rootfs_arg), "%s", rootfs);

    riftprint("Running inside network namespace (net up for '%s')", instance_name);

    int cmd_argc = argc + 20;
    char **cmd_argv = calloc(cmd_argc + 1, sizeof(char *));
    if (!cmd_argv)
        return run_proot(rootfs, argc, argv);

    int i = 0;
    cmd_argv[i++] = "nsenter";
    cmd_argv[i++] = "--net";

    char ns_path[512];
    char pidfile[4096];
    snprintf(pidfile, sizeof(pidfile), "%s/%s.pid", rn_net_dir(), instance_name);
    FILE *f = fopen(pidfile, "r");
    if (f) {
        char pidstr[64] = {0};
        if (fgets(pidstr, sizeof(pidstr), f)) {
            size_t len = strlen(pidstr);
            if (len > 0 && pidstr[len - 1] == '\n')
                pidstr[len - 1] = '\0';
            snprintf(ns_path, sizeof(ns_path), "/proc/%s/ns/net", pidstr);
            cmd_argv[i++] = ns_path;
        }
        fclose(f);
    }

    cmd_argv[i++] = "--";
    cmd_argv[i++] = (char *)bin;
    cmd_argv[i++] = "-r";
    cmd_argv[i++] = rootfs_arg;
    cmd_argv[i++] = "-b";
    cmd_argv[i++] = "/proc";
    cmd_argv[i++] = "-b";
    cmd_argv[i++] = "/sys";
    cmd_argv[i++] = "-b";
    cmd_argv[i++] = "/dev";
    cmd_argv[i++] = "-b";
    cmd_argv[i++] = "/etc/resolv.conf";

    if (argc > 0) {
        for (int j = 0; j < argc; j++)
            cmd_argv[i++] = argv[j];
    } else {
        cmd_argv[i++] = "/bin/sh";
    }

    cmd_argv[i] = NULL;

    pid_t pid = fork();
    if (pid < 0) {
        free(cmd_argv);
        return -1;
    }

    if (pid == 0) {
        execvp("nsenter", cmd_argv);
        _exit(127);
    }

    free(cmd_argv);

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
