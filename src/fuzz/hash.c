/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"

static int secp256k1_fuzz_all_zero(const void *ptr, size_t len) {
    const unsigned char *p = (const unsigned char *)ptr;
    size_t i;
    for (i = 0; i < len; i++) {
        if (p[i] != 0) {
            return 0;
        }
    }
    return 1;
}

static int secp256k1_fuzz_cleared_zero(void *ptr, size_t len) {
    SECP256K1_CHECKMEM_DEFINE(ptr, len);
    return secp256k1_fuzz_all_zero(ptr, len);
}

static void secp256k1_fuzz_hash_derive_bytes(unsigned char *out, size_t outlen, const unsigned char *input, size_t size, unsigned int salt) {
    secp256k1_fuzz_derive(out, outlen, input, size, salt);
    if (outlen > 0 && (secp256k1_fuzz_byte(input, size, salt) & 3u) == 0) {
        memset(out, secp256k1_fuzz_byte(input, size, salt + 1), outlen);
    }
}

/* Keep a complete SHA-256 model here so compression and padding bugs cannot
 * be shared by the implementation and its oracle. */
static uint32_t secp256k1_fuzz_sha256_reference_rotr(uint32_t x, unsigned int n) {
    return (x >> n) | (x << (32 - n));
}

static void secp256k1_fuzz_sha256_reference_compress(uint32_t state[8], const unsigned char block[64]) {
    static const uint32_t k[64] = {
        0x428a2f98ul, 0x71374491ul, 0xb5c0fbcful, 0xe9b5dba5ul,
        0x3956c25bul, 0x59f111f1ul, 0x923f82a4ul, 0xab1c5ed5ul,
        0xd807aa98ul, 0x12835b01ul, 0x243185beul, 0x550c7dc3ul,
        0x72be5d74ul, 0x80deb1feul, 0x9bdc06a7ul, 0xc19bf174ul,
        0xe49b69c1ul, 0xefbe4786ul, 0x0fc19dc6ul, 0x240ca1ccul,
        0x2de92c6ful, 0x4a7484aaul, 0x5cb0a9dcul, 0x76f988daul,
        0x983e5152ul, 0xa831c66dul, 0xb00327c8ul, 0xbf597fc7ul,
        0xc6e00bf3ul, 0xd5a79147ul, 0x06ca6351ul, 0x14292967ul,
        0x27b70a85ul, 0x2e1b2138ul, 0x4d2c6dfcul, 0x53380d13ul,
        0x650a7354ul, 0x766a0abbul, 0x81c2c92eul, 0x92722c85ul,
        0xa2bfe8a1ul, 0xa81a664bul, 0xc24b8b70ul, 0xc76c51a3ul,
        0xd192e819ul, 0xd6990624ul, 0xf40e3585ul, 0x106aa070ul,
        0x19a4c116ul, 0x1e376c08ul, 0x2748774cul, 0x34b0bcb5ul,
        0x391c0cb3ul, 0x4ed8aa4aul, 0x5b9cca4ful, 0x682e6ff3ul,
        0x748f82eeul, 0x78a5636ful, 0x84c87814ul, 0x8cc70208ul,
        0x90befffaul, 0xa4506cebul, 0xbef9a3f7ul, 0xc67178f2ul
    };
    uint32_t w[64];
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];
    size_t i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[4 * i] << 24)
             | ((uint32_t)block[4 * i + 1] << 16)
             | ((uint32_t)block[4 * i + 2] << 8)
             | (uint32_t)block[4 * i + 3];
    }
    for (i = 16; i < 64; i++) {
        uint32_t s0 = secp256k1_fuzz_sha256_reference_rotr(w[i - 15], 7)
                    ^ secp256k1_fuzz_sha256_reference_rotr(w[i - 15], 18)
                    ^ (w[i - 15] >> 3);
        uint32_t s1 = secp256k1_fuzz_sha256_reference_rotr(w[i - 2], 17)
                    ^ secp256k1_fuzz_sha256_reference_rotr(w[i - 2], 19)
                    ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    for (i = 0; i < 64; i++) {
        uint32_t s1 = secp256k1_fuzz_sha256_reference_rotr(e, 6)
                    ^ secp256k1_fuzz_sha256_reference_rotr(e, 11)
                    ^ secp256k1_fuzz_sha256_reference_rotr(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t s0 = secp256k1_fuzz_sha256_reference_rotr(a, 2)
                    ^ secp256k1_fuzz_sha256_reference_rotr(a, 13)
                    ^ secp256k1_fuzz_sha256_reference_rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t1 = h + s1 + ch + k[i] + w[i];
        uint32_t t2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

static void secp256k1_fuzz_sha256_reference(unsigned char out[32], const unsigned char *msg, size_t msglen) {
    static const uint32_t initial_state[8] = {
        0x6a09e667ul, 0xbb67ae85ul, 0x3c6ef372ul, 0xa54ff53aul,
        0x510e527ful, 0x9b05688cul, 0x1f83d9abul, 0x5be0cd19ul
    };
    uint32_t state[8];
    unsigned char block[64];
    uint64_t bit_length;
    size_t offset = 0;
    size_t remaining;
    size_t i;

    memcpy(state, initial_state, sizeof(state));
    while (msglen - offset >= sizeof(block)) {
        secp256k1_fuzz_sha256_reference_compress(state, msg + offset);
        offset += sizeof(block);
    }
    remaining = msglen - offset;
    memset(block, 0, sizeof(block));
    if (remaining != 0) {
        memcpy(block, msg + offset, remaining);
    }
    block[remaining] = 0x80;
    if (remaining >= 56) {
        secp256k1_fuzz_sha256_reference_compress(state, block);
        memset(block, 0, sizeof(block));
    }
    bit_length = (uint64_t)msglen << 3;
    for (i = 0; i < 8; i++) {
        block[63 - i] = (unsigned char)(bit_length >> (8 * i));
    }
    secp256k1_fuzz_sha256_reference_compress(state, block);
    for (i = 0; i < 8; i++) {
        out[4 * i] = (unsigned char)(state[i] >> 24);
        out[4 * i + 1] = (unsigned char)(state[i] >> 16);
        out[4 * i + 2] = (unsigned char)(state[i] >> 8);
        out[4 * i + 3] = (unsigned char)state[i];
    }
    secp256k1_memclear_explicit(state, sizeof(state));
    secp256k1_memclear_explicit(block, sizeof(block));
}

static void secp256k1_fuzz_check_sha256_reference_one(const secp256k1_hash_ctx *hash_ctx, const unsigned char *msg, size_t msglen) {
    secp256k1_sha256 production;
    secp256k1_sha256 chunked;
    unsigned char expected[32];
    unsigned char production_out[32];
    unsigned char chunked_out[32];
    size_t split = msglen / 2;

    secp256k1_fuzz_sha256_reference(expected, msg, msglen);
    secp256k1_sha256_initialize(&production);
    secp256k1_sha256_write(hash_ctx, &production, msg, msglen);
    secp256k1_sha256_finalize(hash_ctx, &production, production_out);
    FUZZ_CHECK(memcmp(production_out, expected, sizeof(expected)) == 0);

    secp256k1_sha256_initialize(&chunked);
    secp256k1_sha256_write(hash_ctx, &chunked, msg, split);
    secp256k1_sha256_write(hash_ctx, &chunked, msg + split, msglen - split);
    secp256k1_sha256_finalize(hash_ctx, &chunked, chunked_out);
    FUZZ_CHECK(memcmp(chunked_out, expected, sizeof(expected)) == 0);
    secp256k1_sha256_clear(&production);
    secp256k1_sha256_clear(&chunked);
}

static void secp256k1_fuzz_check_sha256_reference(const secp256k1_hash_ctx *hash_ctx, const unsigned char *msg, size_t msglen) {
    static const size_t boundary_lengths[] = { 0, 1, 55, 56, 63, 64, 65, 127, 128, 129, 191, 192 };
    static const unsigned char boundary_192_output[32] = {
        0x7d, 0x56, 0xe3, 0x6b, 0x9b, 0xf4, 0xce, 0x9d,
        0xc9, 0xfc, 0x11, 0x73, 0x47, 0x53, 0x28, 0xff,
        0x99, 0xba, 0xfb, 0x06, 0xf2, 0x33, 0xbe, 0xa8,
        0x7b, 0xb2, 0x47, 0x7b, 0xe9, 0x5b, 0x36, 0xc6
    };
    unsigned char boundary[192];
    unsigned char model_output[32];
    size_t i;

    for (i = 0; i < sizeof(boundary); i++) {
        boundary[i] = (unsigned char)(0x31u + i * 73u);
    }
    secp256k1_fuzz_sha256_reference(model_output, boundary, sizeof(boundary));
    FUZZ_CHECK(memcmp(model_output, boundary_192_output, sizeof(model_output)) == 0);
    for (i = 0; i < sizeof(boundary_lengths) / sizeof(boundary_lengths[0]); i++) {
        secp256k1_fuzz_check_sha256_reference_one(hash_ctx, boundary, boundary_lengths[i]);
    }
    secp256k1_fuzz_check_sha256_reference_one(hash_ctx, msg, msglen);
    secp256k1_memclear_explicit(model_output, sizeof(model_output));
    secp256k1_memclear_explicit(boundary, sizeof(boundary));
}

static void secp256k1_fuzz_check_sha256_vectors(const secp256k1_hash_ctx *hash_ctx) {
    static const unsigned char zeroes[65] = { 0 };
    static const size_t zero_lengths[] = { 0, 55, 56, 63, 64, 65 };
    static const unsigned char zero_outputs[][32] = {
        { 0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27,
          0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55 },
        { 0x02, 0x77, 0x94, 0x66, 0xcd, 0xec, 0x16, 0x38, 0x11, 0xd0, 0x78, 0x81, 0x5c, 0x63, 0x3f, 0x21,
          0x90, 0x14, 0x13, 0x08, 0x14, 0x49, 0x00, 0x2f, 0x24, 0xaa, 0x3e, 0x80, 0xf0, 0xb8, 0x8e, 0xf7 },
        { 0xd4, 0x81, 0x7a, 0xa5, 0x49, 0x76, 0x28, 0xe7, 0xc7, 0x7e, 0x6b, 0x60, 0x61, 0x07, 0x04, 0x2b,
          0xbb, 0xa3, 0x13, 0x08, 0x88, 0xc5, 0xf4, 0x7a, 0x37, 0x5e, 0x61, 0x79, 0xbe, 0x78, 0x9f, 0xbb },
        { 0xc7, 0x72, 0x3f, 0xa1, 0xe0, 0x12, 0x79, 0x75, 0xe4, 0x9e, 0x62, 0xe7, 0x53, 0xdb, 0x53, 0x92,
          0x4c, 0x1b, 0xd8, 0x4b, 0x8a, 0xc1, 0xac, 0x08, 0xdf, 0x78, 0xd0, 0x92, 0x70, 0xf3, 0xd9, 0x71 },
        { 0xf5, 0xa5, 0xfd, 0x42, 0xd1, 0x6a, 0x20, 0x30, 0x27, 0x98, 0xef, 0x6e, 0xd3, 0x09, 0x97, 0x9b,
          0x43, 0x00, 0x3d, 0x23, 0x20, 0xd9, 0xf0, 0xe8, 0xea, 0x98, 0x31, 0xa9, 0x27, 0x59, 0xfb, 0x4b },
        { 0x98, 0xce, 0x42, 0xde, 0xef, 0x51, 0xd4, 0x02, 0x69, 0xd5, 0x42, 0xf5, 0x31, 0x4b, 0xef, 0x2c,
          0x74, 0x68, 0xd4, 0x01, 0xad, 0x5d, 0x85, 0x16, 0x8b, 0xfa, 0xb4, 0xc0, 0x10, 0x8f, 0x75, 0xf7 }
    };
    static const unsigned char abc[] = { 'a', 'b', 'c' };
    static const unsigned char abc_output[32] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };
    secp256k1_sha256 one_shot;
    secp256k1_sha256 split;
    unsigned char one_shot_out[32];
    unsigned char split_out[32];
    size_t i;
    size_t split_at;

    for (i = 0; i < sizeof(zero_lengths) / sizeof(zero_lengths[0]); i++) {
        secp256k1_sha256_initialize(&one_shot);
        secp256k1_sha256_write(hash_ctx, &one_shot, zeroes, zero_lengths[i]);
        secp256k1_sha256_finalize(hash_ctx, &one_shot, one_shot_out);
        FUZZ_CHECK(memcmp(one_shot_out, zero_outputs[i], sizeof(one_shot_out)) == 0);

        secp256k1_sha256_initialize(&split);
        split_at = zero_lengths[i] / 2;
        secp256k1_sha256_write(hash_ctx, &split, zeroes, split_at);
        secp256k1_sha256_write(hash_ctx, &split, zeroes + split_at, zero_lengths[i] - split_at);
        secp256k1_sha256_finalize(hash_ctx, &split, split_out);
        FUZZ_CHECK(memcmp(split_out, zero_outputs[i], sizeof(split_out)) == 0);
        FUZZ_CHECK(memcmp(one_shot_out, split_out, sizeof(one_shot_out)) == 0);
        secp256k1_sha256_clear(&one_shot);
        secp256k1_sha256_clear(&split);
        FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&one_shot, sizeof(one_shot)));
        FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&split, sizeof(split)));
    }

    secp256k1_sha256_initialize(&one_shot);
    secp256k1_sha256_write(hash_ctx, &one_shot, abc, sizeof(abc));
    secp256k1_sha256_finalize(hash_ctx, &one_shot, one_shot_out);
    FUZZ_CHECK(memcmp(one_shot_out, abc_output, sizeof(one_shot_out)) == 0);
    secp256k1_sha256_initialize(&split);
    secp256k1_sha256_write(hash_ctx, &split, abc, 1);
    secp256k1_sha256_write(hash_ctx, &split, abc + 1, sizeof(abc) - 1);
    secp256k1_sha256_finalize(hash_ctx, &split, split_out);
    FUZZ_CHECK(memcmp(split_out, abc_output, sizeof(split_out)) == 0);
    FUZZ_CHECK(memcmp(one_shot_out, split_out, sizeof(one_shot_out)) == 0);
    secp256k1_sha256_clear(&one_shot);
    secp256k1_sha256_clear(&split);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&one_shot, sizeof(one_shot)));
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&split, sizeof(split)));
}

