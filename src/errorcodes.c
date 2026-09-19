// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
#include "errorcodes.h"

const char *rn_err_str(rn_err_t err)
{
    switch (err) {
    case RN_OK:                  return "success";
    case RN_ERR_GENERIC:         return "generic error";
    case RN_ERR_NOMEM:           return "out of memory";
    case RN_ERR_NOTFOUND:        return "not found";
    case RN_ERR_EXISTS:          return "already exists";
    case RN_ERR_PERM:            return "permission denied";
    case RN_ERR_BADARG:          return "bad argument";
    case RN_ERR_QEMU_NOT_FOUND:  return "QEMU not found (0xA0001)";
    case RN_ERR_QEMU_TOO_OLD:    return "QEMU version too old (0xA0002)";
    case RN_ERR_PROOT_NOT_FOUND: return "proot not found (0xA0010)";
    case RN_ERR_NO_BACKEND:      return "no backend available (0xA0020)";
    case RN_ERR_IMAGE_PULL:      return "image pull failed (0xA0030)";
    case RN_ERR_IMAGE_EXTRACT:   return "image extract failed (0xA0031)";
    case RN_ERR_INSTANCE_CREATE: return "instance creation failed (0xA0040)";
    case RN_ERR_INSTANCE_RUN:    return "instance run failed (0xA0041)";
    case RN_ERR_INSTANCE_EXEC:   return "instance exec failed (0xA0042)";
    case RN_ERR_NET_SETUP:       return "network setup failed (0xA0050)";
    case RN_ERR_NET_TEARDOWN:    return "network teardown failed (0xA0051)";
    case RN_ERR_9P_MOUNT:        return "9p mount failed (0xA0060)";
    case RN_ERR_QEMU_BOOT:       return "QEMU boot failed (0xA0061)";
    }
    return "unknown error";
}
