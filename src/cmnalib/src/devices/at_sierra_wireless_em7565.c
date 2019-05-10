/*
 *
 *
 *
 *
 *   Copyright (C) 2017 Robert Falkenberg <robert.falkenberg@tu-dortmund.de>
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <regex.h>

#include "cmnalib/at_interface.h"
#include "cmnalib/enumerate.h"
#include "cmnalib/at_sierra_wireless_em7565.h"
#include "cmnalib/logger.h"
#include "cmnalib/tokenfind.h"
#include "cmnalib/conversion.h"

#undef LOGGER_LEVEL
#define LOGGER_LEVEL LOGGER_VERBOSE_INFO

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

#undef ADDCOMMAND
#define ADDCOMMAND( _etype, _cmdstr, _sec, _usec) { _etype, _cmdstr, _sec, _usec }
const at_interface_command_t sw_em7565_command_defs[SW_EM7565_CMD__MAX] = {
#include "cmnalib/at_def_sierra_wireless_em7565.h"
};
#undef ADDCOMMAND

#define SW_MC_7565_MAGIC_GPS_FACTOR (10.0/((double)(0x1C71C7)))

/**
 * @brief sw_em7565_gps_raw_to_double Convert raw GPS output of the modem
 * to canonical latitude/longitude representation. The magic factor is
 * obtained by dividing the raw hex output by the textual values received
 * by AT!GPSLOC?
 * @param int_value signed 32bit integer from modem
 * @return double value representation of latitude/longitude
 */
double sw_em7565_gps_raw_to_double(int32_t int_value) {
    double result = (double)int_value * SW_MC_7565_MAGIC_GPS_FACTOR;
    return result;
}

GSList* sw_em7565_enumerate_devices() {
    return enumerate_supported_devices("1199", "9091", "tty", "03");
}

void sw_em7565_enumerate_devices_free(GSList* list) {
    enumerate_supported_devices_free(list);
}

sw_em7565_t* sw_em7565_init_first() {
    sw_em7565_t* result = NULL;
    GSList* list = sw_em7565_enumerate_devices();

    if(list != NULL) {
        device_list_entry_t* entry = device_list_unpack_entry(list);
        result = sw_em7565_init(entry->device_name);
    }
    else {
        ERROR("No supported device found\n");
    }

    sw_em7565_enumerate_devices_free(list);
    return result;
}

sw_em7565_t* sw_em7565_init(const char* tty_device_path) {
    DEBUG("Opening device %s\n", tty_device_path);

    sw_em7565_t* h = calloc(1, sizeof(sw_em7565_t));
    h->tty = at_interface_open(tty_device_path);
    if(h->tty == NULL) {
        free(h);
        ERROR("Initialization failed\n");
        return NULL;
    }
    return h;
}

void sw_em7565_destroy(sw_em7565_t* h) {
    if(h != NULL) {
        at_interface_close(h->tty);
    }
    free(h);
}

sw_response_t sw_em7565_is_ready(sw_em7565_t* h) {
    sw_response_t result = SW_RESPONSE_UNKNOWN;
    at_interface_response_status_t ret;

    DEBUG("Talking to Modem\n");
    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_READY], NULL, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        INFO("Command succeeded\n");
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        WARNING("Command failed with response status %d\n", ret);
        result = SW_RESPONSE_FAILED;
        break;
    default:    // ret >= AT_RESPONSE_CRITICAL
        ERROR("AT interface error\n");
        result = SW_RESPONSE_ERROR;
        break;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_set_protected_commands(sw_em7565_t* h, int enable) {
    sw_response_t result = SW_RESPONSE_UNKNOWN;
    at_interface_response_status_t ret;

    DEBUG("Talking to Modem\n");
    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    at_interface_response_t* response;
    const char* params = enable ? "\"A710\"" : "\"123\"";
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_SET_ENTERCND], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        INFO("Command succeeded\n");
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        WARNING("Command failed with response status %d\n", ret);
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("AT interface error\n");
        result = SW_RESPONSE_ERROR;
        break;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_get_protected_commands(sw_em7565_t* h, int* enable) {
    at_interface_response_status_t ret;
    sw_response_t result = SW_RESPONSE_UNKNOWN;

    *enable = false;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return result;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GET_ENTERCND], NULL, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_SET_ENTERCND].command_string);
        result = SW_RESPONSE_ERROR;
    }

    char pwd[SW_EM7565_ENTERCND_RESPONSE_STRLEN] = {0};
    int val = 0;
    static regex_t* regex_cache;
    val = tokenfind_string_single(response->response_string, pwd, SW_EM7565_ENTERCND_RESPONSE_STRLEN, "\\([[:alnum:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", &regex_cache);

    if(val > 0 && strcmp("A710", pwd) == 0) *enable = true;

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_reset(sw_em7565_t* h) {
    sw_response_t result = SW_RESPONSE_UNKNOWN;
    at_interface_response_status_t ret;

    DEBUG("Resetting Modem\n");
    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_RESET], NULL, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        INFO("Reset command succeeded\n");
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        WARNING("Reset command failed with response status %d\n", ret);
        result = SW_RESPONSE_FAILED;
        break;
    default:    // ret >= AT_RESPONSE_CRITICAL
        ERROR("AT interface error\n");
        result = SW_RESPONSE_ERROR;
        break;
    }

    at_interface_free_response(response);
    return result;
}

void sw_em7565_free_status(sw_em7565_gstatus_response_t* s) {
    if(s != NULL) {
        free(s);
    }
}

sw_em7565_gstatus_response_t* sw_em7565_allocate_status() {
  sw_em7565_gstatus_response_t* result = NULL;
  result = calloc(1, sizeof(sw_em7565_gstatus_response_t));
  if(result != NULL) {
    result->tx_power = SW_GSTATUS_TX_POWER_INACTIVE;
  }
  return result;
}

