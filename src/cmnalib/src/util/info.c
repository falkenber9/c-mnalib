
#include "cmnalib/info.h"
 
void get_version(char* version_string, int buflen) {
    const char version[] = "Testversion";
    int limit = sizeof(version) > buflen ? buflen : sizeof(version);
    limit = limit - 1;
    for(int i=0; i<limit; i++) {
        version_string[i] = version[i];
	}
    version_string[limit] = 0;
}
