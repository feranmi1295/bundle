#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>
#include "deps.h"
#include "error.h"

/* ── URL builder ──────────────────────────────────────────────────────────
   Maven coordinate → URL
   group:artifact:version → https://repo1.maven.org/maven2/group/artifact/version/artifact-version.aar
   Falls back to .jar if .aar not found
─────────────────────────────────────────────────────────────────────────── */
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

/* ── curl write callback ─────────────────────────────────────────────────*/
static size_t write_cb(void *ptr, size_t size, size_t nmemb, FILE *fp) {
    return fwrite(ptr, size, nmemb, fp);
}

/* ── download single file ────────────────────────────────────────────────*/
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
        remove(dest); /* delete partial file */
        return -1;
    }
    return 0;
}

/* ── try maven central then google maven ─────────────────────────────────*/
static int download_dep(Dep *dep) {
    /* check cache first */
    if (access(dep->local, F_OK) == 0) {
        printf("[Bundle] Cached: %s-%s\n", dep->artifact, dep->version);
        return 0;
    }

    char url[1024];
    printf("[Bundle] Downloading: %s:%s:%s\n",
           dep->group, dep->artifact, dep->version);

    /* try AAR from Maven Central */
    build_url(dep, url, sizeof(url), "aar");
    snprintf(dep->local, sizeof(dep->local),
        ".bundle/deps/%s-%s.aar", dep->artifact, dep->version);
    printf("  -> %s\n", url);
    if (curl_download(url, dep->local) == 0) {
        printf("  -> OK (aar, Maven Central)\n");
        return 0;
    }

    /* try JAR from Maven Central */
    build_url(dep, url, sizeof(url), "jar");
    snprintf(dep->local, sizeof(dep->local),
        ".bundle/deps/%s-%s.jar", dep->artifact, dep->version);
    printf("  -> %s\n", url);
    if (curl_download(url, dep->local) == 0) {
        printf("  -> OK (jar, Maven Central)\n");
        return 0;
    }

    /* try AAR from Google Maven */
    build_google_url(dep, url, sizeof(url), "aar");
    snprintf(dep->local, sizeof(dep->local),
        ".bundle/deps/%s-%s.aar", dep->artifact, dep->version);
    printf("  -> %s\n", url);
    if (curl_download(url, dep->local) == 0) {
        printf("  -> OK (aar, Google Maven)\n");
        return 0;
    }

    /* try JAR from Google Maven */
    build_google_url(dep, url, sizeof(url), "jar");
    snprintf(dep->local, sizeof(dep->local),
        ".bundle/deps/%s-%s.jar", dep->artifact, dep->version);
    printf("  -> %s\n", url);
    if (curl_download(url, dep->local) == 0) {
        printf("  -> OK (jar, Google Maven)\n");
        return 0;
    }

    fprintf(stderr,
        "\n[Bundle Error] Could not download: %s:%s:%s\n"
        "  -> Check group:artifact:version in bundle.nextgen\n"
        "  -> Remember: declare ALL transitive deps manually\n\n",
        dep->group, dep->artifact, dep->version);
    return -1;
}

/* ── parse [dependencies] section ────────────────────────────────────────*/
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

        /* format: group:artifact = "version" */
        char coord[256], ver[32];
        if (sscanf(line, " %255[^=] = \"%31[^\"]\"", coord, ver) != 2 &&
            sscanf(line, " %255[^=] = %31s",          coord, ver) != 2)
            continue;

        /* trim trailing spaces from coord */
        char *end = coord + strlen(coord) - 1;
        while (end > coord && *end == ' ') *end-- = 0;

        /* split group:artifact */
        char *colon = strchr(coord, ':');
        if (!colon) {
            fprintf(stderr,
                "[Bundle Warning] Skipping invalid dependency: %s\n"
                "  -> Format must be: group:artifact = \"version\"\n", coord);
            continue;
        }
        *colon = 0;

        if (list->count >= MAX_DEPS) {
            fprintf(stderr, "[Bundle Warning] Max dependencies (%d) reached.\n", MAX_DEPS);
            break;
        }

        Dep *dep = &list->deps[list->count++];
        char g[128], a[128], v[32];
        snprintf(g, sizeof(g), "%s", coord);
        snprintf(a, sizeof(a), "%s", colon + 1);
        snprintf(v, sizeof(v), "%s", ver);
        memcpy(dep->group,    g, sizeof(dep->group));
        memcpy(dep->artifact, a, sizeof(dep->artifact));
        memcpy(dep->version,  v, sizeof(dep->version));
        /* pragma suppresses false-positive restrict warning */
        snprintf(dep->local, sizeof(dep->local),
            ".bundle/deps/%s-%s.aar", a, v);
    }

    fclose(f);
    return 0;
}

/* ── download all ────────────────────────────────────────────────────────*/
int deps_download(DepList *list) {
    if (list->count == 0) {
        printf("[Bundle] No dependencies declared.\n");
        return 0;
    }

    /* ensure cache dir exists */
    mkdir(".bundle", 0755);
    mkdir(".bundle/deps", 0755);

    printf("[Bundle] Downloading %d dependenc%s...\n\n",
           list->count, list->count == 1 ? "y" : "ies");

    for (int i = 0; i < list->count; i++) {
        if (download_dep(&list->deps[i]) != 0) return -1;
    }

    printf("\n[Bundle] All dependencies ready.\n");
    return 0;
}

/* ── classpath builder ───────────────────────────────────────────────────*/
void deps_classpath(DepList *list, char *out, int outlen) {
    out[0] = 0;
    for (int i = 0; i < list->count; i++) {
        if (access(list->deps[i].local, F_OK) != 0) continue;
        if (strlen(out) > 0)
            strncat(out, ":", outlen - strlen(out) - 1);
        strncat(out, list->deps[i].local, outlen - strlen(out) - 1);
    }
}
