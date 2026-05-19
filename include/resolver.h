#ifndef RESOLVER_H
#define RESOLVER_H

typedef struct {
    char name[64];
    char version[64];
    char min_sdk[64];
    char target_sdk[64];
    char ndk[64];
    char kotlin[64];
} BundleConfig;

int  parse_nextgen(const char *path, BundleConfig *config);
int  check_compatibility(BundleConfig *config);
void print_config(BundleConfig *config);

#endif