sw_response_t sw_em7565_get_status(sw_em7565_t* h, sw_em7565_gstatus_response_t* result) {
    at_interface_response_status_t ret;

    if(h == NULL || h->tty == NULL || result == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GSTATUS], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GSTATUS].command_string);
        at_interface_free_response(response);
        return SW_RESPONSE_ERROR;
    }

    static tokenfind_batch_t int_jobs[] = {
        {offsetof(sw_em7565_gstatus_response_t, current_time), "Current Time:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, temperature), "Temperature:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, reset_counter), "Reset Counter:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_band), "LTE band:[[:space:]]\\{1,\\}[[:alpha:]]\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_bw_MHz), "LTE bw:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}MHz", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_rx_chan), "LTE Rx chan:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_tx_chan), "LTE Tx chan:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc1_band), "LTE SSC1 band:[[:space:]]*[[:alpha:]]\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc1_bw_MHz), "LTE SSC1 bw  :[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}MHz", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc1_chan), "LTE SSC1 chan:[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc2_band), "LTE SSC2 band:[[:space:]]*[[:alpha:]]\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc2_bw_MHz), "LTE SSC2 bw  :[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}MHz", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc2_chan), "LTE SSC2 chan:[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc3_band), "LTE SSC3 band:[[:space:]]*[[:alpha:]]\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc3_bw_MHz), "LTE SSC3 bw  :[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}MHz", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc3_chan), "LTE SSC3 chan:[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc4_band), "LTE SSC4 band:[[:space:]]*[[:alpha:]]\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc4_bw_MHz), "LTE SSC4 bw  :[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}MHz", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc4_chan), "LTE SSC4 chan:[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, pcc_rxm_rssi), "PCC RxM RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, pcc_rxm_rsrp), "PCC RxM RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}PCC RxM RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, pcc_rxd_rssi), "PCC RxD RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, pcc_rxd_rsrp), "PCC RxD RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}PCC RxD RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc1_rxm_rssi), "SCC1 RxM RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc1_rxm_rsrp), "SCC1 RxM RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}SCC1 RxM RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc1_rxd_rssi), "SCC1 RxD RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc1_rxd_rsrp), "SCC1 RxD RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}SCC1 RxD RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc2_rxm_rssi), "SCC2 RxM RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc2_rxm_rsrp), "SCC2 RxM RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}SCC2 RxM RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc2_rxd_rssi), "SCC2 RxD RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc2_rxd_rsrp), "SCC2 RxD RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}SCC2 RxD RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc3_rxm_rssi), "SCC3 RxM RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc3_rxm_rsrp), "SCC3 RxM RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}SCC3 RxM RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc3_rxd_rssi), "SCC3 RxD RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc3_rxd_rsrp), "SCC3 RxD RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}SCC3 RxD RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc4_rxm_rssi), "SCC4 RxM RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc4_rxm_rsrp), "SCC4 RxM RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}SCC4 RxM RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc4_rxd_rssi), "SCC4 RxD RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, scc4_rxd_rsrp), "SCC4 RxD RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}SCC4 RxD RSRP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, tx_power), "Tx Power:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, tac), "TAC:[[:space:]]\\{1,\\}[[:xdigit:]]\\{1,\\} (\\([[:digit:]]\\{1,\\}\\))[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_em7565_gstatus_response_t, cell_id), "Cell ID:[[:space:]]\\{1,\\}[[:xdigit:]]\\{1,\\} (\\([[:digit:]]\\{1,\\}\\))[[:space:]]\\{1,\\}", NULL, 10},
    };
    tokenfind_integer_batch(response->response_string, result, int_jobs, NELEMS(int_jobs));

    static tokenfind_batch_t string_jobs[] = {
        {offsetof(sw_em7565_gstatus_response_t, mode), "Mode:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, system_mode), "System mode:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, ps_state), "PS state:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc1_state), "LTE SSC1 state:[[:space:]]\\{0,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc2_state), "LTE SSC2 state:[[:space:]]\\{0,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc3_state), "LTE SSC3 state:[[:space:]]\\{0,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, lte_scc4_state), "LTE SSC4 state:[[:space:]]\\{0,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, emm_state), "EMM state:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, rrc_state), "RRC state:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, ims_reg_state), "IMS reg state:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
    };
    tokenfind_string_batch(response->response_string, result, string_jobs, NELEMS(string_jobs), SW_EM7565_GSTATUS_RESPONSE_STRLEN);

    static tokenfind_batch_t float_jobs[] = {
        {offsetof(sw_em7565_gstatus_response_t, rsrq), "RSRQ (dB):[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gstatus_response_t, sinr), "SINR (dB):[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
    };
    tokenfind_float_batch(response->response_string, result, float_jobs, NELEMS(float_jobs));

    at_interface_free_response(response);

    return SW_RESPONSE_SUCCESS;
}

sw_em7565_information_response_t* sw_em7565_allocate_information() {
  sw_em7565_information_response_t* result = NULL;
  result = calloc(1, sizeof(sw_em7565_information_response_t));
  return result;
}

sw_response_t sw_em7565_get_information(sw_em7565_t* h, sw_em7565_information_response_t* result) {
    at_interface_response_status_t ret;

    if(h == NULL || h->tty == NULL || result == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_INFO], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_INFO].command_string);
        at_interface_free_response(response);
        return SW_RESPONSE_ERROR;
    }

    static tokenfind_batch_t string_jobs[] = {
        {offsetof(sw_em7565_information_response_t, manufacturer), "Manufacturer:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_em7565_information_response_t, model), "Model:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_em7565_information_response_t, revision), "Revision:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_em7565_information_response_t, meid), "MEID:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_em7565_information_response_t, imei), "IMEI:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_em7565_information_response_t, imei_sv), "IMEI SV:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_em7565_information_response_t, fsn), "FSN:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
    };
    tokenfind_string_batch(response->response_string, result, string_jobs, NELEMS(string_jobs), SW_EM7565_INFORMATION_RESPONSE_STRLEN);

    at_interface_free_response(response);

    return SW_RESPONSE_SUCCESS;
}

