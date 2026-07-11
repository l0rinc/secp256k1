/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"

#ifdef ENABLE_MODULE_ELLSWIFT
static size_t secp256k1_fuzz_ellswift_sha256_compression_calls = 0;
static int secp256k1_fuzz_ellswift_force_zero_u = 0;
static int secp256k1_fuzz_ellswift_zero_u_forced = 0;

typedef struct {
    const void *self;
    unsigned char mask32[32];
    unsigned int calls;
} secp256k1_fuzz_ellswift_hash_data;

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_ellswift_illegal_data;

static void secp256k1_fuzz_ellswift_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    secp256k1_fuzz_ellswift_sha256_compression_calls += n_blocks;
    secp256k1_sha256_transform(state, blocks64, n_blocks);
    if (secp256k1_fuzz_ellswift_force_zero_u && secp256k1_fuzz_ellswift_sha256_compression_calls == 3) {
        secp256k1_fuzz_ellswift_zero_u_forced = 1;
        memset(state, 0, 8 * sizeof(*state));
    }
}

static void secp256k1_fuzz_ellswift_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_ellswift_illegal_data *illegal_data = (secp256k1_fuzz_ellswift_illegal_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(illegal_data != NULL);
    FUZZ_CHECK(illegal_data->self == illegal_data);
    illegal_data->calls++;
}

static int secp256k1_fuzz_ellswift_hash_x32(unsigned char *output, const unsigned char *x32, const unsigned char *ell_a64, const unsigned char *ell_b64, void *data) {
    (void)ell_a64;
    (void)ell_b64;
    (void)data;
    memcpy(output, x32, 32);
    return 1;
}

static int secp256k1_fuzz_ellswift_hash_masked(unsigned char *output, const unsigned char *x32, const unsigned char *ell_a64, const unsigned char *ell_b64, void *data) {
    secp256k1_fuzz_ellswift_hash_data *hash_data = (secp256k1_fuzz_ellswift_hash_data *)data;
    size_t i;

    FUZZ_CHECK(output != NULL);
    FUZZ_CHECK(x32 != NULL);
    FUZZ_CHECK(ell_a64 != NULL);
    FUZZ_CHECK(ell_b64 != NULL);
    FUZZ_CHECK(hash_data != NULL);
    FUZZ_CHECK(hash_data->self == hash_data);
    hash_data->calls++;
    for (i = 0; i < 32; i++) {
        output[i] = (unsigned char)(x32[i] ^ hash_data->mask32[i] ^ ell_a64[i] ^ ell_b64[63 - i]);
    }
    return 1;
}

static int secp256k1_fuzz_ellswift_hash_fail(unsigned char *output, const unsigned char *x32, const unsigned char *ell_a64, const unsigned char *ell_b64, void *data) {
    secp256k1_fuzz_ellswift_hash_data *hash_data = (secp256k1_fuzz_ellswift_hash_data *)data;

    (void)output;
    (void)x32;
    (void)ell_a64;
    (void)ell_b64;
    FUZZ_CHECK(hash_data != NULL);
    FUZZ_CHECK(hash_data->self == hash_data);
    hash_data->calls++;
    return 0;
}

static void secp256k1_fuzz_check_ellswift_decodes_to_pubkey(const secp256k1_context *ctx, const unsigned char *ell64, const secp256k1_pubkey *pubkey) {
    secp256k1_pubkey decoded;

    FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &decoded, ell64) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &decoded, pubkey) == 0);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &decoded);
}

static void secp256k1_fuzz_check_ellswift_zero_u_barrier(secp256k1_context *ctx) {
    unsigned char zero32[32] = { 0 };
    unsigned char ell64[64];
    secp256k1_pubkey expected;
    secp256k1_pubkey decoded;

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &expected, secp256k1_fuzz_scalar_one) == 1);
    secp256k1_fuzz_ellswift_force_zero_u = 0;
    secp256k1_fuzz_ellswift_zero_u_forced = 0;
    secp256k1_fuzz_ellswift_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_ellswift_sha256_compression);
    secp256k1_fuzz_ellswift_sha256_compression_calls = 0;
    secp256k1_fuzz_ellswift_force_zero_u = 1;
    FUZZ_CHECK(secp256k1_ellswift_create(ctx, ell64, secp256k1_fuzz_scalar_one, NULL) == 1);
    secp256k1_fuzz_ellswift_force_zero_u = 0;
    secp256k1_context_set_sha256_compression(ctx, NULL);

    FUZZ_CHECK(secp256k1_fuzz_ellswift_zero_u_forced == 1);
    FUZZ_CHECK(secp256k1_fuzz_ellswift_sha256_compression_calls >= 3);
    FUZZ_CHECK(memcmp(ell64, zero32, sizeof(zero32)) != 0);
    FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &decoded, ell64) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &decoded, &expected) == 0);
}

