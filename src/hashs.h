#ifndef HASHS_H
#define HASHS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void blake2b(uint8_t       *hash   , size_t hash_size,
             const uint8_t *key    , size_t key_size,
             const uint8_t *message, size_t message_size);


#define BASE58_ENCODE_MAXLEN 256
#define BASE58_DECODE_MAXLEN 360

bool base58_decode(char *bin, size_t *binszp, const char *b58, size_t b58sz);
bool base58_encode(char *b58, size_t *b58sz, const char *data, size_t binsz);
bool b58tobin(char *bin, size_t *binszp, const char *b58, size_t b58sz);

#endif

