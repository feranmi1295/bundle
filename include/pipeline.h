#ifndef PIPELINE_H
#define PIPELINE_H

#include "resolver.h"
#include "sdk.h"

int pipeline_build(SdkInfo *sdk, BundleConfig *config);
int pipeline_compile_kotlin(const char *android_jar);

#endif
