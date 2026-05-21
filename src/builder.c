#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bundle.h"
#include "builder.h"
#include "resolver.h"
#include "sdk.h"
#include "pipeline.h"
#include "deps.h"
#include "error.h"

static void make_dir(const char *path) {
    if (mkdir(path, 0755) != 0) {
        printf("[Bundle] Directory already exists: %s\n", path);
    } else {
        printf("[Bundle] Created: %s\n", path);
    }
}

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) bundle_error("Could not create file: %s", path);
    fprintf(f, "%s", content);
    fclose(f);
    printf("[Bundle] Created: %s\n", path);
}

void bundle_init(void) {
    printf("[Bundle] Initializing project...\n\n");
    make_dir("app");
    make_dir("app/src");
    make_dir("app/res");
    make_dir("app/res/layout");
    make_dir("app/res/values");
    make_dir(".bundle");
    write_file("bundle.nextgen",
        "[project]\n"
        "name = \"my-app\"\n"
        "version = \"1.0.0\"\n"
        "author = \"\"\n"
        "\n"
        "[framework]\n"
        "name = \"\"\n"
        "version = \"latest\"\n"
        "\n"
        "[android]\n"
        "min_sdk = 21\n"
        "target_sdk = 34\n"
        "ndk = \"25.x\"\n"
        "kotlin = \"1.9.x\"\n"
        "\n"
        "[dependencies]\n"
        "# format: group:artifact = \"version\"\n"
        "# example: com.squareup.retrofit2:retrofit = \"2.9.0\"\n"
    );
    write_file("README.md",
        "# My Bundle App\n\n"
        "Built with Bundle v" BUNDLE_VERSION " - The Android build system that actually works.\n\n"
        "## Commands\n\n"
        "- `bundle template --blank` - Choose your framework\n"
        "- `bundle add group:artifact version` - Add a dependency\n"
        "- `bundle make` - Resolve dependencies\n"
        "- `bundle build` - Build debug APK\n"
        "- `bundle build --release` - Build release APK\n"
        "- `bundle clean` - Clean build artifacts\n"
    );
    printf("\n[Bundle] Project ready. Edit bundle.nextgen to get started.\n");
}

static void scaffold_react_native(void) {
    printf("[Bundle] Scaffolding React Native...\n");
    make_dir("app/src/components");
    make_dir("app/src/screens");
    make_dir("app/src/navigation");
    make_dir("app/src/assets");
    write_file("app/src/index.js",
        "import { AppRegistry } from 'react-native';\n"
        "import App from './App';\n"
        "AppRegistry.registerComponent('MyApp', () => App);\n"
    );
    write_file("app/src/App.js",
        "import React from 'react';\n"
        "import { View, Text, StyleSheet } from 'react-native';\n\n"
        "export default function App() {\n"
        "  return (\n"
        "    <View style={styles.container}>\n"
        "      <Text>Hello from Bundle!</Text>\n"
        "    </View>\n"
        "  );\n"
        "}\n\n"
        "const styles = StyleSheet.create({\n"
        "  container: { flex: 1, justifyContent: 'center', alignItems: 'center' }\n"
        "});\n"
    );
}

static void scaffold_flutter(void) {
    printf("[Bundle] Scaffolding Flutter...\n");
    make_dir("app/src/screens");
    make_dir("app/src/widgets");
    make_dir("app/src/models");
    write_file("app/src/main.dart",
        "import 'package:flutter/material.dart';\n\n"
        "void main() => runApp(const MyApp());\n\n"
        "class MyApp extends StatelessWidget {\n"
        "  const MyApp({super.key});\n"
        "  @override\n"
        "  Widget build(BuildContext context) {\n"
        "    return MaterialApp(\n"
        "      home: Scaffold(\n"
        "        body: Center(child: Text('Hello from Bundle!')),\n"
        "      ),\n"
        "    );\n"
        "  }\n"
        "}\n"
    );
}

static void scaffold_kotlin(void) {
    printf("[Bundle] Scaffolding Kotlin...\n");
    make_dir("app/src/main");
    make_dir("app/src/main/kotlin");
    make_dir("app/src/main/res");
    write_file("app/src/main/kotlin/MainActivity.kt",
        "package com.bundle.app\n\n"
        "import android.os.Bundle\n"
        "import androidx.appcompat.app.AppCompatActivity\n\n"
        "class MainActivity : AppCompatActivity() {\n"
        "    override fun onCreate(savedInstanceState: Bundle?) {\n"
        "        super.onCreate(savedInstanceState)\n"
        "        setContentView(R.layout.activity_main)\n"
        "    }\n"
        "}\n"
    );
}

