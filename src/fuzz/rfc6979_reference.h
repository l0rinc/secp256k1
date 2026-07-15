/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_FUZZ_RFC6979_REFERENCE_H
#define SECP256K1_FUZZ_RFC6979_REFERENCE_H

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

#endif /* SECP256K1_FUZZ_RFC6979_REFERENCE_H */
