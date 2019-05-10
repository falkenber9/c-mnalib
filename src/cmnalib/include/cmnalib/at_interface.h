#pragma once 

#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AT_INTERFACE_MAX_COMMAND_STRING_LENGTH 255
#define AT_INTERFACE_MAX_RESPONSE_STRING_LENGTH 100000

#define AT_INTERFACE_MAX_LINE_SEPARATOR_LENGTH 3

#define AT_INTERFACE_MAX_CONSECUTIVE_TIMEOUTS 3

/**
  Errorcodes sorted by severity
  */
typedef enum at_interface_response_status {
    //Success
    AT_RESPONSE_SUCCESS = 0,
    //Failures
    AT_RESPONSE_FAILED,
    AT_RESPONSE_TIMEOUT,
    //Critical
    AT_RESPONSE_CRITICAL,   // Placeholder
    AT_RESPONSE_INVAL,
    AT_RESPONSE_IO_ERROR,
    AT_RESPONSE_OUT_OF_MEMORY,
    AT_RESPONSE_TEST_COMMAND_MISMATCH, // For unit tests only
    AT_RESPONSE_UNKNOWN,
    AT_RESPONSE__MAX,
} at_interface_response_status_t;

typedef struct at_interface_t at_interface_t;

typedef struct {
    int id;
    char command_string[AT_INTERFACE_MAX_COMMAND_STRING_LENGTH];
    int timeout_sec;
    int timeout_usec;
} at_interface_command_t;

typedef struct {
    int response_len;
    char response_string[AT_INTERFACE_MAX_RESPONSE_STRING_LENGTH];
} at_interface_response_t;


at_interface_t* at_interface_open(const char* tty_device_path);
void at_interface_close(at_interface_t* h);
at_interface_response_status_t at_interface_command(at_interface_t* h,
                                              const at_interface_command_t* cmd,
                                              const char* params,
                                              at_interface_response_t** response);
void at_interface_free_response(at_interface_response_t* r);

#ifndef AT_MOCK
    #define at_interface_open_tty at_interface_open
    #define at_interface_close_tty at_interface_close
    #define at_interface_command_tty at_interface_command
    #define at_interface_free_response_tty at_interface_free_response
#else
    #define at_interface_open_mock at_interface_open
    #define at_interface_close_mock at_interface_close
    #define at_interface_command_mock at_interface_command
    #define at_interface_free_response_mock at_interface_free_response
#endif

#ifdef __cplusplus
}
#endif
