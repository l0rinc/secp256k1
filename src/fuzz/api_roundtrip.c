/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "pubkey_reference.h"
#include "../hash_impl.h"
#include "rfc6979_reference.h"
#include "../../contrib/lax_der_parsing.c"
#include "../../contrib/lax_der_privatekey_parsing.c"

static size_t secp256k1_fuzz_ecdsa_sha256_compression_calls = 0;

static const unsigned char secp256k1_fuzz_field_p_plus_one[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
};
static const unsigned char secp256k1_fuzz_pubkey_7g_uncompressed[65] = {
    0x04,
    0x5C, 0xBD, 0xF0, 0x64, 0x6E, 0x5D, 0xB4, 0xEA,
    0xA3, 0x98, 0xF3, 0x65, 0xF2, 0xEA, 0x7A, 0x0E,
    0x3D, 0x41, 0x9B, 0x7E, 0x03, 0x30, 0xE3, 0x9C,
    0xE9, 0x2B, 0xDD, 0xED, 0xCA, 0xC4, 0xF9, 0xBC,
    0x6A, 0xEB, 0xCA, 0x40, 0xBA, 0x25, 0x59, 0x60,
    0xA3, 0x17, 0x8D, 0x6D, 0x86, 0x1A, 0x54, 0xDB,
    0xA8, 0x13, 0xD0, 0xB8, 0x13, 0xFD, 0xE7, 0xB5,
    0xA5, 0x08, 0x26, 0x28, 0x08, 0x72, 0x64, 0xDA
};
static const unsigned char secp256k1_fuzz_x_one_even_y[32] = {
    0x42, 0x18, 0xF2, 0x0A, 0xE6, 0xC6, 0x46, 0xB3,
    0x63, 0xDB, 0x68, 0x60, 0x58, 0x22, 0xFB, 0x14,
    0x26, 0x4C, 0xA8, 0xD2, 0x58, 0x7F, 0xDD, 0x6F,
    0xBC, 0x75, 0x0D, 0x58, 0x7E, 0x76, 0xA7, 0xEE
};
/* x coordinate for a valid curve point with y = 1. */
static const unsigned char secp256k1_fuzz_x_for_y_one[32] = {
    0x14, 0x6D, 0x3B, 0x65, 0xAD, 0xD9, 0xF5, 0x4C,
    0xCC, 0xA2, 0x85, 0x33, 0xC8, 0x8E, 0x2C, 0xBC,
    0x63, 0xF7, 0x44, 0x3E, 0x16, 0x58, 0x78, 0x3A,
    0xB4, 0x1F, 0x8E, 0xF9, 0x7C, 0x2A, 0x10, 0xB5
};
/* Compressed point R with x(R) = group_order + 2 and R = 2Q. */
static const unsigned char secp256k1_fuzz_ecdsa_r_plus_order_point[33] = {
    0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
    0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x43
};
static const unsigned char secp256k1_fuzz_ecdsa_r_plus_order_pubkey[33] = {
    0x02, 0xAD, 0x80, 0x2F, 0x21, 0x45, 0xDF, 0x54, 0x8C,
    0xE1, 0xFC, 0x41, 0x8B, 0x85, 0x1C, 0xB3, 0x26,
    0x69, 0xC5, 0xA6, 0xBB, 0x93, 0x9E, 0x6C, 0xAF,
    0x74, 0x4D, 0xE9, 0x03, 0x10, 0xF8, 0xF1, 0xFF
};

static void secp256k1_fuzz_ecdsa_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    secp256k1_fuzz_ecdsa_sha256_compression_calls += n_blocks;
    secp256k1_sha256_transform(state, blocks64, n_blocks);
}

typedef struct {
    const void *self;
    const unsigned char *extra32;
    const unsigned char *expected_key32;
    const unsigned char *expected_msg32;
    unsigned int calls;
} secp256k1_fuzz_ecdsa_nonce_data;

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_api_illegal_data;

typedef struct {
    const void *self;
    const unsigned char *nonce32;
    unsigned int calls;
} secp256k1_fuzz_ecdsa_equation_nonce_data;

typedef int (*secp256k1_fuzz_ecdsa_serialize_der_fn)(const secp256k1_context *ctx, unsigned char *output, size_t *outputlen, const secp256k1_ecdsa_signature *sig);
typedef int (*secp256k1_fuzz_ecdsa_sign_fn)(const secp256k1_context *ctx, secp256k1_ecdsa_signature *signature, const unsigned char *msghash32, const unsigned char *seckey, secp256k1_nonce_function noncefp, const void *noncedata);
typedef int (*secp256k1_fuzz_ec_pubkey_parse_fn)(const secp256k1_context *ctx, secp256k1_pubkey *pubkey, const unsigned char *input, size_t inputlen);
typedef int (*secp256k1_fuzz_ecdsa_parse_der_fn)(const secp256k1_context *ctx, secp256k1_ecdsa_signature *sig, const unsigned char *input, size_t inputlen);
typedef int (*secp256k1_fuzz_ecdsa_parse_compact_fn)(const secp256k1_context *ctx, secp256k1_ecdsa_signature *sig, const unsigned char *input64);
typedef int (*secp256k1_fuzz_ecdsa_normalize_fn)(const secp256k1_context *ctx, secp256k1_ecdsa_signature *sigout, const secp256k1_ecdsa_signature *sigin);
typedef int (*secp256k1_fuzz_ec_seckey_tweak_fn)(const secp256k1_context *ctx, unsigned char *seckey, const unsigned char *tweak32);
typedef int (*secp256k1_fuzz_ec_pubkey_tweak_fn)(const secp256k1_context *ctx, secp256k1_pubkey *pubkey, const unsigned char *tweak32);
typedef int (*secp256k1_fuzz_ec_pubkey_combine_fn)(const secp256k1_context *ctx, secp256k1_pubkey *pubkey, const secp256k1_pubkey * const *pubkeys, size_t n_pubkeys);
typedef int (*secp256k1_fuzz_ec_pubkey_cmp_fn)(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey0, const secp256k1_pubkey *pubkey1);
typedef int (*secp256k1_fuzz_ec_pubkey_sort_fn)(const secp256k1_context *ctx, const secp256k1_pubkey **pubkeys, size_t n_pubkeys);
typedef int (*secp256k1_fuzz_ec_pubkey_serialize_fn)(const secp256k1_context *ctx, unsigned char *output, size_t *outputlen, const secp256k1_pubkey *pubkey, unsigned int flags);

static void secp256k1_fuzz_api_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_api_illegal_data *illegal_data = (secp256k1_fuzz_api_illegal_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(illegal_data != NULL);
    FUZZ_CHECK(illegal_data->self == illegal_data);
    illegal_data->calls++;
}

static int secp256k1_fuzz_ecdsa_nonce(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_ecdsa_nonce_data *nonce_data = (secp256k1_fuzz_ecdsa_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    if (nonce_data->expected_key32 != NULL) {
        FUZZ_CHECK(memcmp(key32, nonce_data->expected_key32, 32) == 0);
    }
    if (nonce_data->expected_msg32 != NULL) {
        FUZZ_CHECK(memcmp(msg32, nonce_data->expected_msg32, 32) == 0);
    }
    FUZZ_CHECK(algo16 == NULL);
    FUZZ_CHECK(attempt == nonce_data->calls);
    nonce_data->calls++;
    return secp256k1_nonce_function_rfc6979(nonce32, msg32, key32, algo16, (void *)nonce_data->extra32, attempt);
}

static int secp256k1_fuzz_ecdsa_nonce_fail(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_ecdsa_nonce_data *nonce_data = (secp256k1_fuzz_ecdsa_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(algo16 == NULL);
    FUZZ_CHECK(attempt == 0);
    nonce_data->calls++;
    memset(nonce32, 0xA5, 32);
    return 0;
}

static int secp256k1_fuzz_ecdsa_nonce_retry_fail(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_ecdsa_nonce_data *nonce_data = (secp256k1_fuzz_ecdsa_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(algo16 == NULL);
    FUZZ_CHECK(attempt == nonce_data->calls);
    nonce_data->calls++;
    if (attempt == 0) {
        memset(nonce32, 0, 32);
        return 1;
    }
    if (attempt == 1) {
        memcpy(nonce32, secp256k1_fuzz_scalar_order, 32);
        return 1;
    }
    FUZZ_CHECK(attempt == 2);
    memset(nonce32, 0xA5, 32);
    return 0;
}

static int secp256k1_fuzz_ecdsa_nonce_retry(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_ecdsa_nonce_data *nonce_data = (secp256k1_fuzz_ecdsa_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(algo16 == NULL);
    FUZZ_CHECK(attempt == nonce_data->calls);
    nonce_data->calls++;
    if (attempt == 0) {
        memset(nonce32, 0, 32);
        return 1;
    }
    if (attempt == 1) {
        memcpy(nonce32, secp256k1_fuzz_scalar_order, 32);
        return 1;
    }
    if (attempt == 2) {
        memset(nonce32, 0xFF, 32);
        return 1;
    }
    return secp256k1_nonce_function_rfc6979(nonce32, msg32, key32, algo16, (void *)nonce_data->extra32, attempt - 3);
}

static int secp256k1_fuzz_ecdsa_nonce_fixed_one(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    (void)data;
    FUZZ_CHECK(nonce32 != NULL);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(algo16 == NULL);
    FUZZ_CHECK(attempt == 0);
    memset(nonce32, 0, 32);
    nonce32[31] = 1;
    return 1;
}

static int secp256k1_fuzz_ecdsa_nonce_equation(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_ecdsa_equation_nonce_data *nonce_data = (secp256k1_fuzz_ecdsa_equation_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(nonce_data->nonce32 != NULL);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(algo16 == NULL);
    FUZZ_CHECK(attempt == 0);
    nonce_data->calls++;
    memcpy(nonce32, nonce_data->nonce32, 32);
    return 1;
}

static int secp256k1_fuzz_ecdsa_nonce_s_zero_then_two(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_ecdsa_nonce_data *nonce_data = (secp256k1_fuzz_ecdsa_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(algo16 == NULL);
    FUZZ_CHECK(attempt == nonce_data->calls);
    nonce_data->calls++;
    if (attempt == 0) {
        memcpy(nonce32, secp256k1_fuzz_scalar_one, 32);
        return 1;
    }
    if (attempt == 1) {
        memset(nonce32, 0, 32);
        nonce32[31] = 2;
        return 1;
    }
    return 0;
}

static const unsigned char *secp256k1_fuzz_runtime_null_tweak(const unsigned char *input, size_t size) {
    return size == (size_t)-1 ? input : NULL;
}

static const secp256k1_pubkey *secp256k1_fuzz_runtime_null_pubkey(const secp256k1_pubkey *valid_pubkey) {
    const secp256k1_pubkey * volatile null_pubkey = valid_pubkey;

    null_pubkey = NULL;
    return null_pubkey;
}

static void secp256k1_fuzz_check_tweak_add(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *seckey, const unsigned char *tweak32) {
    unsigned char tweaked_seckey[32];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_pubkey tweaked_pubkey;
    secp256k1_pubkey pubkey_from_tweaked_seckey;
    int seckey_ret;
    int pubkey_ret;

    memcpy(tweaked_seckey, seckey, sizeof(tweaked_seckey));
    tweaked_pubkey = *pubkey;
    seckey_ret = secp256k1_ec_seckey_tweak_add(ctx, tweaked_seckey, tweak32);
    pubkey_ret = secp256k1_ec_pubkey_tweak_add(ctx, &tweaked_pubkey, tweak32);
    FUZZ_CHECK(seckey_ret == pubkey_ret);
    if (seckey_ret) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_from_tweaked_seckey, tweaked_seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &tweaked_pubkey, &pubkey_from_tweaked_seckey) == 0);
        if (memcmp(tweak32, secp256k1_fuzz_scalar_zero, sizeof(secp256k1_fuzz_scalar_zero)) == 0) {
            FUZZ_CHECK(memcmp(tweaked_seckey, seckey, sizeof(tweaked_seckey)) == 0);
            FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &tweaked_pubkey, pubkey) == 0);
        }
        secp256k1_fuzz_check_pubkey_roundtrip(ctx, &tweaked_pubkey);
    } else {
        FUZZ_CHECK(memcmp(tweaked_seckey, secp256k1_fuzz_scalar_zero, sizeof(tweaked_seckey)) == 0);
        FUZZ_CHECK(memcmp(&tweaked_pubkey, zero_pubkey, sizeof(tweaked_pubkey)) == 0);
    }
}

static void secp256k1_fuzz_check_null_tweak_cleanup(secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *seckey, const unsigned char *input, size_t size) {
    const unsigned char *null_tweak = secp256k1_fuzz_runtime_null_tweak(input, size);
    secp256k1_fuzz_ec_seckey_tweak_fn seckey_tweak_add = secp256k1_ec_seckey_tweak_add;
    secp256k1_fuzz_ec_seckey_tweak_fn seckey_tweak_mul = secp256k1_ec_seckey_tweak_mul;
    secp256k1_fuzz_ec_pubkey_tweak_fn pubkey_tweak_add = secp256k1_ec_pubkey_tweak_add;
    secp256k1_fuzz_ec_pubkey_tweak_fn pubkey_tweak_mul = secp256k1_ec_pubkey_tweak_mul;
    secp256k1_fuzz_api_illegal_data illegal_data;
    unsigned char test_seckey[32];
    unsigned char zero32[32] = { 0 };
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_pubkey test_pubkey;
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    memcpy(test_seckey, seckey, sizeof(test_seckey));
    calls = illegal_data.calls;
    FUZZ_CHECK(seckey_tweak_add(ctx, test_seckey, null_tweak) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(test_seckey, zero32, sizeof(test_seckey)) == 0);

    memcpy(test_seckey, seckey, sizeof(test_seckey));
    calls = illegal_data.calls;
    FUZZ_CHECK(seckey_tweak_mul(ctx, test_seckey, null_tweak) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(test_seckey, zero32, sizeof(test_seckey)) == 0);

    test_pubkey = *pubkey;
    calls = illegal_data.calls;
    FUZZ_CHECK(pubkey_tweak_add(ctx, &test_pubkey, null_tweak) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&test_pubkey, zero_pubkey, sizeof(test_pubkey)) == 0);

    test_pubkey = *pubkey;
    calls = illegal_data.calls;
    FUZZ_CHECK(pubkey_tweak_mul(ctx, &test_pubkey, null_tweak) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&test_pubkey, zero_pubkey, sizeof(test_pubkey)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_tweak_mul(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *seckey, const unsigned char *tweak32) {
    unsigned char tweaked_seckey[32];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_pubkey tweaked_pubkey;
    secp256k1_pubkey pubkey_from_tweaked_seckey;
    int seckey_ret;
    int pubkey_ret;

    memcpy(tweaked_seckey, seckey, sizeof(tweaked_seckey));
    tweaked_pubkey = *pubkey;
    seckey_ret = secp256k1_ec_seckey_tweak_mul(ctx, tweaked_seckey, tweak32);
    pubkey_ret = secp256k1_ec_pubkey_tweak_mul(ctx, &tweaked_pubkey, tweak32);
    FUZZ_CHECK(seckey_ret == pubkey_ret);
    if (seckey_ret) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_from_tweaked_seckey, tweaked_seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &tweaked_pubkey, &pubkey_from_tweaked_seckey) == 0);
        if (memcmp(tweak32, secp256k1_fuzz_scalar_one, sizeof(secp256k1_fuzz_scalar_one)) == 0) {
            FUZZ_CHECK(memcmp(tweaked_seckey, seckey, sizeof(tweaked_seckey)) == 0);
            FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &tweaked_pubkey, pubkey) == 0);
        }
        secp256k1_fuzz_check_pubkey_roundtrip(ctx, &tweaked_pubkey);
    } else {
        FUZZ_CHECK(memcmp(tweaked_seckey, secp256k1_fuzz_scalar_zero, sizeof(tweaked_seckey)) == 0);
        FUZZ_CHECK(memcmp(&tweaked_pubkey, zero_pubkey, sizeof(tweaked_pubkey)) == 0);
    }
}

