#pragma once 

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int conversion_str_to_int(const char* str, int base);
unsigned long conversion_str_to_ulong(const char* str, int base);
float conversion_str_to_float(const char* str);
const char* conversion_duplicate_str(const char* str);

#ifdef __cplusplus
}
#endif
