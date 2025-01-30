#ifndef JSON_HANDLER_H
#define JSON_HANDLER_H

#include "cJSON.h"

// Define job structure
typedef struct {
    char job_id[64];
    char prev_hash[64];
    char merkle_root[128];
    char version[16];
    char timestamp[16];
    char bits[16];
} mining_job_t;
/**
 * @brief Parse mining job JSON.
 * @param response The JSON string to parse.
 * @param job Pointer to a mining_job_t structure to store parsed job details.
 * @return true if parsing is successful, false otherwise.
 */
bool parse_mining_job(const char *response, mining_job_t *job);

/**
 * @brief Extract the method field from JSON.
 * @param response The JSON string to parse.
 * @return The method name if available, NULL otherwise (caller must free the returned string).
 */
char *extract_json_method(const char *response);

#endif // JSON_HANDLER_H

