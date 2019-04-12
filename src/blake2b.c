#include "hashs.h"

// Blake2b context.  Do not rely on its contents or its size, they
// may change without notice.
typedef struct {
    uint64_t hash[8];
    uint64_t input_offset[2];
    uint64_t input[16];
    size_t   input_idx;
    size_t   hash_size;
} blake2b_ctx;

static void blake2b_init(blake2b_ctx *ctx, size_t hash_size,
                         const uint8_t      *key, size_t key_size);

static void blake2b_update(blake2b_ctx *ctx,
                           const uint8_t *message, size_t message_size);

static void blake2b_final(blake2b_ctx *ctx, uint8_t *hash);

#define FOR(i, start, end) for (size_t (i) = (start); (i) < (end); (i)++)

static uint64_t load64_le(const uint8_t s[8])
{
    return (uint64_t)s[0]
        | ((uint64_t)s[1] <<  8)
        | ((uint64_t)s[2] << 16)
        | ((uint64_t)s[3] << 24)
        | ((uint64_t)s[4] << 32)
        | ((uint64_t)s[5] << 40)
        | ((uint64_t)s[6] << 48)
        | ((uint64_t)s[7] << 56);
}

static void store64_le(uint8_t out[8], uint64_t in)
{
    out[0] =  in        & 0xff;
    out[1] = (in >>  8) & 0xff;
    out[2] = (in >> 16) & 0xff;
    out[3] = (in >> 24) & 0xff;
    out[4] = (in >> 32) & 0xff;
    out[5] = (in >> 40) & 0xff;
    out[6] = (in >> 48) & 0xff;
    out[7] = (in >> 56) & 0xff;
}

static uint64_t rotr64(uint64_t x, uint64_t n) { return (x >> n) ^ (x << (64 - n)); }

// Blake2b (taken from the reference implentation in RFC 7693)

static const uint64_t iv[8] = {
    0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
    0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
    0x510e527fade682d1, 0x9b05688c2b3e6c1f,
    0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
};

// increment the input offset
static void blake2b_incr(blake2b_ctx *ctx)
{
    uint64_t   *x = ctx->input_offset;
    size_t y = ctx->input_idx;
    x[0] += y;
    if (x[0] < y) {
        x[1]++;
    }
}

static void blake2b_set_input(blake2b_ctx *ctx, uint8_t input)
{
    size_t word = ctx->input_idx / 8;
    size_t byte = ctx->input_idx % 8;
    ctx->input[word] |= (uint64_t)input << (byte * 8);
    ctx->input_idx++;
}

