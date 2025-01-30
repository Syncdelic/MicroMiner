#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "wifi.h"
#include "networking.h"
#include "hashing.h"
#include "json_handler.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD
#define POOL_ADDRESS CONFIG_POOL_ADDRESS
#define POOL_PORT CONFIG_POOL_PORT
#define WALLET CONFIG_WALLET

#define BLOCK_HEADER_SIZE 80
#define TARGET "0000000000000000000fffffffffffffffffffffffffffffffffffffffffff"

static const char *TAG = "Miner";

// FreeRTOS queue for communication between computation and printing
QueueHandle_t print_queue;

// Structure to hold mining result data
typedef struct {
    uint32_t nonce;
    char hash_hex[65];
} MiningResult;

void mining_task(void *pvParameters) {
    mining_job_t *current_job = (mining_job_t *)pvParameters;
    unsigned char block_header[BLOCK_HEADER_SIZE];
    unsigned char hash[32];
    MiningResult result;
    uint32_t nonce = 0;

    // Construct the initial block header
    char block_header_hex[161];
    snprintf(block_header_hex, sizeof(block_header_hex),
             "%.8s%.64s%.64s%.8s%.8s%.8s", // Limit each field to its expected length
             current_job->version,
             current_job->prev_hash,
             current_job->merkle_root,
             current_job->timestamp,
             current_job->bits,
             "00000000"); // Nonce starts at 0

    hex_to_bytes(block_header_hex, block_header, BLOCK_HEADER_SIZE);

    TickType_t start_tick = xTaskGetTickCount(); // Start time in ticks

    while (1) {
        // Update nonce in block header (last 4 bytes)
        memcpy(&block_header[76], &nonce, sizeof(nonce));

        // Compute double SHA-256 hash
        compute_double_sha256(block_header, BLOCK_HEADER_SIZE, hash);

        // Convert hash to hex for readability
        bytes_to_hex(hash, 32, result.hash_hex, sizeof(result.hash_hex));
        result.nonce = nonce;

        // Send the result to the print task
        if (xQueueSend(print_queue, &result, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "Failed to send to print queue.");
        }

        // Check if hash is less than the target
        if (strcmp(result.hash_hex, TARGET) <= 0) {
            ESP_LOGI(TAG, "Valid hash found! Nonce: %lu, Hash: %s", nonce, result.hash_hex);
            break;
        }

        nonce++;

        // Handle nonce overflow
        if (nonce % 100 == 0) { // Calculate hash rate every 100 hashes
            TickType_t end_tick = xTaskGetTickCount();
            TickType_t elapsed_ticks = end_tick - start_tick;
            float elapsed_seconds = elapsed_ticks / (float)CONFIG_FREERTOS_HZ;
            float hash_rate = nonce / elapsed_seconds;

            ESP_LOGI(TAG, "Hash rate: %.2f H/s", hash_rate);
        }

        // Handle nonce overflow
        if (nonce == 0) {
            ESP_LOGW(TAG, "Nonce overflow! Restarting...");
        }
    }

    vTaskDelete(NULL); // Delete the task once mining is complete
}

void printing_task(void *pvParameters) {
    MiningResult result;

    while (1) {
        // Wait for mining results from the queue
        if (xQueueReceive(print_queue, &result, portMAX_DELAY) == pdPASS) {
            // Print the result
            printf("Nonce: %lu, Hash: %s\n", result.nonce, result.hash_hex);
        }
    }
}

void miner_task(void *pvParameters) {
    int sock = connect_to_pool(POOL_ADDRESS, POOL_PORT);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to connect to the pool!");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Connected to the pool!");

    // Send login messages
    char login_message[256];
    snprintf(login_message, sizeof(login_message),
             "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[\"ESP32_Miner\"]}\n");
    send(sock, login_message, strlen(login_message), 0);

    snprintf(login_message, sizeof(login_message),
             "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"%s\",\"\"]}\n", WALLET);
    send(sock, login_message, strlen(login_message), 0);

    char recv_buffer[4096];
    mining_job_t current_job;

    while (1) {
        int len = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (len > 0) {
            recv_buffer[len] = '\0'; // Null-terminate the received data
            ESP_LOGI(TAG, "Received: %s", recv_buffer);

            if (parse_mining_job(recv_buffer, &current_job)) {
                ESP_LOGI(TAG, "Parsed new mining job!");

                // Start mining computation on Core 1
                xTaskCreatePinnedToCore(mining_task, "MiningTask", 8192, &current_job, 5, NULL, 1);
            }
        } else {
            ESP_LOGE(TAG, "Disconnected from the pool!");
            break;
        }
    }

    close(sock);
    vTaskDelete(NULL);
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init(WIFI_SSID, WIFI_PASS);
    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
    vTaskDelay(pdMS_TO_TICKS(4500)); // Delay to allow connection setup

    // Create the print queue
    print_queue = xQueueCreate(10, sizeof(MiningResult));
    if (print_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create print queue.");
        return;
    }

    // Start the miner task
    xTaskCreate(miner_task, "MinerTask", 8192, NULL, 5, NULL);

    // Start the printing task on Core 0
    xTaskCreatePinnedToCore(printing_task, "PrintingTask", 4096, NULL, 5, NULL, 0);
}

