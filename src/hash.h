#pragma once
typedef unsigned long hash_t;
hash_t hashString(char *str);
hash_t hashStringL(char *str, int length);
hash_t hashStringS(char *str, hash_t seed);
