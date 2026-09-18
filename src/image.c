#define _POSIX_C_SOURCE 200809L
#include "image.h"
#include "paths.h"
#include "misc/riftprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
static void ensure_dir(const char *path)
{
    mkdir(path, 0755);
}

rn_err_t image_pull(const char *name, const char *url)
{
    if (!name || !name[0] || !url || !url[0])
        return RN_ERR_BADARG;

    char dest[4096];
    snprintf(dest, sizeof(dest), "%s/%s", rn_images_dir(), name);

    struct stat st;
    if (stat(dest, &st) == 0 && S_ISDIR(st.st_mode)) {
        riftprint("ERROR: image '%s' already exists", name);
        return RN_ERR_EXISTS;
    }

    ensure_dir(dest);

    char tmpfile[] = "/tmp/riftnest_pull_XXXXXX";
    int tmpfd = mkstemp(tmpfile);
    if (tmpfd < 0) {
        riftprint("ERROR: failed to create temp file");
        return RN_ERR_GENERIC;
    }
    close(tmpfd);

    riftprint("Downloading %s ...", url);
    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "curl -fsSL '%s' -o '%s'", url, tmpfile);
    int rc = system(cmd);
    if (rc != 0) {
        riftprint("ERROR: download failed (curl returned %d)", rc);
        unlink(tmpfile);
        rmdir(dest);
        return RN_ERR_IMAGE_PULL;
    }

    riftprint("Extracting into %s ...", dest);
    snprintf(cmd, sizeof(cmd), "tar -xf '%s' -C '%s'", tmpfile, dest);
    rc = system(cmd);
    unlink(tmpfile);
    if (rc != 0) {
        riftprint("ERROR: extraction failed");
        rmdir(dest);
        return RN_ERR_IMAGE_EXTRACT;
    }

    riftprint("Image '%s' ready.", name);
    return RN_OK;
}

rn_err_t image_list(void)
{
    const char *dir = rn_images_dir();
    DIR *d = opendir(dir);
    if (!d) {
        riftprint("No images found.");
        return RN_OK;
    }

    struct dirent *ent;
    int count = 0;
    riftprint("Images:");
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

rn_err_t image_remove(const char *name)
{
    if (!name || !name[0])
        return RN_ERR_BADARG;

    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", rn_images_dir(), name);

    struct stat st;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        riftprint("ERROR: no image named '%s'", name);
        return RN_ERR_NOTFOUND;
    }

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
    system(cmd);

    riftprint("Image '%s' removed.", name);
    return RN_OK;
}
