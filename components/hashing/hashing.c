#include "hashing.h"
#include <string.h>
#include <stdio.h>
#include "mbedtls/sha256.h"

void compute_double_sha256(const unsigned char *input, size_t length, unsigned char output[32]) {
    unsigned char temp_hash[32];
    mbedtls_sha256(input, length, temp_hash, 0);  // Perform first SHA-256
    mbedtls_sha256(temp_hash, 32, output, 0);    // Perform second SHA-256
}

void hex_to_bytes(const char *hex_string, unsigned char *byte_array, size_t byte_array_size) {
    size_t hex_length = strlen(hex_string);
    if (hex_length / 2 > byte_array_size) {
        fprintf(stderr, "Error: Hex string too large for the buffer.\n");
        return;
    }

    for (size_t i = 0; i < hex_length / 2; i++) {
        sscanf(&hex_string[i * 2], "%2hhx", &byte_array[i]);
    }
}

void bytes_to_hex(const unsigned char *bytes, size_t length, char *hex_string, size_t hex_length) {
    for (size_t i = 0; i < length; i++) {
        snprintf(&hex_string[i * 2], 3, "%02x", bytes[i]);
    }
    hex_string[length * 2] = '\0';
}

