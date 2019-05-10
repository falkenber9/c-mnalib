 
#include <stdlib.h>
#include <stdio.h>
#include <gmodule.h>

#include "cmnalib/logger.h"

#include "cmnalib/enumerate.h"
#include "cmnalib/at_sierra_wireless_mc7455.h"
#include "cmnalib/at_sierra_wireless_em7565.h"

void item_function(void* data, void* user_data) {
    device_list_entry_t* entry = data;
    const char* description = (char*)user_data;
    printf("%s\t%s\n", entry->device_name, description);
}

int main(int argc, char** argv) {
    enable_logger = 0;  //quiet

    GSList* list_7455 = sw_mc7455_enumerate_devices();
    if(list_7455 != NULL) {
        g_slist_foreach(list_7455, item_function, "MC7455");
        sw_mc7455_enumerate_devices_free(list_7455);
    }

    GSList* list_7565 = sw_em7565_enumerate_devices();
    if(list_7565 != NULL) {
        g_slist_foreach(list_7565, item_function, "EM7565");
        sw_em7565_enumerate_devices_free(list_7565);
    }


    return EXIT_SUCCESS;
}
