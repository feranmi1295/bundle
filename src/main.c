#include <stdio.h>
#include <string.h>
#include "bundle.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Bundle v%s - The Android build system that actually works\n\n", BUNDLE_VERSION);
        printf("Usage:\n");
        printf("  bundle init              - Initialize project\n");
        printf("  bundle template --blank  - Scaffold a framework\n");
        printf("  bundle make              - Resolve and check dependencies\n");
        printf("  bundle build             - Build APK\n");
        printf("  bundle install           - Install Bundle to /usr/local/bin\n");
        return 0;
    }

    BundleCommand cmd = parse_command(argv[1]);
    run_command(cmd, argv);
    return 0;
}
