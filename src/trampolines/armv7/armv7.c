#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include "../trampolines.h"
#include "armv7.h"

void initCall(struct SymbolData *data) {
    memcpy(data->address, data->beginning_org, ARCHDEP_TRAMPOLINE_LENGTH);
    memcpy(data->address + ARCHDEP_TRAMPOLINE_LENGTH, data->step_2_trampoline, ARCHDEP_UNTRAMPOLINE_LENGTH);
    __builtin___clear_cache(data->address, data->address + ARCHDEP_UNTRAMPOLINE_LENGTH + ARCHDEP_TRAMPOLINE_LENGTH);
}

void finiCall(struct SymbolData *data) {
    memcpy(data->address, data->beginning_trampoline, ARCHDEP_TRAMPOLINE_LENGTH);
    memcpy(data->address + ARCHDEP_TRAMPOLINE_LENGTH, data->beginning_org + ARCHDEP_TRAMPOLINE_LENGTH, ARCHDEP_UNTRAMPOLINE_LENGTH);
    __builtin___clear_cache(data->address, data->address + ARCHDEP_UNTRAMPOLINE_LENGTH + ARCHDEP_TRAMPOLINE_LENGTH);
}
extern void untrampolineStep2(void);

struct SymbolData *pivotSymbol(const char *symbol, void *newaddr) {
    static int pagesize = 0;
    if(pagesize == 0) pagesize = getpagesize();
    void *symboladdr = dlsym(RTLD_NEXT, symbol);
    if(symboladdr == NULL) {
        printf("!! CANNOT FIND %s !!\n", symbol);
        return NULL;
    }

    int is_thumb_func = (uintptr_t)symboladdr & 1;

    struct SymbolData *s = malloc(sizeof(struct SymbolData));

    uint32_t addr2 = (((unsigned int) newaddr) >> 16) & 0xFFFF;
    uint32_t addr1 = (((unsigned int) newaddr) >> 0) & 0xFFFF;

    instr_t trampoline[3];

    if(!is_thumb_func) {
        memcpy(trampoline, (instr_t[]){
            0xE300C000 | (addr1 & 0xFFF) | ((addr1 & 0xF000) << 4), // MOVW R12
            0xE340C000 | (addr2 & 0xFFF) | ((addr2 & 0xF000) << 4), // MOVT R12
            0xE12FFF1C  // BX R12
        }, 3 * sizeof(instr_t));
    } else {
        memcpy(trampoline, (instr_t[]){
            0xF8DFC008, // LDR R12, [PC, #8]
            0x4760BF00, // BX R12; NOP
            (instr_t)newaddr
        }, 3 * sizeof(instr_t));
    }

    uint32_t addr2f = (((unsigned int) s) >> 16) & 0xFFFF;
    uint32_t addr1f = (((unsigned int) s) >> 0) & 0xFFFF;

    instr_t s2trampoline[4];

    if(!is_thumb_func) {
        memcpy(s2trampoline, (instr_t[]){
            0xE300C000 | (addr1f & 0xFFF) | ((addr1f & 0xF000) << 4), // MOVW R12, s[0-15]
            0xE340C000 | (addr2f & 0xFFF) | ((addr2f & 0xF000) << 4), // MOVT R12, s[16-31]

            0xe51ff004, // LDR PC, [PC, #-4]
            (instr_t)untrampolineStep2 // address loaded by previous instruction, never executed
        }, 4 * sizeof(instr_t));
    } else {
        memcpy(s2trampoline, (instr_t[]){
            0xf8dfc00c, // LDR R12, [PC, #12]
            0xf8dff00c, // LDR PC,  [PC, #12]
            (instr_t)untrampolineStep2,
            (instr_t)s // addresses loaded by previous instructions, never executed
        }, 4 * sizeof(instr_t));
    }

    // During the restore-call, there will be 2 trampolines at the start of the function.
    uint8_t *funcstart = malloc(ARCHDEP_TRAMPOLINE_LENGTH + ARCHDEP_UNTRAMPOLINE_LENGTH);
    // Place the beginning of the function into the allocated region
    memcpy(funcstart, symboladdr, ARCHDEP_TRAMPOLINE_LENGTH + ARCHDEP_UNTRAMPOLINE_LENGTH);

    s->address = symboladdr;
    s->beginning_org = funcstart;
    s->page_address = (void*) (((unsigned int) symboladdr) & ~(pagesize - 1));
    s->size = ARCHDEP_TRAMPOLINE_LENGTH + ARCHDEP_UNTRAMPOLINE_LENGTH;
    s->beginning_trampoline = malloc(ARCHDEP_TRAMPOLINE_LENGTH);
    s->step_2_trampoline = malloc(ARCHDEP_UNTRAMPOLINE_LENGTH);
    pthread_mutex_init (&s->mutex, NULL);
    mprotect(s->page_address, pagesize, PROT_READ | PROT_EXEC | PROT_WRITE);
    memcpy(s->beginning_trampoline, trampoline, ARCHDEP_TRAMPOLINE_LENGTH);
    memcpy(s->step_2_trampoline, s2trampoline, ARCHDEP_UNTRAMPOLINE_LENGTH);
    finiCall(s);
    return s;
}

#define CALL(x, ...)    ({                                                                      \
                            init_call(x);                                                       \
                            unsigned int res =                                        \
                                ((unsigned int (*)()) x->address)(__VA_ARGS__);       \
                            fini_call(x);                                                       \
                            res;                                                                \
                        })

int untrampolineInit(struct SymbolData *symbol) {
    pthread_mutex_lock(&symbol->mutex);
    initCall(symbol);
}
int untrampolineFini(struct SymbolData *symbol) {
    finiCall(symbol);
    pthread_mutex_unlock(&symbol->mutex);
}
extern void untrampolineFunction(void);
void generateUntrampoline(void *function, struct SymbolData *symbol, int bytesRemaining) {
    uint32_t addr2 = (((unsigned int) symbol) >> 16) & 0xFFFF;
    uint32_t addr1 = (((unsigned int) symbol) >> 0) & 0xFFFF;

    instr_t trampoline[] = {
        0xE300C000 | (addr1 & 0xFFF) | ((addr1 & 0xF000) << 4), // MOVW R12, s[0-15]
        0xE340C000 | (addr2 & 0xFFF) | ((addr2 & 0xF000) << 4), // MOVT R12, s[16-31]

        0xe51ff004, // LDR PC, [PC, #-4] 
        (instr_t)untrampolineFunction // address loaded by previous instruction, never executed
    };

    if(sizeof(trampoline) > bytesRemaining) {
        printf("[F]: Fatal error - too little space to generate a trampoline!\n");
        exit(1);
    }
    memcpy(function, trampoline, sizeof(trampoline));
}
