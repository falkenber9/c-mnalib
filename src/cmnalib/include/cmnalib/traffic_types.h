
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct transfer_statusreport {
  double datarate_dl;
  double datarate_ul;
  double total_transfer_time;
  size_t total_transfered_bytes;
} transfer_statusreport_t;

typedef int (progress_callback_func)(void* user_context,
                                      transfer_statusreport_t* statusreport);

#ifdef __cplusplus
}
#endif
