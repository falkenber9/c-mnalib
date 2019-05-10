/*
 *
 *
 *
 *
 *   Copyright (C) 2017 Robert Falkenberg <robert.falkenberg@tu-dortmund.de>
 */

#pragma once

#include <stdint.h>
#include <gmodule.h>

#include "cmnalib/at_interface.h"
#include "cmnalib/at_sierra_wireless_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef ADDCOMMAND
#define ADDCOMMAND( _etype, _cmdstr, _sec, _usec) _etype
typedef enum sw_mc7455_cmd {
#include "cmnalib/at_def_sierra_wireless_mc7455.h"
    SW_MC7455_CMD__MAX
} sw_mc7455_cmd_t;
#undef ADDCOMMAND

extern const at_interface_command_t sw_mc7455_command_defs[SW_MC7455_CMD__MAX];

typedef enum sw_mc7455_gps_mode {
    SW_MC7455_GPS_MODE_STANDALONE = 1,
    SW_MC7455_GPS_MODE_MS_BASED_ONLY = 2,
    SW_MC7455_GPS_MODE_MS_ASSISTED_ONLY = 3,
} sw_mc7455_gps_mode_t;

#define GPS_MAX_FIXTIME_MIN 0
#define GPS_MAX_FIXTIME_DEFAULT 30
#define GPS_MAX_FIXTIME_MAX 255

#define GPS_MAX_INACCURACY_MIN 0
#define GPS_MAX_INACCURACY_DEFAULT 1000
#define GPS_MAX_INACCURACY_MAX 4294967279
#define GPS_MAX_INACCURACY_DONT_CARE 4294967280

#define GPS_FIX_COUNT_MIN 1
#define GPS_FIX_COUNT_MAX 999
#define GPS_FIX_COUNT_INFINITE 1000

#define GPS_FIX_RATE_MIN 0
#define GPS_FIX_RATE_DEFAULT 1
#define GPS_FIX_RATE_MAX 1799999

#define APN_SLOT_MIN 1
#define APN_SLOT_DEFAULT 1
#define APN_SLOW_MAX 3

#define APN_IP_VERSION_ANY "IP"
#define APN_IP_VERSION_IPV4 "IPV4"
#define APN_IP_VERSION_IPV4V6 "IPV4V6"

#define DATA_CONNECTION_STATUS_ERROR -1
#define DATA_CONNECTION_STATUS_DISABLED 0
#define DATA_CONNECTION_STATUS_ENABLED 1


typedef enum sw_mc7455_gps_antenna_power_mode {
    SW_MC7455_GPS_ANTENNA_POWER_NONE = 0,
    SW_MC7455_GPS_ANTENNA_POWER_3V = 1,
    SW_MC7455_GPS_ANTENNA_POWER_ERROR = 99,
    SW_MC7455_GPS_ANTENNA_POWER__MAX
} sw_mc7455_gps_antenna_power_mode_t;

typedef struct {
    at_interface_t* tty;
} sw_mc7455_t;

typedef struct {
    int pci;
    float rsrq;
    float rsrp;
    float rssi;
    int rxlv;
} sw_mc7455_lteinfo_intrafreq_neighbour_t;

typedef struct {
    int earfcn;
    int threshold_low;
    int threshold_high;
    int priority;
    int pci;
    float rsrq;
    float rsrp;
    float rssi;
    int rxlv;
} sw_mc7455_lteinfo_interfreq_neighbour_t;

typedef struct {
    int earfn;
    int mcc;
    int mnc;
    int tac;
    int cid;
    int band;
    int d;
    int u;
    int snr;
    int pci;
    float rsrq;
    float rsrp;
    float rssi;
    int rxlv;
    int nof_intrafreq_neighbours;
    sw_mc7455_lteinfo_intrafreq_neighbour_t* intrafreq_neighbours;
    int nof_interfreq_neighbours;
    sw_mc7455_lteinfo_interfreq_neighbour_t* interfreq_neighbours;
} sw_mc7455_lteinfo_response_t;

#define SW_MC7455_GSTATUS_RESPONSE_STRLEN 64
typedef struct {
    int current_time;
    int temperature;
    int reset_counter;
    char mode[SW_MC7455_GSTATUS_RESPONSE_STRLEN];
    char system_mode[SW_MC7455_GSTATUS_RESPONSE_STRLEN];
    char ps_state[SW_MC7455_GSTATUS_RESPONSE_STRLEN];
    int lte_band;
    int lte_bw_MHz;
    int lte_rx_chan;
    int lte_tx_chan;
    char lte_ca_state[SW_MC7455_GSTATUS_RESPONSE_STRLEN];
    int lte_scell_band;
    int lte_scell_bw_MHz;
    int lte_scell_chan;
    char emm_state[SW_MC7455_GSTATUS_RESPONSE_STRLEN];
    char rrc_state[SW_MC7455_GSTATUS_RESPONSE_STRLEN];
    char ims_reg_state[SW_MC7455_GSTATUS_RESPONSE_STRLEN];

    int pcc_rxm_rssi;
    int pcc_rxm_rsrp;
    int pcc_rxd_rssi;
    int pcc_rxd_rsrp;
    int scc_rxm_rssi;
    int scc_rxm_rsrp;
    int scc_rxd_rssi;
    int scc_rxd_rsrp;
    int tx_power;
    int tac;
    float rsrq;
    int cell_id;
    float sinr;

} sw_mc7455_gstatus_response_t;

