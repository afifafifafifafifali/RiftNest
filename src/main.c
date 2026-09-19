// Copyright (c) Afif Ali Saadman 2026. RiftNest containerization protocol
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "backend.h"
#include "image.h"
#include "instance.h"
#include "misc/riftprint.h"

static void usage(void)
{
    riftprint("Usage:");
    riftprint("  riftnest pull <name> <url>          Download and extract a rootfs image");
    riftprint("  riftnest list                        List available images and instances");
    riftprint("  riftnest run <image> [cmd...]        Run a command in a fresh ephemeral instance");
    riftprint("  riftnest create <name> <image>       Create a persistent instance from an image");
    riftprint("  riftnest exec <instance> [cmd...]    Execute into a named persistent instance");
    riftprint("  riftnest destroy <instance>          Delete a persistent instance");
    riftprint("  riftnest rmi <image>                 Remove an image");
    riftprint("  riftnest net up <instance>           Set up networking for an instance");
    riftprint("  riftnest net down <instance>         Tear down networking for an instance");
    riftprint("%s", "");
    riftprint("Environment:");
    riftprint("  RIFTNEST_HOME  Base dir (default: ~/.riftnest)");
    riftprint("%s", "");
    riftprint("Backends:");
riftprint("  proot (default) - ptrace-based isolation, works without root");
riftprint("  qemu            - full VM via QEMU/TCG, requires root for isolation");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 1;
    }

    if (geteuid() != 0) {
        if (strcmp(argv[1], "run") == 0 ||
            strcmp(argv[1], "exec") == 0 ||
            strcmp(argv[1], "create") == 0 ||
            strcmp(argv[1], "net") == 0) {
            riftprint("ERROR: This operation requires root privileges for isolation.");
            riftprint("       Some features work without root: list, pull, rmi, destroy, help");
            return 1;
        }
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        usage();
        return 0;
    }

    if (strcmp(cmd, "list") == 0) {
        rn_backend_t *b = backend_probe_select();
        if (!b) return 1;
        b->list_images();
        b->list_instances();
        return 0;
    }

    if (strcmp(cmd, "pull") == 0) {
        if (argc < 4) {
            riftprint("Usage: riftnest pull <name> <url>");
            return 1;
        }
        rn_backend_t *b = backend_probe_select();
        if (!b) return 1;
        rn_err_t rc = b->pull(argv[2], argv[3]);
        return rc == RN_OK ? 0 : 1;
    }

    if (strcmp(cmd, "run") == 0) {
        if (argc < 3) {
            riftprint("Usage: riftnest run <image> [cmd...]");
            return 1;
        }
        rn_backend_t *b = backend_probe_select();
        if (!b) return 1;
        rn_err_t rc = b->run(argv[2], argc - 3, argv + 3);
        return rc == RN_OK ? 0 : 1;
    }

    if (strcmp(cmd, "create") == 0) {
        if (argc < 4) {
            riftprint("Usage: riftnest create <name> <image>");
            return 1;
        }
        rn_backend_t *b = backend_probe_select();
        if (!b) return 1;
        rn_err_t rc = b->create(argv[2], argv[3]);
        return rc == RN_OK ? 0 : 1;
    }

    if (strcmp(cmd, "exec") == 0) {
        if (argc < 3) {
            riftprint("Usage: riftnest exec <instance> [cmd...]");
            return 1;
        }
        rn_backend_t *b = backend_probe_select();
        if (!b) return 1;
        rn_err_t rc = b->exec(argv[2], argc - 3, argv + 3);
        return rc == RN_OK ? 0 : 1;
    }

    if (strcmp(cmd, "destroy") == 0) {
        if (argc < 3) {
            riftprint("Usage: riftnest destroy <instance>");
            return 1;
        }
        rn_backend_t *b = backend_probe_select();
        if (!b) return 1;
        rn_err_t rc = b->destroy(argv[2]);
        return rc == RN_OK ? 0 : 1;
    }

    if (strcmp(cmd, "rmi") == 0) {
        if (argc < 3) {
            riftprint("Usage: riftnest rmi <image>");
            return 1;
        }
        rn_err_t rc = image_remove(argv[2]);
        return rc == RN_OK ? 0 : 1;
    }

    if (strcmp(cmd, "net") == 0) {
        if (argc < 4) {
            riftprint("Usage: riftnest net up|down <instance>");
            return 1;
        }
        rn_backend_t *b = backend_probe_select();
        if (!b) return 1;

        const char *subcmd = argv[2];
        const char *inst = argv[3];

        if (strcmp(subcmd, "up") == 0) {
            const char *fwd = argc > 4 ? argv[4] : NULL;
            rn_err_t rc = b->net_setup(inst, fwd, NULL);
            return rc == RN_OK ? 0 : 1;
        } else if (strcmp(subcmd, "down") == 0) {
            rn_err_t rc = b->net_teardown(inst);
            return rc == RN_OK ? 0 : 1;
        } else {
            riftprint("Unknown net subcommand: %s", subcmd);
            return 1;
        }
    }

    riftprint("Unknown command: %s", cmd);
    usage();
    return 1;
}