void sw_em7565_free_information(sw_em7565_information_response_t* s) {
  free(s);
}

sw_em7565_lteinfo_response_t* sw_em7565_allocate_lteinfo() {
  sw_em7565_lteinfo_response_t* result = NULL;
  result = calloc(1, sizeof(sw_em7565_lteinfo_response_t));

  return result;
}

sw_response_t sw_em7565_get_lteinfo(sw_em7565_t* h, sw_em7565_lteinfo_response_t* result) {
    at_interface_response_status_t ret;
    int val = 0;

    if(h == NULL || h->tty == NULL || result == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_LTEINFO], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_LTEINFO].command_string);
        at_interface_free_response(response);
        return SW_RESPONSE_ERROR;
    }

    // parse

    /* slice first part */
    char slice[1024] = {0};
    static regex_t* regex_cache_serving = NULL;
    val = tokenfind_string_single(response->response_string,
                            slice,
                            NELEMS(slice),
                            "Serving:[[:space:]]\\{1,\\}\\(.*\\)IntraFreq:",
                            &regex_cache_serving);
    if(val > 0) {
        tokenfind_string_table_t* serving_info_tbl = tokenfind_parse_table(slice);
        if(serving_info_tbl != NULL && serving_info_tbl->n_rows > 1) {
            (result)->earfn = conversion_str_to_int(serving_info_tbl->row[1].column[0], 10);
            (result)->mcc   = conversion_str_to_int(serving_info_tbl->row[1].column[1], 10);
            (result)->mnc   = conversion_str_to_int(serving_info_tbl->row[1].column[2], 10);
            (result)->tac   = conversion_str_to_int(serving_info_tbl->row[1].column[3], 10);
            (result)->cid   = conversion_str_to_int(serving_info_tbl->row[1].column[4], 16);
            (result)->band  = conversion_str_to_int(serving_info_tbl->row[1].column[5], 10);
            (result)->d     = conversion_str_to_int(serving_info_tbl->row[1].column[6], 10);
            (result)->u     = conversion_str_to_int(serving_info_tbl->row[1].column[7], 10);
            (result)->snr   = conversion_str_to_int(serving_info_tbl->row[1].column[8], 10);
            (result)->pci   = conversion_str_to_int(serving_info_tbl->row[1].column[9], 10);
            (result)->rsrq  = conversion_str_to_float(serving_info_tbl->row[1].column[10]);
            (result)->rsrp  = conversion_str_to_float(serving_info_tbl->row[1].column[11]);
            (result)->rssi  = conversion_str_to_float(serving_info_tbl->row[1].column[12]);
            (result)->rxlv  = conversion_str_to_int(serving_info_tbl->row[1].column[13], 10);
        }
        tokenfind_free_table(serving_info_tbl);
    }

    /* slice second part */
    slice[0] = 0;
    static regex_t* regex_cache_intra = NULL;
    val = tokenfind_string_single(response->response_string,
                            slice,
                            NELEMS(slice),
                            "IntraFreq:[[:space:]]\\{1,\\}\\(.*\\)InterFreq:",
                            &regex_cache_intra);

    if(val > 0) {
        tokenfind_string_table_t* intra_info_tbl = tokenfind_parse_table(slice);
        if(intra_info_tbl != NULL && intra_info_tbl->n_rows > 1) {
            if(result->intrafreq_neighbours != NULL) {
              free(result->intrafreq_neighbours);
            }
            (result)->nof_intrafreq_neighbours = 0;
            (result)->intrafreq_neighbours = calloc((unsigned int)intra_info_tbl->n_rows - 1, sizeof(sw_em7565_lteinfo_intrafreq_neighbour_t));
            for(int row_idx = 1; row_idx < intra_info_tbl->n_rows; row_idx++) {
                (result)->intrafreq_neighbours[(result)->nof_intrafreq_neighbours].pci =
                        conversion_str_to_int(intra_info_tbl->row[row_idx].column[0], 10);
                (result)->intrafreq_neighbours[(result)->nof_intrafreq_neighbours].rsrq =
                        conversion_str_to_float(intra_info_tbl->row[row_idx].column[1]);
                (result)->intrafreq_neighbours[(result)->nof_intrafreq_neighbours].rsrp =
                        conversion_str_to_float(intra_info_tbl->row[row_idx].column[2]);
                (result)->intrafreq_neighbours[(result)->nof_intrafreq_neighbours].rssi =
                        conversion_str_to_float(intra_info_tbl->row[row_idx].column[3]);
                (result)->intrafreq_neighbours[(result)->nof_intrafreq_neighbours].rxlv =
                        conversion_str_to_int(intra_info_tbl->row[row_idx].column[4], 10);

                ((result)->nof_intrafreq_neighbours)++;
            }
        }
        tokenfind_free_table(intra_info_tbl);
    }

    /* slice third part */
    slice[0] = 0;
    static regex_t* regex_cache_inter = NULL;
    val = tokenfind_string_single(response->response_string,
                            slice,
                            NELEMS(slice),
                            "InterFreq:[[:space:]]\\{1,\\}\\(.*\\)WCDMA:",
                            &regex_cache_inter);

    if(val > 0) {
        tokenfind_string_table_t* inter_info_tbl = tokenfind_parse_table(slice);
        if(inter_info_tbl != NULL && inter_info_tbl->n_rows > 1) {
            if(result->interfreq_neighbours != NULL) {
              free(result->interfreq_neighbours);
            }
            (result)->nof_interfreq_neighbours = 0;
            (result)->interfreq_neighbours = calloc((unsigned int)inter_info_tbl->n_rows - 1, sizeof(sw_em7565_lteinfo_interfreq_neighbour_t));
            for(int row_idx = 1; row_idx < inter_info_tbl->n_rows; row_idx++) {
                (result)->interfreq_neighbours[(result)->nof_interfreq_neighbours].earfcn =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[0], 10);
                (result)->interfreq_neighbours[(result)->nof_interfreq_neighbours].threshold_low =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[1], 10);
                (result)->interfreq_neighbours[(result)->nof_interfreq_neighbours].threshold_high =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[2], 10);
                (result)->interfreq_neighbours[(result)->nof_interfreq_neighbours].priority =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[3], 10);
                (result)->interfreq_neighbours[(result)->nof_interfreq_neighbours].pci =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[4], 10);
                (result)->interfreq_neighbours[(result)->nof_interfreq_neighbours].rsrq =
                        conversion_str_to_float(inter_info_tbl->row[row_idx].column[5]);
                (result)->interfreq_neighbours[(result)->nof_interfreq_neighbours].rsrp =
                        conversion_str_to_float(inter_info_tbl->row[row_idx].column[6]);
                (result)->interfreq_neighbours[(result)->nof_interfreq_neighbours].rssi =
                        conversion_str_to_float(inter_info_tbl->row[row_idx].column[7]);
                (result)->interfreq_neighbours[(result)->nof_interfreq_neighbours].rxlv =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[8], 10);

                ((result)->nof_interfreq_neighbours)++;
            }
        }
        tokenfind_free_table(inter_info_tbl);
    }

    at_interface_free_response(response);

    return SW_RESPONSE_SUCCESS;
}