static void secp256k1_fuzz_check_pubkey_combine(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const secp256k1_pubkey *pubkey_neg, const unsigned char *seckey, const secp256k1_pubkey *other_pubkey, const unsigned char *other_seckey) {
    const secp256k1_pubkey *inputs[3];
    secp256k1_pubkey combined;
    secp256k1_pubkey combined_reversed;
    secp256k1_pubkey combined_from_seckey;
    secp256k1_pubkey doubled;
    unsigned char combined_seckey[32];
    unsigned char scalar_two[32] = { 0 };
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    int combine_ret;
    int seckey_ret;

    inputs[0] = pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined, inputs, 1) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, pubkey) == 0);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &combined);

    inputs[1] = pubkey_neg;
    memset(&combined, 0xA5, sizeof(combined));
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined, inputs, 2) == 0);
    FUZZ_CHECK(memcmp(&combined, zero_pubkey, sizeof(combined)) == 0);

    inputs[2] = pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined, inputs, 3) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, pubkey) == 0);

    inputs[1] = pubkey;
    scalar_two[31] = 2;
    doubled = *pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &doubled, scalar_two) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined, inputs, 2) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, &doubled) == 0);

    inputs[0] = pubkey;
    inputs[1] = other_pubkey;
    combine_ret = secp256k1_ec_pubkey_combine(ctx, &combined, inputs, 2);
    inputs[0] = other_pubkey;
    inputs[1] = pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined_reversed, inputs, 2) == combine_ret);
    memcpy(combined_seckey, seckey, sizeof(combined_seckey));
    seckey_ret = secp256k1_ec_seckey_tweak_add(ctx, combined_seckey, other_seckey);
    FUZZ_CHECK(seckey_ret == combine_ret);
    if (combine_ret) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, &combined_reversed) == 0);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &combined_from_seckey, combined_seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, &combined_from_seckey) == 0);
        secp256k1_fuzz_check_pubkey_roundtrip(ctx, &combined);
    }
}

static void secp256k1_fuzz_check_pubkey_combine_invalid(secp256k1_context *ctx, const secp256k1_pubkey *pubkey) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    const secp256k1_pubkey *inputs[2];
    secp256k1_pubkey invalid_pubkey;
    secp256k1_pubkey combined;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    unsigned int calls;

    memset(&invalid_pubkey, 0, sizeof(invalid_pubkey));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    inputs[0] = &invalid_pubkey;
    memset(&combined, 0xA5, sizeof(combined));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined, inputs, 1) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&combined, zero_pubkey, sizeof(combined)) == 0);

    inputs[0] = pubkey;
    inputs[1] = &invalid_pubkey;
    memset(&combined, 0x5A, sizeof(combined));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined, inputs, 2) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&combined, zero_pubkey, sizeof(combined)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_pubkey_combine_null_member(secp256k1_context *ctx, const secp256k1_pubkey *pubkey) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    secp256k1_fuzz_ec_pubkey_combine_fn combine = secp256k1_ec_pubkey_combine;
    const secp256k1_pubkey *inputs[2];
    const secp256k1_pubkey *null_pubkey = secp256k1_fuzz_runtime_null_pubkey(pubkey);
    secp256k1_pubkey combined;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    /* Rejecting after a valid member must not leave a partial aggregate. */
    inputs[0] = pubkey;
    inputs[1] = null_pubkey;
    memset(&combined, 0xA5, sizeof(combined));
    calls = illegal_data.calls;
    FUZZ_CHECK(combine(ctx, &combined, inputs, 2) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&combined, zero_pubkey, sizeof(combined)) == 0);

    /* The first-member NULL case must obey the same output contract. */
    inputs[0] = null_pubkey;
    inputs[1] = pubkey;
    memset(&combined, 0x5A, sizeof(combined));
    calls = illegal_data.calls;
    FUZZ_CHECK(combine(ctx, &combined, inputs, 2) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&combined, zero_pubkey, sizeof(combined)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_pubkey_combine_empty(secp256k1_context *ctx, const secp256k1_pubkey *pubkey) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    secp256k1_fuzz_ec_pubkey_combine_fn combine = secp256k1_ec_pubkey_combine;
    const secp256k1_pubkey *inputs[1];
    secp256k1_pubkey combined;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    unsigned int calls;

    inputs[0] = pubkey;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    /* Empty aggregation is rejected before the input array is inspected. */
    memset(&combined, 0xA5, sizeof(combined));
    calls = illegal_data.calls;
    FUZZ_CHECK(combine(ctx, &combined, inputs, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&combined, zero_pubkey, sizeof(combined)) == 0);

    /* Keep the zero-sized case independent of the array pointer as well. */
    memset(&combined, 0x5A, sizeof(combined));
    calls = illegal_data.calls;
    FUZZ_CHECK(combine(ctx, &combined, NULL, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&combined, zero_pubkey, sizeof(combined)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_pubkey_serialize_short_buffer(secp256k1_context *ctx, const secp256k1_pubkey *pubkey) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    unsigned char output[65];
    unsigned char zero_output[65] = { 0 };
    size_t output_len;
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    memset(output, 0xA5, sizeof(output));
    output_len = 32;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, output, &output_len, pubkey, SECP256K1_EC_COMPRESSED) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(output_len == 0);
    FUZZ_CHECK(memcmp(output, zero_output, 32) == 0);
    FUZZ_CHECK(output[32] == 0xA5);

    memset(output, 0x5A, sizeof(output));
    output_len = 64;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, output, &output_len, pubkey, SECP256K1_EC_UNCOMPRESSED) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(output_len == 0);
    FUZZ_CHECK(memcmp(output, zero_output, 64) == 0);
    FUZZ_CHECK(output[64] == 0x5A);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_pubkey_serialize_null_output(secp256k1_context *ctx, const secp256k1_pubkey *pubkey) {
    secp256k1_fuzz_ec_pubkey_serialize_fn serialize = secp256k1_ec_pubkey_serialize;
    secp256k1_fuzz_api_illegal_data illegal_data;
    size_t output_len;
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    output_len = 33;
    calls = illegal_data.calls;
    FUZZ_CHECK(serialize(ctx, NULL, &output_len, pubkey, SECP256K1_EC_COMPRESSED) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(output_len == 0);

    output_len = 65;
    calls = illegal_data.calls;
    FUZZ_CHECK(serialize(ctx, NULL, &output_len, pubkey, SECP256K1_EC_UNCOMPRESSED) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(output_len == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_pubkey_serialize_flags(secp256k1_context *ctx, const secp256k1_pubkey *pubkey) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    unsigned char output[65];
    unsigned char zero_output[65] = { 0 };
    size_t output_len;
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    /* The type bits belong to the serialization API, not context creation. */
    memset(output, 0xA5, sizeof(output));
    output_len = sizeof(output);
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, output, &output_len, pubkey, SECP256K1_FLAGS_TYPE_CONTEXT) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(output_len == 0);
    FUZZ_CHECK(memcmp(output, zero_output, sizeof(output)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_null_parser_inputs(secp256k1_context *ctx) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    secp256k1_fuzz_ec_pubkey_parse_fn parse_pubkey = secp256k1_ec_pubkey_parse;
    secp256k1_fuzz_ecdsa_parse_der_fn parse_der = secp256k1_ecdsa_signature_parse_der;
    secp256k1_fuzz_ecdsa_parse_compact_fn parse_compact = secp256k1_ecdsa_signature_parse_compact;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_signature sig;
    unsigned char zero_pubkey[sizeof(pubkey)] = { 0 };
    unsigned char zero_sig[sizeof(sig)] = { 0 };
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    /* The parser output must not retain a previous object when its input is NULL. */
    memset(&pubkey, 0xA5, sizeof(pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(parse_pubkey(ctx, &pubkey, NULL, 65) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&pubkey, zero_pubkey, sizeof(pubkey)) == 0);

    memset(&sig, 0x5A, sizeof(sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(parse_der(ctx, &sig, NULL, 72) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);

    memset(&sig, 0x3C, sizeof(sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(parse_compact(ctx, &sig, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_ecdsa_variable_output_cleanup(secp256k1_context *ctx, const secp256k1_ecdsa_signature *sig) {
    secp256k1_fuzz_ecdsa_serialize_der_fn serialize_der = secp256k1_ecdsa_signature_serialize_der;
    secp256k1_fuzz_ecdsa_normalize_fn normalize = secp256k1_ecdsa_signature_normalize;
    secp256k1_fuzz_api_illegal_data illegal_data;
    secp256k1_ecdsa_signature normalized_sig;
    unsigned char output[72];
    unsigned char zero_output[72] = { 0 };
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };
    size_t output_len;
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    memset(output, 0xA5, sizeof(output));
    output_len = sizeof(output);
    calls = illegal_data.calls;
    FUZZ_CHECK(serialize_der(ctx, output, &output_len, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(output_len == 0);
    FUZZ_CHECK(memcmp(output, zero_output, sizeof(output)) == 0);

    memset(output, 0x5A, sizeof(output));
    output_len = 10;
    calls = illegal_data.calls;
    FUZZ_CHECK(serialize_der(ctx, output, &output_len, sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls);
    FUZZ_CHECK(output_len > 10);
    FUZZ_CHECK(memcmp(output, zero_output, 10) == 0);
    FUZZ_CHECK(output[10] == 0x5A);

    memset(&normalized_sig, 0x3C, sizeof(normalized_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(normalize(ctx, &normalized_sig, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&normalized_sig, zero_sig, sizeof(normalized_sig)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_ecdsa_signature_state_barrier(secp256k1_context *ctx, const secp256k1_ecdsa_signature *valid_sig, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    secp256k1_ecdsa_signature invalid_sig;
    secp256k1_ecdsa_signature normalized_sig;
    unsigned char der[74];
    unsigned char compact[64];
    unsigned char zero_der[sizeof(der)] = { 0 };
    unsigned char zero_compact[sizeof(compact)] = { 0 };
    unsigned char zero_sig[sizeof(normalized_sig)] = { 0 };
    unsigned int calls;
    size_t der_len;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    invalid_sig = *valid_sig;
    memset(invalid_sig.data, 0xFF, 32);
    memset(der, 0xA5, sizeof(der));
    der_len = sizeof(der);
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, der, &der_len, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(der_len == 0);
    FUZZ_CHECK(memcmp(der, zero_der, sizeof(der)) == 0);

    memset(compact, 0x5A, sizeof(compact));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(compact, zero_compact, sizeof(compact)) == 0);

    memset(&normalized_sig, 0x3C, sizeof(normalized_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&normalized_sig, zero_sig, sizeof(normalized_sig)) == 0);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &invalid_sig, msg32, pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    invalid_sig = *valid_sig;
    memset(invalid_sig.data + 32, 0xFF, 32);
    memset(der, 0x96, sizeof(der));
    der_len = sizeof(der);
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, der, &der_len, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(der_len == 0);
    FUZZ_CHECK(memcmp(der, zero_der, sizeof(der)) == 0);

    memset(compact, 0xC3, sizeof(compact));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(compact, zero_compact, sizeof(compact)) == 0);

    memset(&normalized_sig, 0xA5, sizeof(normalized_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&normalized_sig, zero_sig, sizeof(normalized_sig)) == 0);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &invalid_sig, msg32, pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    /* Normalization supports in-place operation. Keep that alias contract
     * true on failure as well: a rejected signature must not remain live in
     * the object the caller passed for both input and output. */
    invalid_sig = *valid_sig;
    memset(invalid_sig.data, 0xFF, 32);
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &invalid_sig, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&invalid_sig, zero_sig, sizeof(invalid_sig)) == 0);

    invalid_sig = *valid_sig;
    memset(invalid_sig.data + 32, 0xFF, 32);
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &invalid_sig, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&invalid_sig, zero_sig, sizeof(invalid_sig)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_ecdsa_invalid_pubkey(secp256k1_context *ctx, const secp256k1_ecdsa_signature *valid_sig, const unsigned char *msg32) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    secp256k1_pubkey invalid_pubkey;
    unsigned int calls;

    /* Verification must not treat an opaque zero pubkey as a group point. */
    memset(&invalid_pubkey, 0, sizeof(invalid_pubkey));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, valid_sig, msg32, &invalid_pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_pubkey_cmp_order(const secp256k1_context *ctx, const secp256k1_pubkey *a, const secp256k1_pubkey *b) {
    unsigned char serialized_a[33];
    unsigned char serialized_b[33];
    size_t serialized_a_len = sizeof(serialized_a);
    size_t serialized_b_len = sizeof(serialized_b);
    int cmp_ret;
    int reverse_cmp_ret;
    int serialized_cmp;

    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized_a, &serialized_a_len, a, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(serialized_a_len == sizeof(serialized_a));
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized_b, &serialized_b_len, b, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(serialized_b_len == sizeof(serialized_b));
    cmp_ret = secp256k1_ec_pubkey_cmp(ctx, a, b);
    serialized_cmp = memcmp(serialized_a, serialized_b, sizeof(serialized_a));
    FUZZ_CHECK((cmp_ret == 0) == (serialized_cmp == 0));
    FUZZ_CHECK((cmp_ret < 0) == (serialized_cmp < 0));
    FUZZ_CHECK((cmp_ret > 0) == (serialized_cmp > 0));
    reverse_cmp_ret = secp256k1_ec_pubkey_cmp(ctx, b, a);
    FUZZ_CHECK((reverse_cmp_ret == 0) == (cmp_ret == 0));
    FUZZ_CHECK((reverse_cmp_ret < 0) == (cmp_ret > 0));
    FUZZ_CHECK((reverse_cmp_ret > 0) == (cmp_ret < 0));
}

static void secp256k1_fuzz_check_null_pubkey_cmp(secp256k1_context *ctx, const secp256k1_pubkey *valid_pubkey) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    const secp256k1_pubkey *null_pubkey = secp256k1_fuzz_runtime_null_pubkey(valid_pubkey);
    secp256k1_fuzz_ec_pubkey_cmp_fn cmp = secp256k1_ec_pubkey_cmp;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    /* The comparator's all-zero fallback must make NULL sort below valid keys. */
    FUZZ_CHECK(cmp(ctx, null_pubkey, valid_pubkey) < 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(cmp(ctx, valid_pubkey, null_pubkey) > 0);
    FUZZ_CHECK(illegal_data.calls == 2);
    FUZZ_CHECK(cmp(ctx, null_pubkey, null_pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == 4);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_pubkey_sort(const secp256k1_context *ctx, const secp256k1_pubkey * const*input_pubkeys) {
    const secp256k1_pubkey *sorted_pubkeys[4];
    const secp256k1_pubkey *resorted_pubkeys[4];
    int matched[4] = { 0 };
    size_t i;
    size_t j;

    for (i = 0; i < 4; i++) {
        sorted_pubkeys[i] = input_pubkeys[i];
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            secp256k1_fuzz_check_pubkey_cmp_order(ctx, input_pubkeys[i], input_pubkeys[j]);
        }
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, sorted_pubkeys, 4) == 1);
    for (i = 1; i < 4; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, sorted_pubkeys[i - 1], sorted_pubkeys[i]) <= 0);
    }
    for (i = 0; i < 4; i++) {
        int found = 0;
        for (j = 0; j < 4; j++) {
            if (!matched[j] && secp256k1_ec_pubkey_cmp(ctx, sorted_pubkeys[i], input_pubkeys[j]) == 0) {
                matched[j] = 1;
                found = 1;
                break;
            }
        }
        FUZZ_CHECK(found);
    }
    for (i = 0; i < 4; i++) {
        FUZZ_CHECK(matched[i]);
        resorted_pubkeys[i] = sorted_pubkeys[i];
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, resorted_pubkeys, 4) == 1);
    for (i = 0; i < 4; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, sorted_pubkeys[i], resorted_pubkeys[i]) == 0);
    }
}

static int secp256k1_fuzz_pubkey_serialized_cmp(const secp256k1_context *ctx, const secp256k1_pubkey *a, const secp256k1_pubkey *b) {
    unsigned char serialized_a[33];
    unsigned char serialized_b[33];
    size_t serialized_a_len = sizeof(serialized_a);
    size_t serialized_b_len = sizeof(serialized_b);

    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized_a, &serialized_a_len, a, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized_b, &serialized_b_len, b, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(serialized_a_len == sizeof(serialized_a));
    FUZZ_CHECK(serialized_b_len == sizeof(serialized_b));
    return memcmp(serialized_a, serialized_b, sizeof(serialized_a));
}

/* Check the first sort length beyond the ordinary four-key fixture with an
 * ordering reference that does not call the production comparator. */
static void secp256k1_fuzz_check_pubkey_sort_long(const secp256k1_context *ctx, size_t size) {
    static const unsigned char input_order[8] = { 8, 7, 6, 5, 4, 3, 2, 1 };
    const size_t n_pubkeys = 8;
    secp256k1_pubkey pubkeys[8];
    const secp256k1_pubkey *input_pubkeys[8];
    const secp256k1_pubkey *expected_pubkeys[8];
    const secp256k1_pubkey *resorted_pubkeys[8];
    unsigned char seckey[32] = { 0 };
    const secp256k1_pubkey *tmp;
    size_t i;
    size_t j;
    int last_is_max;

    if (size < 128) {
        return;
    }

    for (i = 0; i < n_pubkeys; i++) {
        seckey[31] = input_order[i];
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckey) == 1);
        input_pubkeys[i] = &pubkeys[i];
    }

    /* Make the omitted tail observably out of order even if the fixed
     * serialization ordering changes in a future implementation. */
    last_is_max = 1;
    for (i = 0; i + 1 < n_pubkeys; i++) {
        if (secp256k1_fuzz_pubkey_serialized_cmp(ctx, input_pubkeys[7], input_pubkeys[i]) < 0) {
            last_is_max = 0;
            break;
        }
    }
    if (last_is_max) {
        tmp = input_pubkeys[6];
        input_pubkeys[6] = input_pubkeys[7];
        input_pubkeys[7] = tmp;
    }

    for (i = 0; i < n_pubkeys; i++) {
        expected_pubkeys[i] = input_pubkeys[i];
    }
    for (i = 1; i < n_pubkeys; i++) {
        tmp = expected_pubkeys[i];
        j = i;
        while (j > 0 && secp256k1_fuzz_pubkey_serialized_cmp(ctx, expected_pubkeys[j - 1], tmp) > 0) {
            expected_pubkeys[j] = expected_pubkeys[j - 1];
            j--;
        }
        expected_pubkeys[j] = tmp;
    }

    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, input_pubkeys, n_pubkeys) == 1);
    for (i = 0; i < n_pubkeys; i++) {
        FUZZ_CHECK(input_pubkeys[i] == expected_pubkeys[i]);
        if (i > 0) {
            FUZZ_CHECK(secp256k1_fuzz_pubkey_serialized_cmp(ctx, input_pubkeys[i - 1], input_pubkeys[i]) <= 0);
        }
        resorted_pubkeys[i] = input_pubkeys[i];
    }

    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, resorted_pubkeys, n_pubkeys) == 1);
    for (i = 0; i < n_pubkeys; i++) {
        FUZZ_CHECK(resorted_pubkeys[i] == expected_pubkeys[i]);
    }
}

/* Exercise a long sort with equal serialized keys. Equal keys may be
 * reordered by the implementation, so the oracle checks byte order and the
 * complete pointer multiset separately. */
static void secp256k1_fuzz_check_pubkey_sort_sixteen(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    enum { N_PUBKEYS = 16, SERIALIZED_SIZE = 33 };
    static const unsigned char trigger[] = "sixteen-key-pubkey-sort\n";
    static const unsigned char scalar_values[N_PUBKEYS] = {
        8, 1, 8, 2, 7, 3, 7, 4,
        6, 5, 6, 5, 4, 3, 2, 1
    };
    secp256k1_pubkey pubkeys[N_PUBKEYS];
    const secp256k1_pubkey *input_pubkeys[N_PUBKEYS];
    const secp256k1_pubkey *original_pubkeys[N_PUBKEYS];
    const secp256k1_pubkey *resorted_pubkeys[N_PUBKEYS];
    unsigned char expected_serialized[N_PUBKEYS][SERIALIZED_SIZE];
    unsigned char actual_serialized[SERIALIZED_SIZE];
    unsigned char serialized_key[SERIALIZED_SIZE];
    int matched[N_PUBKEYS] = { 0 };
    size_t i;
    size_t j;
    size_t serialized_len;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    for (i = 0; i < N_PUBKEYS; i++) {
        unsigned char seckey[32] = { 0 };

        seckey[31] = scalar_values[i];
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckey) == 1);
        input_pubkeys[i] = &pubkeys[i];
        original_pubkeys[i] = input_pubkeys[i];
        serialized_len = sizeof(expected_serialized[i]);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, expected_serialized[i], &serialized_len, input_pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == sizeof(expected_serialized[i]));
    }

    /* Insertion sort is an independent byte-level ordering reference. */
    for (i = 1; i < N_PUBKEYS; i++) {
        memcpy(serialized_key, expected_serialized[i], sizeof(serialized_key));
        j = i;
        while (j > 0 && memcmp(expected_serialized[j - 1], serialized_key, sizeof(serialized_key)) > 0) {
            memcpy(expected_serialized[j], expected_serialized[j - 1], sizeof(expected_serialized[j]));
            j--;
        }
        memcpy(expected_serialized[j], serialized_key, sizeof(serialized_key));
    }

    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, input_pubkeys, N_PUBKEYS) == 1);
    for (i = 0; i < N_PUBKEYS; i++) {
        serialized_len = sizeof(actual_serialized);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, actual_serialized, &serialized_len, input_pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == sizeof(actual_serialized));
        FUZZ_CHECK(memcmp(actual_serialized, expected_serialized[i], sizeof(actual_serialized)) == 0);
        for (j = 0; j < N_PUBKEYS; j++) {
            if (!matched[j] && input_pubkeys[i] == original_pubkeys[j]) {
                matched[j] = 1;
                break;
            }
        }
        FUZZ_CHECK(j < N_PUBKEYS);
    }
    for (i = 0; i < N_PUBKEYS; i++) {
        FUZZ_CHECK(matched[i]);
        resorted_pubkeys[i] = input_pubkeys[i];
    }

    /* Equal serialized values may choose another stable order, but sorting a
     * sorted array must preserve both the byte sequence and pointer multiset. */
    memset(matched, 0, sizeof(matched));
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, resorted_pubkeys, N_PUBKEYS) == 1);
    for (i = 0; i < N_PUBKEYS; i++) {
        serialized_len = sizeof(actual_serialized);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, actual_serialized, &serialized_len, resorted_pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == sizeof(actual_serialized));
        FUZZ_CHECK(memcmp(actual_serialized, expected_serialized[i], sizeof(actual_serialized)) == 0);
        for (j = 0; j < N_PUBKEYS; j++) {
            if (!matched[j] && resorted_pubkeys[i] == original_pubkeys[j]) {
                matched[j] = 1;
                break;
            }
        }
        FUZZ_CHECK(j < N_PUBKEYS);
    }
    for (i = 0; i < N_PUBKEYS; i++) {
        FUZZ_CHECK(matched[i]);
    }
}

