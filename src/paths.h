// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
#pragma once
#define RN_HOME_ENV    "RIFTNEST_HOME"
#define RN_HOME_DEFAULT ".riftnest"
#define RN_IMAGES_DIR  "images"
#define RN_INST_DIR    "instances"
#define RN_KERNEL_DIR  "kernel"
#define RN_NET_DIR     "net"
#define RN_PROOT_BIN   "proot"
#define RN_QEMU_BIN    "qemu-system-x86_64"

const char *rn_home(void);
const char *rn_images_dir(void);
const char *rn_instances_dir(void);
const char *rn_kernel_dir(void);
const char *rn_net_dir(void);
