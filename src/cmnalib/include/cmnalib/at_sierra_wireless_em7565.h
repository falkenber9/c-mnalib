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
typedef enum sw_em7565_cmd {
#include "cmnalib/at_def_sierra_wireless_em7565.h"
    SW_EM7565_CMD__MAX
} sw_em7565_cmd_t;
#undef ADDCOMMAND

extern const at_interface_command_t sw_em7565_command_defs[SW_EM7565_CMD__MAX];

#define SW_EM7565_ENTERCND_RESPONSE_STRLEN 64

typedef enum sw_em7565_gps_mode {
    GPS_MODE_STANDALONE = 1,
    GPS_MODE_MS_BASED_ONLY = 2,
    GPS_MODE_MS_ASSISTED_ONLY = 3,
} sw_em7565_gps_mode_t;

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


typedef enum sw_em7565_gps_autostart_mode {
    GPS_AUTOSTART_DISABLED = 0,
    GPS_AUTOSTART_ON_BOOT = 1,
    GPS_AUTOSTART_ON_NMEA_PORT_OPEN = 2,
    GPS_AUTOSTART_ERROR = 99,
    GPS_AUTOSTART__MAX
} sw_em7565_gps_autostart_mode_t;

typedef enum sw_em7565_gps_antenna_power_mode {
    GPS_ANTENNA_POWER_NONE = 0,
    GPS_ANTENNA_POWER_3V = 1,
    GPS_ANTENNA_POWER_ERROR = 99,
    GPS_ANTENNA_POWER__MAX
} sw_em7565_gps_antenna_power_mode_t;

#define SW_EM7565_GPS_STATUS_VALUE_STRLEN 64

typedef enum sw_em7565_gps_status_value {
  GPS_STATUS_UNKNOWN = 0,
  GPS_STATUS_NONE,
  GPS_STATUS_ACTIVE,
  GPS_STATUS_SUCCESS,
  GPS_STATUS_FAIL,
  GPS_STATUS__MAX
} sw_em7565_gps_status_value_t;

typedef struct sw_em7565_gps_status {
  sw_em7565_gps_status_value_t last_fix_status;
  int last_fix_status_errcode;
  sw_em7565_gps_status_value_t fix_session_status;
  int fix_session_status_errcode;
} sw_em7565_gps_status_t;

typedef struct {
    at_interface_t* tty;
} sw_em7565_t;

typedef struct {
    int pci;
    float rsrq;
    float rsrp;
    float rssi;
    int rxlv;
} sw_em7565_lteinfo_intrafreq_neighbour_t;

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
} sw_em7565_lteinfo_interfreq_neighbour_t;

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
    sw_em7565_lteinfo_intrafreq_neighbour_t* intrafreq_neighbours;
    int nof_interfreq_neighbours;
    sw_em7565_lteinfo_interfreq_neighbour_t* interfreq_neighbours;
} sw_em7565_lteinfo_response_t;

#define SW_EM7565_GSTATUS_RESPONSE_STRLEN 64
typedef struct {
    int current_time;
    int temperature;
    int reset_counter;
    char mode[SW_EM7565_GSTATUS_RESPONSE_STRLEN];
    char system_mode[SW_EM7565_GSTATUS_RESPONSE_STRLEN];
    char ps_state[SW_EM7565_GSTATUS_RESPONSE_STRLEN];
    int lte_band;
    int lte_bw_MHz;
    int lte_rx_chan;
    int lte_tx_chan;
    char lte_scc1_state[SW_EM7565_GSTATUS_RESPONSE_STRLEN];
    int lte_scc1_band;
    int lte_scc1_bw_MHz;
    int lte_scc1_chan;
    char lte_scc2_state[SW_EM7565_GSTATUS_RESPONSE_STRLEN];
    int lte_scc2_band;
    int lte_scc2_bw_MHz;
    int lte_scc2_chan;
    char lte_scc3_state[SW_EM7565_GSTATUS_RESPONSE_STRLEN];
    int lte_scc3_band;
    int lte_scc3_bw_MHz;
    int lte_scc3_chan;
    char lte_scc4_state[SW_EM7565_GSTATUS_RESPONSE_STRLEN];
    int lte_scc4_band;
    int lte_scc4_bw_MHz;
    int lte_scc4_chan;
    char emm_state[SW_EM7565_GSTATUS_RESPONSE_STRLEN];
    char rrc_state[SW_EM7565_GSTATUS_RESPONSE_STRLEN];
    char ims_reg_state[SW_EM7565_GSTATUS_RESPONSE_STRLEN];

    int pcc_rxm_rssi;
    int pcc_rxm_rsrp;
    int pcc_rxd_rssi;
    int pcc_rxd_rsrp;
    int scc1_rxm_rssi;
    int scc1_rxm_rsrp;
    int scc1_rxd_rssi;
    int scc1_rxd_rsrp;
    int scc2_rxm_rssi;
    int scc2_rxm_rsrp;
    int scc2_rxd_rssi;
    int scc2_rxd_rsrp;
    int scc3_rxm_rssi;
    int scc3_rxm_rsrp;
    int scc3_rxd_rssi;
    int scc3_rxd_rsrp;
    int scc4_rxm_rssi;
    int scc4_rxm_rsrp;
    int scc4_rxd_rssi;
    int scc4_rxd_rsrp;
    int tx_power;
    int tac;
    float rsrq;
    int cell_id;
    float sinr;

} sw_em7565_gstatus_response_t;

