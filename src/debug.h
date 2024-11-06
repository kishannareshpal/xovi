#pragma once
#include <stdio.h>
#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG
#endif

#define LOG_F(...) printf(__VA_ARGS__)
