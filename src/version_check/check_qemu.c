// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
#define _POSIX_C_SOURCE 200809L

#include "check_qemu.h"
#include <stdio.h>
#include <stdlib.h>

int check_qemuavail(void)
{
    FILE *qemu;
    qemu = popen("qemu-system-x86_64 --version 2>&1", "r");
    if (!qemu){
        return -1;
    }
    else{
        pclose(qemu);
        return 0;
    }

}

struct qemu_ver {
    int major;
    int minor;
    int patch;
};

int  check_qemu_ver(void){
    FILE *qemu;
    char out[618];
    qemu = popen("qemu-system-x86_64 --version 2>&1", "r");

    while (fgets(out, sizeof(out), qemu)) {
        struct qemu_ver v;
        for (char *p = out; *p; p++) {
            if (*p < '0' || *p > '9'){
                continue;
            }

            int a, b, c;
            char extra;

            if (sscanf(p, "%d.%d.%d%c", &a, &b, &c, &extra) >= 3) {
                v.major = a;
                v.minor = b;
                v.patch = c;
               
                pclose(qemu);
                
                if(v.major >= 10){
                    return 0;
                }
                else{
                    return 1;
                }
            }
        }
    }
    return -1;
}