static void secp256k1_fuzz_check_empty_pubkey_sort(const secp256k1_context *ctx) {
    const secp256k1_pubkey *empty[1] = { NULL };

    /* The array pointer is required, but zero elements are a valid no-op. */
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, empty, 0) == 1);
}

static void secp256k1_fuzz_check_null_pubkey_sort(secp256k1_context *ctx) {
    secp256k1_fuzz_ec_pubkey_sort_fn sort = secp256k1_ec_pubkey_sort;
    secp256k1_fuzz_api_illegal_data illegal_data;
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);
    calls = illegal_data.calls;
    FUZZ_CHECK(sort(ctx, NULL, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    calls = illegal_data.calls;
    FUZZ_CHECK(sort(ctx, NULL, 1) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_invalid_pubkey_sort(secp256k1_context *ctx, const secp256k1_pubkey *valid_pubkey) {
    secp256k1_fuzz_api_illegal_data illegal_data;
    secp256k1_pubkey invalid_pubkeys[2];
    const secp256k1_pubkey *sorted_pubkeys[3];
    unsigned int calls;
    size_t i;

    memset(invalid_pubkeys, 0, sizeof(invalid_pubkeys));
    sorted_pubkeys[0] = valid_pubkey;
    sorted_pubkeys[1] = &invalid_pubkeys[0];
    sorted_pubkeys[2] = &invalid_pubkeys[1];
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &invalid_pubkeys[0], valid_pubkey) < 0);
    FUZZ_CHECK(illegal_data.calls > calls);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, valid_pubkey, &invalid_pubkeys[0]) > 0);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &invalid_pubkeys[0], &invalid_pubkeys[1]) == 0);

    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, sorted_pubkeys, 3) == 1);
    FUZZ_CHECK(sorted_pubkeys[2] == valid_pubkey);
    for (i = 1; i < 3; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, sorted_pubkeys[i - 1], sorted_pubkeys[i]) <= 0);
    }
    FUZZ_CHECK((sorted_pubkeys[0] == &invalid_pubkeys[0] || sorted_pubkeys[0] == &invalid_pubkeys[1]));
    FUZZ_CHECK((sorted_pubkeys[1] == &invalid_pubkeys[0] || sorted_pubkeys[1] == &invalid_pubkeys[1]));
    FUZZ_CHECK(sorted_pubkeys[0] != sorted_pubkeys[1]);
    FUZZ_CHECK(illegal_data.calls > 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static int secp256k1_fuzz_pubkey_parse_reference(const unsigned char *input, size_t inputlen);

static void secp256k1_fuzz_check_pubkey_parse(const secp256k1_context *ctx, const unsigned char *input, size_t inputlen) {
    secp256k1_pubkey parsed_pubkey;
    unsigned char serialized[65];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    size_t serialized_len;
    int parse_ret;

    parse_ret = secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, input, inputlen);
    FUZZ_CHECK(parse_ret == secp256k1_fuzz_pubkey_parse_reference(input, inputlen));
    if (parse_ret) {
        serialized_len = sizeof(serialized);
        if (inputlen == 33) {
            FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized, &serialized_len, &parsed_pubkey, SECP256K1_EC_COMPRESSED) == 1);
            FUZZ_CHECK(serialized_len == inputlen);
            FUZZ_CHECK(memcmp(serialized, input, inputlen) == 0);
        } else {
            FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized, &serialized_len, &parsed_pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
            FUZZ_CHECK(serialized_len == inputlen);
            FUZZ_CHECK(memcmp(serialized + 1, input + 1, inputlen - 1) == 0);
        }
        secp256k1_fuzz_check_pubkey_roundtrip(ctx, &parsed_pubkey);
    } else {
        FUZZ_CHECK(memcmp(&parsed_pubkey, zero_pubkey, sizeof(parsed_pubkey)) == 0);
    }
}

static int secp256k1_fuzz_pubkey_parse_reference(const unsigned char *input, size_t inputlen) {
    unsigned char x_squared32[32];
    unsigned char rhs32[32];
    unsigned char y_squared32[32];
    unsigned char root32[32];
    unsigned char root_squared32[32];
    unsigned char seven32[32] = { 0 };
    int hybrid;

    if (inputlen == 33 && (input[0] == SECP256K1_TAG_PUBKEY_EVEN || input[0] == SECP256K1_TAG_PUBKEY_ODD)) {
        if (memcmp(input + 1, secp256k1_fuzz_pubkey_field_prime, 32) >= 0) {
            return 0;
        }
        secp256k1_fuzz_pubkey_mul_mod(x_squared32, input + 1, input + 1);
        secp256k1_fuzz_pubkey_mul_mod(rhs32, x_squared32, input + 1);
        seven32[31] = 7;
        secp256k1_fuzz_pubkey_add_mod(rhs32, rhs32, seven32);
        secp256k1_fuzz_pubkey_sqrt_mod(root32, rhs32);
        secp256k1_fuzz_pubkey_mul_mod(root_squared32, root32, root32);
        return memcmp(root_squared32, rhs32, sizeof(root_squared32)) == 0;
    }
    if (inputlen != 65 ||
        (input[0] != SECP256K1_TAG_PUBKEY_UNCOMPRESSED &&
         input[0] != SECP256K1_TAG_PUBKEY_HYBRID_EVEN &&
         input[0] != SECP256K1_TAG_PUBKEY_HYBRID_ODD)) {
        return 0;
    }
    if (memcmp(input + 1, secp256k1_fuzz_pubkey_field_prime, 32) >= 0 ||
        memcmp(input + 33, secp256k1_fuzz_pubkey_field_prime, 32) >= 0) {
        return 0;
    }
    secp256k1_fuzz_pubkey_mul_mod(x_squared32, input + 1, input + 1);
    secp256k1_fuzz_pubkey_mul_mod(rhs32, x_squared32, input + 1);
    seven32[31] = 7;
    secp256k1_fuzz_pubkey_add_mod(rhs32, rhs32, seven32);
    secp256k1_fuzz_pubkey_mul_mod(y_squared32, input + 33, input + 33);
    if (memcmp(rhs32, y_squared32, sizeof(rhs32)) != 0) {
        return 0;
    }
    hybrid = input[0] == SECP256K1_TAG_PUBKEY_HYBRID_EVEN || input[0] == SECP256K1_TAG_PUBKEY_HYBRID_ODD;
    return !hybrid || ((input[33 + 31] & 1) == (input[0] == SECP256K1_TAG_PUBKEY_HYBRID_ODD));
}

static void secp256k1_fuzz_check_pubkey_parse_field_overflow(const secp256k1_context *ctx) {
    unsigned char compressed[33];
    unsigned char uncompressed[65];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_pubkey parsed_pubkey;

    memcpy(compressed + 1, secp256k1_fuzz_field_p_plus_one, 32);

    compressed[0] = SECP256K1_TAG_PUBKEY_EVEN;
    memset(&parsed_pubkey, 0xA5, sizeof(parsed_pubkey));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, compressed, sizeof(compressed)) == 0);
    FUZZ_CHECK(memcmp(&parsed_pubkey, zero_pubkey, sizeof(parsed_pubkey)) == 0);

    compressed[0] = SECP256K1_TAG_PUBKEY_ODD;
    memset(&parsed_pubkey, 0xA5, sizeof(parsed_pubkey));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, compressed, sizeof(compressed)) == 0);
    FUZZ_CHECK(memcmp(&parsed_pubkey, zero_pubkey, sizeof(parsed_pubkey)) == 0);

    uncompressed[0] = SECP256K1_TAG_PUBKEY_UNCOMPRESSED;
    memcpy(uncompressed + 1, secp256k1_fuzz_field_p_plus_one, 32);
    memcpy(uncompressed + 33, secp256k1_fuzz_x_one_even_y, 32);
    memset(&parsed_pubkey, 0xA5, sizeof(parsed_pubkey));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, uncompressed, sizeof(uncompressed)) == 0);
    FUZZ_CHECK(memcmp(&parsed_pubkey, zero_pubkey, sizeof(parsed_pubkey)) == 0);

    uncompressed[0] = SECP256K1_TAG_PUBKEY_HYBRID_EVEN;
    memset(&parsed_pubkey, 0xA5, sizeof(parsed_pubkey));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, uncompressed, sizeof(uncompressed)) == 0);
    FUZZ_CHECK(memcmp(&parsed_pubkey, zero_pubkey, sizeof(parsed_pubkey)) == 0);

    uncompressed[0] = SECP256K1_TAG_PUBKEY_UNCOMPRESSED;
    memcpy(uncompressed + 1, secp256k1_fuzz_x_for_y_one, 32);
    memcpy(uncompressed + 33, secp256k1_fuzz_field_p_plus_one, 32);
    memset(&parsed_pubkey, 0xA5, sizeof(parsed_pubkey));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, uncompressed, sizeof(uncompressed)) == 0);
    FUZZ_CHECK(memcmp(&parsed_pubkey, zero_pubkey, sizeof(parsed_pubkey)) == 0);

    uncompressed[0] = SECP256K1_TAG_PUBKEY_HYBRID_ODD;
    memset(&parsed_pubkey, 0xA5, sizeof(parsed_pubkey));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, uncompressed, sizeof(uncompressed)) == 0);
    FUZZ_CHECK(memcmp(&parsed_pubkey, zero_pubkey, sizeof(parsed_pubkey)) == 0);
}

