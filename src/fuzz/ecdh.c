/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"

#ifdef ENABLE_MODULE_ECDH
static void secp256k1_fuzz_check_ecdh_default_hash(const secp256k1_context *ctx, const secp256k1_pubkey *shared_pubkey, const unsigned char *output32) {
    secp256k1_hash_ctx hash_ctx;
    secp256k1_sha256 sha;
    unsigned char compressed[33];
    unsigned char expected[32];
    size_t compressed_len = sizeof(compressed);

    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, compressed, &compressed_len, shared_pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(compressed_len == sizeof(compressed));
    secp256k1_hash_ctx_init(&hash_ctx);
    secp256k1_sha256_initialize(&sha);
    secp256k1_sha256_write(&hash_ctx, &sha, compressed, sizeof(compressed));
    secp256k1_sha256_finalize(&hash_ctx, &sha, expected);
    FUZZ_CHECK(memcmp(output32, expected, sizeof(expected)) == 0);
}

static int fuzz_ecdh_hash_passthrough(unsigned char *output, const unsigned char *x32, const unsigned char *y32, void *data) {
    (void)data;
    memcpy(output, x32, 32);
    memcpy(output + 32, y32, 32);
    return 1;
}

static int fuzz_ecdh_hash_fail(unsigned char *output, const unsigned char *x32, const unsigned char *y32, void *data) {
    (void)output;
    (void)x32;
    (void)y32;
    (void)data;
    return 0;
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_ECDH
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 51);
    unsigned char seckey_a[32];
    unsigned char seckey_b[32];
    secp256k1_pubkey pubkey_a;
    secp256k1_pubkey pubkey_b;
    secp256k1_pubkey shared_pubkey_ab;
    secp256k1_pubkey shared_pubkey_ba;
    unsigned char shared_ab[64];
    unsigned char shared_ba[64];
    unsigned char default_ab[32];
    unsigned char default_ba[32];
    unsigned char explicit_ab[32];
    unsigned char explicit_ba[32];
    unsigned char default_fn_ab[32];
    unsigned char default_fn_ba[32];
    unsigned char shared_ser[65];
    unsigned char fail_output[64];
    size_t shared_ser_len = sizeof(shared_ser);

    secp256k1_fuzz_valid_seckey32(ctx, seckey_a, input, size, 53);
    secp256k1_fuzz_valid_seckey32(ctx, seckey_b, input, size, 59);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_a, seckey_a) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_b, seckey_b) == 1);

    FUZZ_CHECK(secp256k1_ecdh(ctx, default_ab, &pubkey_b, seckey_a, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_ba, &pubkey_a, seckey_b, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, explicit_ab, &pubkey_b, seckey_a, secp256k1_ecdh_hash_function_sha256, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, explicit_ba, &pubkey_a, seckey_b, secp256k1_ecdh_hash_function_sha256, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_fn_ab, &pubkey_b, seckey_a, secp256k1_ecdh_hash_function_default, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_fn_ba, &pubkey_a, seckey_b, secp256k1_ecdh_hash_function_default, NULL) == 1);
    FUZZ_CHECK(memcmp(default_ab, default_ba, sizeof(default_ab)) == 0);
    FUZZ_CHECK(memcmp(default_ab, explicit_ab, sizeof(default_ab)) == 0);
    FUZZ_CHECK(memcmp(default_ba, explicit_ba, sizeof(default_ba)) == 0);
    FUZZ_CHECK(memcmp(default_ab, default_fn_ab, sizeof(default_ab)) == 0);
    FUZZ_CHECK(memcmp(default_ba, default_fn_ba, sizeof(default_ba)) == 0);

    FUZZ_CHECK(secp256k1_ecdh(ctx, shared_ab, &pubkey_b, seckey_a, fuzz_ecdh_hash_passthrough, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, shared_ba, &pubkey_a, seckey_b, fuzz_ecdh_hash_passthrough, NULL) == 1);
    FUZZ_CHECK(memcmp(shared_ab, shared_ba, sizeof(shared_ab)) == 0);

    shared_pubkey_ab = pubkey_b;
    shared_pubkey_ba = pubkey_a;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &shared_pubkey_ab, seckey_a) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &shared_pubkey_ba, seckey_b) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &shared_pubkey_ab, &shared_pubkey_ba) == 0);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, shared_ser, &shared_ser_len, &shared_pubkey_ab, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(shared_ser_len == sizeof(shared_ser));
    FUZZ_CHECK(memcmp(shared_ab, shared_ser + 1, 32) == 0);
    FUZZ_CHECK(memcmp(shared_ab + 32, shared_ser + 33, 32) == 0);
    secp256k1_fuzz_check_ecdh_default_hash(ctx, &shared_pubkey_ab, default_ab);
    secp256k1_fuzz_check_ecdh_default_hash(ctx, &shared_pubkey_ab, explicit_ab);
    secp256k1_fuzz_check_ecdh_default_hash(ctx, &shared_pubkey_ab, default_fn_ab);
    secp256k1_fuzz_check_ecdh_default_hash(ctx, &shared_pubkey_ba, default_ba);

    FUZZ_CHECK(secp256k1_ecdh(ctx, fail_output, &pubkey_b, secp256k1_fuzz_scalar_zero, NULL, NULL) == 0);
    FUZZ_CHECK(secp256k1_ecdh(ctx, fail_output, &pubkey_b, secp256k1_fuzz_scalar_order, NULL, NULL) == 0);
    FUZZ_CHECK(secp256k1_ecdh(ctx, fail_output, &pubkey_b, seckey_a, fuzz_ecdh_hash_fail, NULL) == 0);

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
