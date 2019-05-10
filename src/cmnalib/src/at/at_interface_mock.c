
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>

#include "cmnalib/at_interface.h"
#include "cmnalib/logger.h"

#undef LOGGER_LEVEL
#define LOGGER_LEVEL LOGGER_VERBOSE_INFO

#define LINE_SEPARATOR "\n"

struct at_interface_t {
    FILE* file;
    char* response_file;
    char* line_separator;
};

at_interface_t* at_interface_open_mock(const char* response_file) {
    at_interface_t* h = calloc(1, sizeof(at_interface_t));
    if(h == NULL) {
        ERROR("Error in calloc\n");
        return NULL;
    }
    h->response_file = malloc(strlen(response_file)+1);
    h->line_separator = malloc(sizeof(LINE_SEPARATOR));
    if(h->response_file == NULL || h->line_separator == NULL) {
        ERROR("Error in malloc\n");
        return NULL;
    }
    strcpy(h->response_file, response_file);
    strcpy(h->line_separator, LINE_SEPARATOR);

    h->file = fopen(h->response_file, "r");
    if(h->file == NULL) {
        ERROR("Could not open file '%s': %s\n", h->response_file, strerror(errno));
        return NULL;
    }

    return h;
}

void at_interface_close_mock(at_interface_t* h) {
    if(h != NULL) {
      fclose(h->file);
      free(h->response_file);
      free(h->line_separator);
      free(h);
    }
}

at_interface_response_status_t at_interface_command_mock(at_interface_t* h,
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
    char tty_cmd_from_file[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH +
            AT_INTERFACE_MAX_LINE_SEPARATOR_LENGTH];

    char tty_response_buf[AT_INTERFACE_MAX_RESPONSE_STRING_LENGTH];
    int pos;

    strcpy(tty_cmd, cmd->command_string);
    if(params != NULL) strcat(tty_cmd, params);
    strcat(tty_cmd, h->line_separator);

    pos = 0;
    (*response)->response_len = 0;
    (*response)->response_string[0] = 0;

    // First read and compare command from file
    fscanf(h->file, "%" TOSTRING(AT_INTERFACE_MAX_COMMAND_STRING_LENGTH) "[^" LINE_SEPARATOR "]" LINE_SEPARATOR, tty_cmd_from_file);
    strcat(tty_cmd_from_file, LINE_SEPARATOR);
    if(strcmp(tty_cmd, tty_cmd_from_file) != 0) {
        return AT_RESPONSE_TEST_COMMAND_MISMATCH;
    }

    // Second read and forward the response
    while(EOF != fscanf(h->file, "%" TOSTRING(AT_INTERFACE_MAX_COMMAND_STRING_LENGTH) "[^" LINE_SEPARATOR "]%*[" LINE_SEPARATOR "]", tty_response_buf)) {
        strcat(tty_response_buf, LINE_SEPARATOR);
        pos += sprintf(&(*response)->response_string[pos], "%s", tty_response_buf);

        if(strstr(tty_response_buf, "OK"LINE_SEPARATOR)) {
            DEBUG("Early finished due to complete response: OK\n");
            result = AT_RESPONSE_SUCCESS;
            break;
        }
        else if(strstr(tty_response_buf, "ERROR"LINE_SEPARATOR)) {
            DEBUG("Early finished due to complete response: ERROR\n");
            result = AT_RESPONSE_FAILED;
            break;
        }
        else if(strstr(tty_response_buf, "+CME ERROR")) {
            // TODO: consider error code, e.g. "+CME ERROR: SIM failure"
            DEBUG("Early finished due to +CME ERROR\n");
            result = AT_RESPONSE_FAILED;
            break;
        }
    }

    return result;
}

void at_interface_free_response_mock(at_interface_response_t* r) {
    if(r != NULL) {
        r->response_len = 0;
    }
    free(r);
}
