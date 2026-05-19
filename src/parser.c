#include <string.h>
#include <stdio.h>
#include "bundle.h"
#include "builder.h"
#include "error.h"

BundleCommand parse_command(const char *cmd) {
    if (strcmp(cmd, "init")     == 0) return CMD_INIT;
    if (strcmp(cmd, "template") == 0) return CMD_TEMPLATE;
    if (strcmp(cmd, "make")     == 0) return CMD_MAKE;
    if (strcmp(cmd, "build")    == 0) return CMD_BUILD;
    if (strcmp(cmd, "install")  == 0) return CMD_INSTALL;
    return CMD_UNKNOWN;
}

void run_command(BundleCommand cmd, char **argv) {
    switch (cmd) {
        case CMD_INIT:     bundle_init();          break;
        case CMD_TEMPLATE: bundle_template(argv);  break;
        case CMD_MAKE:     bundle_make(argv);      break;
        case CMD_BUILD:    bundle_build();         break;
        case CMD_INSTALL:  bundle_install();       break;
        default:
            bundle_error("Unknown command: %s", argv[1]);
    }
}
