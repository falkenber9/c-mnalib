

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "cmnalib/logger.h"
#include "cmnalib/tokenfind.h"
#include "cmnalib/conversion.h"

int conversion_str_to_int(const char* str, int base) {
    errno = 0;
    long long_res =  strtol(str, NULL, base);

    if ((errno == ERANGE && (long_res == LONG_MAX || long_res == LONG_MIN))
            || (errno != 0 && long_res == 0)) {
        ERROR("Error in strtol\n");
        return 0;
    }

    if (long_res > INT_MAX || long_res < INT_MIN) {
        WARNING("Number out of bounds of type int\n");
        return 0;
    }

    return (int)long_res;
}

unsigned long conversion_str_to_ulong(const char* str, int base) {
    errno = 0;
    unsigned long long_res =  strtoul(str, NULL, base);

    if ((errno == ERANGE && (long_res == ULONG_MAX))
            || (errno != 0 && long_res == 0)) {
        ERROR("Error in strtoul\n");
        return 0;
    }

    if (long_res > ULONG_MAX) {
        WARNING("Number out of bounds of type unsigned int\n");
        return 0;
    }

    return (unsigned long)long_res;
}

float conversion_str_to_float(const char* str) {
    errno = 0;
    float float_res =  strtof(str, NULL);

    if ((errno == ERANGE)
            || (errno != 0 && float_res == 0)) {
        ERROR("Error in strtof\n");
        return 0.0;
    }
    return float_res;
}

const char* conversion_duplicate_str(const char* str) {
    char* result = NULL;
    if(str != NULL) {
        int len = strlen(str);
        char* result = calloc(len+1, sizeof(char));
        if(result != NULL) {
            strcpy(result, str);
        }
        else {
            ERROR("Error in calloc\n");
        }
    }
    return result;
}