void sw_em7565_free_lteinfo(sw_em7565_lteinfo_response_t* s) {
    if(s != NULL) {
      //clear inter
      if(s->interfreq_neighbours != NULL) {
        free(s->interfreq_neighbours);
        s->interfreq_neighbours = NULL;
      }
      s->nof_interfreq_neighbours = 0;

      //clear intra
      if(s->intrafreq_neighbours != NULL) {
        free(s->intrafreq_neighbours);
        s->intrafreq_neighbours = NULL;
      }
      s->nof_intrafreq_neighbours = 0;
    }
    free(s);
}

sw_em7565_gpsloc_response_t* sw_em7565_allocate_gpsloc() {
  sw_em7565_gpsloc_response_t* result = NULL;
  result = calloc(1, sizeof(sw_em7565_gpsloc_response_t));
  return result;
}

sw_response_t sw_em7565_get_gpsloc(sw_em7565_t* h, sw_em7565_gpsloc_response_t *result) {
    at_interface_response_status_t ret;

    if(h == NULL || h->tty == NULL || result == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GPSLOC], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GPSLOC].command_string);
        at_interface_free_response(response);
        return SW_RESPONSE_ERROR;
    }

//    static regex_t* regex_not_avail;
//    ret = tokenfind_string_single(response->response_string,
//                                  NULL, 0,
//                                  "",
//                                  &regex_not_avail);

    if(strstr(response->response_string, "Not Available")) {
        (result)->is_invalid = 1;
    }
    else {
        (result)->is_invalid = 0;
    }

    static tokenfind_batch_t int_jobs[] = {
        {offsetof(sw_em7565_gpsloc_response_t, _raw_latitude), "Lat:[[:space:]]\\{1,\\}[^(]*(0x\\([[:xdigit:]]\\{1,\\}\\))[[:space:]]\\{1,\\}", NULL, 16},
        {offsetof(sw_em7565_gpsloc_response_t, _raw_longitude), "Lon:[[:space:]]\\{1,\\}[^(]*(0x\\([[:xdigit:]]\\{1,\\}\\))[[:space:]]\\{1,\\}", NULL, 16},
        {offsetof(sw_em7565_gpsloc_response_t, altitude), "Altitude:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
    };
    tokenfind_integer_batch(response->response_string, (result), int_jobs, NELEMS(int_jobs));
    (result)->latitude = sw_em7565_gps_raw_to_double((result)->_raw_latitude);
    (result)->longitude = sw_em7565_gps_raw_to_double((result)->_raw_longitude);

    static tokenfind_batch_t float_jobs[] = {
        {offsetof(sw_em7565_gpsloc_response_t, loc_unc_angle), "LocUncAngle:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gpsloc_response_t, loc_unc_a), "LocUncA:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gpsloc_response_t, loc_unc_p), "LocUncP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gpsloc_response_t, hepe), "HEPE:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gpsloc_response_t, loc_unc_ve), "LocUncVe:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gpsloc_response_t, heading), "Heading:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gpsloc_response_t, velocity_h), "VelHoriz:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_em7565_gpsloc_response_t, velocity_v), "VelVert:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
    };
    tokenfind_float_batch(response->response_string, (result), float_jobs, NELEMS(float_jobs));

    at_interface_free_response(response);

    return SW_RESPONSE_SUCCESS;
}

void sw_em7565_free_get_gpsloc(sw_em7565_gpsloc_response_t* s) {
    free(s);
}

