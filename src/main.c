#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include "dynamiclinker.h"
#include "system.h"
#define XOVI_ROOT "/home/root/xovi/"
#define EXT_ROOT (XOVI_ROOT "extensions.d/")
#define FILES_ROOT (XOVI_ROOT "exthome/")

#ifndef DEBUGFUNC
#define CONSTRUCTOR __attribute__((constructor))
#define TEST_FUNC
#else
#define CONSTRUCTOR

void _ext_init();
#define TEST_FUNC testFunc()

int testFunc() {
    FILE *string = fopen("/tmp/hello", "r");
    fopen("/tmp/hello", "r");fopen("/tmp/hello", "r");open("/tmp/hello", O_RDONLY);fopen("/tmp/hello", "r");fopen("/tmp/hello", "r");
    printf("Opened: %p\n", string);

    printf("%d\n", string->_fileno);

    fclose(string);
}

int main(){
    _ext_init();
}
#endif

char *findBaseName(const char *fileName) {
    const char *extStart = strchr(fileName, '.');
    if(extStart == NULL) return strdup(fileName); // No '.'? The whole name is baseName
    char *allocd = malloc(extStart - fileName + 1);
    allocd[extStart - fileName] = 0; // NULL-term
    memcpy(allocd, fileName, extStart - fileName);
    return allocd;
}

char *mstrcat(const char *a, const char *b, int additionalSpace, char **end) {
    int aLength = strlen(a), bLength = strlen(b);
    char *c = calloc(1, aLength + bLength + 1);
    memcpy(c, a, aLength);
    memcpy(c + aLength, b, bLength);
    c[aLength + bLength] = 0;
    if(end) {
        *end = &c[aLength + bLength];
    }
    return c;
}

char *findFullName(const char *fileName) {
    return mstrcat(EXT_ROOT, fileName, 0, NULL);
}

char *getExtensionDirectory(const char *family){
    char *slash, *ret = mstrcat(FILES_ROOT, family, 1, &slash);
    slash[0] = '/';
    slash[1] = 0;
    return ret;
}

void CONSTRUCTOR _ext_init() {
    struct XoViEnvironment *environment = malloc(sizeof(struct XoViEnvironment));
    environment->getExtensionDirectory = getExtensionDirectory;

    // At this point none of the functions could have been hooked.
    // It's safe to use stdlib.
    DIR *rootDirOfExtensions;
    struct dirent *entry;

    if((rootDirOfExtensions = opendir(EXT_ROOT)) != NULL){
        // Try to load every extension
        while((entry = readdir(rootDirOfExtensions)) != NULL) {
            if(entry->d_type == DT_REG){
                printf("Loading: %s\n", entry->d_name);
                char *fullName = findFullName(entry->d_name);
                loadExtensionPass1(fullName, findBaseName(entry->d_name));
                free(fullName);
            }
        }
        closedir(rootDirOfExtensions);
        loadAllExtensions(environment);

        TEST_FUNC;

        #ifdef DEBUG
        // Only when we're in debug do we actually clean up in the constructor. Otherwise this program will stay resident for the whole life of the
        // attached process.
        free(environment);
        #endif
    } else {
        printf("Cannot find extensions dir! Bailing!\n");
    }
}

