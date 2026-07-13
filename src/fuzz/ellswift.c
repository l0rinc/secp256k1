/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../secp256k1.c"
#include "fuzz.h"
#include "../hash_impl.h"
#include "sha256_reference.h"

#ifdef ENABLE_MODULE_ELLSWIFT
static size_t secp256k1_fuzz_ellswift_sha256_compression_calls = 0;
static int secp256k1_fuzz_ellswift_force_zero_u = 0;
static int secp256k1_fuzz_ellswift_zero_u_forced = 0;

static const unsigned char secp256k1_fuzz_ellswift_field_p_plus_one[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
};

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

static void secp256k1_fuzz_check_ellswift_bip324_hash_reference(const secp256k1_context *ctx, const unsigned char *ell_a64, const unsigned char *ell_b64, const unsigned char *seckey32) {
    static const unsigned char bip324_tag[] = "bip324_ellswift_xonly_ecdh";
    unsigned char shared_x32[32];
    unsigned char taghash[32];
    unsigned char expected[32];
    unsigned char output[32];
    unsigned char prefix64[64];
    unsigned char transcript[224];

    /* Obtain only X through the custom callback, then independently compute
     * the BIP324 transcript with the standalone SHA256 model. */
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, shared_x32, ell_a64, ell_b64, seckey32, 0, secp256k1_fuzz_ellswift_hash_x32, NULL) == 1);
    secp256k1_fuzz_sha256_standalone(taghash, bip324_tag, sizeof(bip324_tag) - 1);
    memcpy(transcript, taghash, sizeof(taghash));
    memcpy(transcript + sizeof(taghash), taghash, sizeof(taghash));
    memcpy(transcript + 2 * sizeof(taghash), ell_a64, 64);
    memcpy(transcript + 2 * sizeof(taghash) + 64, ell_b64, 64);
    memcpy(transcript + 2 * sizeof(taghash) + 128, shared_x32, sizeof(shared_x32));
    secp256k1_fuzz_sha256_standalone(expected, transcript, sizeof(transcript));

    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, seckey32, 0, secp256k1_ellswift_xdh_hash_function_bip324, NULL) == 1);
    FUZZ_CHECK(memcmp(output, expected, sizeof(output)) == 0);

    memcpy(prefix64, taghash, sizeof(taghash));
    memcpy(prefix64 + sizeof(taghash), taghash, sizeof(taghash));
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, seckey32, 0, secp256k1_ellswift_xdh_hash_function_prefix, prefix64) == 1);
    FUZZ_CHECK(memcmp(output, expected, sizeof(output)) == 0);
    memset(taghash, 0, sizeof(taghash));
    memset(transcript, 0, sizeof(transcript));
}

static void secp256k1_fuzz_check_ellswift_decodes_to_pubkey(const secp256k1_context *ctx, const unsigned char *ell64, const secp256k1_pubkey *pubkey) {
    secp256k1_pubkey decoded;

    FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &decoded, ell64) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &decoded, pubkey) == 0);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &decoded);
}

static void secp256k1_fuzz_check_ellswift_randomizer_effect(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *seckey32, const unsigned char *auxrnd32, const unsigned char *rnd32, const unsigned char *ell64, const unsigned char *encoded64) {
    unsigned char auxrnd_alt32[32];
    unsigned char rnd_alt32[32];
    unsigned char ell_alt64[64];
    unsigned char encoded_alt64[64];
    size_t i;

    /* Randomizers are not stable serialization inputs, but eliding either one
     * would remove entropy from an encoding without changing its decoded key. */
    for (i = 0; i < 32; i++) {
        auxrnd_alt32[i] = (unsigned char)~auxrnd32[i];
        rnd_alt32[i] = (unsigned char)~rnd32[i];
    }

    FUZZ_CHECK(secp256k1_ellswift_create(ctx, ell_alt64, seckey32, auxrnd_alt32) == 1);
    FUZZ_CHECK(memcmp(ell64, ell_alt64, sizeof(ell_alt64)) != 0);
    secp256k1_fuzz_check_ellswift_decodes_to_pubkey(ctx, ell_alt64, pubkey);

    FUZZ_CHECK(secp256k1_ellswift_encode(ctx, encoded_alt64, pubkey, rnd_alt32) == 1);
    FUZZ_CHECK(memcmp(encoded64, encoded_alt64, sizeof(encoded_alt64)) != 0);
    secp256k1_fuzz_check_ellswift_decodes_to_pubkey(ctx, encoded_alt64, pubkey);
}

