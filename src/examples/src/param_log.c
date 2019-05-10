#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "cmnalib/logger.h"

#include "cmnalib/at_sierra_wireless_mc7455.h"

int main(int argc, char** argv) {

    sw_mc7455_t* modem;
    modem = sw_mc7455_init("/dev/ttyUSB3");

    sw_mc7455_gstatus_response_t* ltestatus;
    sw_response_t ret = 0;

    if(modem == NULL) ret = SW_RESPONSE_CRITICAL;

    while(ret < SW_RESPONSE_CRITICAL) {
        ret = sw_mc7455_get_status(modem, &ltestatus);
        ERROR("\n### %d\n", ltestatus->tx_power);
        struct timespec t;
        t.tv_nsec = 200000000;
        t.tv_sec = 0;
        nanosleep(&t, NULL);
    }

    return EXIT_SUCCESS;
}