#define SW_MC7455_INFORMATION_RESPONSE_STRLEN 128
typedef struct {
    char manufacturer[SW_MC7455_INFORMATION_RESPONSE_STRLEN];
    char model[SW_MC7455_INFORMATION_RESPONSE_STRLEN];
    char revision[SW_MC7455_INFORMATION_RESPONSE_STRLEN];
    char meid[SW_MC7455_INFORMATION_RESPONSE_STRLEN];
    char imei[SW_MC7455_INFORMATION_RESPONSE_STRLEN];
    char imei_sv[SW_MC7455_INFORMATION_RESPONSE_STRLEN];
    char fsn[SW_MC7455_INFORMATION_RESPONSE_STRLEN];
    //missing flags: +GCAP: +CGSM
} sw_mc7455_information_response_t;

typedef struct {
    int _raw_latitude;
    int _raw_longitude;
    double latitude;        // canonical representation, e.g. 51.2849584
    double longitude;       // canonical representation, e.g. -4.6948374
    // missing time
    float loc_unc_angle;    // degree
    float loc_unc_a;        // meter
    float loc_unc_p;        // meter
    float hepe;             // meter
    int altitude;           // meter
    float loc_unc_ve;       // meter
    float heading;          // degree
    float velocity_h;       // meter/sec
    float velocity_v;       // meter/sec
    int is_invalid;         // if "Not Available" found in response
} sw_mc7455_gpsloc_response_t;

typedef enum sw_mc7455_radio_access_type {
    SW_MC7455_RAT_AUTOMATIC            = 0x00,
    SW_MC7455_RAT_UMTS_ONLY            = 0x01,
    SW_MC7455_RAT_LTE_ONLY             = 0x06,
    SW_MC7455_RAT_UMTS_AND_LTE_ONLY    = 0x11,
} sw_mc7455_radio_access_type_t;

double sw_mc7455_gps_raw_to_double(int32_t int_value);

GSList* sw_mc7455_enumerate_devices();
void sw_mc7455_enumerate_devices_free(GSList* list);

sw_mc7455_t* sw_mc7455_init_first();
sw_mc7455_t* sw_mc7455_init(const char* tty_device_path);
void sw_mc7455_destroy(sw_mc7455_t* h);

sw_response_t sw_mc7455_is_ready(sw_mc7455_t* h);
sw_response_t sw_mc7455_reset(sw_mc7455_t* h);

sw_response_t sw_mc7455_get_status(sw_mc7455_t* h, sw_mc7455_gstatus_response_t **result);
void sw_mc7455_free_status(sw_mc7455_gstatus_response_t* s);

sw_response_t sw_mc7455_get_information(sw_mc7455_t* h, sw_mc7455_information_response_t** result);
void sw_mc7455_free_information(sw_mc7455_information_response_t* s);

sw_response_t sw_mc7455_get_lteinfo(sw_mc7455_t* h, sw_mc7455_lteinfo_response_t **result);
void sw_mc7455_free_lteinfo(sw_mc7455_lteinfo_response_t* s);

sw_response_t sw_mc7455_get_gpsloc(sw_mc7455_t* h, sw_mc7455_gpsloc_response_t **result);
void sw_mc7455_free_get_gpsloc(sw_mc7455_gpsloc_response_t* s);

sw_response_t sw_mc7455_stop_gps(sw_mc7455_t* h);

sw_response_t sw_mc7455_start_gps(sw_mc7455_t* h,
                                         sw_mc7455_gps_mode_t fix_type,
                                         unsigned char max_fixtime_sec,
                                         unsigned int max_inaccuracy,
                                         int fix_count,
                                         int fix_rate);
sw_response_t sw_mc7455_start_gps_default(sw_mc7455_t* h);

int sw_mc7455_set_antenna_power(sw_mc7455_t* h,
                                sw_mc7455_gps_antenna_power_mode_t antenna_mode);

sw_mc7455_gps_antenna_power_mode_t sw_mc7455_get_antenna_power(sw_mc7455_t* h);

int sw_mc7455_set_APN(sw_mc7455_t* h, int slot, const char* ip_vers, const char* apn);

int sw_mc7455_set_data_connection(sw_mc7455_t* h,
                                  int data_connection_status);
int sw_mc7455_get_data_connection(sw_mc7455_t* h);

sw_response_t sw_mc7455_set_radio_access_type(sw_mc7455_t* h,
                                                     sw_mc7455_radio_access_type_t radio_access_type);
sw_response_t sw_mc7455_get_radio_access_type(sw_mc7455_t* h,
                                                     sw_mc7455_radio_access_type_t* radio_access_type);

#ifdef __cplusplus
}
#endif
