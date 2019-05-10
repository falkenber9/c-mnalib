#include <stdio.h>
#include <string.h>

#include <libudev.h>

#include "cmnalib/device_enumerator.h" 

void de_example() {

}


void de_exampleB() {
    struct udev* udev = udev_new();

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);

    udev_enumerate_add_match_subsystem(enumerate, "tty");                   // tty, block,...
        // properties can be found by e.g. "udevadm info --name=/dev/ttyUSBX"
    udev_enumerate_add_match_property(enumerate, "ID_VENDOR_ID", "1199");
        /* properties can be found by e.g.
         * "udevadm info --name=/dev/ttyUSBX"
         *
         * multiple "add_match_property(...)" will
         * be combined by logical OR. Therefore
         * filter here for ID_VENDOR_ID only and
         * in more detail afterwards.
         */
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *device_list = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *device_item;

    udev_list_entry_foreach(device_item, device_list) {
        const char* device_path = udev_list_entry_get_name(device_item);

        //printf("Checking Device: %s\n", device_path);

        struct udev_device* device = udev_device_new_from_syspath(udev, device_path);
        struct udev_list_entry *property_list = udev_device_get_properties_list_entry(device);

        struct udev_list_entry *property_id_model_id = udev_list_entry_get_by_name(property_list, "ID_MODEL_ID");
        struct udev_list_entry *property_id_usb_interface_num = udev_list_entry_get_by_name(property_list, "ID_USB_INTERFACE_NUM");
        struct udev_list_entry *property_devname = udev_list_entry_get_by_name(property_list, "DEVNAME");
        if(property_id_model_id != NULL && property_id_usb_interface_num != NULL) {
            const char* id_model_id = udev_list_entry_get_value(property_id_model_id);
            const char* devname = udev_list_entry_get_value(property_devname);
            const char* id_usb_interface_num = udev_list_entry_get_value(property_id_usb_interface_num);
            if(id_model_id != NULL && id_usb_interface_num != NULL) {
                if(strstr(id_model_id, "9071")          != NULL &&
                   strstr(id_usb_interface_num, "03")   !=NULL) {
                   printf("Matching device: %s (%s)\n", devname, device_path);
                }
            }
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
}
