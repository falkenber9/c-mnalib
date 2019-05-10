
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <time.h>
#include <sys/types.h>

#include "cmnalib/trace_logger.h"
#include "cmnalib/logger.h"

trace_handle_t* trace_init(const char *filename) {
    trace_handle_t* h = calloc(1, sizeof(trace_handle_t));
    if(h != NULL) {
        errno = 0;
        h->tracefile = fopen(filename, "w");
        if(h->tracefile != NULL) {
            DEBUG("Initialized tracefile '%s'\n", filename);
        }
        else {
            ERROR("Could not open file '%s': %s\n", filename, strerror(errno));
            free(h);
            h = NULL;
        }
    }
    else {
        ERROR("Could not alloc memory\n");
    }
    return h;
}

void trace_destroy(trace_handle_t* h) {
    DEBUG("Closing tracefile, releasing resources\n");
    if(h != NULL && h->tracefile != NULL) {
        fclose(h->tracefile);
        h->tracefile = NULL;
    }
    free(h);
    h = NULL;
}

void write_trace_header(trace_handle_t* h) {
    if(h != NULL && h->tracefile != NULL) {
        TEE_STREAM(fprintf, h->tracefile,
                   FINFO, INFO_STREAM,
                   "time_sec, "
                   "trace_transmission_counter, datarate, "
                   "sinr, rsrq, pcc_rsrp, scc_rsrp, pcc_rssi, scc_rssi, tx_power, rxlv, "
                   "lte_band, lte_bw_MHz, lte_rx_chan, lte_tx_chan, lte_scell_band, lte_scell_bw_MHz, lte_scell_chan, "
                   "mcc, mnc, tac, cell_id, pci, "
                   "nof_intrafreq_neighbours, nof_interfreq_neighbours, "
                   "total_distance, latitude, longitude, altitude, velocity_h, velocity_v\n");
    }
}

void write_trace(trace_handle_t* h,
                 trace_data_t* d) {
    if(h != NULL && h->tracefile != NULL && d != NULL) {
        TEE_STREAM(fprintf, h->tracefile,
                   FINFO, INFO_STREAM,
                   "%ld.%06ld, "
                   "%d, %f, "
                   "%.1f, %.1f, %d, %d, %d, %d, %d, %d, "
                   "%d, %d, %d, %d, %d, %d, %d, "
                   "%d, %d, %d, %d, %d, "
                   "%d, %d, "
                   "%f, %f, %f, %d, %.1f, %.1f "
                   "\n",
                   d->time_sec, d->time_usec,
                   d->trace_transmission_counter, d->datarate,
                   d->sinr, d->rsrq, d->pcc_rsrp, d->scc_rsrp, d->pcc_rssi, d->scc_rssi, d->tx_power, d->rxlv,
                   d->lte_band, d->lte_bw_MHz, d->lte_rx_chan, d->lte_tx_chan, d->lte_scell_band, d->lte_scell_bw_MHz, d->lte_scell_chan,
                   d->mcc, d->mnc, d->tac, d->cell_id, d->pci,
                   d->nof_intrafreq_neighbours, d->nof_interfreq_neighbours,
                   d->total_distance, d->latitude, d->longitude, d->altitude, d->velocity_h, d->velocity_v);
    }
}
