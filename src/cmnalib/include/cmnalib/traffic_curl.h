#pragma once

#include "traffic_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void tc_download(const char* url);

int tc_download_and_discard(const char* url,
                            size_t n_bytes_max,
                            progress_callback_func* callback,
                            void* callback_context,
                            double minimal_progress_interval_sec);

int tc_upload_randomdata(const char* url,
                           size_t n_bytes,
                           progress_callback_func* callback,
                           void* callback_context,
                           double minimal_progress_interval_sec);

#ifdef __cplusplus
}
#endif