static void secp256k1_fuzz_check_sha256_midstate(const secp256k1_hash_ctx *hash_ctx) {
    static const unsigned char tag[] = "sha256_midstate_test_tag";
    static const unsigned char suffix[] = "fuzz-midstate-suffix";
    static const uint32_t midstate[8] = {
        0xa9ec59eaul, 0x9b4c2ffful, 0x400821e2ul, 0x0dcf3847ul,
        0xbe7ea179ul, 0xa5772bdcul, 0x7d29bfe3ul, 0xa486b855ul
    };
    secp256k1_sha256 generic;
    secp256k1_sha256 optimized;
    unsigned char generic_out[32];
    unsigned char optimized_out[32];

    secp256k1_sha256_initialize_tagged(hash_ctx, &generic, tag, sizeof(tag) - 1);
    secp256k1_sha256_initialize_midstate(&optimized, 64, midstate);
    FUZZ_CHECK(generic.bytes == optimized.bytes);
    FUZZ_CHECK(memcmp(generic.s, optimized.s, sizeof(generic.s)) == 0);

    secp256k1_sha256_write(hash_ctx, &generic, suffix, sizeof(suffix) - 1);
    secp256k1_sha256_write(hash_ctx, &optimized, suffix, sizeof(suffix) - 1);
    secp256k1_sha256_finalize(hash_ctx, &generic, generic_out);
    secp256k1_sha256_finalize(hash_ctx, &optimized, optimized_out);
    FUZZ_CHECK(memcmp(generic_out, optimized_out, sizeof(generic_out)) == 0);
    secp256k1_sha256_clear(&generic);
    secp256k1_sha256_clear(&optimized);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&generic, sizeof(generic)));
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&optimized, sizeof(optimized)));
}

