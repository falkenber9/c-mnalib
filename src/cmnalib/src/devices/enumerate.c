#include <stdlib.h>
#include <string.h>
#include <gmodule.h>

#include <libudev.h>

#include "cmnalib/enumerate.h"
#include "cmnalib/logger.h"

#define GPOINTER_TO_DLE_POINTER(P) ((device_list_entry_t*)(P))
#define DLE_POINTER_TO_GPOINTER(P) ((gpointer)(P))

char* _alloc_copy_string(const char* src) {
    char* result;
    int len = strlen(src)+1;
    result = calloc(len, sizeof(*src));
    strcpy(result, src);
    return result;
}

device_list_entry_t* device_list_entry_create(const char* device_name, const char* device_path) {
    device_list_entry_t* entry = calloc(1, sizeof(*entry));

    entry->device_name = _alloc_copy_string(device_name);
    entry->device_path = _alloc_copy_string(device_path);

    return entry;
}

device_list_entry_t* device_list_entry_copy(device_list_entry_t * entry) {
    return device_list_entry_create(entry->device_name, entry->device_path);
}

void device_list_entry_destroy(device_list_entry_t *entry) {
    free(entry->device_name);
    free(entry->device_path);
    free(entry);
}

void _wrapper_g_destroy_notify(void* entry) {
    device_list_entry_destroy((device_list_entry_t*)entry);
}

device_list_entry_t* device_list_unpack_entry(GSList* list) {
    device_list_entry_t* entry = GPOINTER_TO_DLE_POINTER(list->data);
    return entry;
}

const char* _extract_property(struct udev_list_entry *property_list, const char* property_name) {
    struct udev_list_entry *property = udev_list_entry_get_by_name(property_list, property_name);
    if(property == NULL) return NULL;
    return udev_list_entry_get_value(property);
}

int _match_property(struct udev_list_entry *property_list,
                    const char* property_name,
                    const char* match_value) {
    if(property_list == NULL) return 0;
    if(property_name == NULL || match_value == NULL) return 1;   // no name/value means match

    const char* property_value = _extract_property(property_list, property_name);
    if(property_value == NULL) return 0;    // property not found - mismatch

    return strstr(property_value, match_value) != NULL;         // found = true, otherwise false
}

/* properties can be found by e.g.
 * "udevadm info --name=/dev/ttyUSBX"
 *
 * multiple "add_match_property(...)" will
 * be combined by logical OR. Therefore
 * filter here for ID_VENDOR_ID only and
 * in more detail afterwards.
 */
GSList* enumerate_supported_devices(const char* vendor_id,              // "1199"
                                    const char* model_id,               // "9071"
                                    const char* subsystem,              // "tty", "block",...
                                    const char* usb_interface_num) {    // "03"
    GSList* result = NULL;

    struct udev* udev = udev_new();
    struct udev_enumerate* enumerate = udev_enumerate_new(udev);

    if(subsystem != NULL) udev_enumerate_add_match_subsystem(enumerate, subsystem);
    if(vendor_id != NULL) udev_enumerate_add_match_property(enumerate, "ID_VENDOR_ID", vendor_id);

    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *device_list = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *device_item;

    // Iterate matching devices for further filtering
    udev_list_entry_foreach(device_item, device_list) {
        const char* device_path = udev_list_entry_get_name(device_item);
        DEBUG("Checking Device: %s\n", device_path);

        struct udev_device* device = udev_device_new_from_syspath(udev, device_path);
        struct udev_list_entry *property_list = udev_device_get_properties_list_entry(device);

        // Filter for other properties
        if(_match_property(property_list, "ID_MODEL_ID", model_id) &&
           _match_property(property_list, "ID_USB_INTERFACE_NUM", usb_interface_num)) {

            const char* device_name = _extract_property(property_list, "DEVNAME");
            DEBUG("Matching device: %s (%s)\n", device_name, device_path);

            device_list_entry_t* elem = device_list_entry_create(device_name, device_path);
            result = g_slist_prepend(result, DLE_POINTER_TO_GPOINTER(elem));
        }

#if 0   //PRINT ALL PROPERY=VALUE PAIRS
        struct udev_list_entry *prop;
        udev_list_entry_foreach(prop, property_list) {
            printf("Property: %s=%s\n", udev_list_entry_get_name(prop), udev_list_entry_get_value(prop));
        }
#endif
        udev_device_unref(device);
    }
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return result;
}

void enumerate_supported_devices_free(GSList* list) {
    g_slist_free_full(list, _wrapper_g_destroy_notify);
}
