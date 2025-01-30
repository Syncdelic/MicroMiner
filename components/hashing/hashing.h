#ifndef HASHING_H
#define HASHING_H

#include <stddef.h>

// Perform a double SHA-256 computation
void compute_double_sha256(const unsigned char *input, size_t length, unsigned char output[32]);

// Convert a hex string to a byte array
void hex_to_bytes(const char *hex_string, unsigned char *byte_array, size_t byte_array_size);

// Convert a byte array to a hex string
void bytes_to_hex(const unsigned char *bytes, size_t length, char *hex_string, size_t hex_length);

#endif // HASHING_H

