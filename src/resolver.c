#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resolver.h"
#include "error.h"

typedef struct {
    char framework[32];
    char fw_version[16];
    char kotlin[16];
    char ndk[16];
    char agp[16];
} CompatRow;

static CompatRow COMPAT[] = {
    { "react-native", "0.73", "1.9", "25", "8.1" },
    { "react-native", "0.74", "1.9", "25", "8.3" },
    { "react-native", "0.75", "2.0", "26", "8.5" },
    { "flutter",      "3.16", "1.9", "25", "8.1" },
    { "flutter",      "3.19", "1.9", "25", "8.3" },
    { "flutter",      "3.22", "2.0", "26", "8.5" },
    { "kotlin",       "1.9",  "1.9", "25", "8.1" },
    { "kotlin",       "2.0",  "2.0", "26", "8.3" },
    { "java",         "11",   "1.9", "25", "8.1" },
    { "java",         "17",   "2.0", "26", "8.3" },
};

static int COMPAT_COUNT = sizeof(COMPAT) / sizeof(COMPAT[0]);

int parse_nextgen(const char *path, BundleConfig *config) {
    FILE *f = fopen(path, "r");
    if (!f) {
        bundle_error("bundle.nextgen not found. Run 'bundle init' first.");
        return -1;
    }

    memset(config, 0, sizeof(BundleConfig));
    char line[256];

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (line[0] == '#' || line[0] == '[' || line[0] == 0) continue;

        char key[64], val[64];
        if (sscanf(line, " %63[^=] = \"%63[^\"]\"", key, val) == 2 ||
            sscanf(line, " %63[^=] = %63s",          key, val) == 2) {

            char *end = key + strlen(key) - 1;
            while (end > key && *end == ' ') *end-- = 0;

            if      (strcmp(key, "name")       == 0) snprintf(config->name,       64, "%s", val);
            else if (strcmp(key, "version")    == 0) snprintf(config->version,    64, "%s", val);
            else if (strcmp(key, "min_sdk")    == 0) snprintf(config->min_sdk,    64, "%s", val);
            else if (strcmp(key, "target_sdk") == 0) snprintf(config->target_sdk, 64, "%s", val);
            else if (strcmp(key, "ndk")        == 0) snprintf(config->ndk,        64, "%s", val);
            else if (strcmp(key, "kotlin")     == 0) snprintf(config->kotlin,     64, "%s", val);
        }
    }

    fclose(f);
    return 0;
}

int check_compatibility(BundleConfig *config) {
    char fw_ver[64], kt_ver[64], ndk_ver[64];
    snprintf(fw_ver,  64, "%s", config->version);
    snprintf(kt_ver,  64, "%s", config->kotlin);
    snprintf(ndk_ver, 64, "%s", config->ndk);

    char *dot;
    dot = strstr(fw_ver,  ".x"); if (dot) *dot = 0;
    dot = strstr(kt_ver,  ".x"); if (dot) *dot = 0;
    dot = strstr(ndk_ver, ".x"); if (dot) *dot = 0;

    for (int i = 0; i < COMPAT_COUNT; i++) {
        CompatRow *r = &COMPAT[i];

        if (strcmp(r->framework,  config->name) != 0) continue;
        if (strcmp(r->fw_version, fw_ver)       != 0) continue;

        if (strcmp(r->kotlin, kt_ver) != 0) {
            fprintf(stderr,
                "\n[Bundle Error] %s v%s is NOT compatible with Kotlin v%s\n"
                "  -> Required Kotlin : %s.x\n"
                "  -> Fix in bundle.nextgen: kotlin = \"%s.x\"\n\n",
                config->name, fw_ver, kt_ver, r->kotlin, r->kotlin);
            return -1;
        }

        if (strcmp(r->ndk, ndk_ver) != 0) {
            fprintf(stderr,
                "\n[Bundle Error] %s v%s is NOT compatible with NDK v%s\n"
                "  -> Required NDK   : %s.x\n"
                "  -> Fix in bundle.nextgen: ndk = \"%s.x\"\n\n",
                config->name, fw_ver, ndk_ver, r->ndk, r->ndk);
            return -1;
        }

        printf("[Bundle] Compatibility check passed.\n");
        printf("  -> %s v%s + Kotlin %s.x + NDK %s.x + AGP %s.x\n\n",
               config->name, fw_ver, r->kotlin, r->ndk, r->agp);
        return 0;
    }

    fprintf(stderr,
        "\n[Bundle Error] Unknown framework or version: %s v%s\n"
        "  -> Supported: react-native (0.73-0.75), flutter (3.16-3.22),\n"
        "                kotlin (1.9-2.0), java (11, 17)\n\n",
        config->name, fw_ver);
    return -1;
}

void print_config(BundleConfig *config) {
    printf("[Bundle] Loaded bundle.nextgen:\n");
    printf("  framework : %s v%s\n", config->name,       config->version);
    printf("  kotlin    : %s\n",     config->kotlin);
    printf("  ndk       : %s\n",     config->ndk);
    printf("  min_sdk   : %s\n",     config->min_sdk);
    printf("  target_sdk: %s\n\n",   config->target_sdk);
}