static void secp256k1_fuzz_check_pubkey_create_failure(const secp256k1_context *ctx, const unsigned char *seckey32) {
    secp256k1_pubkey pubkey;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };

    memset(&pubkey, 0xA5, sizeof(pubkey));
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey32) == 0);
    FUZZ_CHECK(memcmp(&pubkey, zero_pubkey, sizeof(pubkey)) == 0);
}

static void secp256k1_fuzz_check_seckey_negate_failure(const secp256k1_context *ctx) {
    unsigned char zero32[32] = { 0 };
    unsigned char overflow32[32];

    memset(overflow32, 0xFF, sizeof(overflow32));
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, zero32) == 0);
    FUZZ_CHECK(memcmp(zero32, secp256k1_fuzz_scalar_zero, sizeof(zero32)) == 0);
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, overflow32) == 0);
    FUZZ_CHECK(memcmp(overflow32, secp256k1_fuzz_scalar_zero, sizeof(overflow32)) == 0);
}

static void secp256k1_fuzz_check_ecdsa_sign_failure_cleanup(const secp256k1_context *ctx, const unsigned char *msg32, const unsigned char *valid_seckey32) {
    secp256k1_fuzz_ecdsa_nonce_data nonce_data;
    secp256k1_ecdsa_signature sig;
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };

    memset(&sig, 0xA5, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, secp256k1_fuzz_scalar_zero, NULL, NULL) == 0);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);

    memset(&sig, 0xA5, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, secp256k1_fuzz_scalar_order, NULL, NULL) == 0);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);

    nonce_data.self = &nonce_data;
    nonce_data.extra32 = msg32;
    nonce_data.expected_key32 = NULL;
    nonce_data.expected_msg32 = NULL;
    nonce_data.calls = 0;
    memset(&sig, 0xA5, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, valid_seckey32, secp256k1_fuzz_ecdsa_nonce_fail, &nonce_data) == 0);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);

    nonce_data.calls = 0;
    memset(&sig, 0x5A, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, valid_seckey32, secp256k1_fuzz_ecdsa_nonce_retry_fail, &nonce_data) == 0);
    FUZZ_CHECK(nonce_data.calls == 3);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
}

static void secp256k1_fuzz_check_ecdsa_sign_null_input_cleanup(secp256k1_context *ctx, const unsigned char *valid_msg32, const unsigned char *valid_seckey32) {
    struct {
        const unsigned char *msg32;
        const unsigned char *seckey32;
    } invalid_cases[2];
    secp256k1_fuzz_ecdsa_sign_fn sign = secp256k1_ecdsa_sign;
    secp256k1_fuzz_api_illegal_data illegal_data;
    secp256k1_fuzz_ecdsa_nonce_data nonce_data;
    secp256k1_ecdsa_signature sig;
    unsigned char zero_sig[sizeof(sig)] = { 0 };
    unsigned int calls;
    size_t i;

    invalid_cases[0].msg32 = NULL;
    invalid_cases[0].seckey32 = valid_seckey32;
    invalid_cases[1].msg32 = valid_msg32;
    invalid_cases[1].seckey32 = NULL;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    nonce_data.self = &nonce_data;
    nonce_data.extra32 = NULL;
    nonce_data.expected_key32 = NULL;
    nonce_data.expected_msg32 = NULL;
    nonce_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_api_illegal_callback, &illegal_data);

    for (i = 0; i < sizeof(invalid_cases) / sizeof(invalid_cases[0]); i++) {
        memset(&sig, 0xA5, sizeof(sig));
        calls = illegal_data.calls;
        nonce_data.calls = 0;
        FUZZ_CHECK(sign(ctx, &sig, invalid_cases[i].msg32, invalid_cases[i].seckey32, secp256k1_fuzz_ecdsa_nonce_fail, &nonce_data) == 0);
        FUZZ_CHECK(illegal_data.calls == calls + 1);
        FUZZ_CHECK(nonce_data.calls == 0);
        FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
    }
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_ecdsa_invalid_seckey_nonce_domain(const secp256k1_context *ctx) {
    static const unsigned char callback_msg32[32] = { 0x42 };
    const unsigned char *invalid_seckeys[2] = {
        secp256k1_fuzz_scalar_zero,
        secp256k1_fuzz_scalar_order
    };
    secp256k1_fuzz_ecdsa_nonce_data nonce_data;
    secp256k1_ecdsa_signature sig;
    unsigned char zero_sig[sizeof(sig)] = { 0 };
    size_t i;

    nonce_data.self = &nonce_data;
    nonce_data.extra32 = NULL;
    nonce_data.expected_msg32 = callback_msg32;
    for (i = 0; i < sizeof(invalid_seckeys) / sizeof(invalid_seckeys[0]); i++) {
        nonce_data.expected_key32 = invalid_seckeys[i];
        nonce_data.calls = 0;
        memset(&sig, 0xA5, sizeof(sig));
        FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, callback_msg32, invalid_seckeys[i], secp256k1_fuzz_ecdsa_nonce, &nonce_data) == 0);
        FUZZ_CHECK(nonce_data.calls == 1);
        FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
    }
}

static void secp256k1_fuzz_check_ecdsa_message_reduction(const secp256k1_context *ctx, const unsigned char *seckey32, const secp256k1_pubkey *pubkey) {
    unsigned char zero_msg32[32] = { 0 };
    unsigned char zero_sig64[64];
    unsigned char order_sig64[64];
    secp256k1_ecdsa_signature zero_sig;
    secp256k1_ecdsa_signature order_sig;

    /* RFC6979 signs the scalar-reduced message, so hashes that differ by the
     * group order must not produce different deterministic nonces. */
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &zero_sig, zero_msg32, seckey32, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &order_sig, secp256k1_fuzz_scalar_order, seckey32, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, zero_sig64, &zero_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, order_sig64, &order_sig) == 1);
    FUZZ_CHECK(memcmp(zero_sig64, order_sig64, sizeof(zero_sig64)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &zero_sig, zero_msg32, pubkey) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &order_sig, secp256k1_fuzz_scalar_order, pubkey) == 1);
}

static void secp256k1_fuzz_check_ecdsa_fixed_nonce_equation(const secp256k1_context *ctx) {
    static const unsigned char one32[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    };
    static const unsigned char expected_sig64[64] = {
        /* r = x(G), s = k^-1 * (z + r*d), with d = z = k = 1. */
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac,
        0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98,
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac,
        0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x99
    };
    secp256k1_ecdsa_signature sig;
    secp256k1_pubkey pubkey;
    unsigned char serialized[64];

    /* This pins the ECDSA equation independently of both RFC6979 and the
     * library's verification path. */
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, one32) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, one32, one32, secp256k1_fuzz_ecdsa_nonce_fixed_one, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, serialized, &sig) == 1);
    FUZZ_CHECK(memcmp(serialized, expected_sig64, sizeof(serialized)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &sig, one32, &pubkey) == 1);
}

static void secp256k1_fuzz_check_ecdsa_retry_after_zero_s(const secp256k1_context *ctx) {
    static const unsigned char one32[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    };
    secp256k1_fuzz_ecdsa_nonce_data nonce_data;
    secp256k1_ecdsa_signature sig;
    secp256k1_pubkey generator;
    unsigned char generator_uncompressed[65];
    unsigned char zero_s_message[32];
    unsigned char compact[64];
    unsigned char zero32[32] = { 0 };
    size_t generator_len = sizeof(generator_uncompressed);

    /* For d = k = 1, choose z = -x(G) so the first valid nonce reaches the
     * ECDSA equation's s == 0 rejection path. The callback then supplies k=2,
     * making the retry observable instead of relying on a probabilistic case. */
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &generator, one32) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, generator_uncompressed, &generator_len, &generator, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(generator_len == sizeof(generator_uncompressed));
    memcpy(zero_s_message, generator_uncompressed + 1, sizeof(zero_s_message));
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, zero_s_message) == 1);
    FUZZ_CHECK(secp256k1_ec_seckey_verify(ctx, zero_s_message) == 1);

    nonce_data.self = &nonce_data;
    nonce_data.extra32 = NULL;
    nonce_data.expected_key32 = NULL;
    nonce_data.expected_msg32 = NULL;
    nonce_data.calls = 0;
    memset(&sig, 0xA5, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, zero_s_message, one32, secp256k1_fuzz_ecdsa_nonce_s_zero_then_two, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls == 2);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &sig) == 1);
    FUZZ_CHECK(memcmp(compact, zero32, sizeof(zero32)) != 0);
    FUZZ_CHECK(memcmp(compact + 32, zero32, sizeof(zero32)) != 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &sig, zero_s_message, &generator) == 1);
}

static void secp256k1_fuzz_check_rfc6979_nonce_failure_cleanup(const unsigned char *msg32, const unsigned char *key32, const unsigned char *extra32) {
    unsigned char nonce32[32];
    unsigned char zero32[32] = { 0 };

    FUZZ_CHECK(secp256k1_nonce_function_rfc6979(NULL, msg32, key32, NULL, (void *)extra32, 0) == 0);

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_rfc6979(nonce32, NULL, key32, NULL, (void *)extra32, 0) == 0);
    FUZZ_CHECK(memcmp(nonce32, zero32, sizeof(nonce32)) == 0);

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_default(nonce32, msg32, NULL, NULL, (void *)extra32, 0) == 0);
    FUZZ_CHECK(memcmp(nonce32, zero32, sizeof(nonce32)) == 0);

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_rfc6979(nonce32, msg32, key32, NULL, (void *)extra32, 0) == 1);

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_rfc6979(nonce32, msg32, key32, NULL, (void *)extra32, UINT_MAX) == 0);
    FUZZ_CHECK(memcmp(nonce32, zero32, sizeof(nonce32)) == 0);
}

static int secp256k1_fuzz_scalar32_in_order(const unsigned char *input32) {
    return memcmp(input32, secp256k1_fuzz_scalar_order, 32) < 0;
}

static void secp256k1_fuzz_scalar32_reduce(unsigned char *out32, const unsigned char *input32) {
    int i;
    unsigned int borrow = 0;

    memcpy(out32, input32, 32);
    if (secp256k1_fuzz_scalar32_in_order(input32)) {
        return;
    }
    for (i = 31; i >= 0; i--) {
        unsigned int subtrahend = (unsigned int)secp256k1_fuzz_scalar_order[i] + borrow;
        unsigned int value = out32[i];
        out32[i] = (unsigned char)(value - subtrahend);
        borrow = value < subtrahend;
    }
    FUZZ_CHECK(borrow == 0);
}

static void secp256k1_fuzz_check_rfc6979_algorithm_domain(const unsigned char *msg32, const unsigned char *key32, const unsigned char *extra32) {
    static const unsigned char algorithm16[16] = {
        0x73, 0x65, 0x63, 0x70, 0x32, 0x35, 0x36, 0x6B,
        0x31, 0x2D, 0x66, 0x75, 0x7A, 0x7A, 0x00, 0x01
    };
    unsigned char msg_mod32[32];
    unsigned char keydata[112];
    secp256k1_hash_ctx hash_ctx;
    unsigned char reference_k[32];
    unsigned char reference_v[32];
    unsigned char expected32[32];
    unsigned char actual32[32];
    unsigned int counter;

    secp256k1_fuzz_scalar32_reduce(msg_mod32, msg32);
    memcpy(keydata, key32, 32);
    memcpy(keydata + 32, msg_mod32, 32);
    memcpy(keydata + 64, extra32, 32);
    memcpy(keydata + 96, algorithm16, sizeof(algorithm16));

    secp256k1_hash_ctx_init(&hash_ctx);
    secp256k1_fuzz_rfc6979_reference_initialize(&hash_ctx, reference_k, reference_v, keydata, sizeof(keydata));
    for (counter = 0; counter < 3; counter++) {
        secp256k1_fuzz_rfc6979_reference_generate(&hash_ctx, reference_k, reference_v, counter != 0, expected32, sizeof(expected32));
        memset(actual32, 0xA5, sizeof(actual32));
        FUZZ_CHECK(secp256k1_nonce_function_rfc6979(actual32, msg32, key32, algorithm16, (void *)extra32, counter) == 1);
        FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
    }

    secp256k1_memclear_explicit(reference_k, sizeof(reference_k));
    secp256k1_memclear_explicit(reference_v, sizeof(reference_v));
    secp256k1_memclear_explicit(msg_mod32, sizeof(msg_mod32));
    secp256k1_memclear_explicit(keydata, sizeof(keydata));
    secp256k1_memclear_explicit(expected32, sizeof(expected32));
    secp256k1_memclear_explicit(actual32, sizeof(actual32));
}

/* Add two canonical scalars with byte arithmetic, independent of the scalar
 * implementation. The 33-byte accumulator is enough for one addition; the
 * loop also makes the reduction rule explicit at the order boundary. */
static void secp256k1_fuzz_scalar32_add_mod_order(unsigned char *out32, const unsigned char *a32, const unsigned char *b32) {
    unsigned char sum[33] = { 0 };
    unsigned int carry = 0;
    int i;

    for (i = 31; i >= 0; i--) {
        unsigned int value = (unsigned int)a32[i] + b32[i] + carry;
        sum[i + 1] = (unsigned char)value;
        carry = value >> 8;
    }
    sum[0] = (unsigned char)carry;

    while (sum[0] != 0 || memcmp(sum + 1, secp256k1_fuzz_scalar_order, 32) >= 0) {
        unsigned int borrow = 0;

        for (i = 31; i >= 0; i--) {
            unsigned int value = sum[i + 1];
            unsigned int subtrahend = (unsigned int)secp256k1_fuzz_scalar_order[i] + borrow;
            sum[i + 1] = (unsigned char)(value - subtrahend);
            borrow = value < subtrahend;
        }
        FUZZ_CHECK(sum[0] >= borrow);
        sum[0] = (unsigned char)(sum[0] - borrow);
    }
    FUZZ_CHECK(sum[0] == 0);
    memcpy(out32, sum + 1, 32);
}

/* Multiply canonical scalars with byte arithmetic, independent of the scalar
 * implementation. Double-and-add keeps every intermediate value canonical
 * by routing it through the independent addition routine above. */
static void secp256k1_fuzz_scalar32_mul_mod_order(unsigned char *out32, const unsigned char *a32, const unsigned char *b32) {
    unsigned char product[32] = { 0 };
    unsigned int bit;
    int i;

    for (i = 0; i < 32; i++) {
        for (bit = 0x80u; bit != 0; bit >>= 1) {
            secp256k1_fuzz_scalar32_add_mod_order(product, product, product);
            if ((b32[i] & bit) != 0) {
                secp256k1_fuzz_scalar32_add_mod_order(product, product, a32);
            }
        }
    }
    memcpy(out32, product, 32);
}

/* Check the order-minus-one boundary against byte arithmetic. The ordinary
 * tweak checks compare two production paths, so a shared scalar-conversion
 * regression could make both sides agree on the same wrong result. */