static void secp256k1_fuzz_check_ellswift_raw_consistency(const secp256k1_context *ctx, const unsigned char *ell_a64, const unsigned char *ell_b64, const unsigned char *seckey32) {
    secp256k1_pubkey decoded_a;
    secp256k1_pubkey decoded_b;
    secp256k1_pubkey shared_pubkey;
    unsigned char shared_x32[32];
    unsigned char serialized[65];
    size_t serialized_len = sizeof(serialized);

    /* Decode arbitrary wire encodings, then cross-check the independent
     * fraction-based XDH path against ordinary public-key multiplication. */
    FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &decoded_a, ell_a64) == 1);
    FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &decoded_b, ell_b64) == 1);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &decoded_a);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &decoded_b);

    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, shared_x32, ell_a64, ell_b64, seckey32, 0, secp256k1_fuzz_ellswift_hash_x32, NULL) == 1);
    shared_pubkey = decoded_b;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &shared_pubkey, seckey32) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized, &serialized_len, &shared_pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(serialized_len == sizeof(serialized));
    FUZZ_CHECK(memcmp(shared_x32, serialized + 1, 32) == 0);
}

static void secp256k1_fuzz_check_ellswift_failure_cleanup(secp256k1_context *ctx, const unsigned char *rnd32, const unsigned char *auxrnd32) {
    secp256k1_fuzz_ellswift_illegal_data illegal_data;
    secp256k1_pubkey invalid_pubkey;
    secp256k1_pubkey decoded_pubkey;
    unsigned char ell64[64];
    unsigned char zero64[64] = { 0 };
    unsigned char zero_pubkey[sizeof(decoded_pubkey)] = { 0 };
    int (*encode)(const secp256k1_context *, unsigned char *, const secp256k1_pubkey *, const unsigned char *) = secp256k1_ellswift_encode;
    int (*decode)(const secp256k1_context *, secp256k1_pubkey *, const unsigned char *) = secp256k1_ellswift_decode;
    unsigned int calls;

    /* Failed fixed-size output APIs must not leave attacker-controlled or stale
     * bytes for callers that inspect the output after a failed return. */
    memset(&invalid_pubkey, 0, sizeof(invalid_pubkey));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_ellswift_illegal_callback, &illegal_data);

    memset(ell64, 0xA5, sizeof(ell64));
    calls = illegal_data.calls;
    FUZZ_CHECK(encode(ctx, ell64, NULL, rnd32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(ell64, zero64, sizeof(ell64)) == 0);

    memset(ell64, 0xA5, sizeof(ell64));
    calls = illegal_data.calls;
    FUZZ_CHECK(encode(ctx, ell64, &invalid_pubkey, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(ell64, zero64, sizeof(ell64)) == 0);

    memset(ell64, 0xA5, sizeof(ell64));
    calls = illegal_data.calls;
    FUZZ_CHECK(encode(ctx, ell64, &invalid_pubkey, rnd32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(ell64, zero64, sizeof(ell64)) == 0);

    memset(&decoded_pubkey, 0xA5, sizeof(decoded_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(decode(ctx, &decoded_pubkey, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&decoded_pubkey, zero_pubkey, sizeof(decoded_pubkey)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
    memset(ell64, 0xA5, sizeof(ell64));
    FUZZ_CHECK(secp256k1_ellswift_create(ctx, ell64, secp256k1_fuzz_scalar_zero, auxrnd32) == 0);
    FUZZ_CHECK(memcmp(ell64, zero64, sizeof(ell64)) == 0);

    memset(ell64, 0xA5, sizeof(ell64));
    FUZZ_CHECK(secp256k1_ellswift_create(ctx, ell64, secp256k1_fuzz_scalar_order, NULL) == 0);
    FUZZ_CHECK(memcmp(ell64, zero64, sizeof(ell64)) == 0);
}

static void secp256k1_fuzz_check_ellswift_xdh_pair(const secp256k1_context *ctx, const unsigned char *ell_a64, const unsigned char *ell_b64, const unsigned char *seckey_a32, const unsigned char *seckey_b32, secp256k1_ellswift_xdh_hash_function hashfp, void *hash_data) {
    unsigned char shared_a[32];
    unsigned char shared_b[32];

    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, shared_a, ell_a64, ell_b64, seckey_a32, 0, hashfp, hash_data) == 1);
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, shared_b, ell_a64, ell_b64, seckey_b32, 1, hashfp, hash_data) == 1);
    FUZZ_CHECK(memcmp(shared_a, shared_b, sizeof(shared_a)) == 0);
}

static void secp256k1_fuzz_check_ellswift_party_boolean(const secp256k1_context *ctx) {
    unsigned char seckey_a32[32] = { 0 };
    unsigned char seckey_b32[32] = { 0 };
    unsigned char ell_a64[64];
    unsigned char ell_b64[64];
    unsigned char party_one[32];
    unsigned char party_two[32];
    unsigned char party_negative[32];

    /* The API documents party as zero for A and nonzero for B. Keep several
     * nonzero encodings in the oracle so an implementation cannot silently
     * narrow that contract to the literal value 1. */
    seckey_a32[31] = 1;
    seckey_b32[31] = 2;
    FUZZ_CHECK(secp256k1_ellswift_create(ctx, ell_a64, seckey_a32, NULL) == 1);
    FUZZ_CHECK(secp256k1_ellswift_create(ctx, ell_b64, seckey_b32, NULL) == 1);
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, party_one, ell_a64, ell_b64, seckey_b32, 1, secp256k1_fuzz_ellswift_hash_x32, NULL) == 1);
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, party_two, ell_a64, ell_b64, seckey_b32, 2, secp256k1_fuzz_ellswift_hash_x32, NULL) == 1);
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, party_negative, ell_a64, ell_b64, seckey_b32, -1, secp256k1_fuzz_ellswift_hash_x32, NULL) == 1);
    FUZZ_CHECK(memcmp(party_one, party_two, sizeof(party_one)) == 0);
    FUZZ_CHECK(memcmp(party_one, party_negative, sizeof(party_one)) == 0);
}

