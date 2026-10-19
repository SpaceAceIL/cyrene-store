#include <stdbool.h>
#include <stddef.h>
#include "install.h"
#include <orbis/AppInstUtil.h>

int install_init(void) { return sceAppInstUtilInitialize(); }

int install_pkg(const char* local_pkg_path) {
    return sceAppInstUtilAppInstallPkg(local_pkg_path, NULL);
}
