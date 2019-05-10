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
#include "cmnalib/at_sierra_wireless_mc7455.h"
#include "cmnalib/logger.h"
#include "cmnalib/tokenfind.h"
#include "cmnalib/conversion.h"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

#undef ADDCOMMAND
#define ADDCOMMAND( _etype, _cmdstr, _sec, _usec) { _etype, _cmdstr, _sec, _usec }
const at_interface_command_t sw_mc7455_command_defs[SW_MC7455_CMD__MAX] = {
#include "cmnalib/at_def_sierra_wireless_mc7455.h"
};
#undef ADDCOMMAND

#define SW_MC_7455_MAGIC_GPS_FACTOR (10.0/((double)(0x1C71C7)))

/**
 * @brief sw_mc7455_gps_raw_to_double Convert raw GPS output of the modem
 * to canonical latitude/longitude representation. The magic factor is
 * obtained by dividing the raw hex output by the textual values received
 * by AT!GPSLOC?
 * @param int_value signed 32bit integer from modem
 * @return double value representation of latitude/longitude
 */
double sw_mc7455_gps_raw_to_double(int32_t int_value) {
    double result = (double)int_value * SW_MC_7455_MAGIC_GPS_FACTOR;
    return result;
}

GSList* sw_mc7455_enumerate_devices() {
    return enumerate_supported_devices("1199", "9071", "tty", "03");
}

void sw_mc7455_enumerate_devices_free(GSList* list) {
    enumerate_supported_devices_free(list);
}

sw_mc7455_t* sw_mc7455_init_first() {
    sw_mc7455_t* result = NULL;
    GSList* list = sw_mc7455_enumerate_devices();

    if(list != NULL) {
        device_list_entry_t* entry = device_list_unpack_entry(list);
        result = sw_mc7455_init(entry->device_name);
    }
    else {
        ERROR("No supported device found\n");
    }

    sw_mc7455_enumerate_devices_free(list);
    return result;
}

sw_mc7455_t* sw_mc7455_init(const char* tty_device_path) {
    DEBUG("Opening device %s\n", tty_device_path);

    sw_mc7455_t* h = calloc(1, sizeof(sw_mc7455_t));
    h->tty = at_interface_open(tty_device_path);
    if(h->tty == NULL) {
        free(h);
        ERROR("Initialization failed\n");
        return NULL;
    }
    return h;
}

void sw_mc7455_destroy(sw_mc7455_t* h) {
    if(h != NULL) {
        at_interface_close(h->tty);
    }
    free(h);
}

sw_response_t sw_mc7455_is_ready(sw_mc7455_t* h) {
    sw_response_t result = SW_RESPONSE_UNKNOWN;
    at_interface_response_status_t ret;

    DEBUG("Talking to Modem\n");
    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_READY], NULL, &response);

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

sw_response_t sw_mc7455_reset(sw_mc7455_t* h) {
    int result = SW_RESPONSE_UNKNOWN;
    at_interface_response_status_t ret;

    DEBUG("Resetting Modem\n");
    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_RESET], NULL, &response);

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

void sw_mc7455_free_status(sw_mc7455_gstatus_response_t* s) {
    if(s != NULL) {
        free(s);
    }
}