static void secp256k1_fuzz_check_ellswift_built_in_cleanup(secp256k1_context *ctx, const unsigned char *ell_a64, const unsigned char *ell_b64, const unsigned char *seckey32, const unsigned char *prefix64) {
    secp256k1_fuzz_ellswift_illegal_data illegal_data;
    unsigned char output[32];
    unsigned char zero32[32] = { 0 };
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_ellswift_illegal_callback, &illegal_data);
    memset(output, 0xA5, sizeof(output));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, seckey32, 0, secp256k1_ellswift_xdh_hash_function_prefix, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(output, zero32, sizeof(output)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);

    memset(output, 0xA5, sizeof(output));
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, secp256k1_fuzz_scalar_zero, 0, secp256k1_ellswift_xdh_hash_function_bip324, NULL) == 0);
    FUZZ_CHECK(memcmp(output, zero32, sizeof(output)) == 0);

    memset(output, 0xA5, sizeof(output));
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, secp256k1_fuzz_scalar_order, 0, secp256k1_ellswift_xdh_hash_function_prefix, (void *)prefix64) == 0);
    FUZZ_CHECK(memcmp(output, zero32, sizeof(output)) == 0);

    memset(output, 0xA5, sizeof(output));
    FUZZ_CHECK(secp256k1_ellswift_xdh_hash_function_bip324(output, NULL, ell_a64, ell_b64, NULL) == 0);
    FUZZ_CHECK(memcmp(output, zero32, sizeof(output)) == 0);

    memset(output, 0xA5, sizeof(output));
    FUZZ_CHECK(secp256k1_ellswift_xdh_hash_function_prefix(output, secp256k1_fuzz_scalar_one, ell_a64, ell_b64, NULL) == 0);
    FUZZ_CHECK(memcmp(output, zero32, sizeof(output)) == 0);
}

