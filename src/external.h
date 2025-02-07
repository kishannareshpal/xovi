#ifndef XOVI_PUBLIC_API
#define XOVI_PUBLIC_API
#define XOVI_VERSION "0.2.0"
#include <stdbool.h>

typedef union {
    int i;
    bool b;
    struct {
        int sLength;
        char s[];
    };
} XoviMetadataValue;

struct XoviMetadataEntry {
    const char *name;
    char type;
    XoviMetadataValue value;
};

// Public version of the metadata iterator.
struct ExtensionMetadataIterator {
    const char *extensionName;
    const char *functionName;
    const char OPAQUE[];
};

struct XoViEnvironment {
    char *(*getExtensionDirectory)(const char *family);
    void (*requireExtension)(const char *name, unsigned char major, unsigned char minor, unsigned char patch);

    // 0.2.0 API - metadata:
    int (*getExtensionCount)();
    int (*getExtensionNames)(const char **table, int maxCount);
    int (*getExtensionFunctionCount)(const char *key);
    int (*getExtensionFunctionNames)(const char *extension, const char **table, int maxCount);

    int (*getMetadataEntriesCountForFunction)(const char *extension, const char *function, int functionType);
    struct XoviMetadataEntry **(*getMetadataChainForFunction)(const char *extension, const char *function, int functionType);
    struct XoviMetadataEntry *(*getMetadataEntryForFunction)(const char *extension, const char *function, int functionType, const char *metadataEntryName);

    void (*createMetadataSearchingIterator)(struct ExtensionMetadataIterator *iterator, const char *metadataEntryName);
    struct XoviMetadataEntry *(*nextFunctionMetadataEntry)(struct ExtensionMetadataIterator *iterator);
};
#endif