static void secp256k1_fuzz_check_ellswift_inverse_vector(const secp256k1_context *ctx, const unsigned char *ell64, const secp256k1_pubkey *pubkey) {
    secp256k1_ge point;
    secp256k1_fe u;
    secp256k1_fe x;
    secp256k1_fe successful_t[8];
    int successful = 0;
    int c;
    int i;

    /* Check the inverse helper against a fixed vector rather than relying on
     * the encoder to provide both sides of the same relation. */
    FUZZ_CHECK(secp256k1_pubkey_load(ctx, &point, pubkey) == 1);
    x = point.x;
    secp256k1_fe_normalize_var(&x);
    secp256k1_fe_set_b32_mod(&u, ell64);
    secp256k1_fe_normalize_var(&u);
    FUZZ_CHECK(!secp256k1_fe_is_zero(&u));

    for (c = 0; c < 8; c++) {
        secp256k1_fe t;
        secp256k1_fe roundtrip_x;
        int ret;

        ret = secp256k1_ellswift_xswiftec_inv_var(&t, &x, &u, c);
        if (!ret) {
            continue;
        }

        secp256k1_fe_normalize_var(&t);
        FUZZ_CHECK(!secp256k1_fe_is_zero(&t));
        secp256k1_ellswift_xswiftec_var(&roundtrip_x, &u, &t);
        secp256k1_fe_normalize_var(&roundtrip_x);
        FUZZ_CHECK(secp256k1_fe_equal(&x, &roundtrip_x));
        for (i = 0; i < successful; i++) {
            FUZZ_CHECK(!secp256k1_fe_equal(&successful_t[i], &t));
        }
        successful_t[successful++] = t;
    }
    FUZZ_CHECK(successful > 0);
    secp256k1_ge_clear(&point);
}

static void secp256k1_fuzz_check_ellswift_bip324_decode_vector(const secp256k1_context *ctx) {
    /* This vector was generated independently and is part of the BIP324
     * ElligatorSwift decode test set. Do not pin encode/create output: those
     * public APIs explicitly do not guarantee stable encodings across versions. */
    static const unsigned char ell64[64] = {
        0xc5, 0x98, 0x1b, 0xae, 0x27, 0xfd, 0x84, 0x40, 0x1c, 0x72, 0xa1, 0x55, 0xe5, 0x70, 0x7f, 0xbb,
        0x81, 0x1b, 0x2b, 0x62, 0x06, 0x45, 0xd1, 0x02, 0x8e, 0xa2, 0x70, 0xcb, 0xe0, 0xee, 0x22, 0x5d,
        0x4b, 0x62, 0xaa, 0x4d, 0xca, 0x65, 0x06, 0xc1, 0xac, 0xdb, 0xec, 0xc0, 0x55, 0x25, 0x69, 0xb4,
        0xb2, 0x14, 0x36, 0xa5, 0x69, 0x2e, 0x25, 0xd9, 0x0d, 0x3b, 0xc2, 0xeb, 0x7c, 0xe2, 0x40, 0x78
    };
    static const unsigned char expected_compressed[33] = {
        SECP256K1_TAG_PUBKEY_EVEN,
        0x94, 0x8b, 0x40, 0xe7, 0x18, 0x17, 0x13, 0xbc, 0x01, 0x8e, 0xc1, 0x70, 0x2d, 0x3d, 0x05,
        0x4d, 0x15, 0x74, 0x6c, 0x59, 0xa7, 0x02, 0x07, 0x30, 0xdd, 0x13, 0xec, 0xf9, 0x85, 0xa0, 0x10, 0xd7
    };
    secp256k1_pubkey pubkey;
    unsigned char compressed[33];
    size_t compressed_len = sizeof(compressed);

    FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &pubkey, ell64) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, compressed, &compressed_len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(compressed_len == sizeof(compressed));
    FUZZ_CHECK(memcmp(compressed, expected_compressed, sizeof(compressed)) == 0);
    secp256k1_fuzz_check_ellswift_inverse_vector(ctx, ell64, &pubkey);
}

