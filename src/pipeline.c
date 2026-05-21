#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "pipeline.h"
#include "deps.h"
#include "error.h"
#include "rn.h"

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

/* ── GAP 1: kotlinc now gets dep classpath ───────────────────────────── */
int pipeline_compile_kotlin(const char *android_jar, const char *dep_cp) {
    char kotlinc[512] = {0};

    printf("[Bundle] Checking for Kotlin sources...\n");

    FILE *fp = popen("find app/src -name *.kt 2>/dev/null | head -1", "r");
    if (!fp) return 0;
    char found[256] = {0};
    if (fgets(found, sizeof(found), fp))
        found[strcspn(found, "\n")] = 0;
    pclose(fp);

    if (strlen(found) == 0) {
        printf("[Bundle] No Kotlin sources found, skipping.\n\n");
        return 0;
    }

    /* detect kotlinc */
    FILE *kp = popen("which kotlinc 2>/dev/null", "r");
    if (!kp) return -1;
    if (fgets(kotlinc, sizeof(kotlinc), kp))
        kotlinc[strcspn(kotlinc, "\n")] = 0;
    pclose(kp);

    if (strlen(kotlinc) == 0) {
        fprintf(stderr,
            "\n[Bundle Error] Kotlin sources found but kotlinc not installed.\n"
            "  -> Install: sudo snap install kotlin --classic\n\n");
        return -1;
    }

    printf("[Bundle] kotlinc found: %s\n", kotlinc);

    /* build full classpath including deps */
    char full_cp[5120];
    if (dep_cp && strlen(dep_cp) > 0)
        snprintf(full_cp, sizeof(full_cp), "%s:%s", android_jar, dep_cp);
    else
        snprintf(full_cp, sizeof(full_cp), "%s", android_jar);

    char cmd[6144];
    snprintf(cmd, sizeof(cmd),
        "\"%s\" -cp \"%s\" "
        "-d .bundle/build/obj "
        "$(find app/src -name \"*.kt\" 2>/dev/null) 2>&1",
        kotlinc, full_cp);

    printf("[Bundle] $ %s\n", cmd);
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "\n[Bundle Error] Kotlin compilation failed.\n\n");
        return -1;
    }

    printf("[Bundle] Kotlin compiled.\n\n");
    return 0;
}

/* ── GAP 2: extract .so native libs from AAR ────────────────────────── */
static void extract_native_libs(const char *aar_path, const char *artifact) {
    char jni_dir[512];
    snprintf(jni_dir, sizeof(jni_dir), ".bundle/build/jni/%s", artifact);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", jni_dir);
    if (system(cmd) != 0) {}

    /* check if AAR has jni/ folder */
    snprintf(cmd, sizeof(cmd),
        "unzip -l \"%s\" 2>/dev/null | grep -q 'jni/'", aar_path);
    if (system(cmd) != 0) return; /* no native libs */

    /* extract jni/ from AAR */
    snprintf(cmd, sizeof(cmd),
        "unzip -o \"%s\" 'jni/*' -d \"%s\" 2>/dev/null", aar_path, jni_dir);
    if (system(cmd) == 0)
        printf("  -> Extracted native libs (.so) from %s\n", aar_path);
}

