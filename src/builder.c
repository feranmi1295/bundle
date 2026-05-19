#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "bundle.h"
#include "builder.h"
#include "resolver.h"
#include "sdk.h"
#include "pipeline.h"
#include <unistd.h>
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
        "# add your dependencies here\n"
    );
    write_file("README.md",
        "# My Bundle App\n\n"
        "Built with Bundle - The Android build system that actually works.\n\n"
        "## Commands\n\n"
        "- `bundle template --blank` - Choose your framework\n"
        "- `bundle make`             - Resolve dependencies\n"
        "- `bundle build`            - Build APK\n"
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
        bundle_error("No framework specified. Use: bundle template --blank");
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
    switch (choice) {
        case 1: scaffold_react_native(); break;
        case 2: scaffold_flutter();      break;
        case 3: scaffold_kotlin();       break;
        case 4: scaffold_java();         break;
        default:
            bundle_error("Invalid choice. Pick 1-4.");
            return;
    }
    printf("\n[Bundle] Framework ready. Run 'bundle make' next.\n");
}

void bundle_make(char **argv) {
    (void)argv; /* used in --framework flag */
    BundleConfig config;
    if (parse_nextgen(BUNDLE_CONFIG, &config) != 0) return;
    print_config(&config);
    if (check_compatibility(&config) != 0) return;
    printf("[Bundle] All checks passed. Ready to download dependencies.\n");
}

void bundle_build(void) {
    BundleConfig config;
    if (parse_nextgen(BUNDLE_CONFIG, &config) != 0) return;
    print_config(&config);
    if (check_compatibility(&config) != 0) return;

    SdkInfo sdk;
    if (sdk_detect(&sdk) != 0) return;

    pipeline_build(&sdk, &config);
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

    /* already installed at target path */
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