static void secp256k1_fuzz_check_tweak_order_minus_one(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *seckey) {
    const unsigned char *tweak32 = secp256k1_fuzz_scalar_order_minus_one;
    unsigned char expected_seckey[32];
    unsigned char tweaked_seckey[32];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_pubkey tweaked_pubkey;
    secp256k1_pubkey expected_pubkey;
    int expected_ret;
    int seckey_ret;
    int pubkey_ret;

    secp256k1_fuzz_scalar32_add_mod_order(expected_seckey, seckey, tweak32);
    expected_ret = memcmp(expected_seckey, secp256k1_fuzz_scalar_zero, 32) != 0;
    memcpy(tweaked_seckey, seckey, sizeof(tweaked_seckey));
    tweaked_pubkey = *pubkey;
    seckey_ret = secp256k1_ec_seckey_tweak_add(ctx, tweaked_seckey, tweak32);
    pubkey_ret = secp256k1_ec_pubkey_tweak_add(ctx, &tweaked_pubkey, tweak32);
    FUZZ_CHECK(seckey_ret == expected_ret);
    FUZZ_CHECK(pubkey_ret == expected_ret);
    if (expected_ret) {
        FUZZ_CHECK(memcmp(tweaked_seckey, expected_seckey, sizeof(tweaked_seckey)) == 0);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &expected_pubkey, expected_seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &tweaked_pubkey, &expected_pubkey) == 0);
    } else {
        FUZZ_CHECK(memcmp(tweaked_seckey, secp256k1_fuzz_scalar_zero, sizeof(tweaked_seckey)) == 0);
        FUZZ_CHECK(memcmp(&tweaked_pubkey, zero_pubkey, sizeof(tweaked_pubkey)) == 0);
    }

    secp256k1_fuzz_scalar32_mul_mod_order(expected_seckey, seckey, tweak32);
    expected_ret = memcmp(expected_seckey, secp256k1_fuzz_scalar_zero, 32) != 0;
    memcpy(tweaked_seckey, seckey, sizeof(tweaked_seckey));
    tweaked_pubkey = *pubkey;
    seckey_ret = secp256k1_ec_seckey_tweak_mul(ctx, tweaked_seckey, tweak32);
    pubkey_ret = secp256k1_ec_pubkey_tweak_mul(ctx, &tweaked_pubkey, tweak32);
    FUZZ_CHECK(seckey_ret == expected_ret);
    FUZZ_CHECK(pubkey_ret == expected_ret);
    if (expected_ret) {
        FUZZ_CHECK(memcmp(tweaked_seckey, expected_seckey, sizeof(tweaked_seckey)) == 0);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &expected_pubkey, expected_seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &tweaked_pubkey, &expected_pubkey) == 0);
    } else {
        FUZZ_CHECK(memcmp(tweaked_seckey, secp256k1_fuzz_scalar_zero, sizeof(tweaked_seckey)) == 0);
        FUZZ_CHECK(memcmp(&tweaked_pubkey, zero_pubkey, sizeof(tweaked_pubkey)) == 0);
    }
}

/* Check a three-term public-key sum against independently computed scalar
 * addition. The existing two-term check exercises the public tweak wrapper;
 * this relation reaches the multi-input combine loop and a different
 * cancellation boundary while deriving the expected point through generator
 * multiplication. */
static void secp256k1_fuzz_check_pubkey_combine_three(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *seckey, const secp256k1_pubkey *other_pubkey, const unsigned char *other_seckey) {
    const secp256k1_pubkey *inputs[3];
    const secp256k1_pubkey *reordered_inputs[3];
    secp256k1_pubkey generator;
    secp256k1_pubkey combined;
    secp256k1_pubkey reordered;
    secp256k1_pubkey expected;
    unsigned char pair_sum[32];
    unsigned char expected_seckey[32];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    int expected_ret;
    int combine_ret;

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &generator, secp256k1_fuzz_scalar_one) == 1);
    inputs[0] = pubkey;
    inputs[1] = other_pubkey;
    inputs[2] = &generator;
    combine_ret = secp256k1_ec_pubkey_combine(ctx, &combined, inputs, 3);

    secp256k1_fuzz_scalar32_add_mod_order(pair_sum, seckey, other_seckey);
    secp256k1_fuzz_scalar32_add_mod_order(expected_seckey, pair_sum, secp256k1_fuzz_scalar_one);
    expected_ret = memcmp(expected_seckey, secp256k1_fuzz_scalar_zero, 32) != 0;
    FUZZ_CHECK(combine_ret == expected_ret);
    if (expected_ret) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &expected, expected_seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, &expected) == 0);
    } else {
        FUZZ_CHECK(memcmp(&combined, zero_pubkey, sizeof(combined)) == 0);
    }

    reordered_inputs[0] = &generator;
    reordered_inputs[1] = other_pubkey;
    reordered_inputs[2] = pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &reordered, reordered_inputs, 3) == combine_ret);
    if (combine_ret) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, &reordered) == 0);
    } else {
        FUZZ_CHECK(memcmp(&reordered, zero_pubkey, sizeof(reordered)) == 0);
    }
}

/* Check that a multi-input combine can resume after its accumulator reaches
 * infinity. The final zero-sum case alone is insufficient: it can pass even
 * if the loop mishandles a later point after an intermediate cancellation. */
static void secp256k1_fuzz_check_pubkey_combine_intermediate_infinity(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const secp256k1_pubkey *pubkey_neg, const secp256k1_pubkey *other_pubkey, const secp256k1_pubkey *other_pubkey_neg) {
    const secp256k1_pubkey *resume_inputs[3];
    const secp256k1_pubkey *resume_reordered_inputs[3];
    const secp256k1_pubkey *zero_inputs[4];
    const secp256k1_pubkey *zero_reordered_inputs[4];
    secp256k1_pubkey resumed;
    secp256k1_pubkey resumed_reordered;
    secp256k1_pubkey zero_sum;
    secp256k1_pubkey zero_sum_reordered;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };

    resume_inputs[0] = pubkey;
    resume_inputs[1] = pubkey_neg;
    resume_inputs[2] = other_pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &resumed, resume_inputs, 3) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &resumed, other_pubkey) == 0);

    resume_reordered_inputs[0] = other_pubkey;
    resume_reordered_inputs[1] = pubkey;
    resume_reordered_inputs[2] = pubkey_neg;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &resumed_reordered, resume_reordered_inputs, 3) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &resumed_reordered, other_pubkey) == 0);

    zero_inputs[0] = pubkey;
    zero_inputs[1] = pubkey_neg;
    zero_inputs[2] = other_pubkey;
    zero_inputs[3] = other_pubkey_neg;
    memset(&zero_sum, 0xA5, sizeof(zero_sum));
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &zero_sum, zero_inputs, 4) == 0);
    FUZZ_CHECK(memcmp(&zero_sum, zero_pubkey, sizeof(zero_sum)) == 0);

    zero_reordered_inputs[0] = other_pubkey;
    zero_reordered_inputs[1] = other_pubkey_neg;
    zero_reordered_inputs[2] = pubkey;
    zero_reordered_inputs[3] = pubkey_neg;
    memset(&zero_sum_reordered, 0x5A, sizeof(zero_sum_reordered));
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &zero_sum_reordered, zero_reordered_inputs, 4) == 0);
    FUZZ_CHECK(memcmp(&zero_sum_reordered, zero_pubkey, sizeof(zero_sum_reordered)) == 0);
}

/* Exercise the arbitrary-length combine loop with an independent scalar sum.
 * The existing three- and four-term checks do not reach the eighth input, so
 * a stale loop bound or a skipped tail term could remain invisible. The
 * length gate keeps the pre-existing short corpus a useful differential
 * control while allowing normal mutations of the dedicated long seed. */
static void secp256k1_fuzz_check_pubkey_combine_long(const secp256k1_context *ctx, size_t size, const secp256k1_pubkey *pubkey, const unsigned char *seckey, const secp256k1_pubkey *other_pubkey, const unsigned char *other_seckey) {
    const size_t n_pubkeys = 8;
    const secp256k1_pubkey *inputs[8];
    const secp256k1_pubkey *reordered_inputs[8];
    secp256k1_pubkey fixed_pubkeys[6];
    secp256k1_pubkey combined;
    secp256k1_pubkey reordered;
    secp256k1_pubkey expected;
    unsigned char expected_seckey[32] = { 0 };
    unsigned char fixed_seckey[32];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    size_t i;
    int expected_ret;
    int combine_ret;

    if (size < 128) {
        return;
    }

    inputs[0] = pubkey;
    inputs[1] = other_pubkey;
    secp256k1_fuzz_scalar32_add_mod_order(expected_seckey, expected_seckey, seckey);
    secp256k1_fuzz_scalar32_add_mod_order(expected_seckey, expected_seckey, other_seckey);
    for (i = 0; i < sizeof(fixed_pubkeys) / sizeof(fixed_pubkeys[0]); i++) {
        memset(fixed_seckey, 0, sizeof(fixed_seckey));
        fixed_seckey[31] = (unsigned char)(i + 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &fixed_pubkeys[i], fixed_seckey) == 1);
        inputs[i + 2] = &fixed_pubkeys[i];
        secp256k1_fuzz_scalar32_add_mod_order(expected_seckey, expected_seckey, fixed_seckey);
    }

    expected_ret = memcmp(expected_seckey, secp256k1_fuzz_scalar_zero, sizeof(expected_seckey)) != 0;
    memset(&combined, 0xA5, sizeof(combined));
    combine_ret = secp256k1_ec_pubkey_combine(ctx, &combined, inputs, n_pubkeys);
    FUZZ_CHECK(combine_ret == expected_ret);
    if (expected_ret) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &expected, expected_seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, &expected) == 0);
    } else {
        FUZZ_CHECK(memcmp(&combined, zero_pubkey, sizeof(combined)) == 0);
    }

    for (i = 0; i < n_pubkeys; i++) {
        reordered_inputs[i] = inputs[n_pubkeys - 1 - i];
    }
    memset(&reordered, 0x5A, sizeof(reordered));
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &reordered, reordered_inputs, n_pubkeys) == combine_ret);
    if (combine_ret) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, &reordered) == 0);
    } else {
        FUZZ_CHECK(memcmp(&reordered, zero_pubkey, sizeof(reordered)) == 0);
    }
}

/* Exercise repeated infinity transitions before a nonzero tail. The
 * independent scalar model makes a skipped or misplaced tail term visible,
 * while the prefix checks make each cancellation boundary explicit. */
static void secp256k1_fuzz_check_pubkey_combine_sixteen(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    enum { N_PUBKEYS = 16 };
    static const unsigned char trigger[] = "sixteen-term-pubkey-combine\n";
    static const int scalar_values[N_PUBKEYS] = {
        1, -1, 2, -2, 3, -3, 4, -4,
        5, 6, 7, 8, 9, 10, 11, 12
    };
    const secp256k1_pubkey *inputs[N_PUBKEYS];
    const secp256k1_pubkey *reordered_inputs[N_PUBKEYS];
    secp256k1_pubkey fixed_pubkeys[N_PUBKEYS];
    secp256k1_pubkey resumed;
    secp256k1_pubkey resumed_expected;
    secp256k1_pubkey combined;
    secp256k1_pubkey reordered;
    secp256k1_pubkey expected;
    unsigned char seckeys[N_PUBKEYS][32];
    unsigned char resumed_seckey[32];
    unsigned char expected_seckey[32] = { 0 };
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    size_t i;
    int combine_ret;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    for (i = 0; i < N_PUBKEYS; i++) {
        memset(seckeys[i], 0, sizeof(seckeys[i]));
        if (scalar_values[i] > 0) {
            seckeys[i][31] = (unsigned char)scalar_values[i];
        } else {
            memcpy(seckeys[i], secp256k1_fuzz_scalar_order_minus_one, sizeof(seckeys[i]));
            seckeys[i][31] = (unsigned char)(seckeys[i][31] - (-scalar_values[i] - 1));
        }
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &fixed_pubkeys[i], seckeys[i]) == 1);
        inputs[i] = &fixed_pubkeys[i];
        secp256k1_fuzz_scalar32_add_mod_order(expected_seckey, expected_seckey, seckeys[i]);
    }
    for (i = 0; i < N_PUBKEYS; i++) {
        reordered_inputs[i] = inputs[N_PUBKEYS - 1 - i];
    }

    /* The four pair prefixes each end at infinity. */
    for (i = 0; i < 8; i += 2) {
        memset(&resumed, 0xA5, sizeof(resumed));
        FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &resumed, inputs, i + 2) == 0);
        FUZZ_CHECK(memcmp(&resumed, zero_pubkey, sizeof(resumed)) == 0);
    }

    /* The first tail point must be accepted after the fourth cancellation. */
    memcpy(resumed_seckey, seckeys[8], sizeof(resumed_seckey));
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &resumed_expected, resumed_seckey) == 1);
    memset(&resumed, 0x5A, sizeof(resumed));
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &resumed, inputs, 9) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &resumed, &resumed_expected) == 0);

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &expected, expected_seckey) == 1);
    memset(&combined, 0xA5, sizeof(combined));
    combine_ret = secp256k1_ec_pubkey_combine(ctx, &combined, inputs, N_PUBKEYS);
    FUZZ_CHECK(combine_ret == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, &expected) == 0);

    memset(&reordered, 0x5A, sizeof(reordered));
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &reordered, reordered_inputs, N_PUBKEYS) == combine_ret);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined, &reordered) == 0);
}

/* Verify an arbitrary low-S ECDSA signature without using the internal
 * verifier's inverse-and-multiscalar path. For a candidate R reconstructed
 * from r (or r+n), the ECDSA equation is sR = zG + rQ. */
