#ifdef AARCH64
#include "aarch64/aarch64.h"
#elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
#include "armv7/armv7.h"
#endif
