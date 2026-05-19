#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "error.h"

void bundle_error(const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "[Bundle Error] ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}
