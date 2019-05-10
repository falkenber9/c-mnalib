#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct trace_handle {
    FILE* tracefile;
} trace_handle_t;

typedef struct trace_data {
    // time
    long time_sec;
    long time_usec;

    // transfer metrics
    int trace_transmission_counter;
    double datarate;

    // link quality
    float sinr;
    float rsrq;
    int pcc_rsrp;
    int scc_rsrp;
    int pcc_rssi;
    int scc_rssi;
    int tx_power;
    int rxlv;

    // link config
    int lte_band;
    int lte_bw_MHz;
    int lte_rx_chan;
    int lte_tx_chan;
    int lte_scell_band;
    int lte_scell_bw_MHz;
    int lte_scell_chan;

    // network info
    int mcc;
    int mnc;
    int tac;
    int cell_id;
    int pci;

    // position
    int nof_intrafreq_neighbours;
    int nof_interfreq_neighbours;

    // location
    double total_distance;  // meters since start of trace
    double latitude;
    double longitude;
    int altitude;
    float velocity_h;       // meter/sec
    float velocity_v;       // meter/sec

} trace_data_t;

trace_handle_t* trace_init(const char *filename);
void trace_destroy(trace_handle_t* h);

void write_trace_header(trace_handle_t* h);
void write_trace(trace_handle_t* h,
                 trace_data_t* d);

#ifdef __cplusplus
}
#endif
