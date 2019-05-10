#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "cmnalib/logger.h"

#include "cmnalib/at_sierra_wireless_em7565.h"

#define TEST_SUCCESS EXIT_SUCCESS
#define TEST_FAIL    EXIT_FAILURE

int __assert_result_summary__(int res) {
    switch(res) {
    case TEST_SUCCESS:
        INFO("Test passed\n");
        break;
    case TEST_FAIL:
        ERROR("Test failed\n");
        break;
    }
    return res;
}

#define FLOAT_TOLERANCE 0.00001

#define ASSERT_INIT() int __as_result__ = TEST_SUCCESS
#define ASSERT_FAIL() __as_result__ = TEST_FAIL
#define ASSERT_RESULT() __assert_result_summary__(__as_result__)

#define ASSERT_CALL(A) INFO("Testing " TOSTRING(A)"\n"); if(A != TEST_SUCCESS) { ASSERT_FAIL(); }
#define ASSERT_INT(A, B) if(A != B) { ERROR("Assertion failed: %s:%d\n", __FILE__,__LINE__); ASSERT_FAIL(); }
#define ASSERT_FLOAT(A, B, C) if(fabs(A - B) > C) { ERROR("Assertion failed: %s:%d\n", __FILE__,__LINE__); ASSERT_FAIL(); }
#define ASSERT_STRING(A, B) if(strcmp(A, B) != 0) { ERROR("Assertion failed: %s:%d\n", __FILE__,__LINE__); ASSERT_FAIL(); }

