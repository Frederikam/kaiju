#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "./include/bridge.h" // Hardcoded header
#include "./include/bridge/hooks.h"

void *handle = NULL;
libkaiju_bridge_ExportedSymbols* (*getSymbols)(void);

static const char *get_config_path() {
    const char *gradleBuildSo = "./kaiju-bridge/build/bin/linux/releaseShared/libkaiju_bridge.so";
    const char *homeSo = "~/.kaiju/libkaiju_bridge.so";

    if (access(gradleBuildSo, F_OK) != -1) return gradleBuildSo;
    if (access(homeSo, F_OK) != -1) return homeSo;
    return NULL;
}

void config_load() {
    const char *path = get_config_path();
    if (path == NULL) {
        fprintf(stderr, "Unable to find 'libkaiju_bridge.so'\n");
        exit(-1);
    }

    fprintf(stdout, "Using '%s' as configuration\n", path);
    handle = dlopen(path, RTLD_NOW);
    assert(handle);

    fprintf(stdout, "Loaded SO: '%p'\n", handle);
    getSymbols = (libkaiju_bridge_ExportedSymbols* (*)(void))dlsym(handle, "libkaiju_bridge_symbols");
    struct bridge_hooks* hooks = getSymbols()->kotlin.root.com.frederikam.kaiju.kaijuEntry();
    hooks->onUnload();
}