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
    if (strcmp(cmd, "clean")    == 0) return CMD_CLEAN;
    if (strcmp(cmd, "add")      == 0) return CMD_ADD;
    return CMD_UNKNOWN;
}

void run_command(BundleCommand cmd, char **argv) {
    switch (cmd) {
        case CMD_INIT:     bundle_init();         break;
        case CMD_TEMPLATE: bundle_template(argv); break;
        case CMD_MAKE:     bundle_make(argv);     break;
        case CMD_BUILD:    bundle_build(argv);    break;
        case CMD_INSTALL:  bundle_install();      break;
        case CMD_CLEAN:    bundle_clean();        break;
        case CMD_ADD:      bundle_add(argv);      break;
        default:
            bundle_error("Unknown command: %s\n"
                         "  Run 'bundle' for usage.", argv[1]);
    }
}
