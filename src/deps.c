#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>
#include "deps.h"
#include "error.h"

static void group_to_path(const char *group, char *out, int outlen) {
    snprintf(out, outlen, "%s", group);
    for (int i = 0; out[i]; i++)
        if (out[i] == '.') out[i] = '/';
}

static void build_url(Dep *dep, char *url, int urllen, const char *ext) {
    char group_path[256];
    group_to_path(dep->group, group_path, sizeof(group_path));
    snprintf(url, urllen,
        "https://repo1.maven.org/maven2/%s/%s/%s/%s-%s.%s",
        group_path, dep->artifact, dep->version,
        dep->artifact, dep->version, ext);
}

static void build_google_url(Dep *dep, char *url, int urllen, const char *ext) {
    char group_path[256];
    group_to_path(dep->group, group_path, sizeof(group_path));
    snprintf(url, urllen,
        "https://dl.google.com/dl/android/maven2/%s/%s/%s/%s-%s.%s",
        group_path, dep->artifact, dep->version,
        dep->artifact, dep->version, ext);
}

static size_t write_cb(void *ptr, size_t size, size_t nmemb, FILE *fp) {
    return fwrite(ptr, size, nmemb, fp);
}

static int curl_download(const char *url, const char *dest) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    FILE *fp = fopen(dest, "wb");
    if (!fp) { curl_easy_cleanup(curl); return -1; }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        remove(dest);
        return -1;
    }
    return 0;
}

/* ── GAP 2: extract classes.jar from AAR ─────────────────────────────── */
static void extract_aar(Dep *dep) {
    if (strstr(dep->local, ".aar") == NULL) return;

    char jar_path[512];
    snprintf(jar_path, sizeof(jar_path),
        ".bundle/deps/%s-%s-classes.jar", dep->artifact, dep->version);

    /* already extracted */
    if (access(jar_path, F_OK) == 0) {
        snprintf(dep->classpath, sizeof(dep->classpath), "%s", jar_path);
        return;
    }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "unzip -p \"%s\" classes.jar > \"%s\" 2>/dev/null",
        dep->local, jar_path);
    if (system(cmd) == 0) {
        printf("  -> Extracted classes.jar from AAR\n");
        snprintf(dep->classpath, sizeof(dep->classpath), "%s", jar_path);
    } else {
        /* fallback to aar itself */
        snprintf(dep->classpath, sizeof(dep->classpath), "%s", dep->local);
    }
}

/* ── GAP 3: proper cache check ───────────────────────────────────────── */
static int is_cached(Dep *dep) {
    /* check aar */
    char aar[512], jar[512];
    snprintf(aar, sizeof(aar), ".bundle/deps/%s-%s.aar", dep->artifact, dep->version);
    snprintf(jar, sizeof(jar), ".bundle/deps/%s-%s.jar", dep->artifact, dep->version);

    if (access(aar, F_OK) == 0) {
        snprintf(dep->local, sizeof(dep->local), "%s", aar);
        extract_aar(dep);
        return 1;
    }
    if (access(jar, F_OK) == 0) {
        snprintf(dep->local, sizeof(dep->local), "%s", jar);
        snprintf(dep->classpath, sizeof(dep->classpath), "%s", jar);
        return 1;
    }
    return 0;
}