sw_response_t sw_em7565_stop_gps(sw_em7565_t* h) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GPSEND], NULL, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GPSEND].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_start_gps(sw_em7565_t* h,
                                         sw_em7565_gps_mode_t fix_type,
                                         unsigned char max_fixtime_sec,
                                         unsigned int max_inaccuracy,
                                         int fix_count,
                                         int fix_rate) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%d,%d,%d,%d,%d",
             fix_type,
             max_fixtime_sec,
             max_inaccuracy,
             fix_count,
             fix_rate);
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GPSTRACK], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GPSTRACK].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_start_gps_default(sw_em7565_t* h) {
    return sw_em7565_start_gps(h,
                               GPS_MODE_STANDALONE,
                               GPS_MAX_FIXTIME_DEFAULT,
                               GPS_MAX_INACCURACY_DEFAULT,
                               GPS_FIX_COUNT_INFINITE,
                               GPS_FIX_RATE_DEFAULT);
}

/**
 * @brief sw_em7565_set_antenna_power Enable/Disable 3V GPS antenna power
 * INFO: this requires a (soft) reset to apply the changes
 * @param h modem handle
 * @param antenna_mode antenna power mode
 * @return RESPONSE_OK on success
 */
sw_response_t sw_em7565_set_antenna_power(sw_em7565_t* h,
                                const sw_em7565_gps_antenna_power_mode_t antenna_mode) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%d", antenna_mode);
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_WANT_SET], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_WANT_SET].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_em7565_gps_antenna_power_mode_t sw_em7565_get_antenna_power(sw_em7565_t* h) {
    at_interface_response_status_t ret;
    sw_em7565_gps_antenna_power_mode_t result = GPS_ANTENNA_POWER_ERROR;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return result;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_WANT_GET], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_WANT_GET].command_string);
        at_interface_free_response(response);
        return result;
    }

    int ant_mode = 0;
    int val = 0;
    static regex_t* regex_cache;
    val = tokenfind_integer_single(response->response_string, &ant_mode, "WANT:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", &regex_cache, 10);

    if(val > 0) result = (sw_em7565_gps_antenna_power_mode_t)ant_mode;

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_set_APN(sw_em7565_t* h, int slot, const char* ip_vers, const char* apn) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    if(ip_vers == NULL || apn == NULL) {
        ERROR("Invalid parameter\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%d,\"%s\",\"%s\"",
             slot,
             ip_vers,
             apn);
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_CGDCONT], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_CGDCONT].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_set_data_connection(sw_em7565_t* h, int data_connection_status) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    const int PDN_CONNECTION_ID = 1; // always use 1

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%d,%d",
             data_connection_status,
             PDN_CONNECTION_ID);
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_SET_DATA_CONNECTION], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_SET_DATA_CONNECTION].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

int sw_em7565_get_data_connection(sw_em7565_t* h) {
    at_interface_response_status_t ret;
    int result = DATA_CONNECTION_STATUS_ERROR;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return result;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GET_DATA_CONNECTION], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GET_DATA_CONNECTION].command_string);
        at_interface_free_response(response);
        return result;
    }

    int pid = 0;
    int state = 0;
    int val = 0;
    static regex_t* regex_cache_a;
    static regex_t* regex_cache_b;
    val = tokenfind_integer_single(response->response_string, &pid, "SCACT:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\),", &regex_cache_a, 10);
    val += tokenfind_integer_single(response->response_string, &state, "SCACT:[[:space:]]\\{1,\\}[[:digit:]]\\{1,\\},\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", &regex_cache_b, 10);

    if(val > 1) {
        result = state;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_set_radio_access_type(sw_em7565_t* h,
                                                     sw_em7565_radio_access_type_t radio_access_type) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%02x",
             radio_access_type);
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_SET_RAT], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s%02x\n", sw_em7565_command_defs[SW_EM7565_AT_SET_RAT].command_string, radio_access_type);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_get_radio_access_type(sw_em7565_t* h,
                                                     sw_em7565_radio_access_type_t* radio_access_type) {
    at_interface_response_status_t ret;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GET_RAT], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GET_RAT].command_string);
        at_interface_free_response(response);
        return SW_RESPONSE_FAILED;
    }

    sw_em7565_radio_access_type_t state = 0;
    int val = 0;
    static regex_t* regex_cache_a;
    val = tokenfind_integer_single(response->response_string, (int*)&state, "SELRAT:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\),", &regex_cache_a, 10);

    at_interface_free_response(response);

    if(val == 1) {
        *radio_access_type = state;
    }
    else {
        return SW_RESPONSE_FAILED;
    }

    return SW_RESPONSE_SUCCESS;
}

sw_response_t sw_em7565_select_band_config_profile(sw_em7565_t* h, int config_idx) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%02d", config_idx);
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_SET_BAND_PROFILE], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s%02d\n", sw_em7565_command_defs[SW_EM7565_AT_SET_BAND_PROFILE].command_string, config_idx);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}


sw_response_t sw_em7565_remove_band_config_profile(sw_em7565_t* h, int config_idx) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%02d,\"\",0", config_idx);
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_SET_BAND_PROFILE], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s%\n", sw_em7565_command_defs[SW_EM7565_AT_SET_BAND_PROFILE].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

int parse_band_config_profile_line(char* line, int* index, char name[SW_EM7565_BAND_CONFIG_NAME_STRLEN], unsigned long masks[6]) {
    int val;
    int match_start[25];
    int match_end[25];
    char* filter_left = "\\([[:digit:]]\\{1,\\}\\),[[:space:]]\\{1,\\}\\(.*\\)";
    char* filter_right = "[[:space:]]*\\([[:xdigit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}\\([[:xdigit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}\\([[:xdigit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}\\([[:xdigit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}\\([[:xdigit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}\\([[:xdigit:]]\\{1,\\}\\)";
    static regex_t* regex_cache_left;
    static regex_t* regex_cache_right;
    char tmp[1024] = {0};
    char* right = strstr(line, "  ");  // separate by two consecutive spaces
    if(right != NULL) { // cut by setting 0-character instead of a space
        *right = 0;
        right++;
    }
    val = tokenfind_match_regex_multi(line, match_start, match_end, 2, filter_left, &regex_cache_left);
    if(val >= 2) {
        strncpy(tmp, &line[match_start[0]], match_end[0]-match_start[0]);
        tmp[match_end[0]-match_start[0]] = 0;
        *index = conversion_str_to_int(tmp, 10);
        strncpy(name, &line[match_start[1]], match_end[1]-match_start[1]);
        name[match_end[1]-match_start[1]] = 0;
    }
    if(right != NULL) {
        val = tokenfind_match_regex_multi(right, match_start, match_end, 6, filter_right, &regex_cache_right);
        if(val >= 6) {
            for(int i=0; i<6; i++) {
                strncpy(tmp, &right[match_start[i]], match_end[i]-match_start[i]);
                tmp[match_end[i]-match_start[i]] = 0;
                masks[i] = conversion_str_to_ulong(tmp, 16);
            }
        }
    }
    return true;
}