static void secp256k1_fuzz_check_ellswift_modulo_alias(const secp256k1_context *ctx) {
    unsigned char canonical[64] = { 0 };
    unsigned char alias_u[64];
    unsigned char alias_t[64];
    unsigned char alias_both[64];
    unsigned char expected_x[32];
    unsigned char actual_x[32];
    const unsigned char *aliases[3];
    secp256k1_pubkey canonical_pubkey;
    secp256k1_pubkey alias_pubkey;
    size_t i;

    /* EllSwift field inputs are reduced modulo p, so p+1 is an alias for 1. */
    canonical[31] = 1;
    canonical[63] = 1;
    memcpy(alias_u, canonical, sizeof(alias_u));
    memcpy(alias_t, canonical, sizeof(alias_t));
    memcpy(alias_both, canonical, sizeof(alias_both));
    memcpy(alias_u, secp256k1_fuzz_ellswift_field_p_plus_one, 32);
    memcpy(alias_t + 32, secp256k1_fuzz_ellswift_field_p_plus_one, 32);
    memcpy(alias_both, secp256k1_fuzz_ellswift_field_p_plus_one, 32);
    memcpy(alias_both + 32, secp256k1_fuzz_ellswift_field_p_plus_one, 32);
    aliases[0] = alias_u;
    aliases[1] = alias_t;
    aliases[2] = alias_both;

    FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &canonical_pubkey, canonical) == 1);
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, expected_x, canonical, canonical, secp256k1_fuzz_scalar_one, 0, secp256k1_fuzz_ellswift_hash_x32, NULL) == 1);
    for (i = 0; i < sizeof(aliases) / sizeof(aliases[0]); i++) {
        FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &alias_pubkey, aliases[i]) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &canonical_pubkey, &alias_pubkey) == 0);
        FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, actual_x, aliases[i], aliases[i], secp256k1_fuzz_scalar_one, 0, secp256k1_fuzz_ellswift_hash_x32, NULL) == 1);
        FUZZ_CHECK(memcmp(actual_x, expected_x, sizeof(actual_x)) == 0);
    }
}

