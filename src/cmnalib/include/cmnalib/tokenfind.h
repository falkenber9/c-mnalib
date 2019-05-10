#pragma once

#include <stdint.h>
#include <regex.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t member_offset;       /* offset of target member in struct. example: offsetof(some_container_struct_t, some_member) */
    const char* regex_string;
    regex_t* regex_cache;
    int opt_param;              /* optional param, for integer-batch: BASE*/
} tokenfind_batch_t;

typedef struct {
    char** column;
    int n_columns;
} tokenfind_string_table_row_t;

typedef struct {
    tokenfind_string_table_row_t* row;
    int n_rows;
    int n_colums;
} tokenfind_string_table_t;

typedef struct {
    char** token;
    int n_tokens;
    char* _strbuf;
} tokenfind_split_string_t;

int tokenfind_integer_batch(const char* src_sequence,
                            void* base,
                            tokenfind_batch_t* job,
                            int n_jobs);

int tokenfind_integer_single(const char* src_sequence,
                             int* result,
                             const char* regex_string,
                             regex_t** regex_cache,
                             int BASE);

int tokenfind_float_batch(const char* src_sequence,
                          void *base_ptr,
                          tokenfind_batch_t* job,
                          int n_jobs);

int tokenfind_float_single(const char* src_sequence,
                           float* result,
                           const char* regex_string,
                           regex_t** regex_cache);

int tokenfind_string_batch(const char* src_sequence,
                           void *base_ptr,
                           tokenfind_batch_t* job,
                           int n_jobs,
                           int str_len);

int tokenfind_string_single(const char* src_sequence,
                            char* dst_sequence,
                            int dst_length,
                            const char* regex_string,
                            regex_t** regex_cache);

int tokenfind_match_regex_single(const char* src_sequence,
                                 int* match_start,
                                 int* match_end,
                                 const char* regex_string,
                                 regex_t** regex_cache);

int tokenfind_match_regex_multi(const char* src_sequence,
                                int* match_start,
                                int* match_end,
                                int n_matches,
                                const char* regex_string,
                                regex_t** regex_cache);

tokenfind_split_string_t* tokenfind_split_string(const char* src_sequence,
                                                 const char* delimiter);
void tokenfind_split_string_free(tokenfind_split_string_t* s);

tokenfind_string_table_t* tokenfind_parse_table(const char* src_sequence);
void tokenfind_free_table(tokenfind_string_table_t* t);

#ifdef __cplusplus
}
#endif