static int secp256k1_fuzz_ecdsa_check_r_candidate(const secp256k1_context *ctx, const unsigned char *x32, const unsigned char *s32, const secp256k1_pubkey *expected) {
    unsigned char compressed[33];
    secp256k1_pubkey r_point;
    secp256k1_pubkey s_r_point;
    int parity;

    memcpy(compressed + 1, x32, 32);
    for (parity = 0; parity <= 1; parity++) {
        compressed[0] = (unsigned char)(SECP256K1_TAG_PUBKEY_EVEN + parity);
        if (secp256k1_ec_pubkey_parse(ctx, &r_point, compressed, sizeof(compressed))) {
            s_r_point = r_point;
            if (secp256k1_ec_pubkey_tweak_mul(ctx, &s_r_point, s32)
                && secp256k1_ec_pubkey_cmp(ctx, &s_r_point, expected) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

static int secp256k1_fuzz_ecdsa_verify_reference(const secp256k1_context *ctx, const secp256k1_ecdsa_signature *sig, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    unsigned char compact[64];
    unsigned char msg_mod[32];
    unsigned char half_order[32];
    unsigned char r_plus_order[32];
    secp256k1_pubkey r_pubkey;
    secp256k1_pubkey z_generator;
    secp256k1_pubkey expected;
    const secp256k1_pubkey *terms[2];
    unsigned int carry = 0;
    size_t n_terms = 0;
    int i;

    if (!secp256k1_ecdsa_signature_serialize_compact(ctx, compact, sig)) {
        return 0;
    }
    if (memcmp(compact, secp256k1_fuzz_scalar_zero, 32) == 0
        || memcmp(compact + 32, secp256k1_fuzz_scalar_zero, 32) == 0) {
        return 0;
    }

    /* Match the public verifier's low-S policy without calling normalize. */
    for (i = 0; i < 32; i++) {
        unsigned int value = secp256k1_fuzz_scalar_order[i];
        half_order[i] = (unsigned char)((value >> 1) | carry);
        carry = (value & 1u) != 0 ? 0x80u : 0u;
    }
    if (memcmp(compact + 32, half_order, sizeof(half_order)) > 0) {
        return 0;
    }

    secp256k1_fuzz_scalar32_reduce(msg_mod, msg32);
    r_pubkey = *pubkey;
    if (!secp256k1_ec_pubkey_tweak_mul(ctx, &r_pubkey, compact)) {
        return 0;
    }
    terms[n_terms++] = &r_pubkey;
    if (memcmp(msg_mod, secp256k1_fuzz_scalar_zero, sizeof(msg_mod)) != 0) {
        if (!secp256k1_ec_pubkey_create(ctx, &z_generator, msg_mod)) {
            return 0;
        }
        terms[n_terms++] = &z_generator;
    }
    if (!secp256k1_ec_pubkey_combine(ctx, &expected, terms, n_terms)) {
        return 0;
    }

    if (secp256k1_fuzz_ecdsa_check_r_candidate(ctx, compact, compact + 32, &expected)) {
        return 1;
    }

    memcpy(r_plus_order, compact, sizeof(r_plus_order));
    carry = 0;
    for (i = 31; i >= 0; i--) {
        unsigned int value = (unsigned int)r_plus_order[i]
                           + secp256k1_fuzz_scalar_order[i] + carry;
        r_plus_order[i] = (unsigned char)value;
        carry = value >> 8;
    }
    if (carry == 0
        && secp256k1_fuzz_ecdsa_check_r_candidate(ctx, r_plus_order, compact + 32, &expected)) {
        return 1;
    }
    return 0;
}

static void secp256k1_fuzz_check_ecdsa_variable_nonce_equation(const secp256k1_context *ctx, const unsigned char *msg32, const unsigned char *seckey32, const unsigned char *nonce32) {
    secp256k1_fuzz_ecdsa_equation_nonce_data nonce_data;
    secp256k1_ecdsa_signature sig;
    secp256k1_pubkey nonce_pubkey;
    unsigned char sig64[64];
    unsigned char nonce_ser[65];
    unsigned char expected_r[32];
    unsigned char msg_mod[32];
    unsigned char left[32];
    unsigned char right[32];
    unsigned char negated_right[32];
    size_t nonce_ser_len = sizeof(nonce_ser);

    nonce_data.self = &nonce_data;
    nonce_data.nonce32 = nonce32;
    nonce_data.calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, seckey32, secp256k1_fuzz_ecdsa_nonce_equation, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, sig64, &sig) == 1);
    FUZZ_CHECK(secp256k1_fuzz_scalar32_in_order(sig64));
    FUZZ_CHECK(secp256k1_fuzz_scalar32_in_order(sig64 + 32));
    FUZZ_CHECK(memcmp(sig64, secp256k1_fuzz_scalar_zero, 32) != 0);
    FUZZ_CHECK(memcmp(sig64 + 32, secp256k1_fuzz_scalar_zero, 32) != 0);

    /* Pin r to the nonce point independently of the ECDSA signing helper. */
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &nonce_pubkey, nonce32) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, nonce_ser, &nonce_ser_len, &nonce_pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(nonce_ser_len == sizeof(nonce_ser));
    secp256k1_fuzz_scalar32_reduce(expected_r, nonce_ser + 1);
    FUZZ_CHECK(memcmp(sig64, expected_r, sizeof(expected_r)) == 0);

    /* The raw message is reduced modulo n by the signer. Reproduce that
     * reduction with byte arithmetic, then check the scalar signing equation
     * using the public tweak API rather than ecdsa_verify. */
    secp256k1_fuzz_scalar32_reduce(msg_mod, msg32);
    memcpy(left, sig64 + 32, sizeof(left));
    FUZZ_CHECK(secp256k1_ec_seckey_tweak_mul(ctx, left, nonce32) == 1);
    memcpy(right, seckey32, sizeof(right));
    FUZZ_CHECK(secp256k1_ec_seckey_tweak_mul(ctx, right, sig64) == 1);
    FUZZ_CHECK(secp256k1_ec_seckey_tweak_add(ctx, right, msg_mod) == 1);
    if (memcmp(left, right, sizeof(left)) != 0) {
        memcpy(negated_right, right, sizeof(negated_right));
        FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, negated_right) == 1);
        FUZZ_CHECK(memcmp(left, negated_right, sizeof(left)) == 0);
    }
}

static void secp256k1_fuzz_check_ecdsa_r_plus_order(const secp256k1_context *ctx) {
    secp256k1_ecdsa_signature sig;
    secp256k1_pubkey r_point;
    secp256k1_pubkey pubkey;
    secp256k1_pubkey doubled;
    unsigned char compact[64] = { 0 };
    unsigned char scalar_two[32] = { 0 };
    unsigned char msg32[32] = { 0 };

    compact[31] = 2;
    compact[63] = 1;
    scalar_two[31] = 2;
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &r_point, secp256k1_fuzz_ecdsa_r_plus_order_point, sizeof(secp256k1_fuzz_ecdsa_r_plus_order_point)) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, secp256k1_fuzz_ecdsa_r_plus_order_pubkey, sizeof(secp256k1_fuzz_ecdsa_r_plus_order_pubkey)) == 1);
    doubled = pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &doubled, scalar_two) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &doubled, &r_point) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &sig, compact) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &sig, msg32, &pubkey) == 1);
}

static void secp256k1_fuzz_check_ecdsa_high_s(const secp256k1_context *ctx, const secp256k1_ecdsa_signature *sig, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_ecdsa_signature high_sig;
    secp256k1_ecdsa_signature in_place_sig;
    secp256k1_ecdsa_signature low_copy_sig;
    secp256k1_ecdsa_signature normalized_sig;
    unsigned char low_compact[64];
    unsigned char copy_compact[64];
    unsigned char high_compact[64];
    unsigned char normalized_compact[64];

    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, sig) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, low_compact, sig) == 1);
    memset(&low_copy_sig, 0xA5, sizeof(low_copy_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &low_copy_sig, sig) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, copy_compact, &low_copy_sig) == 1);
    FUZZ_CHECK(memcmp(copy_compact, low_compact, sizeof(copy_compact)) == 0);
    in_place_sig = *sig;
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &in_place_sig, &in_place_sig) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, copy_compact, &in_place_sig) == 1);
    FUZZ_CHECK(memcmp(copy_compact, low_compact, sizeof(copy_compact)) == 0);
    memcpy(high_compact, low_compact, sizeof(high_compact));
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, high_compact + 32) == 1);
    FUZZ_CHECK(memcmp(low_compact + 32, high_compact + 32, 32) != 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &high_sig, high_compact) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &high_sig, msg32, pubkey) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &high_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normalized_compact, &normalized_sig) == 1);
    FUZZ_CHECK(memcmp(normalized_compact, low_compact, sizeof(normalized_compact)) == 0);
    in_place_sig = high_sig;
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &in_place_sig, &in_place_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normalized_compact, &in_place_sig) == 1);
    FUZZ_CHECK(memcmp(normalized_compact, low_compact, sizeof(normalized_compact)) == 0);
}

/* Pin the exact low-S threshold with byte constants rather than deriving the
 * expected result through scalar_is_high or scalar_negate. */
static void secp256k1_fuzz_check_ecdsa_normalize_half_order(const secp256k1_context *ctx) {
    static const unsigned char half_order[32] = {
        0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x5D, 0x57, 0x6E, 0x73, 0x57, 0xA4, 0x50, 0x1D,
        0xDF, 0xE9, 0x2F, 0x46, 0x68, 0x1B, 0x20, 0xA0
    };
    unsigned char low_compact[64] = { 0 };
    unsigned char high_compact[64] = { 0 };
    unsigned char expected_compact[64] = { 0 };
    unsigned char serialized[64];
    secp256k1_ecdsa_signature low_sig;
    secp256k1_ecdsa_signature high_sig;
    secp256k1_ecdsa_signature normalized_sig;

    low_compact[31] = 1;
    memcpy(low_compact + 32, half_order, sizeof(half_order));
    memcpy(high_compact, low_compact, sizeof(high_compact));
    high_compact[63]++;
    expected_compact[31] = 1;
    memcpy(expected_compact + 32, half_order, sizeof(half_order));

    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &low_sig, low_compact) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, &low_sig) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &low_sig) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, serialized, &normalized_sig) == 1);
    FUZZ_CHECK(memcmp(serialized, low_compact, sizeof(serialized)) == 0);

    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &high_sig, high_compact) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, &high_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &high_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, serialized, &normalized_sig) == 1);
    FUZZ_CHECK(memcmp(serialized, expected_compact, sizeof(serialized)) == 0);
    normalized_sig = high_sig;
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &normalized_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, serialized, &normalized_sig) == 1);
    FUZZ_CHECK(memcmp(serialized, expected_compact, sizeof(serialized)) == 0);
}

static void secp256k1_fuzz_check_signature_parse_compact(const secp256k1_context *ctx, const unsigned char *input64, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char compact[64];
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };
    int expected_ret;
    int parse_ret;

    expected_ret = secp256k1_fuzz_scalar32_in_order(input64);
    expected_ret &= secp256k1_fuzz_scalar32_in_order(input64 + 32);
    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    parse_ret = secp256k1_ecdsa_signature_parse_compact(ctx, &parsed_sig, input64);
    FUZZ_CHECK(parse_ret == expected_ret);
    if (parse_ret) {
        FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &parsed_sig) == 1);
        FUZZ_CHECK(memcmp(compact, input64, sizeof(compact)) == 0);
        FUZZ_CHECK(secp256k1_fuzz_ecdsa_verify_reference(ctx, &parsed_sig, msg32, pubkey)
                   == secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey));
        secp256k1_fuzz_check_signature_roundtrip(ctx, &parsed_sig);
    } else {
        FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);
    }
}

static int secp256k1_fuzz_scalar32_is_zero(const unsigned char *input32) {
    return memcmp(input32, secp256k1_fuzz_scalar_zero, 32) == 0;
}

static size_t secp256k1_fuzz_write_der_integer(unsigned char *out, const unsigned char *input32) {
    size_t offset = 0;
    size_t input_offset = 0;
    size_t input_len;

    while (input_offset < 31 && input32[input_offset] == 0) {
        input_offset++;
    }
    input_len = 32 - input_offset;

    out[offset++] = 0x02;
    if (input32[input_offset] & 0x80) {
        out[offset++] = (unsigned char)(input_len + 1);
        out[offset++] = 0;
    } else {
        out[offset++] = (unsigned char)input_len;
    }
    memcpy(out + offset, input32 + input_offset, input_len);
    return offset + input_len;
}

static size_t secp256k1_fuzz_make_der_signature(unsigned char *out, const unsigned char *r32, const unsigned char *s32) {
    unsigned char integers[70];
    size_t integers_len = 0;

    integers_len += secp256k1_fuzz_write_der_integer(integers + integers_len, r32);
    integers_len += secp256k1_fuzz_write_der_integer(integers + integers_len, s32);

    out[0] = 0x30;
    out[1] = (unsigned char)integers_len;
    memcpy(out + 2, integers, integers_len);
    return 2 + integers_len;
}

static void secp256k1_fuzz_der_expected_scalar(unsigned char *out32, const unsigned char *input32) {
    if (secp256k1_fuzz_scalar32_in_order(input32)) {
        memcpy(out32, input32, 32);
    } else {
        memcpy(out32, secp256k1_fuzz_scalar_zero, 32);
    }
}

static void secp256k1_fuzz_check_signature_parse_der_input(const secp256k1_context *ctx, const unsigned char *input, size_t inputlen, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_ecdsa_signature parsed_sig;
    secp256k1_ecdsa_signature lax_sig;
    secp256k1_ecdsa_signature reparsed_lax_sig;
    unsigned char compact[64];
    unsigned char lax_compact[64];
    unsigned char reparsed_lax_compact[64];
    unsigned char roundtrip_der[72];
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };
    size_t roundtrip_der_len = sizeof(roundtrip_der);
    int parsed_der;
    int parsed_lax;

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    parsed_der = secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, input, inputlen);
    if (!parsed_der) {
        FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    }
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &parsed_sig) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdsa_verify_reference(ctx, &parsed_sig, msg32, pubkey)
               == secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey));
    if (parsed_der && !secp256k1_fuzz_scalar32_is_zero(compact) && !secp256k1_fuzz_scalar32_is_zero(compact + 32)) {
        FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, roundtrip_der, &roundtrip_der_len, &parsed_sig) == 1);
        FUZZ_CHECK(roundtrip_der_len == inputlen);
        FUZZ_CHECK(memcmp(roundtrip_der, input, inputlen) == 0);
        secp256k1_fuzz_check_signature_roundtrip(ctx, &parsed_sig);
    } else {
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);
    }

    /* The compatibility parser must initialize its output even when it
     * rejects the input, and any accepted strict DER must keep its value. */
    memset(&lax_sig, 0xA5, sizeof(lax_sig));
    parsed_lax = ecdsa_signature_parse_der_lax(ctx, &lax_sig, input, inputlen);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, lax_compact, &lax_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &reparsed_lax_sig, lax_compact) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, reparsed_lax_compact, &reparsed_lax_sig) == 1);
    FUZZ_CHECK(memcmp(lax_compact, reparsed_lax_compact, sizeof(lax_compact)) == 0);
    FUZZ_CHECK(secp256k1_fuzz_ecdsa_verify_reference(ctx, &lax_sig, msg32, pubkey)
               == secp256k1_ecdsa_verify(ctx, &lax_sig, msg32, pubkey));
    if (!parsed_lax) {
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &lax_sig, msg32, pubkey) == 0);
    }
    if (parsed_der) {
        FUZZ_CHECK(parsed_lax == 1);
        FUZZ_CHECK(memcmp(lax_compact, compact, sizeof(lax_compact)) == 0);
    }
}

static void secp256k1_fuzz_check_signature_parse_der_size_boundary(const secp256k1_context *ctx) {
    static const unsigned char non_der[1] = {0};
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = {0};
    secp256k1_ecdsa_signature parsed_sig;

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, non_der, SIZE_MAX) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
}

static void secp256k1_fuzz_check_signature_parse_der_boundary(const secp256k1_context *ctx, const unsigned char *r32, const unsigned char *s32, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char der[72];
    unsigned char roundtrip_der[72];
    unsigned char compact[64];
    unsigned char expected_r[32];
    unsigned char expected_s[32];
    size_t der_len;
    size_t roundtrip_der_len = sizeof(roundtrip_der);
    int expected_verifiable;

    der_len = secp256k1_fuzz_make_der_signature(der, r32, s32);
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, der, der_len) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &parsed_sig) == 1);

    secp256k1_fuzz_der_expected_scalar(expected_r, r32);
    secp256k1_fuzz_der_expected_scalar(expected_s, s32);
    FUZZ_CHECK(memcmp(compact, expected_r, sizeof(expected_r)) == 0);
    FUZZ_CHECK(memcmp(compact + 32, expected_s, sizeof(expected_s)) == 0);
    FUZZ_CHECK(secp256k1_fuzz_ecdsa_verify_reference(ctx, &parsed_sig, msg32, pubkey)
               == secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey));

    expected_verifiable = !secp256k1_fuzz_scalar32_is_zero(expected_r) && !secp256k1_fuzz_scalar32_is_zero(expected_s);
    if (expected_verifiable) {
        FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, roundtrip_der, &roundtrip_der_len, &parsed_sig) == 1);
        FUZZ_CHECK(roundtrip_der_len == der_len);
        FUZZ_CHECK(memcmp(roundtrip_der, der, der_len) == 0);
        secp256k1_fuzz_check_signature_roundtrip(ctx, &parsed_sig);
    } else {
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);
    }
}

static void secp256k1_fuzz_check_signature_parse_der_trailing(const secp256k1_context *ctx, const unsigned char *r32, const unsigned char *s32, unsigned char trailer) {
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char der[73];
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };
    size_t der_len;

    der_len = secp256k1_fuzz_make_der_signature(der, r32, s32);
    FUZZ_CHECK(der_len < sizeof(der));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, der, der_len) == 1);

    der[der_len] = trailer;
    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, der, der_len + 1) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);

    der[1]++;
    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, der, der_len + 1) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
}