static void secp256k1_fuzz_check_hmac_key_boundaries(const secp256k1_hash_ctx *hash_ctx) {
    static const size_t key_lengths[] = { 63, 64, 65, 127, 128, 129 };
    static const unsigned char expected[][32] = {
        { 0xc8, 0x50, 0x8e, 0xad, 0x91, 0xdd, 0x88, 0xb0, 0x92, 0x17, 0xb1, 0x70, 0xa4, 0x98, 0xd3, 0xbb,
          0x13, 0xb0, 0xce, 0x69, 0xfd, 0x8d, 0xdb, 0xb9, 0x71, 0xd1, 0xf2, 0x1c, 0x75, 0x10, 0x41, 0xbf },
        { 0x13, 0xa1, 0xca, 0x66, 0xb8, 0xf3, 0xcc, 0xfb, 0xad, 0x49, 0x42, 0x55, 0xab, 0x8c, 0x01, 0x33,
          0x63, 0x24, 0x42, 0x61, 0x94, 0xba, 0xc1, 0xb2, 0x0f, 0xb6, 0x33, 0xee, 0x65, 0xfb, 0xc7, 0x9e },
        { 0x91, 0x95, 0xe7, 0x3d, 0x31, 0x19, 0x02, 0xb0, 0xd9, 0xab, 0x4d, 0x93, 0xc9, 0x61, 0xf0, 0x9d,
          0x2a, 0x5c, 0x30, 0x31, 0x9f, 0x6b, 0xfc, 0x47, 0x3b, 0xe9, 0xe1, 0xbf, 0x32, 0x08, 0x1d, 0xa0 },
        { 0x09, 0x92, 0x7a, 0x1b, 0x40, 0x25, 0x8f, 0x03, 0xd9, 0x27, 0xf1, 0x2d, 0x7a, 0x52, 0xf7, 0xd1, 0x04,
          0xf0, 0x7c, 0x94, 0xb7, 0x0b, 0x9d, 0x21, 0xc8, 0x8b, 0x92, 0x41, 0x8b, 0xfd, 0xa9, 0xbb },
        { 0xdb, 0xaa, 0xab, 0x26, 0x21, 0x1b, 0x36, 0x7e, 0x2b, 0x37, 0x03, 0x44, 0x21, 0x71, 0x7b, 0x3e,
          0x80, 0x31, 0x9a, 0x31, 0xa4, 0xec, 0xb1, 0x08, 0x88, 0xbf, 0x27, 0x56, 0xef, 0x7b, 0xf5, 0x24 },
        { 0xd6, 0x2b, 0x84, 0xf0, 0x5b, 0x7c, 0x7a, 0xa1, 0xc8, 0x35, 0x7a, 0xc7, 0xbd, 0x5d, 0x16, 0xf8, 0xe7,
          0xf7, 0x62, 0x5f, 0xc9, 0x46, 0x53, 0x11, 0xc3, 0xd4, 0xec, 0xbc, 0xfb, 0x7c, 0x74, 0x28 }
    };
    unsigned char key[129];
    unsigned char msg[64];
    unsigned char output[32];
    secp256k1_hmac_sha256 hmac;
    size_t i;

    memset(key, 0x0b, sizeof(key));
    memset(msg, 0xdd, sizeof(msg));
    for (i = 0; i < sizeof(key_lengths) / sizeof(key_lengths[0]); i++) {
        secp256k1_hmac_sha256_initialize(hash_ctx, &hmac, key, key_lengths[i]);
        secp256k1_hmac_sha256_write(hash_ctx, &hmac, msg, sizeof(msg));
        secp256k1_hmac_sha256_finalize(hash_ctx, &hmac, output);
        FUZZ_CHECK(memcmp(output, expected[i], sizeof(output)) == 0);
        FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&hmac, sizeof(hmac)));
    }
    secp256k1_memclear_explicit(key, sizeof(key));
    secp256k1_memclear_explicit(msg, sizeof(msg));
}

