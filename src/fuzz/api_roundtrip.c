/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"
#include "../../contrib/lax_der_parsing.c"
#include "../../contrib/lax_der_privatekey_parsing.c"

static size_t secp256k1_fuzz_ecdsa_sha256_compression_calls = 0;

static const unsigned char secp256k1_fuzz_field_p_plus_one[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
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
    unsigned int calls;
} secp256k1_fuzz_ecdsa_nonce_data;

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_api_illegal_data;

typedef int (*secp256k1_fuzz_ecdsa_serialize_der_fn)(const secp256k1_context *ctx, unsigned char *output, size_t *outputlen, const secp256k1_ecdsa_signature *sig);
typedef int (*secp256k1_fuzz_ecdsa_normalize_fn)(const secp256k1_context *ctx, secp256k1_ecdsa_signature *sigout, const secp256k1_ecdsa_signature *sigin);
typedef int (*secp256k1_fuzz_ec_seckey_tweak_fn)(const secp256k1_context *ctx, unsigned char *seckey, const unsigned char *tweak32);
typedef int (*secp256k1_fuzz_ec_pubkey_tweak_fn)(const secp256k1_context *ctx, secp256k1_pubkey *pubkey, const unsigned char *tweak32);

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

static const unsigned char *secp256k1_fuzz_runtime_null_tweak(const unsigned char *input, size_t size) {
    return size == (size_t)-1 ? input : NULL;
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

static void secp256k1_fuzz_check_pubkey_parse(const secp256k1_context *ctx, const unsigned char *input, size_t inputlen) {
    secp256k1_pubkey parsed_pubkey;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };

    memset(&parsed_pubkey, 0xA5, sizeof(parsed_pubkey));
    if (secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, input, inputlen)) {
        secp256k1_fuzz_check_pubkey_roundtrip(ctx, &parsed_pubkey);
    } else {
        FUZZ_CHECK(memcmp(&parsed_pubkey, zero_pubkey, sizeof(parsed_pubkey)) == 0);
    }
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
    nonce_data.calls = 0;
    memset(&sig, 0xA5, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, valid_seckey32, secp256k1_fuzz_ecdsa_nonce_fail, &nonce_data) == 0);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
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
    if (!parsed_lax) {
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &lax_sig, msg32, pubkey) == 0);
    }
    if (parsed_der) {
        FUZZ_CHECK(parsed_lax == 1);
        FUZZ_CHECK(memcmp(lax_compact, compact, sizeof(lax_compact)) == 0);
    }
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

static void secp256k1_fuzz_check_privkey_der(const secp256k1_context *ctx, const unsigned char *seckey32, const unsigned char *input, size_t inputlen) {
    static const unsigned char overflow_len_der[] = { 0x30, 0x82, 0xff, 0xff };
    unsigned char exported[300];
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
        memset(imported, 0xA5, sizeof(imported));
        FUZZ_CHECK(ec_privkey_import_der(ctx, imported, exported, exported_len) == 1);
        FUZZ_CHECK(memcmp(imported, seckey32, sizeof(imported)) == 0);
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
    secp256k1_fuzz_check_pubkey_parse_field_overflow(ctx);
    secp256k1_fuzz_check_pubkey_create_failure(ctx, secp256k1_fuzz_scalar_zero);
    secp256k1_fuzz_check_pubkey_create_failure(ctx, secp256k1_fuzz_scalar_order);

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 11);
    secp256k1_fuzz_check_privkey_der(ctx, seckey, input, size);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 17);
    secp256k1_fuzz_derive(nonce_extra32, sizeof(nonce_extra32), input, size, 43);
    secp256k1_fuzz_check_rfc6979_nonce_failure_cleanup(msg32, seckey, nonce_extra32);
    nonce_data.self = &nonce_data;
    nonce_data.extra32 = nonce_extra32;
    nonce_data.calls = 0;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &pubkey);
    secp256k1_fuzz_check_pubkey_serialize_short_buffer(ctx, &pubkey);
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
    secp256k1_fuzz_check_pubkey_combine(ctx, &pubkey, &pubkey_neg_from_seckey, seckey, &combine_pubkey, combine_seckey);
    secp256k1_fuzz_check_pubkey_combine_invalid(ctx, &pubkey);
    secp256k1_fuzz_check_invalid_pubkey_sort(ctx, &pubkey);

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
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, &sig) == 0);
    secp256k1_fuzz_check_signature_roundtrip(ctx, &sig);
    secp256k1_fuzz_check_ecdsa_high_s(ctx, &sig, msg32, &pubkey);
    nonce_data.extra32 = nonce_extra32;
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
    secp256k1_fuzz_check_ecdsa_message_reduction(ctx, seckey, &pubkey);

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
