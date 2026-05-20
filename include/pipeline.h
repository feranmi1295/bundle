#ifndef PIPELINE_H
#define PIPELINE_H

#include "resolver.h"
#include "sdk.h"
#include "deps.h"

int pipeline_build(SdkInfo *sdk, BundleConfig *config, DepList *deps, int release);
int pipeline_compile_kotlin(const char *android_jar, const char *dep_cp);

#endif
