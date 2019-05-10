

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include <regex.h>

#include "cmnalib/logger.h"
#include "cmnalib/tokenfind.h"
#include "cmnalib/conversion.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

int tokenfind_integer_batch(const char* src_sequence,
                            void *base_ptr,
                            tokenfind_batch_t* job,
                            int n_jobs) {
    int n_success = 0;
    int ret = 0;
    int* target = NULL;

    for(int i=0; i<n_jobs; i++) {
        int tmp_result = 0;
        ret = tokenfind_integer_single(src_sequence,
                                       &tmp_result,
                                       job[i].regex_string,
                                       &job[i].regex_cache,
                                       job[i].opt_param);
        if(ret > 0) {
            target = ((int*)base_ptr + job[i].member_offset/sizeof(int));
            *target = tmp_result;
            n_success++;
        }
    }

    return n_success;
}

int tokenfind_integer_single(const char* src_sequence,
                             int* result,
                             const char* regex_string,
                             regex_t** regex_cache,
                             int BASE) {
    int match_start = 0;
    int match_end = 0;
    int ret = 0;

    ret = tokenfind_match_regex_single(src_sequence, &match_start, &match_end, regex_string, regex_cache);
    if (ret <= 0) {
        return ret;
    }

    /* Conversion */
    const char* start = &src_sequence[match_start];
    /* const char* end = &src_sequence[match_end]; */
    errno = 0;
    long long_res =  strtol(start, NULL, BASE);

    if ((errno == ERANGE && (long_res == LONG_MAX || long_res == LONG_MIN))
            || (errno != 0 && long_res == 0)) {
        ERROR("Error in strtol\n");
        return -1;
    }

    if (long_res > INT_MAX || long_res < INT_MIN) {
        WARNING("Number out of bounds of type int\n");
        return -1;
    }

    *result = (int)long_res;
    return 1;
}

int tokenfind_float_batch(const char* src_sequence,
                            void *base_ptr,
                            tokenfind_batch_t* job,
                            int n_jobs) {
    int n_success = 0;
    int ret = 0;
    float* target = NULL;

    for(int i=0; i<n_jobs; i++) {
        float tmp_result = 0;
        ret = tokenfind_float_single(src_sequence,
                                       &tmp_result,
                                       job[i].regex_string,
                                       &job[i].regex_cache);
        if(ret > 0) {
            target = ((float*)base_ptr + job[i].member_offset/sizeof(float));
            *target = tmp_result;
            n_success++;
        }
    }

    return n_success;
}

int tokenfind_float_single(const char* src_sequence,
                             float* result,
                             const char* regex_string,
                             regex_t** regex_cache) {

    int match_start = 0;
    int match_end = 0;
    int ret = 0;

    ret = tokenfind_match_regex_single(src_sequence, &match_start, &match_end, regex_string, regex_cache);
    if (ret <= 0) {
        return ret;
    }

    /* Conversion */
    const char* start = &src_sequence[match_start];
    /* const char* end = &src_sequence[match_end]; */
    errno = 0;
    float float_res =  strtof(start, NULL);

    if ((errno == ERANGE)
            || (errno != 0 && float_res == 0)) {
        ERROR("Error in strtof\n");
        return -1;
    }

    *result = float_res;
    return 1;
}

int tokenfind_string_batch(const char* src_sequence,
                           void *base_ptr,
                           tokenfind_batch_t* job,
                           int n_jobs,
                           int str_len) {
    int n_success = 0;
    int len = 0;
    char* target = NULL;

    char* tmp_result = calloc(str_len, sizeof(char));
    if(tmp_result == NULL) {
        ERROR("calloc failed\n");
        return 0;
    }

    for(int i=0; i<n_jobs; i++) {
        tmp_result[0] = 0;
        len = tokenfind_string_single(src_sequence,
                                      tmp_result,
                                      str_len,
                                      job[i].regex_string,
                                      &job[i].regex_cache);
        if(len > 0) {
            target = ((char*)base_ptr + job[i].member_offset/sizeof(char));
            strncpy(target, tmp_result, MIN(str_len, len));
            target[MIN(str_len, len)] = 0;
            n_success++;
        }
    }

    free(tmp_result);

    return n_success;
}

int tokenfind_string_single(const char* src_sequence,
                            char* dst_sequence,
                            int dst_bufsize,
                            const char* regex_string,
                            regex_t** regex_cache) {

    int match_start = 0;
    int match_end = 0;
    int len = 0;

    len = tokenfind_match_regex_single(src_sequence, &match_start, &match_end, regex_string, regex_cache);
    if (len <= 0) {
        return len;
    }

    strncpy(dst_sequence, &src_sequence[match_start], MIN(dst_bufsize-1, len));
    dst_sequence[MIN(dst_bufsize, len+1)-1] = 0;   /* NULL termination */
    return MIN(dst_bufsize-1, len);
}

