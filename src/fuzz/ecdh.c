/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"

#ifdef ENABLE_MODULE_ECDH
static size_t secp256k1_fuzz_ecdh_sha256_compression_calls = 0;

static void secp256k1_fuzz_ecdh_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    secp256k1_fuzz_ecdh_sha256_compression_calls += n_blocks;
    secp256k1_sha256_transform(state, blocks64, n_blocks);
}

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

static void secp256k1_fuzz_check_ecdh_odd_y_default_hash(const secp256k1_context *ctx) {
    secp256k1_pubkey shared_pubkey;
    unsigned char output32[32];
    unsigned char explicit_output32[32];
    unsigned char compressed[33];
    size_t compressed_len = sizeof(compressed);

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &shared_pubkey, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &shared_pubkey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, compressed, &compressed_len, &shared_pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(compressed_len == sizeof(compressed));
    FUZZ_CHECK(compressed[0] == SECP256K1_TAG_PUBKEY_ODD);
    FUZZ_CHECK(secp256k1_ecdh(ctx, output32, &shared_pubkey, secp256k1_fuzz_scalar_one, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, explicit_output32, &shared_pubkey, secp256k1_fuzz_scalar_one, secp256k1_ecdh_hash_function_sha256, NULL) == 1);
    FUZZ_CHECK(memcmp(output32, explicit_output32, sizeof(output32)) == 0);
    secp256k1_fuzz_check_ecdh_default_hash(ctx, &shared_pubkey, output32);
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

typedef struct {
    const void *self;
    unsigned char mask32[32];
    int calls;
} secp256k1_fuzz_ecdh_hash_data;

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_ecdh_illegal_data;

static void secp256k1_fuzz_ecdh_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_ecdh_illegal_data *illegal_data = (secp256k1_fuzz_ecdh_illegal_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(illegal_data != NULL);
    FUZZ_CHECK(illegal_data->self == illegal_data);
    illegal_data->calls++;
}

static int fuzz_ecdh_hash_with_data(unsigned char *output, const unsigned char *x32, const unsigned char *y32, void *data) {
    secp256k1_fuzz_ecdh_hash_data *hash_data = (secp256k1_fuzz_ecdh_hash_data *)data;
    size_t i;

    FUZZ_CHECK(hash_data != NULL);
    FUZZ_CHECK(hash_data->self == hash_data);
    hash_data->calls++;
    for (i = 0; i < 32; i++) {
        output[i] = (unsigned char)(x32[i] ^ hash_data->mask32[i]);
        output[32 + i] = (unsigned char)(y32[i] ^ hash_data->mask32[31 - i]);
    }
    return 1;
}

static void secp256k1_fuzz_check_ecdh_invalid_pubkey(secp256k1_context *ctx, const unsigned char *valid_seckey32, const unsigned char *mask32) {
    secp256k1_fuzz_ecdh_illegal_data illegal_data;
    secp256k1_fuzz_ecdh_hash_data hash_data;
    secp256k1_pubkey invalid_pubkey;
    unsigned char default_output[32];
    unsigned char custom_output[64];
    unsigned char zero32[32] = { 0 };
    unsigned int calls;

    memset(&invalid_pubkey, 0, sizeof(invalid_pubkey));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_ecdh_illegal_callback, &illegal_data);

    memset(default_output, 0xA5, sizeof(default_output));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_output, &invalid_pubkey, valid_seckey32, NULL, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(default_output, zero32, sizeof(default_output)) == 0);

    hash_data.self = &hash_data;
    memcpy(hash_data.mask32, mask32, sizeof(hash_data.mask32));
    hash_data.calls = 0;
    memset(custom_output, 0xA5, sizeof(custom_output));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdh(ctx, custom_output, &invalid_pubkey, valid_seckey32, fuzz_ecdh_hash_with_data, &hash_data) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(hash_data.calls == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_ECDH
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 51);
    secp256k1_fuzz_ecdh_hash_data hash_data;
    unsigned char seckey_a[32];
    unsigned char seckey_b[32];
    secp256k1_pubkey pubkey_a;
    secp256k1_pubkey pubkey_b;
    secp256k1_pubkey shared_pubkey_ab;
    secp256k1_pubkey shared_pubkey_ba;
    unsigned char shared_ab[64];
    unsigned char shared_ba[64];
    unsigned char masked_ab[64];
    unsigned char masked_ba[64];
    unsigned char default_ab[32];
    unsigned char default_ba[32];
    unsigned char explicit_ab[32];
    unsigned char explicit_ba[32];
    unsigned char default_fn_ab[32];
    unsigned char default_fn_ba[32];
    unsigned char shared_ser[65];
    unsigned char default_fail_output[32];
    unsigned char fail_output[64];
    unsigned char zero32[32] = { 0 };
    size_t shared_ser_len = sizeof(shared_ser);
    size_t i;

    secp256k1_fuzz_valid_seckey32(ctx, seckey_a, input, size, 53);
    secp256k1_fuzz_valid_seckey32(ctx, seckey_b, input, size, 59);
    hash_data.self = &hash_data;
    secp256k1_fuzz_derive(hash_data.mask32, sizeof(hash_data.mask32), input, size, 67);
    hash_data.calls = 0;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_a, seckey_a) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_b, seckey_b) == 1);
    secp256k1_fuzz_check_ecdh_odd_y_default_hash(ctx);

    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_ecdh_sha256_compression);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_ab, &pubkey_b, seckey_a, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_ba, &pubkey_a, seckey_b, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
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
    FUZZ_CHECK(secp256k1_ecdh(ctx, masked_ab, &pubkey_b, seckey_a, fuzz_ecdh_hash_with_data, &hash_data) == 1);
    FUZZ_CHECK(hash_data.calls == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, masked_ba, &pubkey_a, seckey_b, fuzz_ecdh_hash_with_data, &hash_data) == 1);
    FUZZ_CHECK(hash_data.calls == 2);
    FUZZ_CHECK(memcmp(masked_ab, masked_ba, sizeof(masked_ab)) == 0);

    shared_pubkey_ab = pubkey_b;
    shared_pubkey_ba = pubkey_a;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &shared_pubkey_ab, seckey_a) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &shared_pubkey_ba, seckey_b) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &shared_pubkey_ab, &shared_pubkey_ba) == 0);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, shared_ser, &shared_ser_len, &shared_pubkey_ab, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(shared_ser_len == sizeof(shared_ser));
    FUZZ_CHECK(memcmp(shared_ab, shared_ser + 1, 32) == 0);
    FUZZ_CHECK(memcmp(shared_ab + 32, shared_ser + 33, 32) == 0);
    for (i = 0; i < 32; i++) {
        FUZZ_CHECK(masked_ab[i] == (unsigned char)(shared_ser[1 + i] ^ hash_data.mask32[i]));
        FUZZ_CHECK(masked_ab[32 + i] == (unsigned char)(shared_ser[33 + i] ^ hash_data.mask32[31 - i]));
    }
    secp256k1_fuzz_check_ecdh_default_hash(ctx, &shared_pubkey_ab, default_ab);
    secp256k1_fuzz_check_ecdh_default_hash(ctx, &shared_pubkey_ab, explicit_ab);
    secp256k1_fuzz_check_ecdh_default_hash(ctx, &shared_pubkey_ab, default_fn_ab);
    secp256k1_fuzz_check_ecdh_default_hash(ctx, &shared_pubkey_ba, default_ba);

    hash_data.calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, fail_output, &pubkey_b, secp256k1_fuzz_scalar_zero, fuzz_ecdh_hash_with_data, &hash_data) == 0);
    FUZZ_CHECK(hash_data.calls == 1);
    hash_data.calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, fail_output, &pubkey_b, secp256k1_fuzz_scalar_order, fuzz_ecdh_hash_with_data, &hash_data) == 0);
    FUZZ_CHECK(hash_data.calls == 1);
    memset(default_fail_output, 0xA5, sizeof(default_fail_output));
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_fail_output, &pubkey_b, secp256k1_fuzz_scalar_zero, NULL, NULL) == 0);
    FUZZ_CHECK(memcmp(default_fail_output, zero32, sizeof(default_fail_output)) == 0);
    memset(default_fail_output, 0xA5, sizeof(default_fail_output));
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_fail_output, &pubkey_b, secp256k1_fuzz_scalar_order, NULL, NULL) == 0);
    FUZZ_CHECK(memcmp(default_fail_output, zero32, sizeof(default_fail_output)) == 0);
    memset(default_fail_output, 0xA5, sizeof(default_fail_output));
    FUZZ_CHECK(secp256k1_ecdh_hash_function_sha256(default_fail_output, NULL, seckey_a, NULL) == 0);
    FUZZ_CHECK(memcmp(default_fail_output, zero32, sizeof(default_fail_output)) == 0);
    memset(default_fail_output, 0xA5, sizeof(default_fail_output));
    FUZZ_CHECK(secp256k1_ecdh_hash_function_default(default_fail_output, seckey_a, NULL, NULL) == 0);
    FUZZ_CHECK(memcmp(default_fail_output, zero32, sizeof(default_fail_output)) == 0);
    FUZZ_CHECK(secp256k1_ecdh(ctx, fail_output, &pubkey_b, seckey_a, fuzz_ecdh_hash_fail, NULL) == 0);
    secp256k1_fuzz_check_ecdh_invalid_pubkey(ctx, seckey_a, hash_data.mask32);

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