sw_response_t sw_mc7455_get_status(sw_mc7455_t* h, sw_mc7455_gstatus_response_t** result) {
    at_interface_response_status_t ret;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    *result = calloc(1, sizeof(sw_mc7455_gstatus_response_t));
    if(*result == NULL) {
        ERROR("ERROR in calloc\n");
        return SW_RESPONSE_OUT_OF_MEMORY;
    }
    (*result)->tx_power = SW_GSTATUS_TX_POWER_INACTIVE;

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_GSTATUS], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_GSTATUS].command_string);
        sw_mc7455_free_status(*result);
        at_interface_free_response(response);
        return SW_RESPONSE_ERROR;
    }

    static tokenfind_batch_t int_jobs[] = {
        {offsetof(sw_mc7455_gstatus_response_t, current_time), "Current Time:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, temperature), "Temperature:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, reset_counter), "Reset Counter:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, lte_band), "LTE band:[[:space:]]\\{1,\\}[[:alpha:]]\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, lte_bw_MHz), "LTE bw:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}MHz", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, lte_rx_chan), "LTE Rx chan:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, lte_tx_chan), "LTE Tx chan:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, lte_scell_band), "LTE Scell band:[[:space:]]*[[:alpha:]]\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, lte_scell_bw_MHz), "LTE Scell bw:[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}MHz", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, lte_scell_chan), "LTE Scell chan:[[:space:]]*\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, pcc_rxm_rssi), "PCC RxM RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, pcc_rxm_rsrp), "PCC RxM RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}RSRP (dBm):[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, pcc_rxd_rssi), "PCC RxD RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, pcc_rxd_rsrp), "PCC RxD RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}RSRP (dBm):[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, scc_rxm_rssi), "SCC RxM RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, scc_rxm_rsrp), "SCC RxM RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}RSRP (dBm):[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, scc_rxd_rssi), "SCC RxD RSSI:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, scc_rxd_rsrp), "SCC RxD RSSI:[[:space:]]\\{1,\\}-*[[:digit:]]\\{1,\\}[[:space:]]\\{1,\\}RSRP (dBm):[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, tx_power), "Tx Power:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, tac), "TAC:[[:space:]]\\{1,\\}[[:xdigit:]]\\{1,\\} (\\([[:digit:]]\\{1,\\}\\))[[:space:]]\\{1,\\}", NULL, 10},
        {offsetof(sw_mc7455_gstatus_response_t, cell_id), "Cell ID:[[:space:]]\\{1,\\}[[:xdigit:]]\\{1,\\} (\\([[:digit:]]\\{1,\\}\\))[[:space:]]\\{1,\\}", NULL, 10},
    };
    tokenfind_integer_batch(response->response_string, *result, int_jobs, NELEMS(int_jobs));

    static tokenfind_batch_t string_jobs[] = {
        {offsetof(sw_mc7455_gstatus_response_t, mode), "Mode:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gstatus_response_t, system_mode), "System mode:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gstatus_response_t, ps_state), "PS state:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gstatus_response_t, lte_ca_state), "LTE CA state:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gstatus_response_t, emm_state), "EMM state:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gstatus_response_t, rrc_state), "RRC state:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gstatus_response_t, ims_reg_state), "IMS reg state:[[:space:]]\\{1,\\}\\([[:alpha:]]\\{1,\\}\\( [[:alpha:]]\\{1,\\}\\)*\\)[[:space:]]\\{1,\\}", NULL},
    };
    tokenfind_string_batch(response->response_string, *result, string_jobs, NELEMS(string_jobs), SW_MC7455_GSTATUS_RESPONSE_STRLEN);

    static tokenfind_batch_t float_jobs[] = {
        {offsetof(sw_mc7455_gstatus_response_t, rsrq), "RSRQ (dB):[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gstatus_response_t, sinr), "SINR (dB):[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
    };
    tokenfind_float_batch(response->response_string, *result, float_jobs, NELEMS(float_jobs));

    at_interface_free_response(response);

    return SW_RESPONSE_SUCCESS;
}

sw_response_t sw_mc7455_get_information(sw_mc7455_t* h, sw_mc7455_information_response_t** result) {
    at_interface_response_status_t ret;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    *result = calloc(1, sizeof(sw_mc7455_information_response_t));
    if(*result == NULL) {
        ERROR("ERROR in calloc\n");
        return SW_RESPONSE_OUT_OF_MEMORY;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_INFO], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_INFO].command_string);
        sw_mc7455_free_information(*result);
        at_interface_free_response(response);
        return SW_RESPONSE_ERROR;
    }

    static tokenfind_batch_t string_jobs[] = {
        {offsetof(sw_mc7455_information_response_t, manufacturer), "Manufacturer:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_mc7455_information_response_t, model), "Model:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_mc7455_information_response_t, revision), "Revision:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_mc7455_information_response_t, meid), "MEID:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_mc7455_information_response_t, imei), "IMEI:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_mc7455_information_response_t, imei_sv), "IMEI SV:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
        {offsetof(sw_mc7455_information_response_t, fsn), "FSN:[[:space:]]\\{1,\\}\\([^\r\n]*\\)", NULL},
    };
    tokenfind_string_batch(response->response_string, *result, string_jobs, NELEMS(string_jobs), SW_MC7455_INFORMATION_RESPONSE_STRLEN);

    at_interface_free_response(response);

    return SW_RESPONSE_SUCCESS;
}

void sw_mc7455_free_information(sw_mc7455_information_response_t* s) {
    if(s != NULL) {
        free(s);
    }
}