static void secp256k1_fuzz_hmac_sha256_reference(const secp256k1_hash_ctx *hash_ctx, unsigned char out[32], const unsigned char *key, size_t keylen, const unsigned char *part_a, size_t part_a_len, const unsigned char *part_b, size_t part_b_len, const unsigned char *part_c, size_t part_c_len);

static void secp256k1_fuzz_check_hmac_sha256(const secp256k1_hash_ctx *hash_ctx, const unsigned char *key, size_t keylen, const unsigned char *msg, size_t msglen, size_t split) {
    secp256k1_hmac_sha256 one_shot;
    secp256k1_hmac_sha256 chunked;
    unsigned char one_shot_out32[32];
    unsigned char chunked_out32[32];
    unsigned char reference_out32[32];

    FUZZ_CHECK(split <= msglen);

    secp256k1_hmac_sha256_initialize(hash_ctx, &one_shot, key, keylen);
    secp256k1_hmac_sha256_write(hash_ctx, &one_shot, msg, msglen);
    secp256k1_hmac_sha256_finalize(hash_ctx, &one_shot, one_shot_out32);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&one_shot, sizeof(one_shot)));

    secp256k1_fuzz_hmac_sha256_reference(hash_ctx, reference_out32, key, keylen, msg, msglen, NULL, 0, NULL, 0);
    FUZZ_CHECK(memcmp(one_shot_out32, reference_out32, sizeof(one_shot_out32)) == 0);

    secp256k1_hmac_sha256_initialize(hash_ctx, &chunked, key, keylen);
    secp256k1_hmac_sha256_write(hash_ctx, &chunked, msg, split);
    secp256k1_hmac_sha256_write(hash_ctx, &chunked, msg + split, msglen - split);
    secp256k1_hmac_sha256_finalize(hash_ctx, &chunked, chunked_out32);
    FUZZ_CHECK(memcmp(one_shot_out32, chunked_out32, sizeof(one_shot_out32)) == 0);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&chunked, sizeof(chunked)));
    secp256k1_memclear_explicit(reference_out32, sizeof(reference_out32));
}