static void secp256k1_fuzz_check_ellswift_ctx_hash(secp256k1_context *ctx, const unsigned char *ell_a64, const unsigned char *ell_b64, const unsigned char *seckey32, const unsigned char *prefix64) {
    unsigned char output[32];

    secp256k1_fuzz_ellswift_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_ellswift_sha256_compression);
    FUZZ_CHECK(secp256k1_fuzz_ellswift_sha256_compression_calls != 0);

    secp256k1_fuzz_ellswift_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, seckey32, 0, secp256k1_ellswift_xdh_hash_function_bip324, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ellswift_sha256_compression_calls != 0);

    secp256k1_fuzz_ellswift_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, seckey32, 0, secp256k1_ellswift_xdh_hash_function_prefix, (void *)prefix64) == 1);
    FUZZ_CHECK(secp256k1_fuzz_ellswift_sha256_compression_calls != 0);

    secp256k1_context_set_sha256_compression(ctx, NULL);
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_ELLSWIFT
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 173);
    secp256k1_fuzz_ellswift_hash_data hash_data;
    unsigned char seckey_a32[32];
    unsigned char seckey_b32[32];
    unsigned char auxrnd_a32[32];
    unsigned char auxrnd_b32[32];
    unsigned char rnd32[32];
    unsigned char prefix64[64];
    unsigned char raw_ell_a64[64];
    unsigned char raw_ell_b64[64];
    unsigned char ell_a64[64];
    unsigned char ell_b64[64];
    unsigned char ell_encoded_a64[64];
    unsigned char ell_no_aux64[64];
    unsigned char fail_output[32];
    secp256k1_pubkey pubkey_a;
    secp256k1_pubkey pubkey_b;

    secp256k1_fuzz_valid_seckey32(ctx, seckey_a32, input, size, 179);
    secp256k1_fuzz_valid_seckey32(ctx, seckey_b32, input, size, 181);
    secp256k1_fuzz_derive(auxrnd_a32, sizeof(auxrnd_a32), input, size, 191);
    secp256k1_fuzz_derive(auxrnd_b32, sizeof(auxrnd_b32), input, size, 193);
    secp256k1_fuzz_derive(rnd32, sizeof(rnd32), input, size, 197);
    secp256k1_fuzz_derive(prefix64, sizeof(prefix64), input, size, 199);
    secp256k1_fuzz_derive(raw_ell_a64, sizeof(raw_ell_a64), input, size, 223);
    secp256k1_fuzz_derive(raw_ell_b64, sizeof(raw_ell_b64), input, size, 227);
    secp256k1_fuzz_derive(hash_data.mask32, sizeof(hash_data.mask32), input, size, 211);
    hash_data.self = &hash_data;
    hash_data.calls = 0;

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_a, seckey_a32) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_b, seckey_b32) == 1);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &pubkey_a);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &pubkey_b);

    FUZZ_CHECK(secp256k1_ellswift_create(ctx, ell_a64, seckey_a32, auxrnd_a32) == 1);
    secp256k1_fuzz_check_ellswift_decodes_to_pubkey(ctx, ell_a64, &pubkey_a);
    FUZZ_CHECK(secp256k1_ellswift_create(ctx, ell_b64, seckey_b32, auxrnd_b32) == 1);
    secp256k1_fuzz_check_ellswift_decodes_to_pubkey(ctx, ell_b64, &pubkey_b);
    FUZZ_CHECK(secp256k1_ellswift_create(ctx, ell_no_aux64, seckey_a32, NULL) == 1);
    secp256k1_fuzz_check_ellswift_decodes_to_pubkey(ctx, ell_no_aux64, &pubkey_a);
    secp256k1_fuzz_check_ellswift_zero_u_barrier(ctx);

    FUZZ_CHECK(secp256k1_ellswift_encode(ctx, ell_encoded_a64, &pubkey_a, rnd32) == 1);
    secp256k1_fuzz_check_ellswift_decodes_to_pubkey(ctx, ell_encoded_a64, &pubkey_a);
    secp256k1_fuzz_check_ellswift_raw_consistency(ctx, raw_ell_a64, raw_ell_b64, seckey_a32);
    secp256k1_fuzz_check_ellswift_failure_cleanup(ctx, rnd32, auxrnd_a32);

    secp256k1_fuzz_check_ellswift_xdh_pair(ctx, ell_a64, ell_b64, seckey_a32, seckey_b32, secp256k1_fuzz_ellswift_hash_x32, NULL);
    secp256k1_fuzz_check_ellswift_xdh_pair(ctx, ell_a64, ell_b64, seckey_a32, seckey_b32, secp256k1_fuzz_ellswift_hash_masked, &hash_data);
    FUZZ_CHECK(hash_data.calls == 2);
    secp256k1_fuzz_check_ellswift_party_boolean(ctx);
    secp256k1_fuzz_check_ellswift_xdh_pair(ctx, ell_a64, ell_b64, seckey_a32, seckey_b32, secp256k1_ellswift_xdh_hash_function_bip324, NULL);
    secp256k1_fuzz_check_ellswift_xdh_pair(ctx, ell_a64, ell_b64, seckey_a32, seckey_b32, secp256k1_ellswift_xdh_hash_function_prefix, prefix64);

    hash_data.calls = 0;
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, fail_output, ell_a64, ell_b64, seckey_a32, 0, secp256k1_fuzz_ellswift_hash_fail, &hash_data) == 0);
    FUZZ_CHECK(hash_data.calls == 1);

    secp256k1_fuzz_check_ellswift_built_in_cleanup(ctx, ell_a64, ell_b64, seckey_a32, prefix64);
    secp256k1_fuzz_check_ellswift_ctx_hash(ctx, ell_a64, ell_b64, seckey_a32, prefix64);

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
