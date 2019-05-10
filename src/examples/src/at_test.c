#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <termios.h>    // POSIX terminal control definitions

#include "devices/at_sierra_wireless_mc7455.h"
#include "util/logger.h"
#include "util/trace_logger.h"
#include "at/at_interface.h"

#include "util/conversion.h"

const char the_device[] = "/dev/ttyUSB1";
const char the_command[] = "ati";

int main(int argc, char** argv) {

    sw_mc7455_t* modem;
    modem = sw_mc7455_init(the_device);

    if(modem == NULL) {
        return EXIT_FAILURE;
    }



#if 1
    sw_mc7455_gstatus_response_t* status;
    status = sw_mc7455_get_status(modem);
    DEBUG("LTE Bandwidth: %d\n", status->lte_bw_MHz);
    sw_mc7455_free_status(status);
#endif

#if 1
    sw_mc7455_information_response_t* info;
    info = sw_mc7455_get_information(modem);
    DEBUG("IMEI: %s\n", info->imei);
    sw_mc7455_free_information(info);
#endif

    sw_mc7455_lteinfo_response_t* lteinfo;
    lteinfo = sw_mc7455_get_lteinfo(modem);
    //...
    sw_mc7455_free_lteinfo(lteinfo);

    sw_mc7455_response_t ret;
    ret = sw_mc7455_start_gps_default(modem);
    if(ret == SW_RESPONSE_SUCCESS) {
        WARNING("GPS Activated Successfully\n");
    }
    else {
        WARNING("GPS Activation Failed\n");
    }

    sw_mc7455_gpsloc_response_t* gps;
    gps = sw_mc7455_get_gpsloc(modem);
    if(gps != NULL && gps->is_invalid == 0) {
        WARNING("Longitude: %d\n", gps->longitude);
    }
    else {
        WARNING("INVALID GPS Data\n");
    }

    //...
    sw_mc7455_free_get_gpsloc(gps);

    ret = sw_mc7455_stop_gps(modem);
    if(ret == SW_RESPONSE_SUCCESS) {
        WARNING("GPS Stopped Successfully\n");
    }
    else {
        WARNING("GPS Stopped Failed\n");
    }

    sw_mc7455_is_ready(modem);

    //sw_mc7455_reset(modem);

    sw_mc7455_destroy(modem);

    return EXIT_SUCCESS;
}
