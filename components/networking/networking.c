#include "networking.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_log.h"

static const char *TAG = "Networking";

int connect_to_pool(const char *pool_address, int pool_port) {
    struct addrinfo hints = {0};
    struct addrinfo *res = NULL;
    int sock;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(pool_address, NULL, &hints, &res);
    if (err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS resolution failed: %d", err);
        return -1;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    addr->sin_port = htons(pool_port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket creation failed!");
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)addr, sizeof(*addr)) != 0) {
        ESP_LOGE(TAG, "Failed to connect to pool!");
        close(sock);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock;
}

