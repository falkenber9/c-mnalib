#pragma once

#include <gmodule.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct device_list_entry {
  char* device_name;
  char* device_path;
} device_list_entry_t;

device_list_entry_t* device_list_entry_create(const char* device_name, const char* device_path);
device_list_entry_t* device_list_entry_copy(device_list_entry_t * entry);
void device_list_entry_destroy(device_list_entry_t *entry);

device_list_entry_t* device_list_unpack_entry(GSList* list);

GSList* enumerate_supported_devices(const char* vendor_id,
                                    const char* model_id,
                                    const char* subsystem,
                                    const char* usb_interface_num);
void enumerate_supported_devices_free(GSList*list);

#ifdef __cplusplus
}
#endif