static void secp256k1_fuzz_check_ellswift_zero_t_parity(const secp256k1_context *ctx) {
    unsigned char zero_t[64] = { 0 };
    unsigned char one_t[64] = { 0 };
    unsigned char zero_t_ser[33];
    unsigned char one_t_ser[33];
    secp256k1_pubkey zero_t_pubkey;
    secp256k1_pubkey one_t_pubkey;
    size_t zero_t_len = sizeof(zero_t_ser);
    size_t one_t_len = sizeof(one_t_ser);

    /* t == 0 is remapped to 1 for the x-coordinate formula only. The original
     * t parity remains the y-parity bit encoded by the wire format. */
    one_t[63] = 1;
    FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &zero_t_pubkey, zero_t) == 1);
    FUZZ_CHECK(secp256k1_ellswift_decode(ctx, &one_t_pubkey, one_t) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, zero_t_ser, &zero_t_len, &zero_t_pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, one_t_ser, &one_t_len, &one_t_pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(zero_t_len == sizeof(zero_t_ser));
    FUZZ_CHECK(one_t_len == sizeof(one_t_ser));
    FUZZ_CHECK(zero_t_ser[0] == SECP256K1_TAG_PUBKEY_EVEN);
    FUZZ_CHECK(one_t_ser[0] == SECP256K1_TAG_PUBKEY_ODD);
    FUZZ_CHECK(memcmp(zero_t_ser + 1, one_t_ser + 1, 32) == 0);
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

static void secp256k1_fuzz_check_ellswift_overflow_secret(const secp256k1_context *ctx, const unsigned char *ell_a64, const unsigned char *ell_b64) {
    unsigned char order_minus_one[32];
    unsigned char order_plus_one[32];
    unsigned char output[32];

    /* n itself reduces to zero, so it does not distinguish overflow tracking
     * from the zero-scalar check. Use n+1, which reduces to one. */
    memcpy(order_minus_one, secp256k1_fuzz_scalar_order, sizeof(order_minus_one));
    order_minus_one[31]--;
    memcpy(order_plus_one, secp256k1_fuzz_scalar_order, sizeof(order_plus_one));
    order_plus_one[31]++;

    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, order_minus_one, 0, secp256k1_fuzz_ellswift_hash_x32, NULL) == 1);
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, order_minus_one, 1, secp256k1_fuzz_ellswift_hash_x32, NULL) == 1);
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, order_plus_one, 0, secp256k1_fuzz_ellswift_hash_x32, NULL) == 0);
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, output, ell_a64, ell_b64, order_plus_one, 1, secp256k1_fuzz_ellswift_hash_x32, NULL) == 0);
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

    secp256k1_fuzz_check_ellswift_bip324_decode_vector(ctx);
    secp256k1_fuzz_check_ellswift_modulo_alias(ctx);
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
    secp256k1_fuzz_check_ellswift_zero_t_parity(ctx);
    secp256k1_fuzz_check_ellswift_zero_u_barrier(ctx);

    FUZZ_CHECK(secp256k1_ellswift_encode(ctx, ell_encoded_a64, &pubkey_a, rnd32) == 1);
    secp256k1_fuzz_check_ellswift_decodes_to_pubkey(ctx, ell_encoded_a64, &pubkey_a);
    secp256k1_fuzz_check_ellswift_randomizer_effect(ctx, &pubkey_a, seckey_a32, auxrnd_a32, rnd32, ell_a64, ell_encoded_a64);
    secp256k1_fuzz_check_ellswift_raw_consistency(ctx, raw_ell_a64, raw_ell_b64, seckey_a32);
    secp256k1_fuzz_check_ellswift_failure_cleanup(ctx, rnd32, auxrnd_a32);

    secp256k1_fuzz_check_ellswift_xdh_pair(ctx, ell_a64, ell_b64, seckey_a32, seckey_b32, secp256k1_fuzz_ellswift_hash_x32, NULL);
    secp256k1_fuzz_check_ellswift_xdh_pair(ctx, ell_a64, ell_b64, seckey_a32, seckey_b32, secp256k1_fuzz_ellswift_hash_masked, &hash_data);
    FUZZ_CHECK(hash_data.calls == 2);
    secp256k1_fuzz_check_ellswift_party_boolean(ctx);
    secp256k1_fuzz_check_ellswift_bip324_hash_reference(ctx, ell_a64, ell_b64, seckey_a32);
    secp256k1_fuzz_check_ellswift_xdh_pair(ctx, ell_a64, ell_b64, seckey_a32, seckey_b32, secp256k1_ellswift_xdh_hash_function_bip324, NULL);
    secp256k1_fuzz_check_ellswift_xdh_pair(ctx, ell_a64, ell_b64, seckey_a32, seckey_b32, secp256k1_ellswift_xdh_hash_function_prefix, prefix64);

    hash_data.calls = 0;
    FUZZ_CHECK(secp256k1_ellswift_xdh(ctx, fail_output, ell_a64, ell_b64, seckey_a32, 0, secp256k1_fuzz_ellswift_hash_fail, &hash_data) == 0);
    FUZZ_CHECK(hash_data.calls == 1);

    secp256k1_fuzz_check_ellswift_built_in_cleanup(ctx, ell_a64, ell_b64, seckey_a32, prefix64);
    secp256k1_fuzz_check_ellswift_overflow_secret(ctx, ell_a64, ell_b64);
    secp256k1_fuzz_check_ellswift_ctx_hash(ctx, ell_a64, ell_b64, seckey_a32, prefix64);

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
