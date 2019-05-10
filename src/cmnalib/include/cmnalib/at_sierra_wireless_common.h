/*
 *
 *
 *
 *
 *   Copyright (C) 2018 Robert Falkenberg <robert.falkenberg@tu-dortmund.de>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
  Errorcodes sorted by severity
  */
typedef enum sw_response {
    SW_RESPONSE_SUCCESS = 0,
    //Failures
    SW_RESPONSE_FAILED,
    //Critical
    SW_RESPONSE_CRITICAL,   // Placeholder
    SW_RESPONSE_INVAL,
    SW_RESPONSE_ERROR,
    SW_RESPONSE_OUT_OF_MEMORY,
    SW_RESPONSE_UNKNOWN,
    SW_RESPONSE__MAX,
} sw_response_t;

/**
  Magic constants representing non-numeric values of typically numeric keys
  */

#define SW_GSTATUS_TX_POWER_INACTIVE -1000
#define SW_GSTATUS_SCC_BW_UNKNOWN 0

#ifdef __cplusplus
}
#endif
