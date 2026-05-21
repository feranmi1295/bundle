#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "rn.h"
#include "error.h"

static int run(const char *cmd) {
    printf("[Bundle/RN] $ %s\n", cmd);
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "\n[Bundle Error] Command failed (exit %d):\n  %s\n\n", ret, cmd);
        return -1;
    }
    return 0;
}

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

/* ── check node + npm available ─────────────────────────────────────── */
int rn_check_node(void) {
    printf("[Bundle/RN] Checking Node.js...\n");

    FILE *fp = popen("node --version 2>/dev/null", "r");
    if (!fp) goto fail;
    char ver[32] = {0};
    if (fgets(ver, sizeof(ver), fp))
        ver[strcspn(ver, "\n")] = 0;
    pclose(fp);

    if (strlen(ver) == 0) goto fail;
    printf("  -> node  : %s\n", ver);

    fp = popen("npm --version 2>/dev/null", "r");
    if (!fp) goto fail;
    char npm_ver[32] = {0};
    if (fgets(npm_ver, sizeof(npm_ver), fp))
        npm_ver[strcspn(npm_ver, "\n")] = 0;
    pclose(fp);

    printf("  -> npm   : v%s\n", npm_ver);
    return 0;

fail:
    fprintf(stderr,
        "\n[Bundle Error] Node.js not found.\n"
        "  -> Install from: https://nodejs.org\n"
        "  -> Or via nvm: nvm install --lts\n\n");
    return -1;
}

/* ── npm install if node_modules missing ────────────────────────────── */
int rn_install_deps(void) {
    if (file_exists("node_modules")) {
        printf("[Bundle/RN] node_modules found, skipping npm install.\n");
        return 0;
    }

    if (!file_exists("package.json")) {
        fprintf(stderr,
            "\n[Bundle Error] package.json not found.\n"
            "  -> Are you in a React Native project directory?\n\n");
        return -1;
    }

    printf("[Bundle/RN] Installing npm dependencies...\n");
    return run("npm install 2>&1");
}

/* ── run metro bundler ──────────────────────────────────────────────── */
int rn_bundle_js(int dev_mode) {
    printf("[Bundle/RN] Bundling JavaScript (%s)...\n",
           dev_mode ? "DEV" : "RELEASE");

    /* create assets dir */
    if (system("mkdir -p .bundle/build/assets") != 0) {}

    /* detect entry file */
    const char *entry = "index.js";
    if (!file_exists("index.js")) {
        if (file_exists("index.android.js"))
            entry = "index.android.js";
        else {
            fprintf(stderr,
                "\n[Bundle Error] Entry file not found.\n"
                "  -> Expected: index.js or index.android.js\n\n");
            return -1;
        }
    }

    printf("[Bundle/RN] Entry file: %s\n\n", entry);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "npx react-native bundle "
        "--platform android "
        "--dev %s "
        "--entry-file %s "
        "--bundle-output .bundle/build/assets/index.android.bundle "
        "--assets-dest .bundle/build/res 2>&1",
        dev_mode ? "true" : "false",
        entry);

    if (run(cmd) != 0) return -1;

    /* verify bundle was produced */
    if (!file_exists(".bundle/build/assets/index.android.bundle")) {
        fprintf(stderr,
            "\n[Bundle Error] Metro did not produce a bundle.\n"
            "  -> Check your React Native setup.\n\n");
        return -1;
    }

    printf("\n[Bundle/RN] JS bundle ready.\n\n");
    return 0;
}

/* ── embed JS bundle + assets into APK ─────────────────────────────── */
int rn_embed_bundle(const char *apk_path) {
    printf("[Bundle/RN] Embedding JS bundle into APK...\n");

    char cmd[1024];

    /* add index.android.bundle to APK assets/ */
    snprintf(cmd, sizeof(cmd),
        "zip -u \"%s\" .bundle/build/assets/index.android.bundle 2>&1",
        apk_path);
    if (run(cmd) != 0) return -1;

    /* add any image/font assets if they exist */
    if (file_exists(".bundle/build/res/drawable-mdpi") ||
        file_exists(".bundle/build/res/drawable-hdpi")) {
        snprintf(cmd, sizeof(cmd),
            "cd .bundle/build && "
            "find res -name '*.png' -o -name '*.jpg' -o -name '*.ttf' "
            "2>/dev/null | xargs zip -u \"%s\" 2>/dev/null || true",
            apk_path);
        if (system(cmd) != 0) {}
        printf("[Bundle/RN] Assets embedded.\n");
    }

    printf("[Bundle/RN] Bundle embedded successfully.\n\n");
    return 0;
}
