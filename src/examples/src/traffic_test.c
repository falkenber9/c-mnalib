#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>

#include <argp.h>

#include <pthread.h>

#include "cmnalib/traffic_curl.h"

#include "cmnalib/traffic_types.h"
#include "cmnalib/gps_transform.h"
#include "cmnalib/trace_logger.h"
#include "cmnalib/network_interface.h"
#include "cmnalib/logger.h"

#include "cmnalib/at_sierra_wireless_mc7455.h"

#define TRACE_DIR "/tmp"
#define FAULT_RECOVERY_TIME_SEC 5
#define MAX_FAULTS_TO_CANCEL 10

#define VERY_SMALL_VALUE (1e-3)

#define INTERFACE_NAME "eth1"

typedef enum program_state {
    STATE_NORMAL_OPERATION = 0,
    STATE_FAILURE_RESUME,
    STATE_FAILURE_EXIT,
    STATE_FINISH,
} program_state_t;

static volatile program_state_t state = 0;

const char *argp_program_version =
        "traffic_test";
const char *argp_program_bug_address =
        "<robert.falkenberg@tu-dortmund.de>";

static char doc[] =
        "traffic_test -- a mobile network traffic generator and tracer";

static char args_doc[] = "[defunct]ARG1 [defunct]ARG2";

