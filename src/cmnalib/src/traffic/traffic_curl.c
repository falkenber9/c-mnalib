#include <curl/curl.h>

#include "cmnalib/traffic_curl.h"
#include "cmnalib/logger.h"

#include "cmnalib/traffic_types.h"

#define TC_UPLOAD_DISCARD_SERVER_RESPONSE_PAGE

struct xferinfo_callback_data
{
    progress_callback_func* callback;
    void* callback_context;
    double minimal_progress_interval_sec;
    size_t transfer_bytes_total;
    size_t remaining_bytes;

    double lastruntime;
    curl_off_t last_dl;
    curl_off_t last_ul;
    CURL *curl;
};

static size_t discard_silently_callback(void *ptr,
                                        size_t size,
                                        size_t n_memb,
                                        void *userdata) {
  /* discard the downloaded bytes,
     only return the size we would have saved. */
  (void)ptr;  /* unused */
  (void)userdata; /* unused */
  return (size_t)(size * n_memb);
}

static size_t discard_and_truncate_callback(void *ptr,
                                            size_t size,
                                            size_t n_memb,
                                            void *userdata) {
  /* discard the downloaded bytes,
     only return the size we would have saved
     or truncate (=cancel) the download if limit is reached */
  (void)ptr;  /* unused */
  size_t* remaining_bytes = (size_t*)userdata;
  size_t bound = n_memb * size;
  if(*remaining_bytes > 0) {
    if(bound > *remaining_bytes) {
      bound = *remaining_bytes;
    }
    *remaining_bytes -= bound;
  }
  return bound;
}

static size_t read_randomdata_callback(void *ptr,
                                       size_t size,
                                       size_t n_memb,
                                       void *userdata) {
    size_t* remaining_bytes = (size_t*)userdata;
    size_t retcode = 0;
    char* buffer = (char*)ptr;
    size_t bound = n_memb * size;
    // remaining
    if(bound > *remaining_bytes) {
        bound = *remaining_bytes;
    }

    //DEBUG("Uploading chunk of %d bytes\n", bound);

    // fill buffer with junkdata
    while(retcode < bound) {
        buffer[retcode] = (char)retcode;
        retcode++;
    }

    *((size_t*)userdata) -= retcode;

    // always return nof bytes while not exceeding the initial uploadsize
    return retcode;
}

/**
  WARNING: This function should be quick, since it blocks the single-threaded transfer
*/
int xferinfo(void *userdata,
             curl_off_t dltotal,
             curl_off_t dlnow,
             curl_off_t ultotal,
             curl_off_t ulnow) {
    struct xferinfo_callback_data* callbackdata = (struct xferinfo_callback_data*)userdata;

    if(callbackdata != NULL) {
        CURL *curl = callbackdata->curl;
        double curtime, timeinterval = 0.0;
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);
        timeinterval = curtime - callbackdata->lastruntime;
        if(timeinterval > callbackdata->minimal_progress_interval_sec) {
            curl_off_t dl_delta = dlnow - callbackdata->last_dl;
            curl_off_t ul_delta = ulnow - callbackdata->last_ul;
            callbackdata->lastruntime = curtime;
            callbackdata->last_dl = dlnow;
            callbackdata->last_ul = ulnow;
//            DEBUG("UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
//                  "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
//                  "\r\n",
//                  ulnow, ultotal, dlnow, dltotal);
            if(callbackdata->callback != NULL) {
                transfer_statusreport_t statusreport;
                statusreport.datarate_dl = dl_delta/timeinterval;
                statusreport.datarate_ul = ul_delta/timeinterval;
                return callbackdata->callback(callbackdata->callback_context, &statusreport); // provide datarate
            }
        }
        else {
            //DEBUG("Skipping\n");
        }
    }
    // return other than 0 causes transfer to cancel
    return 0;
}

void tc_download(const char* url) {
    CURL *curl = curl_easy_init();
    if(curl) {
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, url);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            ERROR("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
    }
}

void _curl_upload_PUT() {
    size_t uploadsize = 1000*1000;

    CURL *curl = curl_easy_init();
    if(curl) {
        CURLcode res;

        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_randomdata_callback);

        /* enable uploading */
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* specify target */
        curl_easy_setopt(curl, CURLOPT_URL, "mptcp1.pi21.de:5002");

        /* now specify which pointer to pass to our callback */
        curl_easy_setopt(curl, CURLOPT_READDATA, &uploadsize);

        /* Set the size of the file to upload */
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)uploadsize);

        /* Now run off and do what you've been told! */
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            ERROR("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }
    DEBUG("Dropped %d bytes\n", uploadsize);
}