static int download_dep(Dep *dep) {
    /* gap 3: check cache properly before downloading */
    if (is_cached(dep)) {
        printf("[Bundle] Cached  : %s-%s\n", dep->artifact, dep->version);
        return 0;
    }

    char url[1024];
    printf("[Bundle] Fetching: %s:%s:%s\n",
           dep->group, dep->artifact, dep->version);

    /* try AAR from Maven Central */
    build_url(dep, url, sizeof(url), "aar");
    snprintf(dep->local, sizeof(dep->local),
        ".bundle/deps/%s-%s.aar", dep->artifact, dep->version);
    if (curl_download(url, dep->local) == 0) {
        printf("  -> OK (aar, Maven Central)\n");
        extract_aar(dep);
        return 0;
    }

    /* try JAR from Maven Central */
    build_url(dep, url, sizeof(url), "jar");
    snprintf(dep->local, sizeof(dep->local),
        ".bundle/deps/%s-%s.jar", dep->artifact, dep->version);
    if (curl_download(url, dep->local) == 0) {
        printf("  -> OK (jar, Maven Central)\n");
        snprintf(dep->classpath, sizeof(dep->classpath), "%s", dep->local);
        return 0;
    }

    /* try AAR from Google Maven */
    build_google_url(dep, url, sizeof(url), "aar");
    snprintf(dep->local, sizeof(dep->local),
        ".bundle/deps/%s-%s.aar", dep->artifact, dep->version);
    if (curl_download(url, dep->local) == 0) {
        printf("  -> OK (aar, Google Maven)\n");
        extract_aar(dep);
        return 0;
    }

    /* try JAR from Google Maven */
    build_google_url(dep, url, sizeof(url), "jar");
    snprintf(dep->local, sizeof(dep->local),
        ".bundle/deps/%s-%s.jar", dep->artifact, dep->version);
    if (curl_download(url, dep->local) == 0) {
        printf("  -> OK (jar, Google Maven)\n");
        snprintf(dep->classpath, sizeof(dep->classpath), "%s", dep->local);
        return 0;
    }

    fprintf(stderr,
        "\n[Bundle Error] Could not download: %s:%s:%s\n"
        "  -> Check group:artifact:version in bundle.nextgen\n"
        "  -> Declare ALL transitive dependencies manually\n\n",
        dep->group, dep->artifact, dep->version);
    return -1;
}

int deps_parse(const char *path, DepList *list) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    list->count = 0;
    char line[256];
    int in_deps = 0;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (line[0] == '#' || line[0] == 0) continue;
        if (strcmp(line, "[dependencies]") == 0) { in_deps = 1; continue; }
        if (line[0] == '[') { in_deps = 0; continue; }
        if (!in_deps) continue;

        char coord[256], ver[32];
        if (sscanf(line, " %255[^=] = \"%31[^\"]\"", coord, ver) != 2 &&
            sscanf(line, " %255[^=] = %31s",          coord, ver) != 2)
            continue;

        char *end = coord + strlen(coord) - 1;
        while (end > coord && *end == ' ') *end-- = 0;

        char *colon = strchr(coord, ':');
        if (!colon) {
            fprintf(stderr,
                "[Bundle Warning] Skipping: %s\n"
                "  -> Format: group:artifact = \"version\"\n", coord);
            continue;
        }
        *colon = 0;

        if (list->count >= MAX_DEPS) break;

        Dep *dep = &list->deps[list->count++];
        char g[128], a[128], v[32];
        snprintf(g, sizeof(g), "%s", coord);
        snprintf(a, sizeof(a), "%s", colon + 1);
        snprintf(v, sizeof(v), "%s", ver);
        memcpy(dep->group,    g, sizeof(dep->group));
        memcpy(dep->artifact, a, sizeof(dep->artifact));
        memcpy(dep->version,  v, sizeof(dep->version));
        dep->local[0]     = 0;
        dep->classpath[0] = 0;
    }

    fclose(f);
    return 0;
}

int deps_download(DepList *list) {
    if (list->count == 0) {
        printf("[Bundle] No dependencies declared.\n");
        return 0;
    }

    mkdir(".bundle",      0755);
    mkdir(".bundle/deps", 0755);

    printf("[Bundle] Resolving %d dependenc%s...\n\n",
           list->count, list->count == 1 ? "y" : "ies");

    for (int i = 0; i < list->count; i++) {
        if (download_dep(&list->deps[i]) != 0) return -1;
    }

    printf("\n[Bundle] All dependencies ready.\n\n");
    return 0;
}

/* ── GAP 1: classpath builder for javac/kotlinc ──────────────────────── */
void deps_classpath(DepList *list, char *out, int outlen) {
    out[0] = 0;
    for (int i = 0; i < list->count; i++) {
        const char *cp = list->deps[i].classpath;
        if (strlen(cp) == 0) continue;
        if (access(cp, F_OK) != 0) continue;
        if (strlen(out) > 0)
            strncat(out, ":", outlen - strlen(out) - 1);
        strncat(out, cp, outlen - strlen(out) - 1);
    }
}