#define SW_EM7565_INFORMATION_RESPONSE_STRLEN 128
typedef struct {
    char manufacturer[SW_EM7565_INFORMATION_RESPONSE_STRLEN];
    char model[SW_EM7565_INFORMATION_RESPONSE_STRLEN];
    char revision[SW_EM7565_INFORMATION_RESPONSE_STRLEN];
    char meid[SW_EM7565_INFORMATION_RESPONSE_STRLEN];
    char imei[SW_EM7565_INFORMATION_RESPONSE_STRLEN];
    char imei_sv[SW_EM7565_INFORMATION_RESPONSE_STRLEN];
    char fsn[SW_EM7565_INFORMATION_RESPONSE_STRLEN];
    //missing flags: +GCAP: +CGSM
} sw_em7565_information_response_t;

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
} sw_em7565_gpsloc_response_t;

typedef enum sw_em7565_radio_access_type {
    SW_RAT_AUTOMATIC            = 0x00,
    SW_RAT_UMTS_ONLY            = 0x01,
    SW_RAT_LTE_ONLY             = 0x06,
    SW_RAT_UMTS_AND_LTE_ONLY    = 0x11,
} sw_em7565_radio_access_type_t;

#define SW_EM7565_BAND_CONFIG_NAME_STRLEN 64
typedef struct sw_em7565_band_profile {
    int config_idx;
    char config_name[SW_EM7565_BAND_CONFIG_NAME_STRLEN];
    uint64_t mask_gsm_umts;
    uint64_t mask_lte;
    uint64_t mask_tds;
    uint64_t mask_lte2;
    uint64_t mask_lte3;
    uint64_t mask_lte4;
} sw_em7565_band_profile_t;

#define SW_EM7565_BAND_MAX_NOF_PROFILES 64
typedef struct sw_em7565_band_profile_list {
    int nof_profiles;
    sw_em7565_band_profile_t* profile[SW_EM7565_BAND_MAX_NOF_PROFILES];
} sw_em7565_band_profile_list_t;

typedef enum operator_network_selection_mode{
    COPS_MODE_AUTOMATIC = 0,
    COPS_MODE_MANUAL = 1,
    COPS_MODE_DEREGISTER = 2,
    COPS_MODE_SET_FORMAT = 3,
    COPS_MODE_MANUAL_WITH_AUTOMATIC_FALLBACK = 4,
    COPS_MODE__MAX = 5
} operator_network_selection_mode_t;

typedef enum operator_network_selection_format{
    COPS_FORMAT_LONG_NAME = 0,
    COPS_FORMAT_SHORT_NAME = 1,
    COPS_FORMAT_MCCMNC = 2,
    COPS_FORMAT__MAX = 3
} operator_network_selection_format_t;

typedef enum operator_network_state{
    COPS_STATE_UNKNOWN = 0,
    COPS_STATE_AVAILABLE = 1,
    COPS_STATE_CURRENT = 2,
    COPS_STATE_FORBIDDEN = 3,
    COPS_STATE__MAX = 4
} operator_network_state_t;

typedef enum operator_access_technology{
    COPS_AcT_GSM = 0,
    COPS_AcT_GSM_COMPACT = 1,
    COPS_AcT_UTRAN = 2,
    COPS_AcT_GSM_EGPRS = 3,
    COPS_AcT_UTRAN_HSDPA = 4,
    COPS_AcT_UTRAN_HSUPA = 5,
    COPS_AcT_UTRAN_HSDPA_HSUPA = 6,
    COPS_AcT_EUTRAN = 7,
    COPS_AcT_DONTCARE = 8,
    COPS_AcT__MAX = 9
} operator_access_technology_t;

#define OPERATOR_LONG_NAME_LENGTH 32
#define OPERATOR_SHORT_NAME_LENGTH 32
typedef struct network_list_item {
    operator_network_state_t state;
    char name_long[OPERATOR_LONG_NAME_LENGTH];
    char name_short[OPERATOR_SHORT_NAME_LENGTH];
    int mccmnc;
    operator_access_technology_t access_technology;
} network_list_item_t;

#define NETWORK_MAX_NOF_NETWORKS 32
typedef struct sw_em7565_network_list {
    int nof_networks;
    network_list_item_t* network[NETWORK_MAX_NOF_NETWORKS];
} sw_em7565_network_list_t;

typedef struct sw_em7565_current_operator {
  char name_long[OPERATOR_LONG_NAME_LENGTH];
  //operator_access_technology_t access_technology;
} sw_em7565_current_operator_t;

double sw_em7565_gps_raw_to_double(int32_t int_value);

GSList* sw_em7565_enumerate_devices();
void sw_em7565_enumerate_devices_free(GSList* list);