static void blake2b_compress(blake2b_ctx *ctx, int is_last_block)
{
    static const uint8_t sigma[12][16] = {
        {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
        { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
        { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
        {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
        {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
        {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
        { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
        { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
        {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
        { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0 },
        {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
        { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
    };

    // init work vector
    uint64_t v[16];
    FOR (i, 0, 8) {
        v[i  ] = ctx->hash[i];
        v[i+8] = iv[i];
    }
    v[12] ^= ctx->input_offset[0];
    v[13] ^= ctx->input_offset[1];
    if (is_last_block) {
        v[14] = ~v[14];
    }

    // mangle work vector
    uint64_t *input = ctx->input;
    FOR (i, 0, 12) {
#define BLAKE2_G(v, a, b, c, d, x, y)                       \
        v[a] += v[b] + x;  v[d] = rotr64(v[d] ^ v[a], 32);  \
        v[c] += v[d];      v[b] = rotr64(v[b] ^ v[c], 24);  \
        v[a] += v[b] + y;  v[d] = rotr64(v[d] ^ v[a], 16);  \
        v[c] += v[d];      v[b] = rotr64(v[b] ^ v[c], 63);  \

        BLAKE2_G(v, 0, 4,  8, 12, input[sigma[i][ 0]], input[sigma[i][ 1]]);
        BLAKE2_G(v, 1, 5,  9, 13, input[sigma[i][ 2]], input[sigma[i][ 3]]);
        BLAKE2_G(v, 2, 6, 10, 14, input[sigma[i][ 4]], input[sigma[i][ 5]]);
        BLAKE2_G(v, 3, 7, 11, 15, input[sigma[i][ 6]], input[sigma[i][ 7]]);
        BLAKE2_G(v, 0, 5, 10, 15, input[sigma[i][ 8]], input[sigma[i][ 9]]);
        BLAKE2_G(v, 1, 6, 11, 12, input[sigma[i][10]], input[sigma[i][11]]);
        BLAKE2_G(v, 2, 7,  8, 13, input[sigma[i][12]], input[sigma[i][13]]);
        BLAKE2_G(v, 3, 4,  9, 14, input[sigma[i][14]], input[sigma[i][15]]);
    }
    // update hash
    FOR (i, 0, 8) {
        ctx->hash[i] ^= v[i] ^ v[i+8];
    }
}

static void blake2b_reset_input(blake2b_ctx *ctx)
{
    FOR(i, 0, 16) {
        ctx->input[i] = 0;
    }
    ctx->input_idx = 0;
}

static void blake2b_end_block(blake2b_ctx *ctx)
{
    if (ctx->input_idx == 128) {  // If buffer is full,
        blake2b_incr(ctx);        // update the input offset
        blake2b_compress(ctx, 0); // and compress the (not last) block
        blake2b_reset_input(ctx);
    }
}

void blake2b_init(blake2b_ctx *ctx, size_t hash_size,
                  const uint8_t           *key, size_t key_size)
{
    // initial hash
    FOR (i, 0, 8) {
        ctx->hash[i] = iv[i];
    }
    ctx->hash[0] ^= 0x01010000 ^ (key_size << 8) ^ hash_size;

    ctx->input_offset[0] = 0;         // begining of the input, no offset
    ctx->input_offset[1] = 0;         // begining of the input, no offset
    ctx->input_idx       = 0;         // buffer is empty
    ctx->hash_size       = hash_size; // remember the hash size we want
    blake2b_reset_input(ctx);         // clear the input buffer

    // if there is a key, the first block is that key
    if (key_size > 0) {
        blake2b_update(ctx, key, key_size);
        ctx->input_idx = 128;
    }
}

void blake2b_update(blake2b_ctx *ctx,
                           const uint8_t *message, size_t message_size)
{
    // Align ourselves with 8 byte words
    while (ctx->input_idx % 8 != 0 && message_size > 0) {
        blake2b_set_input(ctx, *message);
        message++;
        message_size--;
    }

    // Process the input 8 bytes at a time
    size_t nb_words  = message_size / 8;
    size_t remainder = message_size % 8;
    FOR (i, 0, nb_words) {
        blake2b_end_block(ctx);
        ctx->input[ctx->input_idx / 8] = load64_le(message);
        message        += 8;
        ctx->input_idx += 8;
    }

    // Load the remainder
    if (remainder != 0) {
        blake2b_end_block(ctx);
    }
    FOR (i, 0, remainder) {
        blake2b_set_input(ctx, message[i]);
    }
}

void blake2b_final(blake2b_ctx *ctx, uint8_t *hash)
{
    blake2b_incr(ctx);        // update the input offset
    blake2b_compress(ctx, 1); // compress the last block
    size_t nb_words  = ctx->hash_size / 8;
    FOR (i, 0, nb_words) {
        store64_le(hash + i*8, ctx->hash[i]);
    }
    FOR (i, nb_words * 8, ctx->hash_size) {
        hash[i] = (ctx->hash[i / 8] >> (8 * (i % 8))) & 0xff;
    }
}

void blake2b(uint8_t       *hash   , size_t hash_size,
             const uint8_t *key    , size_t key_size,
             const uint8_t *message, size_t message_size)
{
    blake2b_ctx ctx;
    blake2b_init(&ctx, hash_size, key, key_size);
    blake2b_update(&ctx, message, message_size);
    blake2b_final(&ctx, hash);
}
