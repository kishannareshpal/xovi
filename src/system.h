#pragma once

struct XoViEnvironment {
    char *(*getExtensionDirectory)(const char *family);
    const char *(*getConf)(const char *family, const char *value);
    void (*requireExtension)(const char *name, unsigned int versionCode);
};
