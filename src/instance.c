#define _POSIX_C_SOURCE 200809L
#include "instance.h"
#include "paths.h"
#include "misc/riftprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
rn_err_t instance_create(const char *name, const char *image)
{
    if (!name || !name[0] || !image || !image[0])
        return RN_ERR_BADARG;

    char img_path[4096];
    snprintf(img_path, sizeof(img_path), "%s/%s", rn_images_dir(), image);

    struct stat st;
    if (stat(img_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        riftprint("ERROR: no image named '%s'", image);
        return RN_ERR_NOTFOUND;
    }

    char inst_path[4096];
    snprintf(inst_path, sizeof(inst_path), "%s/%s", rn_instances_dir(), name);

    if (stat(inst_path, &st) == 0) {
        riftprint("ERROR: instance '%s' already exists", name);
        return RN_ERR_EXISTS;
    }

    riftprint("Cloning %s -> instance %s ...", image, name);
    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "cp -a '%s' '%s'", img_path, inst_path);
    int rc = system(cmd);
    if (rc != 0) {
        riftprint("ERROR: clone failed");
        return RN_ERR_INSTANCE_CREATE;
    }

    riftprint("Instance '%s' created.", name);
    return RN_OK;
}

rn_err_t instance_destroy(const char *name)
{
    if (!name || !name[0])
        return RN_ERR_BADARG;

    char inst_path[4096];
    snprintf(inst_path, sizeof(inst_path), "%s/%s", rn_instances_dir(), name);

    struct stat st;
    if (stat(inst_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        riftprint("ERROR: no instance named '%s'", name);
        return RN_ERR_NOTFOUND;
    }

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", inst_path);
    system(cmd);

    riftprint("Instance '%s' removed.", name);
    return RN_OK;
}

rn_err_t instance_list(void)
{
    const char *dir = rn_instances_dir();
    DIR *d = opendir(dir);
    if (!d) {
        riftprint("No instances found.");
        return RN_OK;
    }

    struct dirent *ent;
    int count = 0;
    riftprint("Instances:");
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            riftprint("  %s", ent->d_name);
            count++;
        }
    }
    closedir(d);

    if (count == 0)
        riftprint("  (none)");

    return RN_OK;
}
