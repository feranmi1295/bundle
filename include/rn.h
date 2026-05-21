#ifndef RN_H
#define RN_H

int rn_check_node(void);
int rn_install_deps(void);
int rn_bundle_js(int dev_mode);
int rn_embed_bundle(const char *apk_path);

#endif
