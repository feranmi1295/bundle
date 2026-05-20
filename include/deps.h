#ifndef DEPS_H
#define DEPS_H

/* single dependency entry */
typedef struct {
    char group[128];     /* com.squareup.retrofit2 */
    char artifact[128];  /* retrofit               */
    char version[32];    /* 2.9.0                  */
    char local[512];     /* .bundle/deps/retrofit-2.9.0.aar */
} Dep;

#define MAX_DEPS 64

typedef struct {
    Dep  deps[MAX_DEPS];
    int  count;
} DepList;

/* parse [dependencies] from bundle.nextgen */
int  deps_parse(const char *path, DepList *list);

/* download all deps to .bundle/deps/ */
int  deps_download(DepList *list);

/* return space-separated classpath string */
void deps_classpath(DepList *list, char *out, int outlen);

#endif