int tokenfind_match_regex_single(const char* src_sequence,
                                 int* match_start,
                                 int* match_end,
                                 const char* regex_string,
                                 regex_t** regex_cache) {
    const int N_MATCHES = 1;
    int ret = 0;

#if 1
    ret = tokenfind_match_regex_multi(src_sequence,
                                      match_start,
                                      match_end,
                                      N_MATCHES,
                                      regex_string,
                                      regex_cache);
    if(ret > 0) {
        return *match_end - *match_start;
    }
    else {
        return ret;
    }
#else
    regex_t* regex = NULL;
    regmatch_t matches[N_MATCHES + 1];

    if(src_sequence == NULL ||
            match_start == NULL ||
            match_end == NULL ||
            regex_string == NULL) {
        ERROR("Invalid input in tokenfind_match_regex_single()\n");
        return -1;
    }

    if(regex_cache == NULL || *regex_cache == NULL) {
        /* Allocate new regex automaton */
        regex = calloc(1, sizeof(regex_t));
        if(regex == NULL) {
            ERROR("Could not allocate memory for regex_t\n");
            return -1;
        }
        /* Compile new regex */
        ret = regcomp(regex, regex_string, 0);
        if(ret) {
            regfree(regex);
            free(regex);
            regex = NULL;
            ERROR("Could not compile regex\n");
            return -1;
        }
        /* Save regex automaton for later usage (if applicable)*/
        if(regex_cache != NULL)
            *regex_cache = regex;
    }
    else {
        /* Reuse previous regex automaton */
        regex = *regex_cache;
    }

    /* Exec */
    ret = regexec(regex, src_sequence, N_MATCHES + 1, matches, 0);
    if(ret) {
        regfree(regex);
        free(regex);
        regex = NULL;
        if(regex_cache != NULL) *regex_cache = NULL;
        char err_msg_buf[100];
        regerror(ret, regex, err_msg_buf, sizeof(err_msg_buf));
        ERROR("Regex error %d: %s\n", ret, err_msg_buf);
        return -1;
    }

    /* Validate */
    if(matches[1].rm_so == -1 || matches[1].rm_eo == -1) {
        DEBUG("No match for regex '%s'", regex_string);
        return 0;
    }

    *match_start = matches[1].rm_so;
    *match_end = matches[1].rm_eo;
    return *match_end - *match_start;
#endif
}

int tokenfind_match_regex_multi(const char* src_sequence,
                                int* match_start,
                                int* match_end,
                                int n_matches,
                                const char* regex_string,
                                regex_t** regex_cache) {
    int ret = 0;
    regex_t* regex = NULL;
    regmatch_t matches[n_matches + 1];

    if(src_sequence == NULL ||
            match_start == NULL ||
            match_end == NULL ||
            n_matches <= 0 ||
            regex_string == NULL) {
        ERROR("Invalid input in tokenfind_match_regex_multi()\n");
        return -1;
    }

    if(regex_cache == NULL || *regex_cache == NULL) {
        /* Allocate new regex automaton */
        regex = calloc(1, sizeof(regex_t));
        if(regex == NULL) {
            ERROR("Could not allocate memory for regex_t\n");
            return -1;
        }
        /* Compile new regex */
        ret = regcomp(regex, regex_string, 0);
        if(ret) {
            regfree(regex);
            free(regex);
            regex = NULL;
            ERROR("Could not compile regex\n");
            return -1;
        }
        /* Save regex automaton for later usage (if applicable)*/
        if(regex_cache != NULL)
            *regex_cache = regex;
    }
    else {
        /* Reuse previous regex automaton */
        regex = *regex_cache;
    }

    /* Exec */
    ret = regexec(regex, src_sequence, n_matches + 1, matches, 0);
    if(ret == REG_NOMATCH) {
        /* Valid regex, just nothing found */
        //DEBUG("No Match\n");
        return 0;
    }
    else if(ret != 0) {
        /* Error */
        regfree(regex);
        free(regex);
        regex = NULL;
        if(regex_cache != NULL) *regex_cache = NULL;
        char err_msg_buf[100];
        regerror(ret, regex, err_msg_buf, sizeof(err_msg_buf));
        ERROR("Regex error %d: %s\n", ret, err_msg_buf);
        return -1;
    }

    /* Validate */
    int result = 0;
    for(int i = 0; i < n_matches; i++) {
        if(matches[i+1].rm_so == -1 || matches[i+1].rm_eo == -1) {
            DEBUG("No match in %d of %d expected matches for regex '%s'", i+1, n_matches, regex_string);
        }
        else {
            match_start[i] = matches[i+1].rm_so;
            match_end[i] = matches[i+1].rm_eo;
            result++;
        }
    }

    return result;
}

