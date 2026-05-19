#ifndef BUNDLE_H
#define BUNDLE_H

#define BUNDLE_VERSION "0.1.0"
#define BUNDLE_CONFIG  "bundle.nextgen"

typedef enum {
    CMD_INIT,
    CMD_TEMPLATE,
    CMD_MAKE,
    CMD_BUILD,
    CMD_INSTALL,
    CMD_UNKNOWN
} BundleCommand;

BundleCommand parse_command(const char *cmd);
void run_command(BundleCommand cmd, char **argv);

#endif