int tc_download_and_discard(const char* url,
                            size_t n_bytes_max,
                            progress_callback_func* callback,
                            void* callback_context,
                            double minimal_progress_interval_sec) {
  int result = 0;
  CURL *curl;
  CURLcode res;

  struct xferinfo_callback_data callbackdata;

  /* get a curl handle */
  curl = curl_easy_init();
  if(curl) {
    callbackdata.callback = callback;
    callbackdata.callback_context = callback_context;
    callbackdata.minimal_progress_interval_sec = minimal_progress_interval_sec;
    callbackdata.transfer_bytes_total = n_bytes_max;
    callbackdata.remaining_bytes = n_bytes_max;
    callbackdata.lastruntime = 0;
    callbackdata.last_dl = 0;
    callbackdata.last_ul = 0;
    callbackdata.curl = curl;

    /* First set the URL that is about to receive our POST. */
    curl_easy_setopt(curl, CURLOPT_URL, url);

    /* send all downloaded data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_and_truncate_callback);

    /* pointer to pass to our read function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callbackdata.remaining_bytes);

    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &callbackdata);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    /* some servers don't like requests that are made without a user-agent
         field, so we provide one */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "c-mnalib traffic generator");

    // First statusreport just before starting transmission
    transfer_statusreport_t statusreport;
    statusreport.total_transfer_time = 0;
    statusreport.datarate_ul = 0;
    statusreport.datarate_dl = 0;
    callbackdata.callback(callbackdata.callback_context, &statusreport);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if((res == CURLE_OK) ||
       (res == CURLE_WRITE_ERROR && callbackdata.remaining_bytes == 0) ||
       (res == CURLE_ABORTED_BY_CALLBACK)) {
      /* no error or truncated by user request or canceled by user request */
      result = 0;
    }
    else {
      ERROR("curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
      result = -1;
    }

    double transmissiontime = 0.0;
    double speed;

    /* this was too coarse grained...
    res = curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed);
    */

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &transmissiontime);
    speed = ((double)(n_bytes_max-callbackdata.remaining_bytes)) / transmissiontime;

    DEBUG("Transmission average datarate: %f bytes/sec\n", speed);

    // Final statusreport after finishing transmission
    statusreport.total_transfer_time = transmissiontime;
    statusreport.datarate_dl = speed;
    callbackdata.callback(callbackdata.callback_context, &statusreport);

    /* always cleanup */
    curl_easy_cleanup(curl);

    return result;
  }

  return -1;
}

int tc_upload_randomdata(const char* url,
                           size_t n_bytes,
                           progress_callback_func* callback,
                           void* callback_context,
                           double minimal_progress_interval_sec) {
    int result = 0;
    CURL *curl;
    CURLcode res;

    struct xferinfo_callback_data callbackdata;

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {

        callbackdata.callback = callback;
        callbackdata.callback_context = callback_context;
        callbackdata.minimal_progress_interval_sec = minimal_progress_interval_sec;
        callbackdata.transfer_bytes_total = n_bytes;
        callbackdata.remaining_bytes = n_bytes;
        callbackdata.lastruntime = 0;
        callbackdata.last_dl = 0;
        callbackdata.last_ul = 0;
        callbackdata.curl = curl;

        /* First set the URL that is about to receive our POST. */
        curl_easy_setopt(curl, CURLOPT_URL, url);

        /* Now specify we want to POST data */
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_randomdata_callback);

        /* pointer to pass to our read function */
        curl_easy_setopt(curl, CURLOPT_READDATA, &callbackdata.remaining_bytes);

#ifdef TC_UPLOAD_DISCARD_SERVER_RESPONSE_PAGE
        /* send all downloaded data to this function  */
        /* to discard the server's response page. */
        /* CURL's default function prints it to stdout */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_silently_callback);
#endif

        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &callbackdata);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        /* get verbose debug output please */
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        /*
          If you use POST to a HTTP 1.1 server, you can send data without knowing
          the size before starting the POST if you use chunked encoding. You
          enable this by adding a header like "Transfer-Encoding: chunked" with
          CURLOPT_HTTPHEADER. With HTTP 1.0 or without chunked transfer, you must
          specify the size in the request.
        */
#ifdef USE_CHUNKED
        {
            struct curl_slist *chunk = NULL;

            chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
            res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            /* use curl_slist_free_all() after the *perform() call to free this
             list again */
        }
#else
        /* Set the expected POST size. If you want to POST large amounts of data,
           consider CURLOPT_POSTFIELDSIZE_LARGE */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)(callbackdata.transfer_bytes_total));
#endif

#ifdef DISABLE_EXPECT
        /*
          Using POST with HTTP 1.1 implies the use of a "Expect: 100-continue"
          header.  You can disable this header with CURLOPT_HTTPHEADER as usual.
          NOTE: if you want chunked transfer too, you need to combine these two
          since you can only set one list of headers with CURLOPT_HTTPHEADER. */

        /* A less good option would be to enforce HTTP 1.0, but that might also
           have other implications. */
        {
            struct curl_slist *chunk = NULL;

            chunk = curl_slist_append(chunk, "Expect:");
            res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            /* use curl_slist_free_all() after the *perform() call to free this
             list again */
        }
#endif

        // First statusreport just before starting transmission
        transfer_statusreport_t statusreport;
        statusreport.total_transfer_time = 0;
        statusreport.datarate_ul = 0;
        statusreport.datarate_dl = 0;
        callbackdata.callback(callbackdata.callback_context, &statusreport);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if((res == CURLE_OK) ||
           (res == CURLE_ABORTED_BY_CALLBACK)) {
          /* no error or canceled by user request */
          result = 0;
        }
        else {
          ERROR("curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
          result = -1;
        }

        double transmissiontime = 0.0;
        double speed;

        /* this was too coarse grained...
        res = curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed);
        */

        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &transmissiontime);
        speed = ((double)(n_bytes-callbackdata.remaining_bytes)) / transmissiontime;

        DEBUG("Transmission average datarate: %f bytes/sec\n", speed);

        // Final statusreport after finishing transmission
        statusreport.total_transfer_time = transmissiontime;
        statusreport.datarate_ul = speed;
        callbackdata.callback(callbackdata.callback_context, &statusreport);

        /* always cleanup */
        curl_easy_cleanup(curl);
        return result;
    }

    DEBUG("Dropped %d bytes\n", callbackdata.remaining_bytes);
    return -1;
}