sw_response_t sw_em7565_get_band_config_profile(sw_em7565_t* h, sw_em7565_band_profile_t* profile) {
    at_interface_response_status_t ret;
    int val = 0;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL || profile == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GET_BAND_PROFILE], NULL, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GET_BAND_PROFILE].command_string);
        result = SW_RESPONSE_ERROR;
    }

    char slice[10000] = {0};
    static regex_t* regex_cache_serving = NULL;
    val = tokenfind_string_single(response->response_string,
                            slice,
                            NELEMS(slice),
                            "\\(.*\\)OK",
                            &regex_cache_serving);

    if(val > 0) {
        tokenfind_split_string_t* rows = tokenfind_split_string(slice, "\r\n");
        if(rows != NULL && rows->n_tokens > 1) {
            unsigned long masks[6] = {0};
            parse_band_config_profile_line(rows->token[1],
                                            &profile->config_idx,
                                            profile->config_name,
                                            masks);

            profile->mask_gsm_umts = masks[0];
            profile->mask_lte      = masks[1];
            profile->mask_tds      = masks[2];
            profile->mask_lte2     = masks[3];
            profile->mask_lte3     = masks[4];
            profile->mask_lte4     = masks[5];
        }
        tokenfind_split_string_free(rows);
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_em7565_add_band_config_profile(sw_em7565_t* h, const sw_em7565_band_profile_t* profile) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%d,\"%s\",%lx,%lx,%lx,%lx,%lx,%lx",
             profile->config_idx,
             profile->config_name,
             profile->mask_gsm_umts,
             profile->mask_lte,
             profile->mask_lte2,
             profile->mask_tds,
             profile->mask_lte3,
             profile->mask_lte4);
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_SET_BAND_PROFILE], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_SET_BAND_PROFILE].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_em7565_band_profile_list_t* sw_em7565_allocate_band_config_profile_list() {
  sw_em7565_band_profile_list_t* result;
  result = calloc(1, sizeof(sw_em7565_band_profile_list_t));
  if(result != NULL) {
    result->nof_profiles = 0;
  }
  return result;
}

sw_response_t sw_em7565_get_band_config_profile_list(sw_em7565_t* h, sw_em7565_band_profile_list_t *list) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL || list == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GET_BAND_PROFILE_LIST], NULL, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GET_BAND_PROFILE_LIST].command_string);
        result = SW_RESPONSE_ERROR;
    }

    // clean list first...
    for(int i = 0; i < list->nof_profiles; i++) {
      if(list->profile[i] != NULL) {
        free(list->profile[i]);
        list->profile[i] = NULL;
      }
    }

    tokenfind_split_string_t* rows = tokenfind_split_string(response->response_string, "\r\n");
    if(rows != NULL && rows->n_tokens > 1) {
        (list)->nof_profiles = 0;
        for(int i=1, p=0; i<rows->n_tokens; i++, p++) {
            if(rows->token[i][0] == ' ') {
                //lines starting with space indicate another list. done.
                break;
            }
            (list)->profile[p] = calloc(1, sizeof(sw_em7565_band_profile_t));
            (list)->nof_profiles++;
            unsigned long masks[6] = {0};
            parse_band_config_profile_line(rows->token[i],
                                            &list->profile[p]->config_idx,
                                            list->profile[p]->config_name,
                                            masks);
            (list)->profile[p]->mask_gsm_umts = masks[0];
            (list)->profile[p]->mask_lte      = masks[1];
            (list)->profile[p]->mask_lte2     = masks[2];
            (list)->profile[p]->mask_tds      = masks[3];
            (list)->profile[p]->mask_lte3     = masks[4];
            (list)->profile[p]->mask_lte4     = masks[5];
        }

    }
    tokenfind_split_string_free(rows);
    at_interface_free_response(response);

    return result;
}

void sw_em7565_free_band_config_profile_list(sw_em7565_band_profile_list_t* s) {
    if(s != NULL) {
        for(int i=0; i < s->nof_profiles; i++) {
            free(s->profile[i]);
        }
        free(s);
    }
}

sw_em7565_network_list_t* sw_em7565_allocate_network_list() {
  sw_em7565_network_list_t* result = NULL;
  result = calloc(1, sizeof(sw_em7565_network_list_t));
  if(result != NULL) {
    result->nof_networks = 0;
  }
  return result;
}

