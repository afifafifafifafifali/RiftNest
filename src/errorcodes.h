// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
#pragma once
// ENUMS MAKES OUR LIVES EASY
#include <stdint.h>

typedef enum rn_err {
    RN_OK                  = 0,
    RN_ERR_GENERIC         = -1,
    RN_ERR_NOMEM           = -2,
    RN_ERR_NOTFOUND        = -3,
    RN_ERR_EXISTS          = -4,
    RN_ERR_PERM            = -5,
    RN_ERR_BADARG          = -6,

    RN_ERR_QEMU_NOT_FOUND  = 0xA0001,
    RN_ERR_QEMU_TOO_OLD    = 0xA0002,
    RN_ERR_PROOT_NOT_FOUND = 0xA0010,
    RN_ERR_NO_BACKEND      = 0xA0020,
    RN_ERR_IMAGE_PULL      = 0xA0030,
    RN_ERR_IMAGE_EXTRACT   = 0xA0031,
    RN_ERR_INSTANCE_CREATE = 0xA0040,
    RN_ERR_INSTANCE_RUN    = 0xA0041,
    RN_ERR_INSTANCE_EXEC   = 0xA0042,
    RN_ERR_NET_SETUP       = 0xA0050,
    RN_ERR_NET_TEARDOWN    = 0xA0051,
    RN_ERR_9P_MOUNT        = 0xA0060,
    RN_ERR_QEMU_BOOT       = 0xA0061,
} rn_err_t;

const char *rn_err_str(rn_err_t err);
