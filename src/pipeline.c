#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "pipeline.h"
#include "error.h"

static int run(const char *cmd) {
    printf("[Bundle] $ %s\n", cmd);
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "\n[Bundle Error] Command failed (exit %d):\n  %s\n\n", ret, cmd);
        return -1;
    }
    return 0;
}

static void make_dir(const char *path) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", path);
    if (system(cmd) != 0) {}
}

int pipeline_build(SdkInfo *sdk, BundleConfig *config) {
    printf("[Bundle] Starting APK pipeline...\n\n");

    make_dir(".bundle/build/res");
    make_dir(".bundle/build/obj");
    make_dir(".bundle/build/dex");
    make_dir(".bundle/build/apk");
    make_dir(".bundle/build/gen");

    /* android.jar */
    char android_jar[512];
    snprintf(android_jar, sizeof(android_jar),
        "%s/platforms/android-%s/android.jar",
        sdk->sdk_root, config->target_sdk);

    if (access(android_jar, F_OK) != 0) {
        fprintf(stderr,
            "\n[Bundle Error] android.jar not found: %s\n"
            "  -> Run: sdkmanager \"platforms;android-%s\"\n\n",
            android_jar, config->target_sdk);
        return -1;
    }
    printf("[Bundle] android.jar : %s\n\n", android_jar);

    /* AndroidManifest.xml */
    if (access("app/AndroidManifest.xml", F_OK) != 0) {
        printf("[Bundle] Generating AndroidManifest.xml...\n");
        FILE *f = fopen("app/AndroidManifest.xml", "w");
        if (!f) { bundle_error("Cannot create AndroidManifest.xml"); return -1; }
        fprintf(f,
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"\n"
            "    package=\"com.bundle.app\">\n"
            "    <application\n"
            "        android:label=\"%s\"\n"
            "        android:theme=\"@android:style/Theme.DeviceDefault\">\n"
            "        <activity android:name=\".MainActivity\"\n"
            "            android:exported=\"true\">\n"
            "            <intent-filter>\n"
            "                <action android:name=\"android.intent.action.MAIN\" />\n"
            "                <category android:name=\"android.intent.category.LAUNCHER\" />\n"
            "            </intent-filter>\n"
            "        </activity>\n"
            "    </application>\n"
            "</manifest>\n", config->name);
        fclose(f);
        printf("[Bundle] Created: app/AndroidManifest.xml\n\n");
    }

    /* strings.xml */
    if (access("app/res/values/strings.xml", F_OK) != 0) {
        FILE *f = fopen("app/res/values/strings.xml", "w");
        if (f) {
            fprintf(f,
                "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                "<resources>\n"
                "    <string name=\"app_name\">%s</string>\n"
                "</resources>\n", config->name);
            fclose(f);
        }
    }

    char cmd[2048];

    /* step 1 - aapt2 compile */
    printf("[Bundle] Step 1/5 - Compiling resources...\n");
    snprintf(cmd, sizeof(cmd),
        "\"%s\" compile --dir app/res -o .bundle/build/res/res.zip 2>&1",
        sdk->aapt2);
    if (run(cmd) != 0) return -1;
    printf("[Bundle] Resources compiled.\n\n");

    /* step 2 - aapt2 link */
    printf("[Bundle] Step 2/5 - Linking resources...\n");
    snprintf(cmd, sizeof(cmd),
        "\"%s\" link -o .bundle/build/apk/bundle-unaligned.apk "
        "--manifest app/AndroidManifest.xml "
        "-I \"%s\" "
        ".bundle/build/res/res.zip "
        "--java .bundle/build/gen "
        "--min-sdk-version %s "
        "--target-sdk-version %s 2>&1",
        sdk->aapt2, android_jar, config->min_sdk, config->target_sdk);
    if (run(cmd) != 0) return -1;
    printf("[Bundle] Resources linked.\n\n");

    /* step 3 - javac */
    printf("[Bundle] Step 3/5 - Compiling Java sources...\n");
    snprintf(cmd, sizeof(cmd),
        "javac -cp \"%s\" -d .bundle/build/obj "
        "$(find app/src .bundle/build/gen -name \"*.java\" 2>/dev/null) 2>&1",
        android_jar);
    if (run(cmd) != 0) return -1;
    printf("[Bundle] Java compiled.\n\n");

    /* step 4 - d8 */
    printf("[Bundle] Step 4/5 - DEX compilation...\n");
    snprintf(cmd, sizeof(cmd),
        "\"%s\" --output .bundle/build/dex "
        "$(find .bundle/build/obj -name \"*.class\" 2>/dev/null) 2>&1",
        sdk->d8);
    if (run(cmd) != 0) return -1;
    printf("[Bundle] DEX compiled.\n\n");

    /* add dex to apk */
    snprintf(cmd, sizeof(cmd),
        "cd .bundle/build/dex && zip -u ../apk/bundle-unaligned.apk classes.dex 2>&1");
    if (run(cmd) != 0) return -1;

    /* step 5 - zipalign + sign */
    printf("[Bundle] Step 5/5 - Aligning and signing APK...\n");
    snprintf(cmd, sizeof(cmd),
        "\"%s\" -f 4 .bundle/build/apk/bundle-unaligned.apk "
        ".bundle/build/apk/bundle-aligned.apk 2>&1",
        sdk->zipalign);
    if (run(cmd) != 0) return -1;

    /* debug keystore */
    if (access(".bundle/debug.keystore", F_OK) != 0) {
        printf("[Bundle] Generating debug keystore...\n");
        if (system(
            "keytool -genkeypair -v -keystore .bundle/debug.keystore "
            "-alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 "
            "-storepass android -keypass android "
            "-dname \"CN=Android Debug,O=Android,C=US\" 2>&1") != 0) {}
    }

    /* apksigner */
    snprintf(cmd, sizeof(cmd),
        "\"%s\" sign --ks .bundle/debug.keystore "
        "--ks-pass pass:android --key-pass pass:android "
        "--out output.apk "
        ".bundle/build/apk/bundle-aligned.apk 2>&1",
        sdk->apksigner);
    if (run(cmd) != 0) return -1;

    printf("\n[Bundle] Build complete!\n");
    printf("[Bundle] APK -> output.apk\n\n");
    return 0;
}