sw_response_t sw_em7565_network_search(sw_em7565_t* h, sw_em7565_network_list_t *networks) {
    at_interface_response_status_t ret;
//    int val;
//    int match_start[25];
//    int match_end[25];
//    char* filter_networks = "+cops: \\(([^()]\\{1,\\}\\),*)\\{1,\\},,.*";
//    char* filter_options = ",,(\\([^()]\\{1,\\}\\)),(\\([^()]\\{1,\\}\\))";
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL || networks == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GET_COPS], NULL, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GET_COPS].command_string);
        result = SW_RESPONSE_ERROR;
    }

    // cleanup networks first...
    for(int i=0; i < networks->nof_networks; i++) {
        free(networks->network[i]);
    }

    char* networkstr = response->response_string;
    char* optliststr = strstr(networkstr, ",,");
    char* tmp = NULL;
    if(optliststr != NULL) {
        *optliststr = 0;   // terminate first half
        optliststr += 2;   // seek to real start of optlist (behind second comma)
    }

    int i = 0;
    while(networkstr != NULL) {
        tmp = strstr(networkstr, "(");
        if(tmp == NULL) {
            networkstr = NULL;
            break;
        }
        else {
            networkstr = strstr(tmp, ")");
            *networkstr = 0;
            networkstr += 1;
        }
        if(tmp != NULL) {
            //alloc
            networks->network[i] = calloc(1, sizeof(network_list_item_t));
            sscanf(tmp, "(%d,\"%[^\"]\",\"%[^\"]\",\"%d\",%d",
                   (int*)&(networks)->network[i]->state,
                   (networks)->network[i]->name_long,
                   (networks)->network[i]->name_short,
                   &(networks)->network[i]->mccmnc,
                   (int*)&(networks)->network[i]->access_technology);
            i++;
        }
    }
    (networks)->nof_networks = i;

    at_interface_free_response(response);

    return result;
}

void sw_em7565_free_network_list(sw_em7565_network_list_t* list) {
    if(list != NULL) {
        for(int i=0; i < list->nof_networks; i++) {
            free(list->network[i]);
        }
        free(list);
    }
}

sw_response_t sw_em7565_select_network(sw_em7565_t* h,
                                       const operator_network_selection_mode_t mode,
                                       const operator_network_selection_format_t select_by,
                                       const network_list_item_t* network) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    if(mode == COPS_MODE_MANUAL || mode == COPS_MODE_MANUAL_WITH_AUTOMATIC_FALLBACK) {
        if(network == NULL) {
            ERROR("Missing Network\n");
            return SW_RESPONSE_ERROR;
        }
    }

    at_interface_response_t* response = NULL;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    int len = 0;
    switch(mode) {
    case COPS_MODE_AUTOMATIC:
        snprintf(params, sizeof(params), "%d", mode);
        break;
    case COPS_MODE_SET_FORMAT:
        snprintf(params, sizeof(params), "%d,%d", mode, select_by);
        break;
    case COPS_MODE_MANUAL:
    case COPS_MODE_MANUAL_WITH_AUTOMATIC_FALLBACK:
        switch(select_by) {
        case COPS_FORMAT_SHORT_NAME:
            len = snprintf(params, sizeof(params), "%d,%d,%s",mode, select_by, network->name_short);
            break;
        case COPS_FORMAT_LONG_NAME:
            len = snprintf(params, sizeof(params), "%d,%d,%s",mode, select_by, network->name_long);
            break;
        case COPS_FORMAT_MCCMNC:
            len = snprintf(params, sizeof(params), "%d,%d,%d",mode, select_by, network->mccmnc);
            break;
        default:
            ERROR("Invalid Mode\n");
            at_interface_free_response(response);
            return SW_RESPONSE_ERROR;
        }

        if(network->access_technology < COPS_AcT_DONTCARE) {
            len = snprintf(params+len, sizeof(params)-(unsigned int)len, ",%d", network->access_technology);
        }
        break;
    default:
        ERROR("Invalid Mode\n");
        at_interface_free_response(response);
        return SW_RESPONSE_ERROR;
    }
    ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_SET_COPS], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s%s\n", sw_em7565_command_defs[SW_EM7565_AT_SET_COPS].command_string, params);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_em7565_current_operator_t* sw_em7565_allocate_current_operator() {
  sw_em7565_current_operator_t* result = NULL;
  result = calloc(1, sizeof(sw_em7565_current_operator_t));
  result->name_long[0] = 0;
  return result;
}

sw_response_t sw_em7565_get_current_operator(sw_em7565_t* h, sw_em7565_current_operator_t *op) {
  at_interface_response_status_t ret;
  sw_response_t result = 0;

  if(h == NULL || h->tty == NULL || op == NULL) {
      ERROR("Incomplete handle\n");
      return SW_RESPONSE_ERROR;
  }

  at_interface_response_t* response;
  ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GET_COPS_CURRENT], NULL, &response);

  switch(ret) {
  case AT_RESPONSE_SUCCESS:
      result = SW_RESPONSE_SUCCESS;
      break;
  case AT_RESPONSE_FAILED:
  case AT_RESPONSE_TIMEOUT:
      result = SW_RESPONSE_FAILED;
      break;
  default:
      ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GET_COPS_CURRENT].command_string);
      result = SW_RESPONSE_ERROR;
  }

  if(result < SW_RESPONSE_FAILED) {
    result = SW_RESPONSE_FAILED;

    (op)->name_long[0] = 0;

    char* str = response->response_string;
    char* start = strstr(str, "\"");
    if(start != NULL) {
      start +=1;
      char* end = strstr(start, "\"");
      if(end != NULL) {
        /* found end (second "), terminate string here */
        *end = 0;
        strcpy((op)->name_long, start);
        result = SW_RESPONSE_SUCCESS;
      }
    }
  }
  at_interface_free_response(response);

  return result;
}

void sw_em7565_free_current_operator(sw_em7565_current_operator_t* h) {
  free(h);
}

