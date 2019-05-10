
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cmnalib/at_interface.h" 
#include "cmnalib/logger.h"

#define LINE_SEPARATOR "\r\n"

struct timeval t_start, t_end, t_delta;

struct at_interface_t {
    int filedescr;
    int nof_timeouts;
    char* tty_device_path;
    char* line_separator;
    struct termios old_tty_settings;
    struct termios current_tty_settings;
};

at_interface_t* at_interface_open_tty(const char* tty_device_path) {
    at_interface_t* h = calloc(1, sizeof(at_interface_t));
    if(h == NULL) {
        ERROR("Error in calloc\n");
        return NULL;
    }

    /* open tty device */

    if(tty_device_path == NULL) {
        ERROR("Missing device path\n");
        return NULL;
    }
    h->tty_device_path = malloc(strlen(tty_device_path)+1);
    h->line_separator = malloc(sizeof(LINE_SEPARATOR));
    if(h->tty_device_path == NULL || h->line_separator == NULL) {
        ERROR("Error in malloc\n");
        return NULL;
    }
    strcpy(h->tty_device_path, tty_device_path);
    strcpy(h->line_separator, LINE_SEPARATOR);

    h->filedescr = open(h->tty_device_path,
                        O_RDWR |
                        O_NOCTTY |
                        O_DSYNC
                        );
    if(h->filedescr < 0) {
        ERROR("Could not open device '%s': %s\n", h->tty_device_path, strerror(errno));
        return NULL;
    }

    /* configure tty device into raw mode */
    tcgetattr(h->filedescr, &h->old_tty_settings);
    h->current_tty_settings = h->old_tty_settings;
    h->current_tty_settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                                         | INLCR | IGNCR | ICRNL | IXON);
    h->current_tty_settings.c_oflag &= ~(OPOST);
    h->current_tty_settings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    h->current_tty_settings.c_cflag &= ~(CSIZE | PARENB);
    h->current_tty_settings.c_cflag |= CS8;
    tcsetattr(h->filedescr, TCSANOW, &h->current_tty_settings);

    return h;
}

void at_interface_close_tty(at_interface_t* h) {
    if(h != NULL) {
        DEBUG("Restoring previous tty settings\n");
        tcsetattr(h->filedescr, TCSANOW, &h->old_tty_settings);

        DEBUG("Closing interface %s\n", h->tty_device_path);
        close(h->filedescr);

        DEBUG("Release AT interface resources\n");
        free(h->tty_device_path);
        free(h->line_separator);
        free(h);
    }
}

at_interface_response_status_t at_interface_command_tty(at_interface_t* h,
                                              const at_interface_command_t* cmd,
                                              const char* params,
                                              at_interface_response_t** response) {

    at_interface_response_status_t result = AT_RESPONSE_UNKNOWN;

    if(response == NULL) {
        ERROR("Invalid argument\n");
        return AT_RESPONSE_INVAL;
    }

    *response = calloc(1, sizeof(at_interface_response_t));
    if(*response == NULL) {
        ERROR("Error in calloc\n");
        return AT_RESPONSE_OUT_OF_MEMORY;
    }

    char tty_cmd[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH +
            AT_INTERFACE_MAX_LINE_SEPARATOR_LENGTH];
    fd_set fdset;
    struct timeval timeout;
    int ret, len, pos;

    strcpy(tty_cmd, cmd->command_string);
    if(params != NULL) strcat(tty_cmd, params);
    strcat(tty_cmd, h->line_separator);

    pos = 0;
    (*response)->response_len = 0;
    (*response)->response_string[0] = 0;

    DEBUG("Flushing tty input and output buffers\n");
    ret = tcflush(h->filedescr, TCIOFLUSH);
    if(ret == -1) {
        ERROR("Error flushing tty IO buffers: %s\n", strerror(errno));
        return AT_RESPONSE_IO_ERROR;
    }

    DEBUG("Writing command to tty: %s\n", tty_cmd);
    gettimeofday(&t_start, NULL);
    ret = write(h->filedescr, tty_cmd, strlen(tty_cmd));
    if(ret == -1) {
        ERROR("Error while writing to tty: %s\n", strerror(errno));
        return AT_RESPONSE_IO_ERROR;
    }

    FD_ZERO(&fdset);
    FD_SET(h->filedescr, &fdset);

    DEBUG("Reading response from tty\n");
    while(1){
        timeout.tv_sec = cmd->timeout_sec;
        timeout.tv_usec = cmd->timeout_usec;
        /* wait for new data in buffer (or timeout)*/
        ret = select(h->filedescr + 1, &fdset, NULL, NULL, &timeout);
        if(ret == -1) {
            ERROR("Error in select(): %s\n", strerror(errno));
            result = AT_RESPONSE_IO_ERROR;
            break;
        }
        else if(ret == 0) {
            WARNING("Timeout for AT command response\n");
            h->nof_timeouts++;
            if(h->nof_timeouts > AT_INTERFACE_MAX_CONSECUTIVE_TIMEOUTS) {
                ERROR("Too many consecutive timeouts (%d), assuming IO error", AT_INTERFACE_MAX_CONSECUTIVE_TIMEOUTS);
                result = AT_RESPONSE_IO_ERROR;
                break;
            }
            else {
                result = AT_RESPONSE_TIMEOUT;
                break;
            }
        }
        else {
            len = read(h->filedescr,
                       &(*response)->response_string[pos],
                       sizeof((*response)->response_string)-pos-1);
            if(len == -1) {
                ERROR("Error while reading from tty: %s\n", strerror(errno));
                result = AT_RESPONSE_IO_ERROR;
                break;
            }

            h->nof_timeouts = 0;

            DEBUG("Read %d chars from tty\n", len);
            pos+=len;
            (*response)->response_string[pos] = 0;

            if(len == 0) {
                WARNING("Received only 0 bytes, probably I/O issue\n");
                result = AT_RESPONSE_IO_ERROR;
                break;
            }
            if(strstr((*response)->response_string, "OK\r\n")) {
                DEBUG("Early finished due to complete response: OK\n");
                result = AT_RESPONSE_SUCCESS;
                break;
            }
            if(strstr((*response)->response_string, "ERROR\r\n")) {
                DEBUG("Early finished due to complete response: ERROR\n");
                result = AT_RESPONSE_FAILED;
                break;
            }
            if(strstr((*response)->response_string, "+CME ERROR")) {
                // TODO: consider error code, e.g. "+CME ERROR: SIM failure"
                DEBUG("Early finished due to +CME ERROR\n");
                result = AT_RESPONSE_FAILED;
                break;
            }
        }
    }
    gettimeofday(&t_end, NULL);
    timeval_subtract(&t_delta, &t_end, &t_start);
    DEBUG("Command took %d.%06d response:\n%s", t_delta.tv_sec, t_delta.tv_usec, (*response)->response_string);

    return result;
}

void at_interface_free_response_tty(at_interface_response_t* r) {
    if(r != NULL) {
        r->response_len = 0;
    }
    free(r);
}