static void secp256k1_fuzz_hmac_sha256_reference(const secp256k1_hash_ctx *hash_ctx, unsigned char out[32], const unsigned char *key, size_t keylen, const unsigned char *part_a, size_t part_a_len, const unsigned char *part_b, size_t part_b_len, const unsigned char *part_c, size_t part_c_len) {
    secp256k1_sha256 inner;
    secp256k1_sha256 outer;
    secp256k1_sha256 key_hash;
    unsigned char rkey[64];
    unsigned char inner_out[32];
    size_t i;

    if (keylen <= sizeof(rkey)) {
        if (keylen != 0) {
            memcpy(rkey, key, keylen);
        }
        memset(rkey + keylen, 0, sizeof(rkey) - keylen);
    } else {
        secp256k1_sha256_initialize(&key_hash);
        secp256k1_sha256_write(hash_ctx, &key_hash, key, keylen);
        secp256k1_sha256_finalize(hash_ctx, &key_hash, rkey);
        secp256k1_sha256_clear(&key_hash);
        memset(rkey + 32, 0, 32);
    }

    for (i = 0; i < sizeof(rkey); i++) {
        rkey[i] ^= 0x36;
    }
    secp256k1_sha256_initialize(&inner);
    secp256k1_sha256_write(hash_ctx, &inner, rkey, sizeof(rkey));
    if (part_a_len != 0) {
        secp256k1_sha256_write(hash_ctx, &inner, part_a, part_a_len);
    }
    if (part_b_len != 0) {
        secp256k1_sha256_write(hash_ctx, &inner, part_b, part_b_len);
    }
    if (part_c_len != 0) {
        secp256k1_sha256_write(hash_ctx, &inner, part_c, part_c_len);
    }
    secp256k1_sha256_finalize(hash_ctx, &inner, inner_out);

    for (i = 0; i < sizeof(rkey); i++) {
        rkey[i] ^= 0x6A;
    }
    secp256k1_sha256_initialize(&outer);
    secp256k1_sha256_write(hash_ctx, &outer, rkey, sizeof(rkey));
    secp256k1_sha256_write(hash_ctx, &outer, inner_out, sizeof(inner_out));
    secp256k1_sha256_finalize(hash_ctx, &outer, out);

    secp256k1_sha256_clear(&inner);
    secp256k1_sha256_clear(&outer);
    secp256k1_memclear_explicit(rkey, sizeof(rkey));
    secp256k1_memclear_explicit(inner_out, sizeof(inner_out));
}

