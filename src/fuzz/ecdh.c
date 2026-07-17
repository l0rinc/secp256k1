/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "pubkey_reference.h"
#include "../hash_impl.h"
#include "sha256_reference.h"

#ifdef ENABLE_MODULE_ECDH
static size_t secp256k1_fuzz_ecdh_sha256_compression_calls = 0;

static void secp256k1_fuzz_ecdh_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    secp256k1_fuzz_ecdh_sha256_compression_calls += n_blocks;
    secp256k1_sha256_transform(state, blocks64, n_blocks);
}

static void secp256k1_fuzz_check_ecdh_default_hash(const secp256k1_context *ctx, const secp256k1_pubkey *shared_pubkey, const unsigned char *output32) {
    unsigned char uncompressed[65];
    unsigned char compressed[33];
    unsigned char expected[32];
    size_t uncompressed_len = sizeof(uncompressed);

    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, uncompressed, &uncompressed_len, shared_pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(uncompressed_len == sizeof(uncompressed));
    compressed[0] = (unsigned char)(SECP256K1_TAG_PUBKEY_EVEN | (uncompressed[64] & 1u));
    memcpy(compressed + 1, uncompressed + 1, 32);
    secp256k1_fuzz_sha256_standalone(expected, compressed, sizeof(compressed));
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

/* The existing ECDH comparison derives its expected shared point through the
 * public tweak-multiply API. Bind one fixed scalar multiplication to the curve
 * equation instead, keeping the expected coordinates outside that path. */
static void secp256k1_fuzz_check_ecdh_generator_two(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "ecdh-generator-2g\n";
    static const unsigned char generator_two_x[32] = {
        0xC6, 0x04, 0x7F, 0x94, 0x41, 0xED, 0x7D, 0x6D,
        0x30, 0x45, 0x40, 0x6E, 0x95, 0xC0, 0x7C, 0xD8,
        0x5C, 0x77, 0x8E, 0x4B, 0x8C, 0xEF, 0x3C, 0xA7,
        0xAB, 0xAC, 0x09, 0xB9, 0x5C, 0x70, 0x9E, 0xE5
    };
    static const unsigned char scalar_one[32] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
    };
    static const unsigned char scalar_two[32] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2
    };
    secp256k1_pubkey generator;
    unsigned char actual64[64];
    unsigned char actual_hash[32];
    unsigned char expected_hash[32];
    unsigned char compressed[33];
    unsigned char x_squared[32];
    unsigned char x_cubed[32];
    unsigned char y_squared[32];
    unsigned char curve_rhs[32];
    unsigned char seven[32] = { 0 };

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    seven[31] = 7;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &generator, scalar_one) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, actual64, &generator, scalar_two, fuzz_ecdh_hash_passthrough, NULL) == 1);
    FUZZ_CHECK(memcmp(actual64, generator_two_x, sizeof(generator_two_x)) == 0);
    FUZZ_CHECK((actual64[63] & 1u) == 0);

    /* Check y^2 = x^3 + 7 with standalone byte-field arithmetic. The fixed
     * even parity selects the same root as the compressed 2G encoding. */
    secp256k1_fuzz_pubkey_mul_mod(x_squared, actual64, actual64);
    secp256k1_fuzz_pubkey_mul_mod(x_cubed, x_squared, actual64);
    secp256k1_fuzz_pubkey_add_mod(curve_rhs, x_cubed, seven);
    secp256k1_fuzz_pubkey_mul_mod(y_squared, actual64 + 32, actual64 + 32);
    FUZZ_CHECK(memcmp(y_squared, curve_rhs, sizeof(y_squared)) == 0);

    compressed[0] = SECP256K1_TAG_PUBKEY_EVEN;
    memcpy(compressed + 1, generator_two_x, sizeof(generator_two_x));
    secp256k1_fuzz_sha256_standalone(expected_hash, compressed, sizeof(compressed));
    FUZZ_CHECK(secp256k1_ecdh(ctx, actual_hash, &generator, scalar_two, NULL, NULL) == 1);
    FUZZ_CHECK(memcmp(actual_hash, expected_hash, sizeof(actual_hash)) == 0);
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
    unsigned char x32[32];
    unsigned char y32[32];
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
    memcpy(hash_data->x32, x32, 32);
    memcpy(hash_data->y32, y32, 32);
    for (i = 0; i < 32; i++) {
        output[i] = (unsigned char)(x32[i] ^ hash_data->mask32[i]);
        output[32 + i] = (unsigned char)(y32[i] ^ hash_data->mask32[31 - i]);
    }
    return 1;
}

