// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
#define _POSIX_C_SOURCE 200809L
#include "paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static char home_buf[4096];
static char images_buf[4096];
static char inst_buf[4096];
static char kernel_buf[4096];
static char net_buf[4096];
static int home_init = 0;

static void ensure_dir(const char *path)
{
    mkdir(path, 0755);
}

const char *rn_home(void)
{
    if (home_init)
        return home_buf;

    const char *env = getenv(RN_HOME_ENV);
    if (env && env[0]) {
        snprintf(home_buf, sizeof(home_buf), "%s", env);
    } else {
        const char *homedir = getenv("HOME");
        if (!homedir)
            homedir = "/tmp";
        snprintf(home_buf, sizeof(home_buf), "%s/%s", homedir, RN_HOME_DEFAULT);
    }

    ensure_dir(home_buf);
    home_init = 1;
    return home_buf;
}

const char *rn_images_dir(void)
{
    snprintf(images_buf, sizeof(images_buf), "%s/%s", rn_home(), RN_IMAGES_DIR);
    ensure_dir(images_buf);
    return images_buf;
}

const char *rn_instances_dir(void)
{
    snprintf(inst_buf, sizeof(inst_buf), "%s/%s", rn_home(), RN_INST_DIR);
    ensure_dir(inst_buf);
    return inst_buf;
}

const char *rn_kernel_dir(void)
{
    snprintf(kernel_buf, sizeof(kernel_buf), "%s/%s", rn_home(), RN_KERNEL_DIR);
    ensure_dir(kernel_buf);
    return kernel_buf;
}

const char *rn_net_dir(void)
{
    snprintf(net_buf, sizeof(net_buf), "%s/%s", rn_home(), RN_NET_DIR);
    ensure_dir(net_buf);
    return net_buf;
}