static void secp256k1_fuzz_check_signature_parse_der_nonminimal(const secp256k1_context *ctx, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    static const unsigned char minimal_der[8] = { 0x30, 0x06, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01 };
    static const unsigned char padded_r_der[9] = { 0x30, 0x07, 0x02, 0x02, 0x00, 0x01, 0x02, 0x01, 0x01 };
    static const unsigned char padded_s_der[9] = { 0x30, 0x07, 0x02, 0x01, 0x01, 0x02, 0x02, 0x00, 0x01 };
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char roundtrip_der[72];
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };
    size_t roundtrip_der_len = sizeof(roundtrip_der);

    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, minimal_der, sizeof(minimal_der)) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, roundtrip_der, &roundtrip_der_len, &parsed_sig) == 1);
    FUZZ_CHECK(roundtrip_der_len == sizeof(minimal_der));
    FUZZ_CHECK(memcmp(roundtrip_der, minimal_der, sizeof(minimal_der)) == 0);

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, padded_r_der, sizeof(padded_r_der)) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, padded_s_der, sizeof(padded_s_der)) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);
}

static void secp256k1_fuzz_check_signature_parse_der_lengths(const secp256k1_context *ctx, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    static const unsigned char long_seq_der[9] = { 0x30, 0x81, 0x06, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01 };
    static const unsigned char long_r_der[9] = { 0x30, 0x07, 0x02, 0x81, 0x01, 0x01, 0x02, 0x01, 0x01 };
    static const unsigned char long_s_der[9] = { 0x30, 0x07, 0x02, 0x01, 0x01, 0x02, 0x81, 0x01, 0x01 };
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, long_seq_der, sizeof(long_seq_der)) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, long_r_der, sizeof(long_r_der)) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, long_s_der, sizeof(long_s_der)) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);
}

static void secp256k1_fuzz_check_signature_parse_der_lax_long_lengths(const secp256k1_context *ctx) {
    static const unsigned char ber_sig[16] = {
        0x30, 0x82, 0x00, 0x0C,
        0x02, 0x82, 0x00, 0x02, 0x00, 0x80,
        0x02, 0x82, 0x00, 0x02, 0x00, 0x80
    };
    unsigned char compact[64];
    unsigned char expected[64] = { 0 };
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };
    secp256k1_ecdsa_signature parsed_sig;

    expected[31] = 0x80;
    expected[63] = 0x80;

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, ber_sig, sizeof(ber_sig)) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    FUZZ_CHECK(ecdsa_signature_parse_der_lax(ctx, &parsed_sig, ber_sig, sizeof(ber_sig)) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &parsed_sig) == 1);
    FUZZ_CHECK(memcmp(compact, expected, sizeof(compact)) == 0);
}

static void secp256k1_fuzz_check_signature_parse_der_empty_integer(const secp256k1_context *ctx, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    static const unsigned char empty_r_der[7] = { 0x30, 0x05, 0x02, 0x00, 0x02, 0x01, 0x01 };
    static const unsigned char empty_s_der[7] = { 0x30, 0x05, 0x02, 0x01, 0x01, 0x02, 0x00 };
    static const unsigned char empty_rs_der[6] = { 0x30, 0x04, 0x02, 0x00, 0x02, 0x00 };
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, empty_r_der, sizeof(empty_r_der)) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, empty_s_der, sizeof(empty_s_der)) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, empty_rs_der, sizeof(empty_rs_der)) == 0);
    FUZZ_CHECK(memcmp(&parsed_sig, zero_sig, sizeof(parsed_sig)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);
}

static void secp256k1_fuzz_check_signature_parse_der_negative(const secp256k1_context *ctx, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    static const unsigned char negative_r_der[8] = { 0x30, 0x06, 0x02, 0x01, 0x80, 0x02, 0x01, 0x01 };
    static const unsigned char negative_s_der[8] = { 0x30, 0x06, 0x02, 0x01, 0x01, 0x02, 0x01, 0x80 };
    static const unsigned char zero_one_der[8] = { 0x30, 0x06, 0x02, 0x01, 0x00, 0x02, 0x01, 0x01 };
    static const unsigned char one_zero_der[8] = { 0x30, 0x06, 0x02, 0x01, 0x01, 0x02, 0x01, 0x00 };
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char compact[64];
    unsigned char expected_compact[64] = { 0 };
    unsigned char roundtrip_der[72];
    size_t roundtrip_der_len = sizeof(roundtrip_der);

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, negative_r_der, sizeof(negative_r_der)) == 1);
    expected_compact[63] = 1;
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &parsed_sig) == 1);
    FUZZ_CHECK(memcmp(compact, expected_compact, sizeof(compact)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, roundtrip_der, &roundtrip_der_len, &parsed_sig) == 1);
    FUZZ_CHECK(roundtrip_der_len == sizeof(zero_one_der));
    FUZZ_CHECK(memcmp(roundtrip_der, zero_one_der, sizeof(zero_one_der)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);

    memset(&parsed_sig, 0xA5, sizeof(parsed_sig));
    memset(expected_compact, 0, sizeof(expected_compact));
    roundtrip_der_len = sizeof(roundtrip_der);
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, negative_s_der, sizeof(negative_s_der)) == 1);
    expected_compact[31] = 1;
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &parsed_sig) == 1);
    FUZZ_CHECK(memcmp(compact, expected_compact, sizeof(compact)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, roundtrip_der, &roundtrip_der_len, &parsed_sig) == 1);
    FUZZ_CHECK(roundtrip_der_len == sizeof(one_zero_der));
    FUZZ_CHECK(memcmp(roundtrip_der, one_zero_der, sizeof(one_zero_der)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);
}

static int secp256k1_fuzz_der_read_tlv(const unsigned char *data, size_t data_len, size_t *pos, unsigned char tag, size_t value_len, unsigned int length_bytes, const unsigned char **value) {
    if (length_bytes > 2 || *pos > data_len || data_len - *pos < 2 + length_bytes || data[*pos] != tag) {
        return 0;
    }
    (*pos)++;
    if (length_bytes == 0) {
        if (value_len > 0x7F || data[*pos] != (unsigned char)value_len) {
            return 0;
        }
        (*pos)++;
    } else if (length_bytes == 1) {
        if (value_len > 0xFF || data[*pos] != 0x81 || data[*pos + 1] != (unsigned char)value_len) {
            return 0;
        }
        *pos += 2;
    } else {
        if (value_len > 0xFFFF || data[*pos] != 0x82
            || data[*pos + 1] != (unsigned char)(value_len >> 8)
            || data[*pos + 2] != (unsigned char)value_len) {
            return 0;
        }
        *pos += 3;
    }
    if (value_len > data_len - *pos) {
        return 0;
    }
    *value = data + *pos;
    *pos += value_len;
    return 1;
}

static int secp256k1_fuzz_check_privkey_der_structure(const secp256k1_context *ctx, const unsigned char *privkey, size_t privkeylen, const unsigned char *seckey32, int compressed) {
    static const unsigned char curve_oid[7] = { 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x01, 0x01 };
    static const unsigned char field_prime[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
    };
    static const unsigned char generator_compressed[33] = {
        0x02, 0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC,
        0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B, 0x07, 0x02,
        0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9, 0x59, 0xF2,
        0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
    };
    static const unsigned char generator_uncompressed[65] = {
        0x04, 0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC,
        0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B, 0x07, 0x02,
        0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9, 0x59, 0xF2,
        0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98, 0x48, 0x3A, 0xDA,
        0x77, 0x26, 0xA3, 0xC4, 0x65, 0x5D, 0xA4, 0xFB, 0xFC,
        0x0E, 0x11, 0x08, 0xA8, 0xFD, 0x17, 0xB4, 0x48, 0xA6,
        0x85, 0x54, 0x19, 0x9C, 0x47, 0xD0, 0x8F, 0xFB, 0x10,
        0xD4, 0xB8
    };
    const unsigned char *outer;
    const unsigned char *value;
    const unsigned char *parameters;
    const unsigned char *parameter_sequence;
    const unsigned char *algorithm;
    const unsigned char *curve;
    const unsigned char *base;
    const unsigned char *public_key_container;
    const unsigned char *public_key_bits;
    unsigned char expected_pubkey[65];
    secp256k1_pubkey pubkey;
    size_t outer_len = compressed ? 0xD3 : 0x113;
    size_t parameter_len = compressed ? 0x85 : 0xA5;
    size_t parameter_sequence_len = compressed ? 0x82 : 0xA2;
    size_t base_len = compressed ? sizeof(generator_compressed) : sizeof(generator_uncompressed);
    size_t public_key_bits_len = compressed ? 0x22 : 0x42;
    size_t public_key_container_len = compressed ? 0x24 : 0x44;
    size_t expected_pubkey_len = base_len;
    size_t outer_pos = 0;
    size_t parameter_pos = 0;
    size_t parameter_sequence_pos = 0;
    size_t sequence_pos = 0;
    size_t algorithm_pos = 0;
    size_t curve_pos = 0;
    size_t public_key_pos = 0;
    size_t serialized_len = sizeof(expected_pubkey);

    if (privkeylen != (compressed ? 214 : 279)) {
        return 0;
    }
    if (!secp256k1_fuzz_der_read_tlv(privkey, privkeylen, &outer_pos, 0x30, outer_len, compressed ? 1 : 2, &outer)
        || outer_pos != privkeylen) {
        return 0;
    }
    if (!secp256k1_fuzz_der_read_tlv(outer, outer_len, &sequence_pos, 0x02, 1, 0, &value) || value[0] != 1) {
        return 0;
    }
    if (!secp256k1_fuzz_der_read_tlv(outer, outer_len, &sequence_pos, 0x04, 32, 0, &value)
        || memcmp(value, seckey32, 32) != 0) {
        return 0;
    }
    if (!secp256k1_fuzz_der_read_tlv(outer, outer_len, &sequence_pos, 0xA0, parameter_len, 1, &parameters)
        || !secp256k1_fuzz_der_read_tlv(parameters, parameter_len, &parameter_pos, 0x30, parameter_sequence_len, 1, &parameter_sequence)
        || parameter_pos != parameter_len) {
        return 0;
    }
    if (!secp256k1_fuzz_der_read_tlv(parameter_sequence, parameter_sequence_len, &parameter_sequence_pos, 0x02, 1, 0, &value) || value[0] != 1) {
        return 0;
    }
    if (!secp256k1_fuzz_der_read_tlv(parameter_sequence, parameter_sequence_len, &parameter_sequence_pos, 0x30, 44, 0, &algorithm)
        || !secp256k1_fuzz_der_read_tlv(algorithm, 44, &algorithm_pos, 0x06, sizeof(curve_oid), 0, &value)
        || memcmp(value, curve_oid, sizeof(curve_oid)) != 0
        || !secp256k1_fuzz_der_read_tlv(algorithm, 44, &algorithm_pos, 0x02, 33, 0, &value)
        || value[0] != 0 || memcmp(value + 1, field_prime, sizeof(field_prime)) != 0
        || algorithm_pos != 44) {
        return 0;
    }
    if (!secp256k1_fuzz_der_read_tlv(parameter_sequence, parameter_sequence_len, &parameter_sequence_pos, 0x30, 6, 0, &curve)
        || !secp256k1_fuzz_der_read_tlv(curve, 6, &curve_pos, 0x04, 1, 0, &value) || value[0] != 0
        || !secp256k1_fuzz_der_read_tlv(curve, 6, &curve_pos, 0x04, 1, 0, &value) || value[0] != 7
        || curve_pos != 6) {
        return 0;
    }
    if (!secp256k1_fuzz_der_read_tlv(parameter_sequence, parameter_sequence_len, &parameter_sequence_pos, 0x04, base_len, 0, &base)
        || memcmp(base, compressed ? generator_compressed : generator_uncompressed, base_len) != 0
        || !secp256k1_fuzz_der_read_tlv(parameter_sequence, parameter_sequence_len, &parameter_sequence_pos, 0x02, 33, 0, &value)
        || value[0] != 0 || memcmp(value + 1, secp256k1_fuzz_scalar_order, 32) != 0
        || !secp256k1_fuzz_der_read_tlv(parameter_sequence, parameter_sequence_len, &parameter_sequence_pos, 0x02, 1, 0, &value)
        || value[0] != 1 || parameter_sequence_pos != parameter_sequence_len) {
        return 0;
    }
    if (!secp256k1_fuzz_der_read_tlv(outer, outer_len, &sequence_pos, 0xA1, public_key_container_len, 0, &public_key_container)
        || sequence_pos != outer_len
        || !secp256k1_fuzz_der_read_tlv(public_key_container, public_key_container_len, &public_key_pos, 0x03, public_key_bits_len, 0, &public_key_bits)
        || public_key_pos != public_key_container_len || public_key_bits[0] != 0) {
        return 0;
    }
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, seckey32)
        || !secp256k1_ec_pubkey_serialize(ctx, expected_pubkey, &serialized_len, &pubkey, compressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED)
        || serialized_len != expected_pubkey_len
        || memcmp(public_key_bits + 1, expected_pubkey, expected_pubkey_len) != 0) {
        return 0;
    }
    return 1;
}