sw_response_t sw_mc7455_get_lteinfo(sw_mc7455_t* h, sw_mc7455_lteinfo_response_t** result) {
    at_interface_response_status_t ret;
    int val = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    *result = calloc(1, sizeof(sw_mc7455_lteinfo_response_t));
    if(*result == NULL) {
        ERROR("ERROR in calloc\n");
        return SW_RESPONSE_OUT_OF_MEMORY;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_LTEINFO], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_LTEINFO].command_string);
        sw_mc7455_free_lteinfo(*result);
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
            (*result)->earfn = conversion_str_to_int(serving_info_tbl->row[1].column[0], 10);
            (*result)->mcc   = conversion_str_to_int(serving_info_tbl->row[1].column[1], 10);
            (*result)->mnc   = conversion_str_to_int(serving_info_tbl->row[1].column[2], 10);
            (*result)->tac   = conversion_str_to_int(serving_info_tbl->row[1].column[3], 10);
            (*result)->cid   = conversion_str_to_int(serving_info_tbl->row[1].column[4], 16);
            (*result)->band  = conversion_str_to_int(serving_info_tbl->row[1].column[5], 10);
            (*result)->d     = conversion_str_to_int(serving_info_tbl->row[1].column[6], 10);
            (*result)->u     = conversion_str_to_int(serving_info_tbl->row[1].column[7], 10);
            (*result)->snr   = conversion_str_to_int(serving_info_tbl->row[1].column[8], 10);
            (*result)->pci   = conversion_str_to_int(serving_info_tbl->row[1].column[9], 10);
            (*result)->rsrq  = conversion_str_to_float(serving_info_tbl->row[1].column[10]);
            (*result)->rsrp  = conversion_str_to_float(serving_info_tbl->row[1].column[11]);
            (*result)->rssi  = conversion_str_to_float(serving_info_tbl->row[1].column[12]);
            (*result)->rxlv  = conversion_str_to_int(serving_info_tbl->row[1].column[13], 10);
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
            (*result)->nof_intrafreq_neighbours = 0;
            (*result)->intrafreq_neighbours = calloc(intra_info_tbl->n_rows - 1, sizeof(sw_mc7455_lteinfo_intrafreq_neighbour_t));
            for(int row_idx = 1; row_idx < intra_info_tbl->n_rows; row_idx++) {
                (*result)->intrafreq_neighbours[(*result)->nof_intrafreq_neighbours].pci =
                        conversion_str_to_int(intra_info_tbl->row[row_idx].column[0], 10);
                (*result)->intrafreq_neighbours[(*result)->nof_intrafreq_neighbours].rsrq =
                        conversion_str_to_float(intra_info_tbl->row[row_idx].column[1]);
                (*result)->intrafreq_neighbours[(*result)->nof_intrafreq_neighbours].rsrp =
                        conversion_str_to_float(intra_info_tbl->row[row_idx].column[2]);
                (*result)->intrafreq_neighbours[(*result)->nof_intrafreq_neighbours].rssi =
                        conversion_str_to_float(intra_info_tbl->row[row_idx].column[3]);
                (*result)->intrafreq_neighbours[(*result)->nof_intrafreq_neighbours].rxlv =
                        conversion_str_to_int(intra_info_tbl->row[row_idx].column[4], 10);

                ((*result)->nof_intrafreq_neighbours)++;
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
                            "InterFreq:[[:space:]]\\{1,\\}\\(.*\\)GSM:",
                            &regex_cache_inter);

    if(val > 0) {
        tokenfind_string_table_t* inter_info_tbl = tokenfind_parse_table(slice);
        if(inter_info_tbl != NULL && inter_info_tbl->n_rows > 1) {
            (*result)->nof_interfreq_neighbours = 0;
            (*result)->interfreq_neighbours = calloc(inter_info_tbl->n_rows - 1, sizeof(sw_mc7455_lteinfo_interfreq_neighbour_t));
            for(int row_idx = 1; row_idx < inter_info_tbl->n_rows; row_idx++) {
                (*result)->interfreq_neighbours[(*result)->nof_interfreq_neighbours].earfcn =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[0], 10);
                (*result)->interfreq_neighbours[(*result)->nof_interfreq_neighbours].threshold_low =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[1], 10);
                (*result)->interfreq_neighbours[(*result)->nof_interfreq_neighbours].threshold_high =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[2], 10);
                (*result)->interfreq_neighbours[(*result)->nof_interfreq_neighbours].priority =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[3], 10);
                (*result)->interfreq_neighbours[(*result)->nof_interfreq_neighbours].pci =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[4], 10);
                (*result)->interfreq_neighbours[(*result)->nof_interfreq_neighbours].rsrq =
                        conversion_str_to_float(inter_info_tbl->row[row_idx].column[5]);
                (*result)->interfreq_neighbours[(*result)->nof_interfreq_neighbours].rsrp =
                        conversion_str_to_float(inter_info_tbl->row[row_idx].column[6]);
                (*result)->interfreq_neighbours[(*result)->nof_interfreq_neighbours].rssi =
                        conversion_str_to_float(inter_info_tbl->row[row_idx].column[7]);
                (*result)->interfreq_neighbours[(*result)->nof_interfreq_neighbours].rxlv =
                        conversion_str_to_int(inter_info_tbl->row[row_idx].column[8], 10);

                ((*result)->nof_interfreq_neighbours)++;
            }
        }
        tokenfind_free_table(inter_info_tbl);
    }

    at_interface_free_response(response);

    return SW_RESPONSE_SUCCESS;
}

