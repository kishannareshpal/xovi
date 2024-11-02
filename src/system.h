#pragma once

#define XOVI_VERSION "0.1.0"

struct XoViEnvironment {
    char *(*getExtensionDirectory)(const char *family);
    void (*requireExtension)(const char *name, unsigned char major, unsigned char minor, unsigned char patch);
};

#define REQUIRE_ENVIRONMENT extern struct XoViEnvironment *Environment