static void secp256k1_fuzz_check_ecdh_callback_point(const secp256k1_context *ctx, const secp256k1_pubkey *point, const secp256k1_fuzz_ecdh_hash_data *hash_data) {
    unsigned char uncompressed[65];
    size_t uncompressed_len = sizeof(uncompressed);

    FUZZ_CHECK(hash_data->calls == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, uncompressed, &uncompressed_len, point, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(uncompressed_len == sizeof(uncompressed));
    FUZZ_CHECK(memcmp(hash_data->x32, uncompressed + 1, 32) == 0);
    FUZZ_CHECK(memcmp(hash_data->y32, uncompressed + 33, 32) == 0);
}

static void secp256k1_fuzz_check_ecdh_invalid_scalar_callback_point(const secp256k1_context *ctx, const secp256k1_pubkey *point, const unsigned char *mask32) {
    unsigned char order_plus_one[32];
    unsigned char custom_output[64];
    const unsigned char *invalid_scalars[3];
    secp256k1_fuzz_ecdh_hash_data hash_data;
    size_t i;

    memcpy(order_plus_one, secp256k1_fuzz_scalar_order, sizeof(order_plus_one));
    order_plus_one[31]++;
    invalid_scalars[0] = secp256k1_fuzz_scalar_zero;
    invalid_scalars[1] = secp256k1_fuzz_scalar_order;
    invalid_scalars[2] = order_plus_one;
    hash_data.self = &hash_data;
    memcpy(hash_data.mask32, mask32, sizeof(hash_data.mask32));
    for (i = 0; i < sizeof(invalid_scalars) / sizeof(invalid_scalars[0]); i++) {
        hash_data.calls = 0;
        memset(custom_output, 0xA5, sizeof(custom_output));
        FUZZ_CHECK(secp256k1_ecdh(ctx, custom_output, point, invalid_scalars[i], fuzz_ecdh_hash_with_data, &hash_data) == 0);
        secp256k1_fuzz_check_ecdh_callback_point(ctx, point, &hash_data);
    }
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

static void secp256k1_fuzz_check_ecdh_null_inputs(secp256k1_context *ctx, const secp256k1_pubkey *point, const unsigned char *scalar) {
    secp256k1_ecdh_hash_function known_hashes[2];
    int (*ecdh_fn)(const secp256k1_context *, unsigned char *, const secp256k1_pubkey *, const unsigned char *, secp256k1_ecdh_hash_function, void *) = secp256k1_ecdh;
    secp256k1_fuzz_ecdh_illegal_data illegal_data;
    unsigned char output32[32];
    unsigned char custom_output[64];
    unsigned char custom_expected[64];
    unsigned char zero32[32] = { 0 };
    size_t i;
    unsigned int calls;

    known_hashes[0] = NULL;
    known_hashes[1] = secp256k1_ecdh_hash_function_sha256;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_ecdh_illegal_callback, &illegal_data);

    /* Built-in callbacks own a fixed 32-byte output and must clear it before
     * the public argument check returns failure. */
    for (i = 0; i < sizeof(known_hashes) / sizeof(known_hashes[0]); i++) {
        memset(output32, 0xA5, sizeof(output32));
        calls = illegal_data.calls;
        FUZZ_CHECK(ecdh_fn(ctx, output32, NULL, scalar, known_hashes[i], NULL) == 0);
        FUZZ_CHECK(illegal_data.calls == calls + 1);
        FUZZ_CHECK(memcmp(output32, zero32, sizeof(output32)) == 0);

        memset(output32, 0x5A, sizeof(output32));
        calls = illegal_data.calls;
        FUZZ_CHECK(ecdh_fn(ctx, output32, point, NULL, known_hashes[i], NULL) == 0);
        FUZZ_CHECK(illegal_data.calls == calls + 1);
        FUZZ_CHECK(memcmp(output32, zero32, sizeof(output32)) == 0);
    }

    /* A custom callback has no fixed output size or failure-cleanup contract. */
    memset(custom_expected, 0xC3, sizeof(custom_expected));
    memset(custom_output, 0xC3, sizeof(custom_output));
    calls = illegal_data.calls;
    FUZZ_CHECK(ecdh_fn(ctx, custom_output, NULL, scalar, fuzz_ecdh_hash_passthrough, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(custom_output, custom_expected, sizeof(custom_output)) == 0);

    memset(custom_expected, 0x96, sizeof(custom_expected));
    memset(custom_output, 0x96, sizeof(custom_output));
    calls = illegal_data.calls;
    FUZZ_CHECK(ecdh_fn(ctx, custom_output, point, NULL, fuzz_ecdh_hash_passthrough, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(custom_output, custom_expected, sizeof(custom_output)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_ecdh_order_plus_one(const secp256k1_context *ctx, const secp256k1_pubkey *point, const unsigned char *mask32) {
    secp256k1_fuzz_ecdh_hash_data hash_data;
    unsigned char order_plus_one[32];
    unsigned char custom_output[64];
    unsigned char default_output[32];
    unsigned char zero32[32] = { 0 };

    /* Group order itself reduces to zero, so it also trips the zero-scalar
     * check. Use n+1, which reduces to one, to distinguish overflow tracking
     * from the zero check. */
    memcpy(order_plus_one, secp256k1_fuzz_scalar_order, sizeof(order_plus_one));
    order_plus_one[31]++;
    hash_data.self = &hash_data;
    memcpy(hash_data.mask32, mask32, sizeof(hash_data.mask32));
    hash_data.calls = 0;

    memset(custom_output, 0xA5, sizeof(custom_output));
    FUZZ_CHECK(secp256k1_ecdh(ctx, custom_output, point, order_plus_one, fuzz_ecdh_hash_with_data, &hash_data) == 0);
    FUZZ_CHECK(hash_data.calls == 1);

    memset(default_output, 0xA5, sizeof(default_output));
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_output, point, order_plus_one, NULL, NULL) == 0);
    FUZZ_CHECK(memcmp(default_output, zero32, sizeof(default_output)) == 0);

    memset(default_output, 0xA5, sizeof(default_output));
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_output, point, order_plus_one, secp256k1_ecdh_hash_function_default, NULL) == 0);
    FUZZ_CHECK(memcmp(default_output, zero32, sizeof(default_output)) == 0);
}

static void secp256k1_fuzz_check_ecdh_static_context(const secp256k1_context *ctx, const secp256k1_pubkey *point, const unsigned char *scalar) {
    unsigned char dynamic_custom[64];
    unsigned char static_custom[64];
    unsigned char static_default[32];
    unsigned char static_explicit[32];
    secp256k1_pubkey shared_pubkey;

    /* ECDH is the documented exception to the static-context restriction.
     * Keep a context-independent callback beside the built-in hash path so a
     * mistaken generator-context check cannot hide behind hash routing. */
    FUZZ_CHECK(secp256k1_ecdh(ctx, dynamic_custom, point, scalar, fuzz_ecdh_hash_passthrough, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(secp256k1_context_static, static_custom, point, scalar, fuzz_ecdh_hash_passthrough, NULL) == 1);
    FUZZ_CHECK(memcmp(static_custom, dynamic_custom, sizeof(static_custom)) == 0);

    FUZZ_CHECK(secp256k1_ecdh(secp256k1_context_static, static_default, point, scalar, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(secp256k1_context_static, static_explicit, point, scalar, secp256k1_ecdh_hash_function_default, NULL) == 1);
    FUZZ_CHECK(memcmp(static_default, static_explicit, sizeof(static_default)) == 0);

    shared_pubkey = *point;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &shared_pubkey, scalar) == 1);
    secp256k1_fuzz_check_ecdh_default_hash(secp256k1_context_static, &shared_pubkey, static_default);
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
    if (size == sizeof("static context ecdh barrier\n") - 1
        && memcmp(input, "static context ecdh barrier\n", sizeof("static context ecdh barrier\n") - 1) == 0) {
        secp256k1_fuzz_check_ecdh_static_context(ctx, &pubkey_b, seckey_a);
    }
    secp256k1_fuzz_check_ecdh_generator_two(ctx, input, size);
    secp256k1_fuzz_check_ecdh_odd_y_default_hash(ctx);
    secp256k1_fuzz_check_ecdh_order_plus_one(ctx, &pubkey_b, hash_data.mask32);
    secp256k1_fuzz_check_ecdh_invalid_scalar_callback_point(ctx, &pubkey_b, hash_data.mask32);
    secp256k1_fuzz_check_ecdh_null_inputs(ctx, &pubkey_b, seckey_a);

    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_ecdh_sha256_compression);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_ab, &pubkey_b, seckey_a, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_ba, &pubkey_a, seckey_b, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, explicit_ab, &pubkey_b, seckey_a, secp256k1_ecdh_hash_function_sha256, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, explicit_ba, &pubkey_a, seckey_b, secp256k1_ecdh_hash_function_sha256, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_fn_ab, &pubkey_b, seckey_a, secp256k1_ecdh_hash_function_default, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
    secp256k1_fuzz_ecdh_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_fn_ba, &pubkey_a, seckey_b, secp256k1_ecdh_hash_function_default, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdh_sha256_compression_calls != 0);
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
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_fail_output, &pubkey_b, secp256k1_fuzz_scalar_zero, secp256k1_ecdh_hash_function_sha256, NULL) == 0);
    FUZZ_CHECK(memcmp(default_fail_output, zero32, sizeof(default_fail_output)) == 0);
    memset(default_fail_output, 0xA5, sizeof(default_fail_output));
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_fail_output, &pubkey_b, secp256k1_fuzz_scalar_order, secp256k1_ecdh_hash_function_default, NULL) == 0);
    FUZZ_CHECK(memcmp(default_fail_output, zero32, sizeof(default_fail_output)) == 0);
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