static void scaffold_java(void) {
    printf("[Bundle] Scaffolding Java...\n");
    make_dir("app/src/main");
    make_dir("app/src/main/java");
    make_dir("app/src/main/res");
    write_file("app/src/main/java/MainActivity.java",
        "package com.bundle.app;\n\n"
        "import android.os.Bundle;\n"
        "import androidx.appcompat.app.AppCompatActivity;\n\n"
        "public class MainActivity extends AppCompatActivity {\n"
        "    @Override\n"
        "    protected void onCreate(Bundle savedInstanceState) {\n"
        "        super.onCreate(savedInstanceState);\n"
        "        setContentView(R.layout.activity_main);\n"
        "    }\n"
        "}\n"
    );
}

void bundle_template(char **argv) {
    int blank = 0;
    if (argv[2] && strcmp(argv[2], "--blank") == 0) blank = 1;
    if (!blank) {
        bundle_error("Use: bundle template --blank");
        return;
    }
    printf("\n[Bundle] Select a framework:\n\n");
    printf("  1. React Native\n");
    printf("  2. Flutter\n");
    printf("  3. Kotlin\n");
    printf("  4. Java\n\n");
    printf("Enter number: ");
    int choice = 0;
    if (scanf("%d", &choice) != 1) choice = 0;
    printf("\n");
    const char *fw_name    = NULL;
    const char *fw_version = NULL;

    switch (choice) {
        case 1:
            scaffold_react_native();
            fw_name = "react-native"; fw_version = "0.73";
            break;
        case 2:
            scaffold_flutter();
            fw_name = "flutter"; fw_version = "3.19";
            break;
        case 3:
            scaffold_kotlin();
            fw_name = "kotlin"; fw_version = "1.9";
            break;
        case 4:
            scaffold_java();
            fw_name = "java"; fw_version = "17";
            break;
        default:
            bundle_error("Invalid choice. Pick 1-4.");
            return;
    }

    /* fix 2: write chosen framework into bundle.nextgen */
    if (fw_name) {
        FILE *f = fopen(BUNDLE_CONFIG, "r");
        if (f) {
            char content[8192] = {0};
            char line[256];
            while (fgets(line, sizeof(line), f))
                strncat(content, line, sizeof(content) - strlen(content) - 1);
            fclose(f);

            /* replace framework name and version */
            char *name_pos = strstr(content, "name = \"\"");
            if (name_pos) {
                char updated[8192] = {0};
                int  prefix_len = (int)(name_pos - content);
                char suffix[4096] = {0};
                snprintf(suffix, sizeof(suffix), "%s",
                    name_pos + strlen("name = \"\""));
                /* skip old version line in suffix */
                char *ver_pos = strstr(suffix, "version = \"latest\"");
                if (ver_pos) {
                    char after_ver[4096] = {0};
                    snprintf(after_ver, sizeof(after_ver), "%s",
                        ver_pos + strlen("version = \"latest\""));
                    snprintf(updated, sizeof(updated),
                        "%.*sname = \"%s\"\nversion = \"%s\"%s",
                        prefix_len, content, fw_name, fw_version, after_ver);
                } else {
                    snprintf(updated, sizeof(updated),
                        "%.*sname = \"%s\"\n%s",
                        prefix_len, content, fw_name, suffix);
                }
                f = fopen(BUNDLE_CONFIG, "w");
                if (f) { fprintf(f, "%s", updated); fclose(f); }
            }
        }
        printf("[Bundle] bundle.nextgen updated: framework = %s v%s\n", fw_name, fw_version);
    }

    printf("\n[Bundle] Framework ready. Run 'bundle make' next.\n");
}

void bundle_make(char **argv) {
    char fw_override[64]  = {0};
    char ver_override[32] = {0};

    for (int i = 2; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "--framework") == 0 && argv[i+1] != NULL) {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%s", argv[i+1]);
            char *at = strchr(tmp, '@');
            if (at) {
                *at = 0;
                snprintf(fw_override,  sizeof(fw_override),  "%s", tmp);
                snprintf(ver_override, sizeof(ver_override), "%s", at + 1);
            } else {
                bundle_error("--framework format: name@version e.g. react-native@0.73");
                return;
            }
            break;
        }
    }

    BundleConfig config;
    if (parse_nextgen(BUNDLE_CONFIG, &config) != 0) return;

    if (strlen(fw_override) > 0) {
        snprintf(config.name,    sizeof(config.name),    "%s", fw_override);
        snprintf(config.version, sizeof(config.version), "%s", ver_override);
        printf("[Bundle] Framework override: %s v%s\n", config.name, config.version);
    }

    print_config(&config);
    if (check_compatibility(&config) != 0) return;
    printf("[Bundle] All checks passed.\n\n");

    DepList deps;
    if (deps_parse(BUNDLE_CONFIG, &deps) != 0) return;
    if (deps_download(&deps) != 0) return;
}