static void secp256k1_fuzz_check_privkey_der(const secp256k1_context *ctx, const unsigned char *seckey32, const unsigned char *input, size_t inputlen) {
    static const unsigned char overflow_len_der[] = { 0x30, 0x82, 0xff, 0xff };
    unsigned char exported[300];
    unsigned char failed_export[300];
    unsigned char expected_failed_export[300];
    unsigned char imported[32];
    unsigned char zero32[32] = { 0 };
    size_t exported_len;
    int compressed;
    int import_ret;

    for (compressed = 0; compressed <= 1; compressed++) {
        memset(exported, 0xA5, sizeof(exported));
        exported_len = sizeof(exported);
        FUZZ_CHECK(ec_privkey_export_der(ctx, exported, &exported_len, seckey32, compressed) == 1);
        FUZZ_CHECK(exported_len <= sizeof(exported));
        FUZZ_CHECK(secp256k1_fuzz_check_privkey_der_structure(ctx, exported, exported_len, seckey32, compressed));
        memset(imported, 0xA5, sizeof(imported));
        FUZZ_CHECK(ec_privkey_import_der(ctx, imported, exported, exported_len) == 1);
        FUZZ_CHECK(memcmp(imported, seckey32, sizeof(imported)) == 0);

        memset(failed_export, 0xA5, sizeof(failed_export));
        memset(expected_failed_export, 0, 279);
        memset(expected_failed_export + 279, 0xA5, sizeof(expected_failed_export) - 279);
        exported_len = sizeof(failed_export);
        FUZZ_CHECK(ec_privkey_export_der(ctx, failed_export, &exported_len, secp256k1_fuzz_scalar_order, compressed) == 0);
        FUZZ_CHECK(exported_len == 0);
        FUZZ_CHECK(memcmp(failed_export, expected_failed_export, sizeof(failed_export)) == 0);
    }

    memset(imported, 0xA5, sizeof(imported));
    import_ret = ec_privkey_import_der(ctx, imported, input, inputlen);
    if (import_ret) {
        FUZZ_CHECK(secp256k1_ec_seckey_verify(ctx, imported) == 1);
    } else {
        FUZZ_CHECK(memcmp(imported, zero32, sizeof(imported)) == 0);
    }

    memset(imported, 0xA5, sizeof(imported));
    FUZZ_CHECK(ec_privkey_import_der(ctx, imported, overflow_len_der, sizeof(overflow_len_der)) == 0);
    FUZZ_CHECK(memcmp(imported, zero32, sizeof(imported)) == 0);
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    unsigned char canonical_32_der[70] = { 0 };
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 1);
    secp256k1_fuzz_ecdsa_nonce_data nonce_data;
    secp256k1_pubkey pubkey;
    secp256k1_pubkey combine_pubkey;
    secp256k1_pubkey combine_pubkey_neg;
    secp256k1_pubkey pubkey_neg;
    secp256k1_pubkey pubkey_neg_from_seckey;
    secp256k1_pubkey sort_pubkeys[4];
    secp256k1_ecdsa_signature sig;
    secp256k1_ecdsa_signature parsed_sig;
    secp256k1_ecdsa_signature checked_sig;
    secp256k1_ecdsa_signature checked_default_sig;
    secp256k1_ecdsa_signature retry_sig;
    unsigned char seckey[32];
    unsigned char combine_seckey[32];
    unsigned char seckey_neg[32];
    unsigned char sort_seckey[32];
    unsigned char tweak32[32];
    unsigned char nonce_extra32[32];
    unsigned char equation_nonce32[32];
    unsigned char zero_compact[64] = { 0 };
    unsigned char sig64[64];
    unsigned char sig_extra64[64];
    unsigned char sig_checked64[64];
    unsigned char sig_retry64[64];
    unsigned char msg32[32];
    const secp256k1_pubkey *sorted_pubkeys[4];
    const secp256k1_pubkey *permuted_pubkeys[4];
    const secp256k1_pubkey *duplicate_pubkeys[4];
    const secp256k1_pubkey *duplicate_permuted_pubkeys[4];
    size_t i;

    canonical_32_der[0] = 0x30;
    canonical_32_der[1] = 0x44;
    canonical_32_der[2] = 0x02;
    canonical_32_der[3] = 0x20;
    canonical_32_der[4] = 0x01;
    canonical_32_der[36] = 0x02;
    canonical_32_der[37] = 0x20;
    canonical_32_der[38] = 0x01;

    secp256k1_fuzz_check_pubkey_parse(ctx, input, 0);
    secp256k1_fuzz_check_pubkey_parse(ctx, input, size);
    secp256k1_fuzz_check_pubkey_parse(ctx, secp256k1_fuzz_pubkey_7g_uncompressed, sizeof(secp256k1_fuzz_pubkey_7g_uncompressed));
    secp256k1_fuzz_check_pubkey_parse_field_overflow(ctx);
    secp256k1_fuzz_check_pubkey_create_failure(ctx, secp256k1_fuzz_scalar_zero);
    secp256k1_fuzz_check_pubkey_create_failure(ctx, secp256k1_fuzz_scalar_order);

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 11);
    secp256k1_fuzz_check_privkey_der(ctx, seckey, input, size);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 17);
    secp256k1_fuzz_derive(nonce_extra32, sizeof(nonce_extra32), input, size, 43);
    secp256k1_fuzz_valid_seckey32(ctx, equation_nonce32, input, size, 53);
    secp256k1_fuzz_check_rfc6979_nonce_failure_cleanup(msg32, seckey, nonce_extra32);
    if (size == sizeof("rfc6979 algorithm domain\n") - 1
        && memcmp(input, "rfc6979 algorithm domain\n", sizeof("rfc6979 algorithm domain\n") - 1) == 0) {
        secp256k1_fuzz_check_rfc6979_algorithm_domain(msg32, seckey, nonce_extra32);
    }
    nonce_data.self = &nonce_data;
    nonce_data.extra32 = nonce_extra32;
    nonce_data.expected_key32 = NULL;
    nonce_data.expected_msg32 = NULL;
    nonce_data.calls = 0;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &pubkey);
    secp256k1_fuzz_check_pubkey_serialize_short_buffer(ctx, &pubkey);
    secp256k1_fuzz_check_pubkey_serialize_flags(ctx, &pubkey);
    if (size == sizeof("pubkey serialize null output\n") - 1
        && memcmp(input, "pubkey serialize null output\n", sizeof("pubkey serialize null output\n") - 1) == 0) {
        secp256k1_fuzz_check_pubkey_serialize_null_output(ctx, &pubkey);
    }
    secp256k1_fuzz_check_null_parser_inputs(ctx);
    secp256k1_fuzz_check_null_pubkey_cmp(ctx, &pubkey);
    secp256k1_fuzz_check_null_tweak_cleanup(ctx, &pubkey, seckey, input, size);

    secp256k1_fuzz_scalar32(tweak32, input, size, 31);
    secp256k1_fuzz_check_tweak_add(ctx, &pubkey, seckey, secp256k1_fuzz_scalar_zero);
    secp256k1_fuzz_check_tweak_add(ctx, &pubkey, seckey, secp256k1_fuzz_scalar_one);
    secp256k1_fuzz_check_tweak_add(ctx, &pubkey, seckey, secp256k1_fuzz_scalar_order);
    secp256k1_fuzz_check_tweak_add(ctx, &pubkey, seckey, tweak32);
    secp256k1_fuzz_check_tweak_mul(ctx, &pubkey, seckey, secp256k1_fuzz_scalar_zero);
    secp256k1_fuzz_check_tweak_mul(ctx, &pubkey, seckey, secp256k1_fuzz_scalar_one);
    secp256k1_fuzz_check_tweak_mul(ctx, &pubkey, seckey, secp256k1_fuzz_scalar_order);
    secp256k1_fuzz_check_tweak_mul(ctx, &pubkey, seckey, tweak32);
    if (size >= 160) {
        secp256k1_fuzz_check_tweak_order_minus_one(ctx, &pubkey, seckey);
    }

    memcpy(seckey_neg, seckey, sizeof(seckey_neg));
    pubkey_neg = pubkey;
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, seckey_neg) == 1);
    secp256k1_fuzz_check_tweak_add(ctx, &pubkey, seckey, seckey_neg);
    FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &pubkey_neg) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_neg_from_seckey, seckey_neg) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey_neg, &pubkey_neg_from_seckey) == 0);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &pubkey_neg);
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, seckey_neg) == 1);
    FUZZ_CHECK(memcmp(seckey, seckey_neg, sizeof(seckey)) == 0);
    FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &pubkey_neg) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &pubkey_neg) == 0);

    secp256k1_fuzz_valid_seckey32(ctx, combine_seckey, input, size, 19);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &combine_pubkey, combine_seckey) == 1);
    combine_pubkey_neg = combine_pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &combine_pubkey_neg) == 1);
    secp256k1_fuzz_check_pubkey_combine(ctx, &pubkey, &pubkey_neg_from_seckey, seckey, &combine_pubkey, combine_seckey);
    secp256k1_fuzz_check_pubkey_combine_three(ctx, &pubkey, seckey, &combine_pubkey, combine_seckey);
    secp256k1_fuzz_check_pubkey_combine_intermediate_infinity(ctx, &pubkey, &pubkey_neg_from_seckey, &combine_pubkey, &combine_pubkey_neg);
    secp256k1_fuzz_check_pubkey_combine_long(ctx, size, &pubkey, seckey, &combine_pubkey, combine_seckey);
    secp256k1_fuzz_check_pubkey_combine_sixteen(ctx, input, size);
    secp256k1_fuzz_check_pubkey_combine_invalid(ctx, &pubkey);
    secp256k1_fuzz_check_pubkey_combine_null_member(ctx, &pubkey);
    secp256k1_fuzz_check_pubkey_combine_empty(ctx, &pubkey);
    secp256k1_fuzz_check_invalid_pubkey_sort(ctx, &pubkey);
    secp256k1_fuzz_check_empty_pubkey_sort(ctx);
    secp256k1_fuzz_check_null_pubkey_sort(ctx);
    secp256k1_fuzz_check_pubkey_sort_long(ctx, size);
    secp256k1_fuzz_check_pubkey_sort_sixteen(ctx, input, size);

    sort_pubkeys[0] = pubkey;
    sort_pubkeys[1] = pubkey_neg_from_seckey;
    secp256k1_fuzz_valid_seckey32(ctx, sort_seckey, input, size, 23);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &sort_pubkeys[2], sort_seckey) == 1);
    secp256k1_fuzz_valid_seckey32(ctx, sort_seckey, input, size, 29);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &sort_pubkeys[3], sort_seckey) == 1);
    sorted_pubkeys[0] = &sort_pubkeys[0];
    sorted_pubkeys[1] = &sort_pubkeys[1];
    sorted_pubkeys[2] = &sort_pubkeys[2];
    sorted_pubkeys[3] = &sort_pubkeys[3];
    permuted_pubkeys[0] = &sort_pubkeys[2];
    permuted_pubkeys[1] = &sort_pubkeys[0];
    permuted_pubkeys[2] = &sort_pubkeys[3];
    permuted_pubkeys[3] = &sort_pubkeys[1];
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, sorted_pubkeys, 4) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, permuted_pubkeys, 4) == 1);
    for (i = 1; i < 4; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, sorted_pubkeys[i - 1], sorted_pubkeys[i]) <= 0);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, permuted_pubkeys[i - 1], permuted_pubkeys[i]) <= 0);
    }
    for (i = 0; i < 4; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, sorted_pubkeys[i], permuted_pubkeys[i]) == 0);
    }
    secp256k1_fuzz_check_pubkey_sort(ctx, sorted_pubkeys);
    duplicate_pubkeys[0] = &sort_pubkeys[2];
    duplicate_pubkeys[1] = &sort_pubkeys[0];
    duplicate_pubkeys[2] = &sort_pubkeys[2];
    duplicate_pubkeys[3] = &sort_pubkeys[1];
    duplicate_permuted_pubkeys[0] = &sort_pubkeys[1];
    duplicate_permuted_pubkeys[1] = &sort_pubkeys[2];
    duplicate_permuted_pubkeys[2] = &sort_pubkeys[0];
    duplicate_permuted_pubkeys[3] = &sort_pubkeys[2];
    secp256k1_fuzz_check_pubkey_sort(ctx, duplicate_pubkeys);
    secp256k1_fuzz_check_pubkey_sort(ctx, duplicate_permuted_pubkeys);
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, duplicate_pubkeys, 4) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, duplicate_permuted_pubkeys, 4) == 1);
    for (i = 0; i < 4; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, duplicate_pubkeys[i], duplicate_permuted_pubkeys[i]) == 0);
    }

    secp256k1_fuzz_ecdsa_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_ecdsa_sha256_compression);
    FUZZ_CHECK(secp256k1_fuzz_ecdsa_sha256_compression_calls != 0);
    secp256k1_fuzz_ecdsa_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ecdsa_sha256_compression_calls != 0);
    nonce_data.extra32 = NULL;
    nonce_data.expected_key32 = seckey;
    nonce_data.expected_msg32 = msg32;
    nonce_data.calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &checked_default_sig, msg32, seckey, secp256k1_fuzz_ecdsa_nonce, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls >= 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, sig64, &sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, sig_checked64, &checked_default_sig) == 1);
    FUZZ_CHECK(memcmp(sig64, sig_checked64, sizeof(sig64)) == 0);
    nonce_data.calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &retry_sig, msg32, seckey, secp256k1_fuzz_ecdsa_nonce_retry, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls >= 4);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, sig_retry64, &retry_sig) == 1);
    FUZZ_CHECK(memcmp(sig64, sig_retry64, sizeof(sig64)) == 0);
    secp256k1_fuzz_check_ecdsa_variable_output_cleanup(ctx, &sig);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &sig, msg32, &pubkey) == 1);
    secp256k1_fuzz_check_ecdsa_signature_state_barrier(ctx, &sig, msg32, &pubkey);
    secp256k1_fuzz_check_ecdsa_invalid_pubkey(ctx, &sig, msg32);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, &sig) == 0);
    secp256k1_fuzz_check_signature_roundtrip(ctx, &sig);
    secp256k1_fuzz_check_ecdsa_high_s(ctx, &sig, msg32, &pubkey);
    if (size >= 320) {
        secp256k1_fuzz_check_ecdsa_normalize_half_order(ctx);
    }
    nonce_data.extra32 = nonce_extra32;
    nonce_data.expected_key32 = seckey;
    nonce_data.expected_msg32 = msg32;
    nonce_data.calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &parsed_sig, msg32, seckey, NULL, nonce_extra32) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &checked_sig, msg32, seckey, secp256k1_fuzz_ecdsa_nonce, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls >= 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, sig_extra64, &parsed_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, sig_checked64, &checked_sig) == 1);
    FUZZ_CHECK(memcmp(sig_extra64, sig_checked64, sizeof(sig_extra64)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &checked_sig, msg32, &pubkey) == 1);
    secp256k1_context_set_sha256_compression(ctx, NULL);
    secp256k1_fuzz_check_ecdsa_r_plus_order(ctx);
    secp256k1_fuzz_check_seckey_negate_failure(ctx);
    secp256k1_fuzz_check_ecdsa_sign_failure_cleanup(ctx, msg32, seckey);
    secp256k1_fuzz_check_ecdsa_sign_null_input_cleanup(ctx, msg32, seckey);
    secp256k1_fuzz_check_ecdsa_invalid_seckey_nonce_domain(ctx);
    secp256k1_fuzz_check_ecdsa_message_reduction(ctx, seckey, &pubkey);
    secp256k1_fuzz_check_ecdsa_fixed_nonce_equation(ctx);
    secp256k1_fuzz_check_ecdsa_variable_nonce_equation(ctx, msg32, seckey, equation_nonce32);
    secp256k1_fuzz_check_ecdsa_retry_after_zero_s(ctx);

    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &parsed_sig, zero_compact) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, zero_compact, &parsed_sig) == 1);
    FUZZ_CHECK(memcmp(zero_compact, secp256k1_fuzz_scalar_zero, sizeof(secp256k1_fuzz_scalar_zero)) == 0);
    FUZZ_CHECK(memcmp(zero_compact + 32, secp256k1_fuzz_scalar_zero, sizeof(secp256k1_fuzz_scalar_zero)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, &pubkey) == 0);

    secp256k1_fuzz_check_signature_parse_compact(ctx, zero_compact, msg32, &pubkey);
    memcpy(sig64, secp256k1_fuzz_scalar_order_minus_one, 32);
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_order_minus_one, 32);
    secp256k1_fuzz_check_signature_parse_compact(ctx, sig64, msg32, &pubkey);
    memcpy(sig64, secp256k1_fuzz_scalar_order, 32);
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_zero, 32);
    secp256k1_fuzz_check_signature_parse_compact(ctx, sig64, msg32, &pubkey);
    memcpy(sig64, secp256k1_fuzz_scalar_zero, 32);
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_order, 32);
    secp256k1_fuzz_check_signature_parse_compact(ctx, sig64, msg32, &pubkey);
    secp256k1_fuzz_derive(sig64, sizeof(sig64), input, size, 37);
    secp256k1_fuzz_check_signature_parse_compact(ctx, sig64, msg32, &pubkey);
    if (size >= 64) {
        secp256k1_fuzz_check_signature_parse_compact(ctx, input, msg32, &pubkey);
    }

    secp256k1_fuzz_check_signature_parse_der_boundary(ctx, secp256k1_fuzz_scalar_zero, secp256k1_fuzz_scalar_one, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_boundary(ctx, secp256k1_fuzz_scalar_one, secp256k1_fuzz_scalar_zero, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_size_boundary(ctx);
    secp256k1_fuzz_check_signature_parse_der_boundary(ctx, secp256k1_fuzz_scalar_order_minus_one, secp256k1_fuzz_scalar_order_minus_one, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_boundary(ctx, secp256k1_fuzz_scalar_order, secp256k1_fuzz_scalar_one, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_boundary(ctx, secp256k1_fuzz_scalar_one, secp256k1_fuzz_scalar_order, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_trailing(ctx, secp256k1_fuzz_scalar_one, secp256k1_fuzz_scalar_one, secp256k1_fuzz_byte(input, size, 47));
    secp256k1_fuzz_check_signature_parse_der_nonminimal(ctx, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_lengths(ctx, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_lax_long_lengths(ctx);
    secp256k1_fuzz_check_signature_parse_der_empty_integer(ctx, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_negative(ctx, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_input(ctx, canonical_32_der, sizeof(canonical_32_der), msg32, &pubkey);
    secp256k1_fuzz_derive(sig64, sizeof(sig64), input, size, 41);
    secp256k1_fuzz_check_signature_parse_der_boundary(ctx, sig64, sig64 + 32, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_trailing(ctx, sig64, sig64 + 32, secp256k1_fuzz_byte(input, size, 49));
    secp256k1_fuzz_check_signature_parse_der_input(ctx, input, size, msg32, &pubkey);

    secp256k1_context_destroy(ctx);
    return 0;
}
