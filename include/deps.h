#ifndef DEPS_H
#define DEPS_H

typedef struct {
    char group[128];
    char artifact[128];
    char version[32];
    char local[512];
    char classpath[512];  /* jar to pass to javac/kotlinc */
} Dep;

#define MAX_DEPS 64

typedef struct {
    Dep  deps[MAX_DEPS];
    int  count;
} DepList;

int  deps_parse(const char *path, DepList *list);
int  deps_download(DepList *list);
void deps_classpath(DepList *list, char *out, int outlen);

#endif