/* Keep RFC6979's state transitions independent of the production RNG and HMAC
 * wrapper. The reference below uses raw SHA256 to model each HMAC transition. */
static void secp256k1_fuzz_rfc6979_reference_initialize(const secp256k1_hash_ctx *hash_ctx, unsigned char k[32], unsigned char v[32], const unsigned char *key, size_t keylen) {
    static const unsigned char zero = 0x00;
    static const unsigned char one = 0x01;

    memset(k, 0x00, 32);
    memset(v, 0x01, 32);
    secp256k1_fuzz_hmac_sha256_reference(hash_ctx, k, k, 32, v, 32, &zero, 1, key, keylen);
    secp256k1_fuzz_hmac_sha256_reference(hash_ctx, v, k, 32, v, 32, NULL, 0, NULL, 0);
    secp256k1_fuzz_hmac_sha256_reference(hash_ctx, k, k, 32, v, 32, &one, 1, key, keylen);
    secp256k1_fuzz_hmac_sha256_reference(hash_ctx, v, k, 32, v, 32, NULL, 0, NULL, 0);
}

static void secp256k1_fuzz_rfc6979_reference_generate(const secp256k1_hash_ctx *hash_ctx, unsigned char k[32], unsigned char v[32], int retry, unsigned char *out, size_t outlen) {
    static const unsigned char zero = 0x00;

    if (retry) {
        secp256k1_fuzz_hmac_sha256_reference(hash_ctx, k, k, 32, v, 32, &zero, 1, NULL, 0);
        secp256k1_fuzz_hmac_sha256_reference(hash_ctx, v, k, 32, v, 32, NULL, 0, NULL, 0);
    }
    while (outlen != 0) {
        size_t now = outlen < 32 ? outlen : 32;
        secp256k1_fuzz_hmac_sha256_reference(hash_ctx, v, k, 32, v, 32, NULL, 0, NULL, 0);
        memcpy(out, v, now);
        out += now;
        outlen -= now;
    }
}