/* ── GAP 3: bundle clean ─────────────────────────────────────────────── */
void bundle_clean(void) {
    printf("[Bundle] Cleaning build artifacts...\n");
    if (system("rm -rf .bundle/build output.apk") != 0) {}
    printf("[Bundle] Cleaned: .bundle/build\n");
    printf("[Bundle] Cleaned: output.apk\n");
    printf("[Bundle] Done. Run 'bundle build' to rebuild.\n");
}

/* ── GAP 4: bundle add ───────────────────────────────────────────────── */
void bundle_add(char **argv) {
    if (!argv[2] || !argv[3]) {
        bundle_error("Usage: bundle add group:artifact version\n"
                     "  e.g: bundle add com.squareup.retrofit2:retrofit 2.9.0");
        return;
    }

    const char *coord   = argv[2];
    const char *version = argv[3];

    /* validate format */
    if (!strchr(coord, ':')) {
        bundle_error("Invalid format: %s\n"
                     "  Must be group:artifact e.g. com.squareup.retrofit2:retrofit", coord);
        return;
    }

    /* read bundle.nextgen */
    FILE *f = fopen(BUNDLE_CONFIG, "r");
    if (!f) {
        bundle_error("bundle.nextgen not found. Run 'bundle init' first.");
        return;
    }

    char content[8192] = {0};
    char line[256];
    while (fgets(line, sizeof(line), f))
        strncat(content, line, sizeof(content) - strlen(content) - 1);
    fclose(f);

    /* check not already present */
    if (strstr(content, coord)) {
        printf("[Bundle] Dependency already exists: %s\n", coord);
        return;
    }

    /* find [dependencies] section and append */
    char new_entry[256];
    snprintf(new_entry, sizeof(new_entry), "%s = \"%s\"\n", coord, version);

    char *dep_section = strstr(content, "[dependencies]");
    if (!dep_section) {
        bundle_error("[dependencies] section not found in bundle.nextgen");
        return;
    }

    /* find end of file and append after [dependencies] */
    char *end = dep_section + strlen(dep_section);
    /* find last newline */
    while (end > dep_section && *(end-1) == '\n') end--;
    *end = 0;

    /* write back */
    f = fopen(BUNDLE_CONFIG, "w");
    if (!f) { bundle_error("Cannot write bundle.nextgen"); return; }
    fprintf(f, "%s\n%s", content, new_entry);
    fclose(f);

    printf("[Bundle] Added: %s = \"%s\"\n", coord, version);
    printf("[Bundle] Run 'bundle make' to download.\n");
}

void bundle_build(char **argv) {
    int release = 0;
    for (int i = 2; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "--release") == 0) { release = 1; break; }
    }

    BundleConfig config;
    if (parse_nextgen(BUNDLE_CONFIG, &config) != 0) return;
    print_config(&config);
    if (check_compatibility(&config) != 0) return;

    SdkInfo sdk;
    if (sdk_detect(&sdk) != 0) return;

    /* fix 3: parse + download deps once, pass to pipeline */
    DepList deps;
    if (deps_parse(BUNDLE_CONFIG, &deps) != 0) return;
    if (deps_download(&deps) != 0) return;

    pipeline_build(&sdk, &config, &deps, release);
}

void bundle_install(void) {
    printf("[Bundle] Installing Bundle to /usr/local/bin...\n");
    char self[512];
    ssize_t len = readlink("/proc/self/exe", self, sizeof(self) - 1);
    if (len < 0) {
        bundle_error("Could not find Bundle binary path.");
        return;
    }
    self[len] = 0;

    if (strstr(self, "/usr/local/bin/bundle") != NULL) {
        printf("[Bundle] Already installed at /usr/local/bin/bundle\n");
        printf("[Bundle] To reinstall: sudo cp ./bundle /usr/local/bin/bundle\n");
        return;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "sudo cp \"%s\" /usr/local/bin/bundle", self);
    if (system(cmd) != 0) {
        bundle_error("Install failed. Try: sudo cp %s /usr/local/bin/bundle", self);
        return;
    }
    printf("[Bundle] Installed successfully.\n");
    printf("[Bundle] Run 'bundle' from anywhere now.\n");
}
