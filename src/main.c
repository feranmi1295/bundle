#include <stdio.h>
#include <string.h>
#include "bundle.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Bundle v%s - The Android build system that actually works\n", BUNDLE_VERSION);
        printf("Platform: Linux (x86_64)\n\n");
        printf("Usage:\n");
        printf("  bundle init                        - Initialize project\n");
        printf("  bundle template --blank            - Scaffold a framework\n");
        printf("  bundle add group:artifact version  - Add a dependency\n");
        printf("  bundle make                        - Resolve dependencies\n");
        printf("  bundle make --framework name@ver   - Override framework\n");
        printf("  bundle build                       - Build debug APK\n");
        printf("  bundle build --release             - Build release APK\n");
        printf("  bundle clean                       - Clean build artifacts\n");
        printf("  bundle install                     - Install Bundle system-wide\n");
        return 0;
    }

    BundleCommand cmd = parse_command(argv[1]);
    run_command(cmd, argv);
    return 0;
}