void sw_mc7455_free_lteinfo(sw_mc7455_lteinfo_response_t* s) {
    if(s != NULL) {
        free(s->intrafreq_neighbours);
        s->nof_intrafreq_neighbours = 0;
        free(s->interfreq_neighbours);
        s->nof_interfreq_neighbours = 0;
    }
    free(s);
}

sw_response_t sw_mc7455_get_gpsloc(sw_mc7455_t* h, sw_mc7455_gpsloc_response_t** result) {
    at_interface_response_status_t ret;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_INVAL;
    }

    *result = calloc(1, sizeof(sw_mc7455_gpsloc_response_t));
    if(*result == NULL) {
        ERROR("ERROR in calloc\n");
        return SW_RESPONSE_OUT_OF_MEMORY;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_GPSLOC], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_GPSLOC].command_string);
        sw_mc7455_free_get_gpsloc(*result);
        *result = NULL;
        at_interface_free_response(response);
        return SW_RESPONSE_ERROR;
    }

//    static regex_t* regex_not_avail;
//    ret = tokenfind_string_single(response->response_string,
//                                  NULL, 0,
//                                  "",
//                                  &regex_not_avail);

    if(strstr(response->response_string, "Not Available")) {
        (*result)->is_invalid = 1;
    }

    static tokenfind_batch_t int_jobs[] = {
        {offsetof(sw_mc7455_gpsloc_response_t, _raw_latitude), "Lat:[[:space:]]\\{1,\\}[^(]*(0x\\([[:xdigit:]]\\{1,\\}\\))[[:space:]]\\{1,\\}", NULL, 16},
        {offsetof(sw_mc7455_gpsloc_response_t, _raw_longitude), "Lon:[[:space:]]\\{1,\\}[^(]*(0x\\([[:xdigit:]]\\{1,\\}\\))[[:space:]]\\{1,\\}", NULL, 16},
        {offsetof(sw_mc7455_gpsloc_response_t, altitude), "Altitude:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", NULL, 10},
    };
    tokenfind_integer_batch(response->response_string, (*result), int_jobs, NELEMS(int_jobs));
    (*result)->latitude = sw_mc7455_gps_raw_to_double((*result)->_raw_latitude);
    (*result)->longitude = sw_mc7455_gps_raw_to_double((*result)->_raw_longitude);

    static tokenfind_batch_t float_jobs[] = {
        {offsetof(sw_mc7455_gpsloc_response_t, loc_unc_angle), "LocUncAngle:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gpsloc_response_t, loc_unc_a), "LocUncA:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gpsloc_response_t, loc_unc_p), "LocUncP:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gpsloc_response_t, hepe), "HEPE:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gpsloc_response_t, loc_unc_ve), "LocUncVe:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gpsloc_response_t, heading), "Heading:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gpsloc_response_t, velocity_h), "VelHoriz:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
        {offsetof(sw_mc7455_gpsloc_response_t, velocity_v), "VelVert:[[:space:]]\\{1,\\}\\(-*[[:digit:]]\\{1,\\}\\.*[[:digit:]]*\\)[[:space:]]\\{1,\\}", NULL},
    };
    tokenfind_float_batch(response->response_string, (*result), float_jobs, NELEMS(float_jobs));

    at_interface_free_response(response);

    return SW_RESPONSE_SUCCESS;
}

void sw_mc7455_free_get_gpsloc(sw_mc7455_gpsloc_response_t* s) {
    free(s);
}

sw_response_t sw_mc7455_stop_gps(sw_mc7455_t* h) {
    at_interface_response_status_t ret;
    sw_response_t result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_GPSEND], NULL, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_GPSEND].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_mc7455_start_gps(sw_mc7455_t* h,
                                         sw_mc7455_gps_mode_t fix_type,
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
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_GPSTRACK], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_GPSTRACK].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_mc7455_start_gps_default(sw_mc7455_t* h) {
    return sw_mc7455_start_gps(h,
                               SW_MC7455_GPS_MODE_STANDALONE,
                               GPS_MAX_FIXTIME_DEFAULT,
                               GPS_MAX_INACCURACY_DEFAULT,
                               GPS_FIX_COUNT_INFINITE,
                               GPS_FIX_RATE_DEFAULT);
}