static void secp256k1_fuzz_check_rfc6979_reference(const secp256k1_hash_ctx *hash_ctx, const unsigned char *key, size_t keylen) {
    secp256k1_rfc6979_hmac_sha256 production;
    secp256k1_rfc6979_hmac_sha256 production_one_shot;
    unsigned char reference_k[32];
    unsigned char reference_v[32];
    unsigned char reference_one_shot_k[32];
    unsigned char reference_one_shot_v[32];
    unsigned char production_out[96];
    unsigned char production_one_shot_out[96];
    unsigned char reference_out[96];
    unsigned char reference_one_shot_out[96];

    secp256k1_rfc6979_hmac_sha256_initialize(hash_ctx, &production, key, keylen);
    secp256k1_rfc6979_hmac_sha256_generate(hash_ctx, &production, production_out, 32);
    secp256k1_rfc6979_hmac_sha256_generate(hash_ctx, &production, production_out + 32, sizeof(production_out) - 32);
    secp256k1_rfc6979_hmac_sha256_finalize(&production);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&production, sizeof(production)));

    memset(production_one_shot_out, 0xA5, sizeof(production_one_shot_out));
    secp256k1_rfc6979_hmac_sha256_initialize(hash_ctx, &production_one_shot, key, keylen);
    secp256k1_rfc6979_hmac_sha256_generate(hash_ctx, &production_one_shot, production_one_shot_out, sizeof(production_one_shot_out));
    secp256k1_rfc6979_hmac_sha256_finalize(&production_one_shot);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&production_one_shot, sizeof(production_one_shot)));

    secp256k1_fuzz_rfc6979_reference_initialize(hash_ctx, reference_k, reference_v, key, keylen);
    secp256k1_fuzz_rfc6979_reference_generate(hash_ctx, reference_k, reference_v, 0, reference_out, 32);
    secp256k1_fuzz_rfc6979_reference_generate(hash_ctx, reference_k, reference_v, 1, reference_out + 32, sizeof(reference_out) - 32);
    FUZZ_CHECK(memcmp(production_out, reference_out, sizeof(production_out)) == 0);
    secp256k1_fuzz_rfc6979_reference_initialize(hash_ctx, reference_one_shot_k, reference_one_shot_v, key, keylen);
    secp256k1_fuzz_rfc6979_reference_generate(hash_ctx, reference_one_shot_k, reference_one_shot_v, 0, reference_one_shot_out, sizeof(reference_one_shot_out));
    FUZZ_CHECK(memcmp(production_one_shot_out, reference_one_shot_out, sizeof(production_one_shot_out)) == 0);
    secp256k1_memclear_explicit(reference_k, sizeof(reference_k));
    secp256k1_memclear_explicit(reference_v, sizeof(reference_v));
    secp256k1_memclear_explicit(reference_one_shot_k, sizeof(reference_one_shot_k));
    secp256k1_memclear_explicit(reference_one_shot_v, sizeof(reference_one_shot_v));
    secp256k1_memclear_explicit(production_out, sizeof(production_out));
    secp256k1_memclear_explicit(production_one_shot_out, sizeof(production_one_shot_out));
    secp256k1_memclear_explicit(reference_out, sizeof(reference_out));
    secp256k1_memclear_explicit(reference_one_shot_out, sizeof(reference_one_shot_out));
}

