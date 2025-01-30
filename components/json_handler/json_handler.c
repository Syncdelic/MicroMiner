#include <stdbool.h>
#include <string.h>
#include "json_handler.h"
#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "JSON_Handler";

bool parse_mining_job(const char *response, mining_job_t *job) {
    cJSON *json = cJSON_Parse(response);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON.");
        return false;
    }

    const cJSON *method = cJSON_GetObjectItemCaseSensitive(json, "method");
    if (method && cJSON_IsString(method) && strcmp(method->valuestring, "mining.notify") == 0) {
        const cJSON *params = cJSON_GetObjectItemCaseSensitive(json, "params");
        if (cJSON_IsArray(params)) {
            // Extract fields from the params array
            cJSON *job_id = cJSON_GetArrayItem(params, 0);
            cJSON *prev_hash = cJSON_GetArrayItem(params, 1);
            cJSON *merkle_root = cJSON_GetArrayItem(params, 2);
            cJSON *version = cJSON_GetArrayItem(params, 5);
            cJSON *timestamp = cJSON_GetArrayItem(params, 6);
            cJSON *bits = cJSON_GetArrayItem(params, 7);

            // Log extracted values for debugging
            ESP_LOGI(TAG, "job_id: %s", job_id ? job_id->valuestring : "NULL");
            ESP_LOGI(TAG, "prev_hash: %s", prev_hash ? prev_hash->valuestring : "NULL");
            ESP_LOGI(TAG, "merkle_root: %s", merkle_root ? merkle_root->valuestring : "NULL");
            ESP_LOGI(TAG, "version: %s", version ? version->valuestring : "NULL");
            ESP_LOGI(TAG, "timestamp: %s", timestamp ? timestamp->valuestring : "NULL");
            ESP_LOGI(TAG, "bits: %s", bits ? bits->valuestring : "NULL");

            // Ensure all required fields are present
            if (job_id && prev_hash && merkle_root && version && bits && timestamp) {
                strncpy(job->job_id, job_id->valuestring, sizeof(job->job_id) - 1);
                strncpy(job->prev_hash, prev_hash->valuestring, sizeof(job->prev_hash) - 1);
                strncpy(job->merkle_root, merkle_root->valuestring, sizeof(job->merkle_root) - 1);
                strncpy(job->version, version->valuestring, sizeof(job->version) - 1);
                strncpy(job->timestamp, timestamp->valuestring, sizeof(job->timestamp) - 1);
                strncpy(job->bits, bits->valuestring, sizeof(job->bits) - 1);
                cJSON_Delete(json);
                return true;
            } else {
                ESP_LOGE(TAG, "Missing one or more required fields in the mining job.");
            }
        } else {
            ESP_LOGE(TAG, "Params array is invalid or missing.");
        }
    } else {
        ESP_LOGE(TAG, "Invalid method or missing 'mining.notify'.");
    }

    cJSON_Delete(json);
    return false;
}
