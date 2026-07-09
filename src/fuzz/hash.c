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

static void secp256k1_fuzz_check_hmac_sha256(const secp256k1_hash_ctx *hash_ctx, const unsigned char *key, size_t keylen, const unsigned char *msg, size_t msglen, size_t split) {
    secp256k1_hmac_sha256 one_shot;
    secp256k1_hmac_sha256 chunked;
    unsigned char one_shot_out32[32];
    unsigned char chunked_out32[32];

    FUZZ_CHECK(split <= msglen);

    secp256k1_hmac_sha256_initialize(hash_ctx, &one_shot, key, keylen);
    secp256k1_hmac_sha256_write(hash_ctx, &one_shot, msg, msglen);
    secp256k1_hmac_sha256_finalize(hash_ctx, &one_shot, one_shot_out32);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&one_shot, sizeof(one_shot)));

    secp256k1_hmac_sha256_initialize(hash_ctx, &chunked, key, keylen);
    secp256k1_hmac_sha256_write(hash_ctx, &chunked, msg, split);
    secp256k1_hmac_sha256_write(hash_ctx, &chunked, msg + split, msglen - split);
    secp256k1_hmac_sha256_finalize(hash_ctx, &chunked, chunked_out32);
    FUZZ_CHECK(memcmp(one_shot_out32, chunked_out32, sizeof(one_shot_out32)) == 0);
    FUZZ_CHECK(secp256k1_fuzz_cleared_zero(&chunked, sizeof(chunked)));
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

    secp256k1_fuzz_check_hmac_sha256(&hash_ctx, key, keylen, msg, msglen, split);
    secp256k1_fuzz_check_hmac_sha256(&hash_ctx, long_key, sizeof(long_key), msg, msglen, split);
    secp256k1_fuzz_check_rfc6979(&hash_ctx, key, keylen);
    secp256k1_fuzz_check_rfc6979(&hash_ctx, long_key, sizeof(long_key));

    return 0;
}