static void secp256k1_fuzz_check_rfc6979(const secp256k1_hash_ctx *hash_ctx, const unsigned char *key, size_t keylen) {
    secp256k1_rfc6979_hmac_sha256 one_shot;
    secp256k1_rfc6979_hmac_sha256 chunked_a;
    secp256k1_rfc6979_hmac_sha256 chunked_b;
    unsigned char one_shot_out[96];
    unsigned char chunked_a_out[96];
    unsigned char chunked_b_out[96];

    secp256k1_rfc6979_hmac_sha256_initialize(hash_ctx, &one_shot, key, keylen);
    secp256k1_rfc6979_hmac_sha256_generate(hash_ctx, &one_shot, one_shot_out, sizeof(one_shot_out));
    secp256k1_rfc6979_hmac_sha256_finalize(&one_shot);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&one_shot, sizeof(one_shot)));

    secp256k1_rfc6979_hmac_sha256_initialize(hash_ctx, &chunked_a, key, keylen);
    secp256k1_rfc6979_hmac_sha256_generate(hash_ctx, &chunked_a, chunked_a_out, 32);
    secp256k1_rfc6979_hmac_sha256_generate(hash_ctx, &chunked_a, chunked_a_out + 32, sizeof(chunked_a_out) - 32);
    secp256k1_rfc6979_hmac_sha256_finalize(&chunked_a);
    FUZZ_CHECK(memcmp(one_shot_out, chunked_a_out, 32) == 0);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&chunked_a, sizeof(chunked_a)));

    secp256k1_rfc6979_hmac_sha256_initialize(hash_ctx, &chunked_b, key, keylen);
    secp256k1_rfc6979_hmac_sha256_generate(hash_ctx, &chunked_b, chunked_b_out, 32);
    secp256k1_rfc6979_hmac_sha256_generate(hash_ctx, &chunked_b, chunked_b_out + 32, sizeof(chunked_b_out) - 32);
    secp256k1_rfc6979_hmac_sha256_finalize(&chunked_b);
    FUZZ_CHECK(memcmp(chunked_a_out, chunked_b_out, sizeof(chunked_a_out)) == 0);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&chunked_b, sizeof(chunked_b)));
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_hash_ctx hash_ctx;
    unsigned char key[160];
    unsigned char long_key[96];
    unsigned char msg[192];
    size_t keylen;
    size_t msglen;
    size_t split;

    keylen = secp256k1_fuzz_byte(input, size, 3) % (sizeof(key) + 1);
    msglen = secp256k1_fuzz_byte(input, size, 7) % (sizeof(msg) + 1);
    split = msglen == 0 ? 0 : secp256k1_fuzz_byte(input, size, 11) % (msglen + 1);
    secp256k1_fuzz_hash_derive_bytes(key, sizeof(key), input, size, 13);
    secp256k1_fuzz_hash_derive_bytes(long_key, sizeof(long_key), input, size, 17);
    secp256k1_fuzz_hash_derive_bytes(msg, sizeof(msg), input, size, 19);
    secp256k1_hash_ctx_init(&hash_ctx);

    secp256k1_fuzz_check_sha256_vectors(&hash_ctx);
    secp256k1_fuzz_check_sha256_reference(&hash_ctx, msg, msglen);
    secp256k1_fuzz_check_sha256_midstate(&hash_ctx);
    secp256k1_fuzz_check_hmac_key_boundaries(&hash_ctx);
    secp256k1_fuzz_check_hmac_sha256(&hash_ctx, key, keylen, msg, msglen, split);
    secp256k1_fuzz_check_hmac_sha256(&hash_ctx, long_key, sizeof(long_key), msg, msglen, split);
    secp256k1_fuzz_check_rfc6979(&hash_ctx, key, keylen);
    secp256k1_fuzz_check_rfc6979(&hash_ctx, long_key, sizeof(long_key));
    secp256k1_fuzz_check_rfc6979_reference(&hash_ctx, key, keylen);
    secp256k1_fuzz_check_rfc6979_reference(&hash_ctx, long_key, sizeof(long_key));

    return 0;
}
