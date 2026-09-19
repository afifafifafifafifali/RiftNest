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
struct qemu_ver {
    int major;
    int minor;
    int patch;
};

static rn_err_t parse_qemu_version(struct qemu_ver *out)
{
    FILE *fp = popen("qemu-system-x86_64 --version 2>&1", "r");
    if (!fp)
        return RN_ERR_QEMU_NOT_FOUND;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        for (char *p = line; *p; p++) {
            if (*p < '0' || *p > '9')
                continue;
            int a, b, c;
            char extra;
            if (sscanf(p, "%d.%d.%d%c", &a, &b, &c, &extra) >= 3) {
                out->major = a;
                out->minor = b;
                out->patch = c;
                pclose(fp);
                return RN_OK;
            }
        }
    }
    pclose(fp);
    return RN_ERR_QEMU_NOT_FOUND;
}

static rn_err_t qemu_probe(void)
{
    if (system("command -v qemu-system-x86_64 >/dev/null 2>&1") != 0) {
        riftprint("QEMU not found on PATH");
        return RN_ERR_QEMU_NOT_FOUND;
    }

    struct qemu_ver ver;
    rn_err_t rc = parse_qemu_version(&ver);
    if (rc != RN_OK) {
        riftprint("Could not determine QEMU version");
        return rc;
    }

    riftprint("QEMU version %d.%d.%d detected", ver.major, ver.minor, ver.patch);

    if (ver.major < 10) {
        riftprint("QEMU version %d.%d.%d is below minimum 10.0.0",
                   ver.major, ver.minor, ver.patch);
        return RN_ERR_QEMU_TOO_OLD;
    }

    return RN_OK;
}

static rn_err_t qemu_pull(const char *name, const char *url)
{
    return image_pull(name, url);
}

static rn_err_t qemu_list_images(void)
{
    return image_list();
}

static rn_err_t qemu_list_instances(void)
{
    return instance_list();
}

static int find_file_in_dir(const char *dir, const char *pattern, char *buf, size_t buflen)
{
    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
        "find '%s' -maxdepth 1 -name '%s' 2>/dev/null | head -1", dir, pattern);
    FILE *fp = popen(cmd, "r");
    if (!fp)
        return -1;
    if (fgets(buf, buflen, fp)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        pclose(fp);
        return 0;
    }
    pclose(fp);
    return -1;
}

static int find_kernel(char *buf, size_t buflen)
{
    return find_file_in_dir(rn_kernel_dir(), "vmlinuz*", buf, buflen);
}

static int find_initrd(char *buf, size_t buflen)
{
    return find_file_in_dir(rn_kernel_dir(), "initr*", buf, buflen);
}

static void build_qemu_cmd(char *out, size_t outlen,
                           const char *kernel, const char *initrd,
                           const char *rootfs_path, const char *cmd,
                           const char *extra_args)
{
    snprintf(out, outlen,
        "exec qemu-system-x86_64 "
        "-nographic "
        "-m 512 "
        "-accel tcg "
        "-kernel '%s' "
        "%s%s%s "
        "-append 'console=ttyS0 riftnest_rootfs=%s riftnest_cmd=%s' "
        "-fsdev local,id=rootfs,path='%s',security_model=mapped-xattr "
        "-device virtio-9p-pci,fsdev=rootfs,mount_tag=rootfs "
        "-netdev user,id=net0 "
        "-device virtio-net-pci,netdev=net0 "
        "%s"
        "-no-reboot",
        kernel,
        initrd[0] ? "-initrd '" : "",
        initrd[0] ? initrd : "",
        initrd[0] ? "'" : "",
        rootfs_path,
        cmd,
        rootfs_path,
        extra_args ? extra_args : "");
}

static rn_err_t qemu_exec_rootfs(const char *rootfs, const char *label,
                                  int argc, char **argv)
{
    char kernel[4096] = {0};
    char initrd[4096] = {0};

    if (find_kernel(kernel, sizeof(kernel)) != 0) {
        riftprint("ERROR: no kernel found in %s", rn_kernel_dir());
        riftprint("  Download a vmlinuz and initramfs into %s", rn_kernel_dir());
        return RN_ERR_NOTFOUND;
    }

    find_initrd(initrd, sizeof(initrd));

    const char *cmd = "/bin/sh";
    if (argc > 0 && argv[0])
        cmd = argv[0];

    riftprint("Running '%s' in %s (QEMU/TCG)", cmd, label);

    char qemu_cmd[16384];
    build_qemu_cmd(qemu_cmd, sizeof(qemu_cmd), kernel, initrd, rootfs, cmd, NULL);

    int rc = system(qemu_cmd);
    if (rc != 0) {
        riftprint("QEMU exited with code %d", rc);
        return RN_ERR_QEMU_BOOT;
    }

    return RN_OK;
}

static rn_err_t qemu_run(const char *image, int argc, char **argv)
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

    return qemu_exec_rootfs(img_path, image, argc, argv);
}

static rn_err_t qemu_create(const char *instance, const char *image)
{
    return instance_create(instance, image);
}

static rn_err_t qemu_exec(const char *instance, int argc, char **argv)
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

    return qemu_exec_rootfs(inst_path, instance, argc, argv);
}

static rn_err_t qemu_destroy(const char *instance)
{
    return instance_destroy(instance);
}

static rn_err_t qemu_net_setup_wrap(const char *instance, const char *host_fwd, const char *guest_port)
{
    return qemu_net_setup(instance, host_fwd, guest_port);
}

static rn_err_t qemu_net_teardown_wrap(const char *instance)
{
    return qemu_net_teardown(instance);
}

rn_backend_t qemu_backend = {
    .name         = "qemu",
    .probe        = qemu_probe,
    .pull         = qemu_pull,
    .list_images  = qemu_list_images,
    .list_instances = qemu_list_instances,
    .run          = qemu_run,
    .create       = qemu_create,
    .exec         = qemu_exec,
    .destroy      = qemu_destroy,
    .net_setup    = qemu_net_setup_wrap,
    .net_teardown = qemu_net_teardown_wrap,
};