int cmd_gstatus_1() {

    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_gstatus_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_em7565_gstatus_response_t* ltestatus;
    sw_response_t ret = 0;

    ltestatus = sw_em7565_allocate_status();
    ret = sw_em7565_get_status(modem, ltestatus);

    if(ret >= SW_RESPONSE_CRITICAL) {
        return TEST_FAIL;
    }


    //if(ltestatus->current_time != 7480) return TEST_FAIL;
    ASSERT_INT(ltestatus->current_time, 7480);
    ASSERT_INT(ltestatus->temperature, 34);
    ASSERT_INT(ltestatus->reset_counter, 1);
    ASSERT_STRING(ltestatus->mode, "ONLINE");
    ASSERT_STRING(ltestatus->system_mode, "LTE");
    ASSERT_STRING(ltestatus->ps_state, "Not attached");
    ASSERT_INT(ltestatus->lte_band, 3);
    ASSERT_INT(ltestatus->lte_bw_MHz, 20);
    ASSERT_INT(ltestatus->lte_rx_chan, 1300);
    ASSERT_INT(ltestatus->lte_tx_chan, 19300);
    ASSERT_STRING(ltestatus->lte_scc1_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->lte_scc2_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->lte_scc3_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->lte_scc4_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->emm_state, "Deregistered");
    ASSERT_STRING(ltestatus->rrc_state, "RRC Idle");
    ASSERT_STRING(ltestatus->ims_reg_state, "No Srv");
    ASSERT_INT(ltestatus->pcc_rxm_rssi, -49);
    ASSERT_INT(ltestatus->pcc_rxm_rsrp, -80);
    ASSERT_INT(ltestatus->pcc_rxd_rssi, -92);
    ASSERT_INT(ltestatus->pcc_rxd_rsrp, -131);
    ASSERT_INT(ltestatus->tx_power, SW_GSTATUS_TX_POWER_INACTIVE);
    ASSERT_INT(ltestatus->tac, 0x34bb);
    ASSERT_FLOAT(ltestatus->rsrq, -10.7, FLOAT_TOLERANCE);
    ASSERT_INT(ltestatus->cell_id, 0x01c07902);
    ASSERT_FLOAT(ltestatus->sinr, 5.2, FLOAT_TOLERANCE);

    sw_em7565_free_status(ltestatus);
    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_gstatus_2() {

    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_gstatus_2.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_em7565_gstatus_response_t* ltestatus;
    sw_response_t ret = 0;

    ltestatus = sw_em7565_allocate_status();
    ret = sw_em7565_get_status(modem, ltestatus);

    if(ret >= SW_RESPONSE_CRITICAL) {
        return TEST_FAIL;
    }

    ASSERT_INT(ltestatus->current_time, 6334);
    ASSERT_INT(ltestatus->temperature, 37);
    ASSERT_INT(ltestatus->reset_counter, 2);
    ASSERT_STRING(ltestatus->mode, "LOW POWER MODE");

    sw_em7565_free_status(ltestatus);
    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_gstatus_3() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_gstatus_3.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_em7565_gstatus_response_t* ltestatus;
    sw_response_t ret = 0;

    ltestatus = sw_em7565_allocate_status();
    ret = sw_em7565_get_status(modem, ltestatus);

    if(ret >= SW_RESPONSE_CRITICAL) {
        return TEST_FAIL;
    }

    //if(ltestatus->current_time != 7480) return TEST_FAIL;
    ASSERT_INT(ltestatus->current_time, 7184);
    ASSERT_INT(ltestatus->temperature, 40);
    ASSERT_INT(ltestatus->reset_counter, 2);
    ASSERT_STRING(ltestatus->mode, "ONLINE");
    ASSERT_STRING(ltestatus->system_mode, "LTE");
    ASSERT_STRING(ltestatus->ps_state, "Attached");
    ASSERT_INT(ltestatus->lte_band, 3);
    ASSERT_INT(ltestatus->lte_bw_MHz, 20);
    ASSERT_INT(ltestatus->lte_rx_chan, 1300);
    ASSERT_INT(ltestatus->lte_tx_chan, 19300);
    ASSERT_STRING(ltestatus->lte_scc1_state, "ACTIVE");
    ASSERT_INT(ltestatus->lte_scc1_band, 3);
    ASSERT_INT(ltestatus->lte_scc1_bw_MHz, SW_GSTATUS_SCC_BW_UNKNOWN);
    ASSERT_INT(ltestatus->lte_scc1_chan, 1444);
    ASSERT_STRING(ltestatus->lte_scc2_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->lte_scc3_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->lte_scc4_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->emm_state, "Registered");
    ASSERT_STRING(ltestatus->rrc_state, "RRC Connected");
    ASSERT_STRING(ltestatus->ims_reg_state, "No Srv");
    ASSERT_INT(ltestatus->pcc_rxm_rssi, -61);
    ASSERT_INT(ltestatus->pcc_rxm_rsrp, -82);
    ASSERT_INT(ltestatus->pcc_rxd_rssi, -53);
    ASSERT_INT(ltestatus->pcc_rxd_rsrp, -79);
    ASSERT_INT(ltestatus->scc1_rxm_rssi, -58);
    ASSERT_INT(ltestatus->scc1_rxm_rsrp, -86);
    ASSERT_INT(ltestatus->scc1_rxd_rssi, -52);
    ASSERT_INT(ltestatus->scc1_rxd_rsrp, -80);
    ASSERT_INT(ltestatus->tx_power, 20);
    ASSERT_INT(ltestatus->tac, 0x34bb);
    ASSERT_FLOAT(ltestatus->rsrq, -10.0, FLOAT_TOLERANCE);
    ASSERT_INT(ltestatus->cell_id, 0x01c07902);
    ASSERT_FLOAT(ltestatus->sinr, 5.6, FLOAT_TOLERANCE);

    sw_em7565_free_status(ltestatus);
    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_gstatus_4() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_gstatus_4.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_em7565_gstatus_response_t* ltestatus;
    sw_response_t ret = 0;

    ltestatus = sw_em7565_allocate_status();
    ret = sw_em7565_get_status(modem, ltestatus);

    if(ret >= SW_RESPONSE_CRITICAL) {
        return TEST_FAIL;
    }

    ASSERT_INT(ltestatus->current_time, 7278);
    ASSERT_INT(ltestatus->temperature, 41);
    ASSERT_INT(ltestatus->reset_counter, 2);
    ASSERT_STRING(ltestatus->mode, "ONLINE");
    ASSERT_STRING(ltestatus->system_mode, "LTE");
    ASSERT_STRING(ltestatus->ps_state, "Attached");
    ASSERT_INT(ltestatus->lte_band, 3);
    ASSERT_INT(ltestatus->lte_bw_MHz, 20);
    ASSERT_INT(ltestatus->lte_rx_chan, 1300);
    ASSERT_INT(ltestatus->lte_tx_chan, 19300);
    ASSERT_STRING(ltestatus->lte_scc1_state, "INACTIVE");
    ASSERT_INT(ltestatus->lte_scc1_band, 3);
    ASSERT_INT(ltestatus->lte_scc1_bw_MHz, SW_GSTATUS_SCC_BW_UNKNOWN);
    ASSERT_INT(ltestatus->lte_scc1_chan, 1444);
    ASSERT_STRING(ltestatus->lte_scc2_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->lte_scc3_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->lte_scc4_state, "NOT ASSIGNED");
    ASSERT_STRING(ltestatus->emm_state, "Registered");
    ASSERT_STRING(ltestatus->rrc_state, "RRC Connected");
    ASSERT_STRING(ltestatus->ims_reg_state, "No Srv");
    ASSERT_INT(ltestatus->pcc_rxm_rssi, -56);
    ASSERT_INT(ltestatus->pcc_rxm_rsrp, -83);
    ASSERT_INT(ltestatus->pcc_rxd_rssi, -51);
    ASSERT_INT(ltestatus->pcc_rxd_rsrp, -80);
    ASSERT_INT(ltestatus->scc1_rxm_rssi, -63);
    ASSERT_INT(ltestatus->scc1_rxm_rsrp, -84);
    ASSERT_INT(ltestatus->scc1_rxd_rssi, -60);
    ASSERT_INT(ltestatus->scc1_rxd_rsrp, -84);
    ASSERT_INT(ltestatus->tx_power, SW_GSTATUS_TX_POWER_INACTIVE);
    ASSERT_INT(ltestatus->tac, 0x34bb);
    ASSERT_FLOAT(ltestatus->rsrq, -10.8, FLOAT_TOLERANCE);
    ASSERT_INT(ltestatus->cell_id, 0x01c07902);
    ASSERT_FLOAT(ltestatus->sinr, 5.2, FLOAT_TOLERANCE);

    sw_em7565_free_status(ltestatus);
    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_lteinfo_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_lteinfo_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_em7565_lteinfo_response_t* lteinfo;
    sw_response_t ret = 0;

    lteinfo = sw_em7565_allocate_lteinfo();
    ret = sw_em7565_get_lteinfo(modem, lteinfo);

    if(ret >= SW_RESPONSE_CRITICAL) {
        return TEST_FAIL;
    }

    ASSERT_INT(lteinfo->earfn, 1300);
    ASSERT_INT(lteinfo->mcc, 262);
    ASSERT_INT(lteinfo->mnc, 1);
    ASSERT_INT(lteinfo->tac, 13499);
    ASSERT_INT(lteinfo->cid, 0x01c07901);
    ASSERT_INT(lteinfo->band, 3);
    ASSERT_INT(lteinfo->d, 5);
    ASSERT_INT(lteinfo->u, 5);
    ASSERT_INT(lteinfo->snr, 6);
    ASSERT_INT(lteinfo->pci, 167);
    ASSERT_FLOAT(lteinfo->rsrq, -9.4, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->rsrp, -78.5, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->rssi, -46.1, FLOAT_TOLERANCE);
    ASSERT_INT(lteinfo->rxlv, 0);
    ASSERT_INT(lteinfo->nof_intrafreq_neighbours, 4);

    ASSERT_INT(lteinfo->intrafreq_neighbours[0].pci, 166);
    ASSERT_FLOAT(lteinfo->intrafreq_neighbours[0].rsrq, -13.0, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->intrafreq_neighbours[0].rsrp, -80.9, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->intrafreq_neighbours[0].rssi, -58.9, FLOAT_TOLERANCE);
    ASSERT_INT(lteinfo->intrafreq_neighbours[0].rxlv, 0);

    ASSERT_INT(lteinfo->intrafreq_neighbours[3].pci, 417);
    ASSERT_FLOAT(lteinfo->intrafreq_neighbours[3].rsrq, -20.0, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->intrafreq_neighbours[3].rsrp, -94.5, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->intrafreq_neighbours[3].rssi, -60.1, FLOAT_TOLERANCE);
    ASSERT_INT(lteinfo->intrafreq_neighbours[3].rxlv, 0);


    ASSERT_INT(lteinfo->nof_interfreq_neighbours, 5);

    ASSERT_INT(lteinfo->interfreq_neighbours[0].earfcn, 1444);
    ASSERT_INT(lteinfo->interfreq_neighbours[0].threshold_low, 0);
    ASSERT_INT(lteinfo->interfreq_neighbours[0].threshold_high, 0);
    ASSERT_INT(lteinfo->interfreq_neighbours[0].priority, 0);
    ASSERT_INT(lteinfo->interfreq_neighbours[0].pci, 293);
    ASSERT_FLOAT(lteinfo->interfreq_neighbours[0].rsrq, -13.8, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->interfreq_neighbours[0].rsrp, -82.8, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->interfreq_neighbours[0].rssi, -59.2, FLOAT_TOLERANCE);
    ASSERT_INT(lteinfo->interfreq_neighbours[0].rxlv, 0);

    ASSERT_INT(lteinfo->interfreq_neighbours[4].earfcn, 1444);
    ASSERT_INT(lteinfo->interfreq_neighbours[4].threshold_low, 0);
    ASSERT_INT(lteinfo->interfreq_neighbours[4].threshold_high, 0);
    ASSERT_INT(lteinfo->interfreq_neighbours[4].priority, 0);
    ASSERT_INT(lteinfo->interfreq_neighbours[4].pci, 0);
    ASSERT_FLOAT(lteinfo->interfreq_neighbours[4].rsrq, 0, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->interfreq_neighbours[4].rsrp, 0, FLOAT_TOLERANCE);
    ASSERT_FLOAT(lteinfo->interfreq_neighbours[4].rssi, 0, FLOAT_TOLERANCE);
    ASSERT_INT(lteinfo->interfreq_neighbours[4].rxlv, 0);


    sw_em7565_free_lteinfo(lteinfo);
    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_APN_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_apn_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;

    ret = sw_em7565_set_APN(modem, 1, "IP", "internet.telekom");

    if(ret >= SW_RESPONSE_CRITICAL) {
        return TEST_FAIL;
    }

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_scact_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_scact_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;

    ret = sw_em7565_set_data_connection(modem, 1);
    if(ret >= SW_RESPONSE_CRITICAL) {
        return TEST_FAIL;
    }

    ret = sw_em7565_get_data_connection(modem);
    ASSERT_INT(ret, 1);

    ret = sw_em7565_set_data_connection(modem, 0);
    if(ret >= SW_RESPONSE_CRITICAL) {
        return TEST_FAIL;
    }

    ret = sw_em7565_get_data_connection(modem);
    ASSERT_INT(ret, 0);

    ret = sw_em7565_set_data_connection(modem, 0);
    if(ret != SW_RESPONSE_ERROR) {
        return TEST_FAIL;
    }

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_selrat_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_selrat_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_em7565_radio_access_type_t rat;
    sw_response_t ret = 0;

    ret = sw_em7565_get_radio_access_type(modem, &rat);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(rat, SW_RAT_LTE_ONLY);

    ret = sw_em7565_set_radio_access_type(modem, SW_RAT_AUTOMATIC);
    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    ret = sw_em7565_get_radio_access_type(modem, &rat);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(rat, SW_RAT_AUTOMATIC);

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_reset_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_reset_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;

    ret = sw_em7565_reset(modem);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_ready_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_ready_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;

    ret = sw_em7565_is_ready(modem);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_want_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_want_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;
    sw_em7565_gps_antenna_power_mode_t mode = GPS_ANTENNA_POWER_ERROR;

    mode = sw_em7565_get_antenna_power(modem);
    ASSERT_INT(mode, GPS_ANTENNA_POWER_NONE);

    ret = sw_em7565_set_antenna_power(modem, GPS_ANTENNA_POWER_3V);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    mode = GPS_ANTENNA_POWER_ERROR;
    mode = sw_em7565_get_antenna_power(modem);
    ASSERT_INT(mode, GPS_ANTENNA_POWER_3V);

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_gpsautostart_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_gpsautostart_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;
    sw_em7565_gps_autostart_mode_t mode = GPS_AUTOSTART_ERROR;

    ret = sw_em7565_get_gps_autostart_mode(modem, &mode);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(mode, GPS_AUTOSTART_DISABLED);

    ret = sw_em7565_set_gps_autostart_mode(modem, GPS_AUTOSTART_ON_BOOT);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    mode = GPS_AUTOSTART_ERROR;
    ret = sw_em7565_get_gps_autostart_mode(modem, &mode);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(mode, GPS_AUTOSTART_ON_BOOT);

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_gpsstatus_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_gpsstatus_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;
    sw_em7565_gps_status_t status;
    status.last_fix_status = GPS_STATUS_UNKNOWN;
    status.fix_session_status = GPS_STATUS_UNKNOWN;

    ret = sw_em7565_gps_status(modem, &status);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(status.last_fix_status, GPS_STATUS_SUCCESS);
    ASSERT_INT(status.fix_session_status, GPS_STATUS_ACTIVE);

    ret = sw_em7565_gps_status(modem, &status);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(status.last_fix_status, GPS_STATUS_NONE);
    ASSERT_INT(status.fix_session_status, GPS_STATUS_NONE);

    ret = sw_em7565_gps_status(modem, &status);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(status.last_fix_status, GPS_STATUS_FAIL);
    ASSERT_INT(status.fix_session_status, GPS_STATUS_ACTIVE);

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_gps_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_gps_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;

    ret = sw_em7565_start_gps_default(modem);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    sw_em7565_gpsloc_response_t* gps;

    gps = sw_em7565_allocate_gpsloc();
    ret = sw_em7565_get_gpsloc(modem, gps);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(gps->is_invalid, 1);
    sw_em7565_free_get_gpsloc(gps);

    gps = sw_em7565_allocate_gpsloc();
    ret = sw_em7565_get_gpsloc(modem, gps);
    if(ret !=  SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(gps->is_invalid, 0);
    ASSERT_INT(gps->_raw_latitude, 0x0092774B);
    ASSERT_INT(gps->_raw_longitude, 0x00151559);
    // canonical
    ASSERT_FLOAT(gps->loc_unc_angle, 0.7, FLOAT_TOLERANCE);
    ASSERT_FLOAT(gps->loc_unc_a, 4.0, FLOAT_TOLERANCE);
    ASSERT_FLOAT(gps->loc_unc_p, 3.0, FLOAT_TOLERANCE);
    ASSERT_FLOAT(gps->hepe, 5.0, FLOAT_TOLERANCE);
    ASSERT_INT(gps->altitude, 151);
    ASSERT_FLOAT(gps->loc_unc_ve, 8.0, FLOAT_TOLERANCE);
    ASSERT_FLOAT(gps->heading, 1.1, FLOAT_TOLERANCE);
    ASSERT_FLOAT(gps->velocity_h, 3.2, FLOAT_TOLERANCE);
    ASSERT_FLOAT(gps->velocity_v, 6.6, FLOAT_TOLERANCE);
    sw_em7565_free_get_gpsloc(gps);


    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_entercnd_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_entercnd_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;

    int state;
    ret = sw_em7565_get_protected_commands(modem, &state);

    if(ret != SW_RESPONSE_FAILED) {
        return TEST_FAIL;
    }
    ASSERT_INT(state, false);

    ret = sw_em7565_set_protected_commands(modem, true);

    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    ret = sw_em7565_get_protected_commands(modem, &state);

    if(ret >= SW_RESPONSE_CRITICAL) {
        return TEST_FAIL;
    }
    ASSERT_INT(state, true);

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_band_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_band_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;

    ret = sw_em7565_select_band_config_profile(modem, 9);

    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    sw_em7565_band_profile_t profile;

    ret = sw_em7565_get_band_config_profile(modem, &profile);

    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(profile.config_idx, 9);
    ASSERT_STRING(profile.config_name, "LTE ALL")

    ASSERT_INT(profile.mask_gsm_umts, 0x0);
    ASSERT_INT(profile.mask_lte, 0xA700BA0E19DF);
    ASSERT_INT(profile.mask_tds, 0x0);
    ASSERT_INT(profile.mask_lte2, 0x2);
    ASSERT_INT(profile.mask_lte3, 0x0);
    ASSERT_INT(profile.mask_lte4, 0x0);

    sw_em7565_band_profile_list_t* list;
    list = sw_em7565_allocate_band_config_profile_list();
    ret = sw_em7565_get_band_config_profile_list(modem, list);
    ASSERT_INT(list->nof_profiles, 7);
    ASSERT_INT(list->profile[0]->mask_gsm_umts, 0x100600000FC00000);
    ASSERT_INT(list->profile[0]->mask_lte, 0x0000A700BA0E19DF);
    ASSERT_INT(list->profile[0]->mask_lte2, 0x2);
    ASSERT_INT(list->profile[0]->mask_lte4, 0x0);

    ASSERT_INT(list->profile[5]->mask_gsm_umts, 0x100600000FC00000);
    ASSERT_INT(list->profile[5]->mask_lte, 0x0);
    ASSERT_INT(list->profile[5]->mask_lte2, 0x0);
    ASSERT_INT(list->profile[5]->mask_lte4, 0x0);

    ASSERT_INT(list->profile[6]->mask_gsm_umts, 0x0);
    ASSERT_INT(list->profile[6]->mask_lte, 0x0000A700BA0E19DF);
    ASSERT_INT(list->profile[6]->mask_lte2, 0x2);
    ASSERT_INT(list->profile[6]->mask_lte4, 0x0);

    sw_em7565_free_band_config_profile_list(list);



    profile.config_idx = 10;
    strcpy(profile.config_name, "Test");
    profile.mask_gsm_umts = 0x0;
    profile.mask_lte = 0x7;
    profile.mask_lte2 = 0x0;
    profile.mask_lte3 = 0x0;
    profile.mask_lte4 = 0x0;
    profile.mask_tds = 0x0;

    ret = sw_em7565_add_band_config_profile(modem, &profile);
    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    ret = sw_em7565_remove_band_config_profile(modem, profile.config_idx);
    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int cmd_network_selection_1() {
    ASSERT_INIT();

    const char responses[] = TOSTRING(TEST_PATH)"/test/devices/sierra_wireless_em7565/at_cops_1.txt";
    sw_em7565_t* modem;
    modem = sw_em7565_init(responses);
    if(modem == NULL) return TEST_FAIL;

    sw_response_t ret = 0;

    network_list_item_t network;
    network.access_technology = COPS_AcT_DONTCARE;
    network.mccmnc = 26201;
    ret = sw_em7565_select_network(modem, COPS_MODE_MANUAL, COPS_FORMAT_MCCMNC, &network);
    if(ret != SW_RESPONSE_FAILED) {
        return TEST_FAIL;
    }

    network.access_technology = COPS_AcT_UTRAN;
    network.mccmnc = 26201;
    ret = sw_em7565_select_network(modem, COPS_MODE_MANUAL, COPS_FORMAT_MCCMNC, &network);
    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    network.access_technology = COPS_AcT_EUTRAN;
    network.mccmnc = 26201;
    ret = sw_em7565_select_network(modem, COPS_MODE_MANUAL, COPS_FORMAT_MCCMNC, &network);
    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }

    sw_em7565_network_list_t* list;
    list = sw_em7565_allocate_network_list();
    ret = sw_em7565_network_search(modem, list);
    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(list->nof_networks, 5);
    ASSERT_INT(list->network[0]->state, COPS_STATE_AVAILABLE);
    ASSERT_INT(list->network[0]->mccmnc, 26202);
    ASSERT_STRING(list->network[0]->name_long, "Vodafone.de");
    ASSERT_STRING(list->network[0]->name_short, "Vodafone");
    ASSERT_INT(list->network[0]->access_technology, COPS_AcT_UTRAN);

    ASSERT_INT(list->network[1]->state, COPS_STATE_AVAILABLE);
    ASSERT_INT(list->network[1]->mccmnc, 26203);
    ASSERT_STRING(list->network[1]->name_long, "E-Plus");
    ASSERT_STRING(list->network[1]->name_short, "E-Plus");
    ASSERT_INT(list->network[1]->access_technology, COPS_AcT_EUTRAN);
    sw_em7565_free_network_list(list);

    list = sw_em7565_allocate_network_list();
    ret = sw_em7565_network_search(modem, list);
    if(ret != SW_RESPONSE_SUCCESS) {
        return TEST_FAIL;
    }
    ASSERT_INT(list->nof_networks, 1);
    ASSERT_INT(list->network[0]->state, COPS_STATE_AVAILABLE);
    ASSERT_INT(list->network[0]->mccmnc, 26201);
    ASSERT_STRING(list->network[0]->name_long, "Telekom.de");
    ASSERT_STRING(list->network[0]->name_short, "TDG");
    ASSERT_INT(list->network[0]->access_technology, COPS_AcT_EUTRAN);
    sw_em7565_free_network_list(list);


    sw_em7565_current_operator_t* operator;
    operator = sw_em7565_allocate_current_operator();
    ret = sw_em7565_get_current_operator(modem, operator);
    if(ret != SW_RESPONSE_FAILED) {
      return TEST_FAIL;
    }
    sw_em7565_free_current_operator(operator);


    operator = sw_em7565_allocate_current_operator();
    ret = sw_em7565_get_current_operator(modem, operator);
    if(ret != SW_RESPONSE_SUCCESS) {
      return TEST_FAIL;
    }
    ASSERT_STRING(operator->name_long, "Telekom.de Telekom.de");
    sw_em7565_free_current_operator(operator);

    sw_em7565_destroy(modem);

    return ASSERT_RESULT();
}

int main(int argc, char** argv) {

    ASSERT_INIT();

    ASSERT_CALL(cmd_reset_1());
    ASSERT_CALL(cmd_ready_1());
    ASSERT_CALL(cmd_APN_1());
    ASSERT_CALL(cmd_gstatus_1());
    ASSERT_CALL(cmd_gstatus_2());
    ASSERT_CALL(cmd_gstatus_3());
    ASSERT_CALL(cmd_gstatus_4());
    ASSERT_CALL(cmd_lteinfo_1());
    ASSERT_CALL(cmd_scact_1());
    ASSERT_CALL(cmd_selrat_1());
    ASSERT_CALL(cmd_gpsautostart_1());
    ASSERT_CALL(cmd_gpsstatus_1());
    ASSERT_CALL(cmd_want_1());
    ASSERT_CALL(cmd_gps_1());
    ASSERT_CALL(cmd_entercnd_1());
    ASSERT_CALL(cmd_band_1());
    ASSERT_CALL(cmd_network_selection_1());

    return ASSERT_RESULT();
}