sw_response_t sw_em7565_get_gps_autostart_mode(sw_em7565_t *h, sw_em7565_gps_autostart_mode_t *autostart_mode) {
  at_interface_response_status_t ret;
  sw_response_t result = 0;

  if(h == NULL || h->tty == NULL || autostart_mode == NULL) {
      ERROR("Incomplete handle\n");
      return result;
  }
  *autostart_mode = GPS_AUTOSTART_ERROR;

  at_interface_response_t* response;
  ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GPSAUTOSTART_GET], NULL, &response);

  if(ret >= AT_RESPONSE_FAILED) {
      ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GPSAUTOSTART_GET].command_string);
      at_interface_free_response(response);
      return result;
  }

  int mode = 0;
  int val = 0;
  static regex_t* regex_cache;
  val = tokenfind_integer_single(response->response_string, &mode, "function:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", &regex_cache, 10);

  if(val > 0) *autostart_mode = (sw_em7565_gps_autostart_mode_t)mode;

  at_interface_free_response(response);

  return result;
}

sw_response_t sw_em7565_set_gps_autostart_mode(sw_em7565_t *h, const sw_em7565_gps_autostart_mode_t autostart_mode) {
  at_interface_response_status_t ret;
  sw_response_t result = 0;

  if(h == NULL || h->tty == NULL) {
      ERROR("Incomplete handle\n");
      return SW_RESPONSE_ERROR;
  }

  at_interface_response_t* response;
  char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
  snprintf(params, sizeof(params), "%d", autostart_mode);
  ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GPSAUTOSTART_SET], params, &response);

  switch(ret) {
  case AT_RESPONSE_SUCCESS:
      result = SW_RESPONSE_SUCCESS;
      break;
  case AT_RESPONSE_FAILED:
  case AT_RESPONSE_TIMEOUT:
      result = SW_RESPONSE_FAILED;
      break;
  default:
      ERROR("Command failed: %s\n", sw_em7565_command_defs[SW_EM7565_AT_GPSAUTOSTART_SET].command_string);
      result = SW_RESPONSE_ERROR;
  }

  at_interface_free_response(response);

  return result;
}

sw_response_t sw_em7565_gps_status(sw_em7565_t *h, sw_em7565_gps_status_t *status) {
  at_interface_response_status_t ret;
  sw_response_t result  = SW_RESPONSE_UNKNOWN;
  status->last_fix_status = GPS_STATUS_UNKNOWN;
  status->last_fix_status_errcode = -1;
  status->fix_session_status = GPS_STATUS_UNKNOWN;
  status->fix_session_status_errcode = -1;

  if(h == NULL || h->tty == NULL) {
      ERROR("Incomplete handle\n");
      result = SW_RESPONSE_INVAL;
      return result;
  }

  at_interface_response_t* response;
  ret = at_interface_command(h->tty, &sw_em7565_command_defs[SW_EM7565_AT_GPSSTATUS], NULL, &response);

  switch(ret) {
  case AT_RESPONSE_SUCCESS:
      INFO("Command succeeded\n");
      result = SW_RESPONSE_SUCCESS;
      break;
  case AT_RESPONSE_FAILED:
  case AT_RESPONSE_TIMEOUT:
      WARNING("Command failed with response status %d\n", ret);
      result = SW_RESPONSE_FAILED;
      break;
  default:
      ERROR("AT interface error\n");
      result = SW_RESPONSE_ERROR;
      break;
  }

  if(ret != AT_RESPONSE_SUCCESS) {
      at_interface_free_response(response);
      return result;
  }

  typedef struct gpsstatus_response_values {
    char last_fix_status_str[SW_EM7565_GPS_STATUS_VALUE_STRLEN];
    char fix_session_status_str[SW_EM7565_GPS_STATUS_VALUE_STRLEN];
  } gpsstatus_response_values_t;

  gpsstatus_response_values_t response_values;

  static tokenfind_batch_t string_jobs[] = {
      {offsetof(gpsstatus_response_values_t, last_fix_status_str), "Last Fix Status[[:space:]]\\{1,\\}=[[:space:]]\\([^\r\n\\,]*\\)", NULL},
      {offsetof(gpsstatus_response_values_t, fix_session_status_str), "Fix Session Status[[:space:]]\\{1,\\}=[[:space:]]\\([^\r\n\\,]*\\)", NULL},
  };

  int val = 0;
  val = tokenfind_string_batch(response->response_string, &response_values, string_jobs, NELEMS(string_jobs), SW_EM7565_GPS_STATUS_VALUE_STRLEN);

  if(val == NELEMS(string_jobs)) {
    //parse last_fix_status
    if(strcmp("NONE", response_values.last_fix_status_str) == 0) {
      status->last_fix_status = GPS_STATUS_NONE;
    }
    else if(strcmp("ACTIVE", response_values.last_fix_status_str) == 0) {
      status->last_fix_status = GPS_STATUS_ACTIVE;
    }
    else if(strcmp("SUCCESS", response_values.last_fix_status_str) == 0) {
      status->last_fix_status = GPS_STATUS_SUCCESS;
    }
    else if(strcmp("FAIL", response_values.last_fix_status_str) == 0) {
      status->last_fix_status = GPS_STATUS_FAIL;
      //TODO: Set errcode
    }

    //parse fix_session_status
    if(strcmp("NONE", response_values.fix_session_status_str) == 0) {
      status->fix_session_status = GPS_STATUS_NONE;
    }
    else if(strcmp("ACTIVE", response_values.fix_session_status_str) == 0) {
      status->fix_session_status = GPS_STATUS_ACTIVE;
    }
    else if(strcmp("SUCCESS", response_values.fix_session_status_str) == 0) {
      status->fix_session_status = GPS_STATUS_SUCCESS;
    }
    else if(strcmp("FAIL", response_values.fix_session_status_str) == 0) {
      status->fix_session_status = GPS_STATUS_FAIL;
      //TODO: Set errcode
    }

  }
  else {
   result = SW_RESPONSE_FAILED;
  }

  at_interface_free_response(response);

  return result;
}