/**
 * @brief sw_mc7455_set_antenna_power Enable/Disable 3V GPS antenna power
 * INFO: this requires a (soft) reset to apply the changes
 * @param h modem handle
 * @param antenna_mode antenna power mode
 * @return RESPONSE_OK on success
 */
int sw_mc7455_set_antenna_power(sw_mc7455_t* h,
                                sw_mc7455_gps_antenna_power_mode_t antenna_mode) {
    at_interface_response_status_t ret;
    int result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%d", antenna_mode);
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_WANT_SET], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_WANT_SET].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_mc7455_gps_antenna_power_mode_t sw_mc7455_get_antenna_power(sw_mc7455_t* h) {
    at_interface_response_status_t ret;
    sw_mc7455_gps_antenna_power_mode_t result = SW_MC7455_GPS_ANTENNA_POWER_ERROR;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return result;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_WANT_GET], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_WANT_GET].command_string);
        at_interface_free_response(response);
        return result;
    }

    int ant_mode = 0;
    int val = 0;
    static regex_t* regex_cache;
    val = tokenfind_integer_single(response->response_string, &ant_mode, "WANT:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\)[[:space:]]\\{1,\\}", &regex_cache, 10);

    if(val > 0) result = ant_mode;

    at_interface_free_response(response);

    return result;
}

int sw_mc7455_set_APN(sw_mc7455_t* h, int slot, const char* ip_vers, const char* apn) {
    at_interface_response_status_t ret;
    int result = 0;

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
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_CGDCONT], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_CGDCONT].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

int sw_mc7455_set_data_connection(sw_mc7455_t* h, int data_connection_status) {
    at_interface_response_status_t ret;
    int result = 0;

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
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_SET_DATA_CONNECTION], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_SET_DATA_CONNECTION].command_string);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

int sw_mc7455_get_data_connection(sw_mc7455_t* h) {
    at_interface_response_status_t ret;
    int result = DATA_CONNECTION_STATUS_ERROR;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return result;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_GET_DATA_CONNECTION], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_GET_DATA_CONNECTION].command_string);
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

sw_response_t sw_mc7455_set_radio_access_type(sw_mc7455_t* h,
                                                     sw_mc7455_radio_access_type_t radio_access_type) {
    at_interface_response_status_t ret;
    int result = 0;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    char params[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    snprintf(params, sizeof(params), "%02x",
             radio_access_type);
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_SET_RAT], params, &response);

    switch(ret) {
    case AT_RESPONSE_SUCCESS:
        result = SW_RESPONSE_SUCCESS;
        break;
    case AT_RESPONSE_FAILED:
    case AT_RESPONSE_TIMEOUT:
        result = SW_RESPONSE_FAILED;
        break;
    default:
        ERROR("Command failed: %s%02x\n", sw_mc7455_command_defs[SW_MC7455_AT_SET_RAT].command_string, radio_access_type);
        result = SW_RESPONSE_ERROR;
    }

    at_interface_free_response(response);

    return result;
}

sw_response_t sw_mc7455_get_radio_access_type(sw_mc7455_t* h,
                                                     sw_mc7455_radio_access_type_t* radio_access_type) {
    at_interface_response_status_t ret;

    if(h == NULL || h->tty == NULL) {
        ERROR("Incomplete handle\n");
        return SW_RESPONSE_ERROR;
    }

    at_interface_response_t* response;
    ret = at_interface_command(h->tty, &sw_mc7455_command_defs[SW_MC7455_AT_GET_RAT], NULL, &response);

    if(ret >= AT_RESPONSE_FAILED) {
        ERROR("Command failed: %s\n", sw_mc7455_command_defs[SW_MC7455_AT_GET_RAT].command_string);
        at_interface_free_response(response);
        return SW_RESPONSE_FAILED;
    }

    sw_mc7455_radio_access_type_t state = 0;
    int val = 0;
    static regex_t* regex_cache_a;
    val = tokenfind_integer_single(response->response_string, (int*)&state, "SELRAT:[[:space:]]\\{1,\\}\\([[:digit:]]\\{1,\\}\\),", &regex_cache_a, 10);

    if(val == 1) {
        *radio_access_type = state;
    }
    else {
        return SW_RESPONSE_FAILED;
    }

    return SW_RESPONSE_SUCCESS;
}
