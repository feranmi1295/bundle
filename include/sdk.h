#ifndef SDK_H
#define SDK_H

typedef struct {
    char sdk_root[256];
    char aapt2[512];
    char d8[512];
    char apksigner[512];
    char zipalign[512];
    int  ready;
} SdkInfo;

int sdk_detect(SdkInfo *info);

#endif

int sdk_detect_kotlin(char *kotlinc_path, int pathlen);