static struct argp_option options[] = {
    {"verbose",  'v', 0,      0,  "[defunct] Produce verbose output" },
    {"quiet",    'q', 0,      0,  "[defunct] Don't produce any output" },
//  {"silent",   's', 0,      OPTION_ALIAS },
    {"output",   'o', "DIR",  0,   "Write traces to DIR instead of /tmp" },
    {"address",  'a', "URL",  0,   "Set target URL instead of mptcp1.pi21.de:5002" },
    {"repeats",  'n', "N",    0,   "Number of repetitions (default: 1)" },
    {"pause",    'p', "sec",  0,   "Pause between repetitions in sec (default: 30)" },
    {"size",     's', "bytes",0,   "Payload size in bytes (default: 5e6)" },
    {"interval", 'i', "sec",  0,   "Minimal interval for status reports in sec (default: 1.0)" },
    {"wait",     'w', "sec",  0,   "Waittime between modem init and transmission (default: 3)" },
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments {
    char *args[2];                /* arg1 & arg2 */
    int silent, verbose;
    char *trace_dir;
    char *url;
    int wait_sec;
    int repeats;
    int repeat_pause;
    int payload_size;
    double interval_sec;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state) {
    /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    switch (key)
    {
    case 'q':
//    case 's':
        arguments->silent = 1;
        break;
    case 'v':
        arguments->verbose = 1;
        break;
    case 'o':
        arguments->trace_dir = arg;
        break;

//    case ARGP_KEY_ARG:
//        if (state->arg_num >= 2) {
//            /* Too many arguments. */
//            argp_usage (state);
//        }
//        arguments->args[state->arg_num] = arg;
//        break;

//    case ARGP_KEY_END:
//        if (state->arg_num < 2)
//            /* Not enough arguments. */
//            argp_usage (state);
//        break;
    case 'a':
        arguments->url = arg;
        break;

    case 'n':
        arguments->repeats = atoi(arg);
        break;

    case 'p':
        arguments->repeat_pause = atoi(arg);
        break;

    case 's':
        arguments->payload_size = atoi(arg);
        break;

    case 'i':
        arguments->interval_sec = atof(arg);
        break;

    case 'w':
        arguments->wait_sec = atoi(arg);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void int_handler(int sig) {
    signal(sig, SIG_IGN);
    INFO("Terminating by Signal %d\n", sig);
    state = STATE_FINISH;
}

int very_small(double value) {
    return fabs(value) < VERY_SMALL_VALUE;
}

int generate_filename(char* buf, int buf_len, const char *logdir, struct arguments* args) {

    time_t rawtime;
    struct tm* timeinfo;
    char timestring[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(timestring, sizeof(timestring), "%Y%m%d-%H%M%S", timeinfo);

    // Construct filename
    sprintf(buf, "%s/cmna-trace-%s-n%d-p%d-s%d-i%f-w%d.log", logdir, timestring,
            args->repeats, args->repeat_pause, args->payload_size, args->interval_sec, args->wait_sec);

    return 0;
}


typedef struct modem_status {
    pthread_mutex_t lock;
    sw_mc7455_gstatus_response_t* ltestatus;
    sw_mc7455_lteinfo_response_t* lteinfo;
    sw_mc7455_gpsloc_response_t* gps;
} modem_status_t;

typedef struct status_collector_context {
    sw_mc7455_t* modem;
    modem_status_t* modem_status;
    volatile program_state_t* state;
} status_collector_context_t;

void* status_collector(void* void_context) {
    status_collector_context_t* context = (status_collector_context_t*)void_context;

    sw_response_t ret;
    sw_mc7455_gstatus_response_t* ltestatus_old;
    sw_mc7455_lteinfo_response_t* lteinfo_old;
    sw_mc7455_gpsloc_response_t* gps_old;
    sw_mc7455_gstatus_response_t* ltestatus;
    sw_mc7455_lteinfo_response_t* lteinfo;
    sw_mc7455_gpsloc_response_t* gps;

    struct timeval t_start, t_end, t_delta, t_min, t_remain;
    t_min.tv_sec = 1;
    t_min.tv_usec = 0;

    while(*context->state == STATE_NORMAL_OPERATION) {
        DEBUG("Asynchronous status collection\n");

        gettimeofday(&t_start, NULL);

        ret = sw_mc7455_get_status(context->modem, &ltestatus);
        if(ret >= SW_RESPONSE_CRITICAL) {
            *context->state = STATE_FAILURE_RESUME;
            break;
        }
        ret = sw_mc7455_get_lteinfo(context->modem, &lteinfo);
        if(ret >= SW_RESPONSE_CRITICAL) {
            *context->state = STATE_FAILURE_RESUME;
            break;
        }
        ret = sw_mc7455_get_gpsloc(context->modem, &gps);
        if(ret >= SW_RESPONSE_CRITICAL) {
            *context->state = STATE_FAILURE_RESUME;
            break;
        }

        pthread_mutex_lock(&(context->modem_status->lock));
        ltestatus_old = context->modem_status->ltestatus;
        lteinfo_old = context->modem_status->lteinfo;
        gps_old = context->modem_status->gps;

        context->modem_status->ltestatus = ltestatus;
        context->modem_status->lteinfo = lteinfo;
        context->modem_status->gps = gps;
        pthread_mutex_unlock(&(context->modem_status->lock));

        sw_mc7455_free_status(ltestatus_old);
        sw_mc7455_free_lteinfo(lteinfo_old);
        sw_mc7455_free_get_gpsloc(gps_old);

        gettimeofday(&t_end, NULL);
        timeval_subtract(&t_delta, &t_end, &t_start);
        if(!timeval_subtract(&t_remain, &t_min, &t_delta)) {
            struct timespec t_remain_ns;
            t_remain_ns.tv_sec = t_remain.tv_sec;
            t_remain_ns.tv_nsec = 1000 * t_remain.tv_usec;
            nanosleep(&t_remain_ns, NULL);
        }
    }
    DEBUG("Asynchronous status collection finished\n");

    return NULL;
}

/**
  Container structure for passing access/handles to callback functions
  which are frequently called by the traffic-generator during transfer
*/
typedef struct progress_callback_context {
    volatile program_state_t* state;
    modem_status_t* modem_status;
    trace_handle_t* trace_handle;
    int trace_transmission_counter;
    double old_latitude;
    double old_longitude;
    double total_distance;
} progress_callback_context_t;

int progress_callback(void* void_context, transfer_statusreport_t* status) {
    progress_callback_context_t* context = (progress_callback_context_t*)void_context;
    if(context != NULL && status != NULL) {
        sw_mc7455_gstatus_response_t* ltestatus = NULL;
        sw_mc7455_lteinfo_response_t* lteinfo = NULL;
        sw_mc7455_gpsloc_response_t* gps = NULL;
        trace_data_t d = {0};
        struct timeval t;
        gettimeofday(&t, NULL);

        if(pthread_mutex_lock(&(context->modem_status->lock)) != 0) {
            ERROR("Failed to lock mutex in progress_callback\n");
        };

        ltestatus = context->modem_status->ltestatus;
        lteinfo = context->modem_status->lteinfo;
        gps = context->modem_status->gps;

        d.time_sec = t.tv_sec;
        d.time_usec = t.tv_usec;

        d.trace_transmission_counter = context->trace_transmission_counter;
        d.datarate = status->datarate_ul;

        if(ltestatus != NULL) {
            d.sinr = ltestatus->sinr;
            d.rsrq = ltestatus->rsrq;
            d.pcc_rsrp = (ltestatus->pcc_rxm_rsrp + ltestatus->pcc_rxd_rsrp)/2;
            d.scc_rsrp = (ltestatus->scc_rxm_rsrp + ltestatus->scc_rxd_rsrp)/2;
            d.pcc_rssi = (ltestatus->pcc_rxm_rssi + ltestatus->pcc_rxd_rssi)/2;
            d.scc_rssi = (ltestatus->scc_rxm_rssi + ltestatus->scc_rxd_rssi)/2;
            d.tx_power = ltestatus->tx_power;

            d.lte_band = ltestatus->lte_band;
            d.lte_bw_MHz = ltestatus->lte_bw_MHz;
            d.lte_rx_chan = ltestatus->lte_rx_chan;
            d.lte_tx_chan = ltestatus->lte_tx_chan;
            d.lte_scell_band = ltestatus->lte_scell_band;
            d.lte_scell_bw_MHz = ltestatus->lte_scell_bw_MHz;
            d.lte_scell_chan = ltestatus->lte_scell_chan;

            d.cell_id = ltestatus->cell_id;
        }
        else {
            WARNING("INVALID ltestatus Data\n");
            d.sinr = 0;
            d.rsrq = 0;
            d.pcc_rsrp = 0;
            d.scc_rsrp = 0;
            d.pcc_rssi = 0;
            d.scc_rssi = 0;
            d.tx_power = 0;

            d.lte_band = 0;
            d.lte_bw_MHz = 0;
            d.lte_rx_chan = 0;
            d.lte_tx_chan = 0;
            d.lte_scell_band = 0;
            d.lte_scell_bw_MHz = 0;
            d.lte_scell_chan = 0;

            d.cell_id = 0;
        }

        if(lteinfo != NULL) {
            d.rxlv = lteinfo->rxlv;

            d.mcc = lteinfo->mcc;
            d.mnc = lteinfo->mnc;
            d.tac = lteinfo->tac;
            d.pci = lteinfo->pci;

            d.nof_intrafreq_neighbours = lteinfo->nof_intrafreq_neighbours;
            d.nof_interfreq_neighbours = lteinfo->nof_interfreq_neighbours;
        }
        else {
            WARNING("INVALID lteinfo Data\n");
            d.rxlv = 0;

            d.mcc = 0;
            d.mnc = 0;
            d.tac = 0;
            d.pci = 0;

            d.nof_intrafreq_neighbours = 0;
            d.nof_interfreq_neighbours = 0;
        }


        if(gps != NULL && gps->is_invalid == 0) {
            DEBUG("Got GPS\n");
            d.latitude = gps->latitude;
            d.longitude = gps->longitude;
            d.altitude = gps->altitude;
            d.velocity_h = gps->velocity_h;
            d.velocity_v = gps->velocity_v;
        }
        else {
            WARNING("INVALID gps Data\n");
            d.latitude = 0;
            d.longitude = 0;
            d.altitude = 0;
            d.velocity_h = 0;
            d.velocity_v = 0;
        }

        if(pthread_mutex_unlock(&(context->modem_status->lock)) != 0) {
            ERROR("Failed to unlock mutex in progress_callback\n");
        };

        // Compute distance since last measurement
        if(     !very_small(context->old_latitude) &&
                !very_small(context->old_longitude) &&
                !very_small(d.latitude) &&
                !very_small(d.longitude)) {
            context->total_distance += gt_get_distance(context->old_latitude, context->old_longitude,
                                                       d.latitude, d.longitude);
            d.total_distance = context->total_distance;
        }
        // Save current pos for next time
        context->old_latitude = d.latitude;
        context->old_longitude = d.longitude;

        write_trace(context->trace_handle, &d);
    }
    else {
        ERROR("context or status not set\n");
    }
    if(context != NULL) {
            // trigger cancel of transmission if required
        if(*context->state != STATE_NORMAL_OPERATION) return -1;
    }
    return 0;
}

void init_default_arguments(struct arguments* arguments) {
    arguments->silent = 0;
    arguments->verbose = 0;
    arguments->trace_dir = "/tmp";
    arguments->url = "mptcp1.pi21.de:5002";
    arguments->wait_sec = 3;
    arguments->repeats = 1;
    arguments->repeat_pause = 30;
    arguments->payload_size = 5e6;
    arguments->interval_sec = 1.0;
}

int configure_modem(sw_mc7455_t* modem) {
    sw_response_t ret;

    // Init data connection

    //ret = sw_mc7455_set_APN(modem, APN_SLOT_DEFAULT, APN_IP_VERSION_ANY, "internet.telekom");
    //if(ret != RESPONSE_OK) {
    //    WARNING("Failed to setup APN, continue\n");
    //}

    ret = sw_mc7455_set_radio_access_type(modem, SW_MC7455_RAT_LTE_ONLY);
    if(ret > SW_RESPONSE_SUCCESS) {
        WARNING("Could not set radio access type to LTE only\n");
    }

    ret = sw_mc7455_get_data_connection(modem);
    if(ret == DATA_CONNECTION_STATUS_ERROR) {
        WARNING("Failed to get data connection status\n");
    }
    if(ret != DATA_CONNECTION_STATUS_ENABLED) {
        // try to establish data connection if disabled or error
        ret = sw_mc7455_set_data_connection(modem, DATA_CONNECTION_STATUS_ENABLED);
        if(ret > SW_RESPONSE_SUCCESS) {
            ERROR("Could not establish data connection\n");
            return -1;
        }
        DEBUG("Data connection established\n");
    }

    // Restart ethernet interface
//#define _RESTART_INTERFACE_
#ifdef _RESTART_INTERFACE_
    set_if_down(INTERFACE_NAME, 0);
    sleep(2);
    set_if_up(INTERFACE_NAME, 0);
#endif

    // Configure GPS
    /*
    WARNING("ANTENNA POWER MODE: %d\n", sw_mc7455_get_antenna_power(modem));
    sw_mc7455_set_antenna_power(modem, GPS_ANTENNA_POWER_NONE);
    WARNING("ANTENNA POWER MODE: %d\n", sw_mc7455_get_antenna_power(modem));
    sw_mc7455_set_antenna_power(modem, GPS_ANTENNA_POWER_3V);
    WARNING("ANTENNA POWER MODE: %d\n", sw_mc7455_get_antenna_power(modem));
    */

    /*
    ret = sw_mc7455_stop_gps(modem);
    if(ret == RESPONSE_OK) {
        DEBUG("GPS Stopped Successfully\n");
    }
    else {
        WARNING("GPS Stop Failed\n");
    }
    */

    // Start GPS
    ret = sw_mc7455_start_gps_default(modem);
    if(ret == SW_RESPONSE_SUCCESS) {
        DEBUG("GPS Activated Successfully\n");
    }
    else {
        WARNING("GPS Activation Failed\n");
    }

    return 0;
}

void init_modem_status(modem_status_t* modem_status) {
    modem_status->ltestatus = NULL;
    modem_status->lteinfo = NULL;
    modem_status->gps = NULL;

    if(pthread_mutex_init(&modem_status->lock, NULL) != 0) {
        ERROR("Could not init mutex\n");
        abort();
    }
}

void release_modem_status(modem_status_t* modem_status) {
    pthread_mutex_destroy(&modem_status->lock);
}

typedef struct status_collector_thread {
    pthread_t* thread;
    status_collector_context_t* collector_context;
} status_collector_thread_t;

status_collector_thread_t* launch_status_collector(sw_mc7455_t* modem, modem_status_t* modem_status) {
    status_collector_thread_t* sct = calloc(1, sizeof(status_collector_thread_t));
    sct->collector_context = calloc(1, sizeof(status_collector_context_t));
    sct->thread = calloc(1, sizeof(pthread_t));

    sct->collector_context->modem = modem;
    sct->collector_context->modem_status = modem_status;
    sct->collector_context->state = &state;

    if(pthread_create(sct->thread, NULL, status_collector, (void*)sct->collector_context)) {
        ERROR("Could not create status collector thread");
        abort();
    }

    return sct;
}

void terminate_status_collector(status_collector_thread_t* sct) {
    pthread_join(*sct->thread, NULL);
    free(sct->thread);
    free(sct->collector_context);
    free(sct);
}

progress_callback_context_t* init_progress_callback_context(modem_status_t* modem_status, trace_handle_t* trace, int iteration) {
    progress_callback_context_t* context = calloc(1, sizeof(progress_callback_context_t));

    context->state = &state;
    context->modem_status = modem_status;
    context->trace_handle = trace;
    context->trace_transmission_counter = iteration;
    context->old_latitude = 0;
    context->old_longitude = 0;
    context->total_distance = 0;

    return context;
}

void release_progress_callback_context(progress_callback_context_t* context) {
    free(context);
}

int perform_transmissions(int repeats,
                          int skip,
                          int repeat_pause,
                          const char* url,
                          int payload_size,
                          progress_callback_context_t* context,
                          double interval_sec) {
    int i = skip;
    while(i<repeats) {
        if(i > skip) sleep(repeat_pause);    // skip sleep on first loop

        INFO("Starting Transmission %d/%d\n", i+1, repeats);
        tc_upload_randomdata(url, payload_size, &progress_callback, context, interval_sec);
        context->trace_transmission_counter++;

        i++;
        if(state != STATE_NORMAL_OPERATION) break;
    }
    return i;
}

int main(int argc, char** argv) {
    int finished_repeats = 0;
    int faultcount = 0;
    struct arguments arguments;
    init_default_arguments(&arguments);
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    state = STATE_NORMAL_OPERATION;

    // Setup signal handler
    signal(SIGINT, int_handler);

    // Init tracefile
    char tracefilename[255];
    generate_filename(tracefilename, sizeof(tracefilename), arguments.trace_dir, &arguments);
    trace_handle_t* trace = trace_init(tracefilename);
    if(trace == NULL) {
        return EXIT_FAILURE;
    }
    write_trace_header(trace);

    while(state <= STATE_FAILURE_RESUME) {
        if(state == STATE_FAILURE_RESUME) {
            faultcount++;

            if(faultcount > MAX_FAULTS_TO_CANCEL) {
                ERROR("Too many faults, giving up.\n");
                state = STATE_FAILURE_EXIT;
                break;
            }
            WARNING("Try to recover from failure (%d/%d)\n", faultcount, MAX_FAULTS_TO_CANCEL);
            sleep(FAULT_RECOVERY_TIME_SEC);
            // was there an ctrl+c ? -> quit
            if(state > STATE_FAILURE_RESUME) continue;

            state = STATE_NORMAL_OPERATION;
        }

        status_collector_thread_t* status_collector_thread;

        // Open modem interface
        sw_mc7455_t* modem;
        modem = sw_mc7455_init_first();
        if(modem == NULL) {
            ERROR("Could not initialize modem\n");
            state = STATE_FAILURE_RESUME;
            continue;
        }
        DEBUG("Opened modem\n");

        // Setup Modem for experiment
        if(configure_modem(modem) != 0) {
            ERROR("Modem setup failed due to critical error\n");
            state = STATE_FAILURE_RESUME;
            sw_mc7455_destroy(modem);
            continue;
        }
        DEBUG("Modem setup completed\n");

        // Wait for modem to initialize
        sleep(arguments.wait_sec);

        // Setup and launch status collector
        modem_status_t modem_status;
        init_modem_status(&modem_status);
        status_collector_thread = launch_status_collector(modem, &modem_status);

        // Setup progress reporter context
        progress_callback_context_t* context = init_progress_callback_context(&modem_status, trace, finished_repeats);

        // Reset faultcounter
        faultcount = 0;

        // Run the transmission loop
        finished_repeats = perform_transmissions(arguments.repeats,
                                                 finished_repeats,
                                                 arguments.repeat_pause,
                                                 arguments.url,
                                                 arguments.payload_size,
                                                 context,
                                                 arguments.interval_sec);
        release_progress_callback_context(context);

        if(state == STATE_NORMAL_OPERATION) state = STATE_FINISH;

        terminate_status_collector(status_collector_thread);
        release_modem_status(&modem_status);
        sw_mc7455_destroy(modem);
    }

    trace_destroy(trace);

    return EXIT_SUCCESS;
}