int pipeline_build(SdkInfo *sdk, BundleConfig *config, DepList *deps, int release) {
    printf("[Bundle] Starting APK pipeline (%s)...\n\n",
           release ? "RELEASE" : "DEBUG");

    make_dir(".bundle/build/res");
    make_dir(".bundle/build/obj");
    make_dir(".bundle/build/dex");
    make_dir(".bundle/build/apk");
    make_dir(".bundle/build/gen");
    make_dir(".bundle/build/jni");

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

    /* detect project structure - RN vs plain Android */
    int is_rn_structure = (access("android/app/src/main/AndroidManifest.xml", F_OK) == 0);

    char manifest_path[256];
    char res_path[256];
    char java_src[256];

    if (is_rn_structure) {
        /* React Native project - use existing android/ structure */
        snprintf(manifest_path, sizeof(manifest_path),
            "android/app/src/main/AndroidManifest.xml");
        snprintf(res_path, sizeof(res_path),
            "android/app/src/main/res");
        snprintf(java_src, sizeof(java_src),
            "android/app/src/main/java");
        printf("[Bundle/RN] Using existing Android structure.\n\n");
    } else {
        /* plain Bundle project - generate manifest */
        snprintf(manifest_path, sizeof(manifest_path), "app/AndroidManifest.xml");
        snprintf(res_path, sizeof(res_path), "app/res");
        snprintf(java_src, sizeof(java_src), "app/src");

        if (access(manifest_path, F_OK) != 0) {
            printf("[Bundle] Generating AndroidManifest.xml...\n");
            FILE *f = fopen(manifest_path, "w");
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
            printf("[Bundle] Created: %s\n\n", manifest_path);
        }

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
    }

    /* build dep classpath + extract native libs from AARs */
    char dep_cp[4096] = {0};
    deps_classpath(deps, dep_cp, sizeof(dep_cp));

    /* gap 2: extract .so from any AARs */
    for (int i = 0; i < deps->count; i++) {
        if (strstr(deps->deps[i].local, ".aar"))
            extract_native_libs(deps->deps[i].local, deps->deps[i].artifact);
    }

    /* detect React Native and run Metro bundler */
    int is_rn = (strcmp(config->name, "react-native") == 0);
    if (is_rn) {
        printf("[Bundle/RN] React Native project detected.\n\n");
        if (rn_check_node()    != 0) return -1;
        if (rn_install_deps()  != 0) return -1;
        if (rn_bundle_js(!release) != 0) return -1;
    }

    char cmd[6144];
    char full_cp[5120];
    if (strlen(dep_cp) > 0)
        snprintf(full_cp, sizeof(full_cp), "%s:%s", android_jar, dep_cp);
    else
        snprintf(full_cp, sizeof(full_cp), "%s", android_jar);

    /* step 1 - aapt2 compile */
    printf("[Bundle] Step 1/5 - Compiling resources...\n");
    snprintf(cmd, sizeof(cmd),
        "\"%s\" compile --dir \"%s\" -o .bundle/build/res/res.zip 2>&1",
        sdk->aapt2, res_path);
    if (run(cmd) != 0) return -1;
    printf("[Bundle] Resources compiled.\n\n");

    /* step 2 - aapt2 link */
    printf("[Bundle] Step 2/5 - Linking resources...\n");
    /* read applicationId from build.gradle if RN project */
    char pkg_id[128] = "com.bundle.app";
    if (is_rn_structure) {
        FILE *gf = popen(
            "grep applicationId android/app/build.gradle"
            " | awk -F\"\\\"\" '{print $2}'"
            " | tr -d '\n'",
            "r");
        if (gf) {
            char tmp[128] = {0};
            if (fgets(tmp, sizeof(tmp), gf) && strlen(tmp) > 3)
                snprintf(pkg_id, sizeof(pkg_id), "%s", tmp);
            pclose(gf);
        }
        pkg_id[strcspn(pkg_id, "\n\r ")] = 0;

        /* inject package attribute into a temp manifest */
        printf("[Bundle] Package ID: %s\n", pkg_id);
        char patch_cmd[1024];
        snprintf(patch_cmd, sizeof(patch_cmd),
            "sed 's/<manifest /<manifest package=\"%s\" /' "
            "android/app/src/main/AndroidManifest.xml "
            "> .bundle/build/AndroidManifest.xml",
            pkg_id);
        if (system(patch_cmd) == 0)
            snprintf(manifest_path, sizeof(manifest_path),
                ".bundle/build/AndroidManifest.xml");
    }

    /* find androidx AARs from gradle cache for -I flags */
    char aapt2_extra_I[4096] = {0};
    if (is_rn_structure) {
        FILE *aar_fp = popen(
            "find $HOME/.gradle/caches -name appcompat-1.7.1.aar"
            " -o -name appcompat-resources-1.7.1.aar 2>/dev/null | sort -u",
            "r");
        if (aar_fp) {
            char aar_line[512];
            while (fgets(aar_line, sizeof(aar_line), aar_fp)) {
                aar_line[strcspn(aar_line, "\n")] = 0;
                if (strlen(aar_line) > 0) {
                    char itmp[600];
                    snprintf(itmp, sizeof(itmp), "-I \"%s\" ", aar_line);
                    strncat(aapt2_extra_I, itmp,
                        sizeof(aapt2_extra_I) - strlen(aapt2_extra_I) - 1);
                }
            }
            pclose(aar_fp);
        }
    }

    snprintf(cmd, sizeof(cmd),
        "\"%s\" link -o .bundle/build/apk/bundle-unaligned.apk "
        "--manifest \"%s\" "
        "-I \"%s\" "
        "%s "
        ".bundle/build/res/res.zip "
        "--java .bundle/build/gen "
        "--min-sdk-version %s "
        "--target-sdk-version %s 2>&1",
        sdk->aapt2, manifest_path, android_jar,
        aapt2_extra_I,
        config->min_sdk, config->target_sdk);
    if (run(cmd) != 0) return -1;
    printf("[Bundle] Resources linked.\n\n");

    /* step 3a - kotlinc with dep classpath */
    if (pipeline_compile_kotlin(android_jar, dep_cp) < 0) return -1;

    /* step 3b - javac with dep classpath */
    printf("[Bundle] Step 3/5 - Compiling Java sources...\n");
    snprintf(cmd, sizeof(cmd),
        "javac -cp \"%s\" -d .bundle/build/obj "
        "$(find \"%s\" .bundle/build/gen -name \"*.java\" 2>/dev/null) 2>&1",
        full_cp, java_src);
    if (run(cmd) != 0) return -1;
    printf("[Bundle] Java compiled.\n\n");

    /* step 4 - d8 with --lib to silence activity warning */
    printf("[Bundle] Step 4/5 - DEX compilation...\n");
    snprintf(cmd, sizeof(cmd),
        "\"%s\" --lib \"%s\" --output .bundle/build/dex "
        "$(find .bundle/build/obj -name \"*.class\" 2>/dev/null) 2>&1",
        sdk->d8, android_jar);
    if (run(cmd) != 0) return -1;
    printf("[Bundle] DEX compiled.\n\n");

    /* add dex to apk */
    snprintf(cmd, sizeof(cmd),
        "zip -j .bundle/build/apk/bundle-unaligned.apk .bundle/build/dex/classes.dex 2>&1");
    if (run(cmd) != 0) return -1;

    /* embed RN JS bundle into APK before aligning */
    if (is_rn) {
        if (rn_embed_bundle(".bundle/build/apk/bundle-unaligned.apk") != 0)
            return -1;
    }

    /* step 5 - zipalign */
    printf("[Bundle] Step 5/5 - Aligning and signing APK...\n");
    snprintf(cmd, sizeof(cmd),
        "\"%s\" -f 4 .bundle/build/apk/bundle-unaligned.apk "
        ".bundle/build/apk/bundle-aligned.apk 2>&1",
        sdk->zipalign);
    if (run(cmd) != 0) return -1;

    /* ── GAP 5: release vs debug signing ─────────────────────────────── */
    if (release) {
        /* check for release keystore */
        if (access(".bundle/release.keystore", F_OK) != 0) {
            printf("[Bundle] No release keystore found.\n");
            printf("[Bundle] Generating release keystore...\n");
            printf("[Bundle] Enter keystore details:\n\n");
            if (system(
                "keytool -genkeypair -v "
                "-keystore .bundle/release.keystore "
                "-alias releasekey "
                "-keyalg RSA -keysize 2048 -validity 10000") != 0) {
                fprintf(stderr,
                    "\n[Bundle Error] Failed to generate release keystore.\n\n");
                return -1;
            }
        }
        printf("[Bundle] Signing with release keystore...\n");
        printf("Enter keystore password: ");
        char ks_pass[64] = {0};
        if (fgets(ks_pass, sizeof(ks_pass), stdin))
            ks_pass[strcspn(ks_pass, "\n")] = 0;

        snprintf(cmd, sizeof(cmd),
            "\"%s\" sign --ks .bundle/release.keystore "
            "--ks-pass pass:%s --key-pass pass:%s "
            "--ks-key-alias releasekey "
            "--out output-release.apk "
            ".bundle/build/apk/bundle-aligned.apk 2>&1",
            sdk->apksigner, ks_pass, ks_pass);
        if (run(cmd) != 0) return -1;
        printf("\n[Bundle] Build complete!\n");
        printf("[Bundle] APK -> output-release.apk\n\n");
    } else {
        /* debug signing */
        if (access(".bundle/debug.keystore", F_OK) != 0) {
            printf("[Bundle] Generating debug keystore...\n");
            if (system(
                "keytool -genkeypair -v -keystore .bundle/debug.keystore "
                "-alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 "
                "-storepass android -keypass android "
                "-dname \"CN=Android Debug,O=Android,C=US\" 2>&1") != 0) {}
        }
        snprintf(cmd, sizeof(cmd),
            "\"%s\" sign --ks .bundle/debug.keystore "
            "--ks-pass pass:android --key-pass pass:android "
            "--out output.apk "
            ".bundle/build/apk/bundle-aligned.apk 2>&1",
            sdk->apksigner);
        if (run(cmd) != 0) return -1;
        printf("\n[Bundle] Build complete!\n");
        printf("[Bundle] APK -> output.apk\n\n");
    }

    return 0;
}