void tokenfind_split_string_free(tokenfind_split_string_t* s) {
    if(s != NULL) {
        if(s->_strbuf != NULL) {
            free(s->_strbuf);
        }
        if(s->token != NULL) {
            free(s->token);
        }
        s->n_tokens = 0;
    }
    free(s);
}

tokenfind_split_string_t* tokenfind_split_string(const char* src_sequence, const char* delimiter) {
    char* context = NULL;

    tokenfind_split_string_t* result = malloc(sizeof(tokenfind_split_string_t));
    if(result == NULL) {
        ERROR("Error in calloc\n");
        return NULL;
    }
    result->n_tokens = 0;
    result->token = NULL;
    result->_strbuf = calloc(strlen(src_sequence)+1, sizeof(char));
    if(result->_strbuf == NULL) {
        ERROR("Error in calloc\n");
        tokenfind_split_string_free(result);
        return NULL;
    }

    strcpy(result->_strbuf, src_sequence);
    char* pos = strtok_r(result->_strbuf, delimiter, &context);
    while(pos != NULL) {
        result->token = realloc(result->token, sizeof(char*) * ++(result->n_tokens));
        if(result->token == NULL) {
            ERROR("Error in realloc\n");
            tokenfind_split_string_free(result);
            return NULL;
        }
        result->token[result->n_tokens - 1] = pos;
        pos = strtok_r(NULL, delimiter, &context);
    }

    return result;
}

tokenfind_string_table_t* tokenfind_parse_table(const char* src_sequence) {
    tokenfind_string_table_t* t = malloc(sizeof(tokenfind_string_table_t));
    if(t == NULL) {
        ERROR("Error in malloc\n");
        return NULL;
    }
    t->n_colums = 0;
    t->n_rows = 0;
    t->row = NULL;

    /* split input sequence into rows */
    tokenfind_split_string_t* rows = tokenfind_split_string(src_sequence, "\r\n");

    if(rows != NULL && rows->n_tokens > 0) {
        t->row = calloc(rows->n_tokens, sizeof(tokenfind_string_table_row_t));
        if(t->row == NULL) {
            ERROR("Error in calloc\n");
            tokenfind_free_table(t);
            return NULL;
        }
        t->n_rows = rows->n_tokens;
        for(int row_idx = 0; row_idx < rows->n_tokens; row_idx++) {
            /* split each row into columns */
            tokenfind_split_string_t* columns = tokenfind_split_string(rows->token[row_idx], " \t");
            if(columns != NULL && columns->n_tokens > 0) {
                t->row[row_idx].column = calloc(columns->n_tokens, sizeof(char*));
                if(t->row[row_idx].column == NULL) {
                    ERROR("Error in calloc\n");
                    tokenfind_free_table(t);
                    return NULL;
                }
                t->n_colums = columns->n_tokens;
                t->row[row_idx].n_columns = columns->n_tokens;
                for(int col_idx = 0; col_idx < columns->n_tokens; col_idx++) {
                    // copy each item
                    int elem_len = strlen(columns->token[col_idx]);
                    t->row[row_idx].column[col_idx] = calloc(elem_len+1, sizeof(char));
                    if(t->row[row_idx].column[col_idx] == NULL) {
                        ERROR("Error in calloc\n");
                        tokenfind_free_table(t);
                        return NULL;
                    }
                    strcpy(t->row[row_idx].column[col_idx], columns->token[col_idx]);
                }
            }
            tokenfind_split_string_free(columns);
        }
    }
    tokenfind_split_string_free(rows);

    return t;
}

void tokenfind_free_table(tokenfind_string_table_t* t) {
    if(t != NULL) {
        if(t->row != NULL) {
            for(int row_idx = 0; row_idx < t->n_rows; row_idx++) {
                if(t->row[row_idx].column != NULL) {
                    for(int col_idx = 0; col_idx < t->row[row_idx].n_columns; col_idx++) {
                        free(t->row[row_idx].column[col_idx]);
                    }
                    t->row[row_idx].n_columns = 0;
                }
                free(t->row[row_idx].column);
            }
        }
        free(t->row);
        t->n_colums = 0;
        t->n_rows = 0;
        free(t);
    }
}