sw_em7565_t* sw_em7565_init_first();
sw_em7565_t* sw_em7565_init(const char* tty_device_path);
void sw_em7565_destroy(sw_em7565_t* h);

sw_response_t sw_em7565_is_ready(sw_em7565_t* h);
sw_response_t sw_em7565_reset(sw_em7565_t* h);

sw_response_t sw_em7565_set_protected_commands(sw_em7565_t* h, int enable);
sw_response_t sw_em7565_get_protected_commands(sw_em7565_t* h, int* enable);

sw_response_t sw_em7565_select_band_config_profile(sw_em7565_t* h, int config_idx);
sw_response_t sw_em7565_get_band_config_profile(sw_em7565_t* h, sw_em7565_band_profile_t* result);
sw_response_t sw_em7565_add_band_config_profile(sw_em7565_t* h, const sw_em7565_band_profile_t *profile);
sw_response_t sw_em7565_remove_band_config_profile(sw_em7565_t* h, int config_idx);
sw_response_t sw_em7565_get_band_config_profile_list(sw_em7565_t* h, sw_em7565_band_profile_list_t* result);
sw_em7565_band_profile_list_t* sw_em7565_allocate_band_config_profile_list();
void sw_em7565_free_band_config_profile_list(sw_em7565_band_profile_list_t* s);

sw_response_t sw_em7565_get_status(sw_em7565_t* h, sw_em7565_gstatus_response_t *result);
sw_em7565_gstatus_response_t* sw_em7565_allocate_status();
void sw_em7565_free_status(sw_em7565_gstatus_response_t* s);

sw_response_t sw_em7565_get_information(sw_em7565_t* h, sw_em7565_information_response_t *result);
sw_em7565_information_response_t* sw_em7565_allocate_information();
void sw_em7565_free_information(sw_em7565_information_response_t* s);

sw_response_t sw_em7565_get_lteinfo(sw_em7565_t* h, sw_em7565_lteinfo_response_t* result);
sw_em7565_lteinfo_response_t* sw_em7565_allocate_lteinfo();
void sw_em7565_free_lteinfo(sw_em7565_lteinfo_response_t* s);

sw_response_t sw_em7565_get_gps_autostart_mode(sw_em7565_t* h, sw_em7565_gps_autostart_mode_t* autostart_mode);
sw_response_t sw_em7565_set_gps_autostart_mode(sw_em7565_t* h, const sw_em7565_gps_autostart_mode_t autostart_mode);

sw_response_t sw_em7565_gps_status(sw_em7565_t* h, sw_em7565_gps_status_t* status);

sw_response_t sw_em7565_get_gpsloc(sw_em7565_t* h, sw_em7565_gpsloc_response_t* result);
sw_em7565_gpsloc_response_t* sw_em7565_allocate_gpsloc();
void sw_em7565_free_get_gpsloc(sw_em7565_gpsloc_response_t* s);

sw_response_t sw_em7565_stop_gps(sw_em7565_t* h);

sw_response_t sw_em7565_start_gps(sw_em7565_t* h,
                                         sw_em7565_gps_mode_t fix_type,
                                         unsigned char max_fixtime_sec,
                                         unsigned int max_inaccuracy,
                                         int fix_count,
                                         int fix_rate);
sw_response_t sw_em7565_start_gps_default(sw_em7565_t* h);

sw_response_t sw_em7565_set_antenna_power(sw_em7565_t* h,
                                const sw_em7565_gps_antenna_power_mode_t antenna_mode);
sw_em7565_gps_antenna_power_mode_t sw_em7565_get_antenna_power(sw_em7565_t* h);

sw_response_t sw_em7565_set_APN(sw_em7565_t* h, int slot, const char* ip_vers, const char* apn);

sw_response_t sw_em7565_set_data_connection(sw_em7565_t* h,
                                  int data_connection_status);
int sw_em7565_get_data_connection(sw_em7565_t* h);

sw_response_t sw_em7565_set_radio_access_type(sw_em7565_t* h,
                                                     sw_em7565_radio_access_type_t radio_access_type);
sw_response_t sw_em7565_get_radio_access_type(sw_em7565_t* h,
                                                     sw_em7565_radio_access_type_t* radio_access_type);

sw_response_t sw_em7565_network_search(sw_em7565_t* h, sw_em7565_network_list_t* networks);
sw_em7565_network_list_t* sw_em7565_allocate_network_list();
void sw_em7565_free_network_list(sw_em7565_network_list_t* list);

sw_response_t sw_em7565_select_network(sw_em7565_t* h,
                                       const operator_network_selection_mode_t mode,
                                       const operator_network_selection_format_t select_by,
                                       const network_list_item_t *network);

sw_response_t sw_em7565_get_current_operator(sw_em7565_t* h, sw_em7565_current_operator_t* op);
sw_em7565_current_operator_t* sw_em7565_allocate_current_operator();
void sw_em7565_free_current_operator(sw_em7565_current_operator_t* op);

#ifdef __cplusplus
}
#endif
