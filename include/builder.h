#ifndef BUILDER_H
#define BUILDER_H

void bundle_init(void);
void bundle_template(char **argv);
void bundle_make(char **argv);
void bundle_build(char **argv);
void bundle_install(void);
void bundle_clean(void);
void bundle_add(char **argv);

#endif
