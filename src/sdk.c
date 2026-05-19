#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "sdk.h"
#include "error.h"

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static const char *find_android_home(void) {
    const char *home = getenv("ANDROID_HOME");
    if (home && file_exists(home)) return home;

    home = getenv("ANDROID_SDK_ROOT");
    if (home && file_exists(home)) return home;

    static char path[512];
    const char *user = getenv("HOME");
    if (!user) return NULL;

    snprintf(path, sizeof(path), "%s/Android/Sdk", user);
    if (file_exists(path)) return path;

    snprintf(path, sizeof(path), "%s/.android/sdk", user);
    if (file_exists(path)) return path;

    return NULL;
}

static int find_tool(const char *sdk, const char *tool, char *out, int outlen) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "ls \"%s/build-tools\" 2>/dev/null | sort -rV | head -1", sdk);

    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;

    char version[64] = {0};
    if (fgets(version, sizeof(version), fp))
        version[strcspn(version, "\n")] = 0;
    pclose(fp);

    if (strlen(version) == 0) return 0;

    /* use snprintf with a large enough intermediate buffer */
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s/build-tools/%s/%s", sdk, version, tool);
    if (!file_exists(tmp)) return 0;

    /* safely copy into caller's buffer */
    int len = (int)strlen(tmp);
    if (len >= outlen) return 0;
    memcpy(out, tmp, len + 1);
    return 1;
}

int sdk_detect(SdkInfo *info) {
    memset(info, 0, sizeof(SdkInfo));

    printf("[Bundle] Checking Android SDK...\n");

    const char *home = find_android_home();
    if (!home) {
        fprintf(stderr,
            "\n[Bundle Error] Android SDK not found.\n"
            "  -> Install command-line tools from:\n"
            "     https://developer.android.com/studio#command-tools\n"
            "  -> Then set in your ~/.bashrc:\n"
            "     export ANDROID_HOME=$HOME/Android/Sdk\n"
            "     export PATH=$PATH:$ANDROID_HOME/tools:$ANDROID_HOME/platform-tools\n\n");
        return -1;
    }

    snprintf(info->sdk_root, sizeof(info->sdk_root), "%s", home);
    printf("  -> SDK root   : %s\n", home);

    if (find_tool(home, "aapt2", info->aapt2, sizeof(info->aapt2))) {
        printf("  -> aapt2      : found\n");
    } else {
        fprintf(stderr, "\n[Bundle Error] aapt2 not found.\n"
                        "  -> Run: sdkmanager \"build-tools;34.0.0\"\n\n");
        return -1;
    }

    if (find_tool(home, "d8", info->d8, sizeof(info->d8))) {
        printf("  -> d8         : found\n");
    } else {
        fprintf(stderr, "\n[Bundle Error] d8 not found.\n"
                        "  -> Run: sdkmanager \"build-tools;34.0.0\"\n\n");
        return -1;
    }

    if (find_tool(home, "apksigner", info->apksigner, sizeof(info->apksigner))) {
        printf("  -> apksigner  : found\n");
    } else {
        fprintf(stderr, "\n[Bundle Error] apksigner not found.\n"
                        "  -> Run: sdkmanager \"build-tools;34.0.0\"\n\n");
        return -1;
    }

    if (find_tool(home, "zipalign", info->zipalign, sizeof(info->zipalign))) {
        printf("  -> zipalign   : found\n");
    } else {
        fprintf(stderr, "\n[Bundle Error] zipalign not found.\n"
                        "  -> Run: sdkmanager \"build-tools;34.0.0\"\n\n");
        return -1;
    }

    printf("  -> All tools ready.\n\n");
    info->ready = 1;
    return 0;
}

int sdk_detect_kotlin(char *kotlinc_path, int pathlen) {
    /* check KOTLIN_HOME first */
    const char *kh = getenv("KOTLIN_HOME");
    if (kh) {
        char tmp[512];
        snprintf(tmp, sizeof(tmp), "%s/bin/kotlinc", kh);
        if (access(tmp, X_OK) == 0) {
            snprintf(kotlinc_path, pathlen, "%s", tmp);
            return 1;
        }
    }

    /* check PATH via which */
    FILE *fp = popen("which kotlinc 2>/dev/null", "r");
    if (!fp) return 0;

    char path[512] = {0};
    if (fgets(path, sizeof(path), fp))
        path[strcspn(path, "\n")] = 0;
    pclose(fp);

    if (strlen(path) > 0 && access(path, X_OK) == 0) {
        snprintf(kotlinc_path, pathlen, "%s", path);
        return 1;
    }

    return 0;
}
