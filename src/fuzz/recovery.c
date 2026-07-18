/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"

#ifdef ENABLE_MODULE_RECOVERY
static size_t secp256k1_fuzz_recovery_sha256_compression_calls = 0;

static const unsigned char secp256k1_fuzz_recovery_p_minus_order_plus_x_for_y_one[32] = {
    0x14, 0x6D, 0x3B, 0x65, 0xAD, 0xD9, 0xF5, 0x4C,
    0xCC, 0xA2, 0x85, 0x33, 0xC8, 0x8E, 0x2C, 0xBD,
    0xA9, 0x48, 0x67, 0x57, 0x67, 0x0F, 0xD7, 0xFE,
    0xF4, 0x4D, 0x30, 0x6B, 0xAB, 0xF3, 0xCB, 0xA3
};

static void secp256k1_fuzz_recovery_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    secp256k1_fuzz_recovery_sha256_compression_calls += n_blocks;
    secp256k1_sha256_transform(state, blocks64, n_blocks);
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

typedef struct {
    const void *self;
    const unsigned char *expected_msg32;
    const unsigned char *expected_key32;
    unsigned int calls;
} secp256k1_fuzz_recovery_nonce_data;

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_recovery_illegal_data;

static void secp256k1_fuzz_recovery_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_recovery_illegal_data *illegal_data = (secp256k1_fuzz_recovery_illegal_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(illegal_data != NULL);
    FUZZ_CHECK(illegal_data->self == illegal_data);
    illegal_data->calls++;
}

static void secp256k1_fuzz_recovery_nonce_check_domain(const secp256k1_fuzz_recovery_nonce_data *nonce_data, const unsigned char *msg32, const unsigned char *key32) {
    FUZZ_CHECK(nonce_data->expected_msg32 != NULL);
    FUZZ_CHECK(nonce_data->expected_key32 != NULL);
    FUZZ_CHECK(memcmp(msg32, nonce_data->expected_msg32, 32) == 0);
    FUZZ_CHECK(memcmp(key32, nonce_data->expected_key32, 32) == 0);
}

/* For z = 0, r = s = x(G), and recid = 0, the ECDSA recovery equation is
 * Q = r^-1 * (s*G - z*G) = G. Keep the expected point as fixed wire data so
 * this check does not share the production tweak/combine path below. */
static void secp256k1_fuzz_check_recovery_generator_vector(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "recovery-generator-vector\n";
    static const unsigned char generator_compressed[33] = {
        SECP256K1_TAG_PUBKEY_EVEN,
        0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC,
        0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B, 0x07,
        0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9,
        0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
    };
    unsigned char compact[64];
    unsigned char compact_roundtrip[64];
    unsigned char recovered_compressed[33];
    unsigned char zero32[32] = { 0 };
    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_ecdsa_recoverable_signature reparsed_sig;
    secp256k1_pubkey recovered_pubkey;
    size_t recovered_len = sizeof(recovered_compressed);
    int recid;
    int roundtrip_recid;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    memcpy(compact, generator_compressed + 1, 32);
    memcpy(compact + 32, generator_compressed + 1, 32);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &sig, compact, 0) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &sig, zero32) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, recovered_compressed, &recovered_len, &recovered_pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(recovered_len == sizeof(recovered_compressed));
    FUZZ_CHECK(memcmp(recovered_compressed, generator_compressed, sizeof(recovered_compressed)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact_roundtrip, &recid, &sig) == 1);
    FUZZ_CHECK(recid == 0);
    FUZZ_CHECK(memcmp(compact_roundtrip, compact, sizeof(compact_roundtrip)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, compact_roundtrip, recid) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact_roundtrip, &roundtrip_recid, &reparsed_sig) == 1);
    FUZZ_CHECK(roundtrip_recid == recid);
    FUZZ_CHECK(memcmp(compact_roundtrip, compact, sizeof(compact_roundtrip)) == 0);
}

static void secp256k1_fuzz_check_static_context_recovery(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "static context recovery barrier\n";
    static const unsigned char generator_compressed[33] = {
        SECP256K1_TAG_PUBKEY_EVEN,
        0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC,
        0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B, 0x07,
        0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9,
        0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
    };
    const secp256k1_context *contexts[2];
    unsigned char compact[64];
    unsigned char expected_compact[64];
    unsigned char zero32[32] = { 0 };
    unsigned char serialized_pubkeys[2][33];
    unsigned char serialized_normal[2][64];
#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
    secp256k1_ecdsa_recoverable_signature static_sig;
    unsigned char zero_static_sig[sizeof(static_sig)] = { 0 };
    unsigned int calls;
#endif
    secp256k1_ecdsa_recoverable_signature recoverable[2];
    secp256k1_ecdsa_signature normal[2];
    secp256k1_pubkey pubkeys[2];
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    contexts[0] = ctx;
    contexts[1] = secp256k1_context_static;
    memcpy(compact, generator_compressed + 1, 32);
    memcpy(compact + 32, generator_compressed + 1, 32);
    memcpy(expected_compact, compact, sizeof(expected_compact));
    for (i = 0; i < sizeof(contexts) / sizeof(contexts[0]); i++) {
        size_t pubkey_len = sizeof(serialized_pubkeys[i]);
        int recid;

        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(contexts[i], &recoverable[i], compact, 0) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(contexts[i], compact, &recid, &recoverable[i]) == 1);
        FUZZ_CHECK(recid == 0);
        FUZZ_CHECK(memcmp(compact, expected_compact, sizeof(compact)) == 0);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(contexts[i], &normal[i], &recoverable[i]) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(contexts[i], serialized_normal[i], &normal[i]) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_recover(contexts[i], &pubkeys[i], &recoverable[i], zero32) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(contexts[i], serialized_pubkeys[i], &pubkey_len, &pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(pubkey_len == sizeof(serialized_pubkeys[i]));
        FUZZ_CHECK(memcmp(serialized_pubkeys[i], generator_compressed, sizeof(generator_compressed)) == 0);
        FUZZ_CHECK(secp256k1_ecdsa_verify(contexts[i], &normal[i], zero32, &pubkeys[i]) == 1);
    }
    FUZZ_CHECK(memcmp(serialized_normal[0], serialized_normal[1], sizeof(serialized_normal[0])) == 0);
    FUZZ_CHECK(memcmp(serialized_pubkeys[0], serialized_pubkeys[1], sizeof(serialized_pubkeys[0])) == 0);

#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
    /* Recovery, conversion, and verification are public-data paths. Recoverable
     * signing still requires generator precomputation and must reject the static
     * singleton without leaking stale output bytes. */
    calls = secp256k1_fuzz_default_illegal_calls;
    memset(&static_sig, 0xA5, sizeof(static_sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(secp256k1_context_static, &static_sig, zero32, secp256k1_fuzz_scalar_one, NULL, NULL) == 0);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == calls + 1);
    FUZZ_CHECK(memcmp(&static_sig, zero_static_sig, sizeof(static_sig)) == 0);
#endif
}

static int secp256k1_fuzz_recovery_nonce_retry(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_recovery_nonce_data *nonce_data = (secp256k1_fuzz_recovery_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    secp256k1_fuzz_recovery_nonce_check_domain(nonce_data, msg32, key32);
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
    return secp256k1_nonce_function_rfc6979(nonce32, msg32, key32, algo16, NULL, attempt - 3);
}

static int secp256k1_fuzz_recovery_nonce_s_zero_then_two(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_recovery_nonce_data *nonce_data = (secp256k1_fuzz_recovery_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    secp256k1_fuzz_recovery_nonce_check_domain(nonce_data, msg32, key32);
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

static int secp256k1_fuzz_recovery_nonce_one(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_recovery_nonce_data *nonce_data = (secp256k1_fuzz_recovery_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    secp256k1_fuzz_recovery_nonce_check_domain(nonce_data, msg32, key32);
    FUZZ_CHECK(algo16 == NULL);
    FUZZ_CHECK(attempt == 0);
    nonce_data->calls++;
    memcpy(nonce32, secp256k1_fuzz_scalar_one, 32);
    return 1;
}

/* The recoverable signer has a 65-byte output, including recid. Check exact
 * message/output and seckey/output aliases against fixed wire data so an
 * early output write cannot be masked by the same signing implementation. */
static void secp256k1_fuzz_check_sign_recoverable_input_output_alias(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "recoverable sign input-output overlap\n";
    static const unsigned char generator_x32[32] = {
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac,
        0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98
    };
    static const unsigned char zero32[32] = { 0 };
    union {
        secp256k1_ecdsa_recoverable_signature sig;
        unsigned char bytes[sizeof(secp256k1_ecdsa_recoverable_signature)];
    } alias;
    secp256k1_fuzz_recovery_nonce_data nonce_data;
    unsigned char compact[64];
    int recid;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    nonce_data.self = &nonce_data;
    nonce_data.expected_msg32 = zero32;
    nonce_data.expected_key32 = secp256k1_fuzz_scalar_one;
    nonce_data.calls = 0;

    /* With z = 0 and d = k = 1, r = s = x(G) and the recovery id is zero. */
    memset(&alias, 0xA5, sizeof(alias));
    memcpy(alias.bytes, zero32, sizeof(zero32));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &alias.sig, alias.bytes, secp256k1_fuzz_scalar_one, secp256k1_fuzz_recovery_nonce_one, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &alias.sig) == 1);
    FUZZ_CHECK(recid == 0);
    FUZZ_CHECK(memcmp(compact, generator_x32, sizeof(generator_x32)) == 0);
    FUZZ_CHECK(memcmp(compact + 32, generator_x32, sizeof(generator_x32)) == 0);

    nonce_data.calls = 0;
    memset(&alias, 0xA5, sizeof(alias));
    memcpy(alias.bytes, secp256k1_fuzz_scalar_one, sizeof(secp256k1_fuzz_scalar_one));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &alias.sig, zero32, alias.bytes, secp256k1_fuzz_recovery_nonce_one, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &alias.sig) == 1);
    FUZZ_CHECK(recid == 0);
    FUZZ_CHECK(memcmp(compact, generator_x32, sizeof(generator_x32)) == 0);
    FUZZ_CHECK(memcmp(compact + 32, generator_x32, sizeof(generator_x32)) == 0);
}

static int secp256k1_fuzz_recovery_nonce_fail(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_recovery_nonce_data *nonce_data = (secp256k1_fuzz_recovery_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    secp256k1_fuzz_recovery_nonce_check_domain(nonce_data, msg32, key32);
    FUZZ_CHECK(algo16 == NULL);
    FUZZ_CHECK(attempt == 0);
    nonce_data->calls++;
    memset(nonce32, 0xA5, 32);
    return 0;
}

static int secp256k1_fuzz_recovery_nonce_retry_fail(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int attempt) {
    secp256k1_fuzz_recovery_nonce_data *nonce_data = (secp256k1_fuzz_recovery_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(key32 != NULL);
    secp256k1_fuzz_recovery_nonce_check_domain(nonce_data, msg32, key32);
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

/* Check recoverable ECDSA with public point operations instead of relying on
 * ecdsa_verify or the recovery implementation's internal multiscalar path. */
static void secp256k1_fuzz_check_recovery_equation(const secp256k1_context *ctx, const secp256k1_ecdsa_recoverable_signature *sig, const unsigned char *msg32, const secp256k1_pubkey *recovered_pubkey) {
    secp256k1_pubkey r_point;
    secp256k1_pubkey s_r_point;
    secp256k1_pubkey generator;
    secp256k1_pubkey z_g_point;
    secp256k1_pubkey left;
    secp256k1_pubkey expected;
    const secp256k1_pubkey *terms[2];
    unsigned char compact[64];
    unsigned char rx32[32];
    unsigned char z32[32];
    unsigned char r_point33[33];
    unsigned char zero32[32] = { 0 };
    int recid;
    int i;
    unsigned int carry = 0;
    size_t term_count = 0;

    FUZZ_CHECK(sig != NULL);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(recovered_pubkey != NULL);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, sig) == 1);
    FUZZ_CHECK(recid >= 0 && recid <= 3);
    FUZZ_CHECK(secp256k1_fuzz_scalar32_in_order(compact));
    FUZZ_CHECK(secp256k1_fuzz_scalar32_in_order(compact + 32));
    FUZZ_CHECK(memcmp(compact, zero32, sizeof(zero32)) != 0);
    FUZZ_CHECK(memcmp(compact + 32, zero32, sizeof(zero32)) != 0);

    memcpy(rx32, compact, sizeof(rx32));
    if (recid & 2) {
        for (i = 31; i >= 0; i--) {
            unsigned int value = (unsigned int)rx32[i] + secp256k1_fuzz_scalar_order[i] + carry;
            rx32[i] = (unsigned char)value;
            carry = value >> 8;
        }
        FUZZ_CHECK(carry == 0);
    }
    r_point33[0] = (unsigned char)(SECP256K1_TAG_PUBKEY_EVEN + (recid & 1));
    memcpy(r_point33 + 1, rx32, sizeof(rx32));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &r_point, r_point33, sizeof(r_point33)) == 1);

    /* The recovery API reduces the message scalar modulo the group order. */
    secp256k1_fuzz_scalar32_reduce(z32, msg32);

    left = *recovered_pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &left, compact) == 1);
    s_r_point = r_point;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &s_r_point, compact + 32) == 1);
    terms[term_count++] = &s_r_point;
    if (memcmp(z32, zero32, sizeof(z32)) != 0) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &generator, secp256k1_fuzz_scalar_one) == 1);
        z_g_point = generator;
        FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &z_g_point, z32) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &z_g_point) == 1);
        terms[term_count++] = &z_g_point;
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected, terms, term_count) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &left, &expected) == 0);
}

static void secp256k1_fuzz_check_recoverable_parse_compact(const secp256k1_context *ctx, const unsigned char *input64, int recid) {
    secp256k1_ecdsa_recoverable_signature recoverable_sig;
    secp256k1_ecdsa_recoverable_signature reparsed_sig;
    secp256k1_ecdsa_signature normal_sig;
    unsigned char compact[64];
    unsigned char normal_compact[64];
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_recoverable_signature)] = { 0 };
    int expected_ret;
    int parse_ret;
    int serialized_recid;

    FUZZ_CHECK(recid >= 0 && recid <= 3);
    expected_ret = secp256k1_fuzz_scalar32_in_order(input64);
    expected_ret &= secp256k1_fuzz_scalar32_in_order(input64 + 32);
    memset(&recoverable_sig, 0xA5, sizeof(recoverable_sig));
    parse_ret = secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &recoverable_sig, input64, recid);
    FUZZ_CHECK(parse_ret == expected_ret);
    if (parse_ret) {
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &serialized_recid, &recoverable_sig) == 1);
        FUZZ_CHECK(serialized_recid == recid);
        FUZZ_CHECK(memcmp(compact, input64, sizeof(compact)) == 0);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, compact, serialized_recid) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &serialized_recid, &reparsed_sig) == 1);
        FUZZ_CHECK(serialized_recid == recid);
        FUZZ_CHECK(memcmp(compact, input64, sizeof(compact)) == 0);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &recoverable_sig) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normal_compact, &normal_sig) == 1);
        FUZZ_CHECK(memcmp(normal_compact, input64, sizeof(normal_compact)) == 0);
    } else {
        FUZZ_CHECK(memcmp(&recoverable_sig, zero_sig, sizeof(recoverable_sig)) == 0);
    }
}

static void secp256k1_fuzz_check_recoverable_signature_state_barrier(secp256k1_context *ctx, const secp256k1_ecdsa_recoverable_signature *valid_sig, const unsigned char *msg32) {
    secp256k1_fuzz_recovery_illegal_data illegal_data;
    secp256k1_ecdsa_recoverable_signature invalid_sig;
    secp256k1_ecdsa_signature normal_sig;
    secp256k1_pubkey recovered_pubkey;
    unsigned char compact[64];
    unsigned char zero64[64] = { 0 };
    unsigned char zero_normal_sig[sizeof(normal_sig)] = { 0 };
    unsigned char zero_pubkey[sizeof(recovered_pubkey)] = { 0 };
    unsigned int calls;
    int recid;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_recovery_illegal_callback, &illegal_data);

    invalid_sig = *valid_sig;
    memset(invalid_sig.data, 0xFF, 32);
    memset(compact, 0xA5, sizeof(compact));
    recid = 7;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(recid == 0);
    FUZZ_CHECK(memcmp(compact, zero64, sizeof(compact)) == 0);

    memset(&normal_sig, 0xA5, sizeof(normal_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&normal_sig, zero_normal_sig, sizeof(normal_sig)) == 0);

    memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &invalid_sig, msg32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);

    invalid_sig = *valid_sig;
    memset(invalid_sig.data + 32, 0xFF, 32);
    memset(compact, 0x5A, sizeof(compact));
    recid = 7;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(recid == 0);
    FUZZ_CHECK(memcmp(compact, zero64, sizeof(compact)) == 0);

    memset(&normal_sig, 0xA5, sizeof(normal_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&normal_sig, zero_normal_sig, sizeof(normal_sig)) == 0);

    memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &invalid_sig, msg32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);

    invalid_sig = *valid_sig;
    invalid_sig.data[64] = 4;
    memset(compact, 0x3C, sizeof(compact));
    recid = 7;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(recid == 0);
    FUZZ_CHECK(memcmp(compact, zero64, sizeof(compact)) == 0);

    memset(&normal_sig, 0xA5, sizeof(normal_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&normal_sig, zero_normal_sig, sizeof(normal_sig)) == 0);

    memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &invalid_sig, msg32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_recovery_illegal_failure_cleanup(secp256k1_context *ctx, const secp256k1_ecdsa_recoverable_signature *valid_sig, const unsigned char *compact, const unsigned char *msg32, const unsigned char *valid_seckey32) {
    secp256k1_fuzz_recovery_illegal_data illegal_data;
    secp256k1_ecdsa_recoverable_signature recoverable_sig;
    secp256k1_ecdsa_signature normal_sig;
    secp256k1_pubkey recovered_pubkey;
    unsigned char output64[64];
    unsigned char zero64[64] = { 0 };
    unsigned char zero_recoverable_sig[sizeof(secp256k1_ecdsa_recoverable_signature)] = { 0 };
    unsigned char zero_normal_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    int (*parse_compact_fn)(const secp256k1_context *, secp256k1_ecdsa_recoverable_signature *, const unsigned char *, int) = secp256k1_ecdsa_recoverable_signature_parse_compact;
    int (*serialize_compact_fn)(const secp256k1_context *, unsigned char *, int *, const secp256k1_ecdsa_recoverable_signature *) = secp256k1_ecdsa_recoverable_signature_serialize_compact;
    int (*convert_fn)(const secp256k1_context *, secp256k1_ecdsa_signature *, const secp256k1_ecdsa_recoverable_signature *) = secp256k1_ecdsa_recoverable_signature_convert;
    int (*sign_recoverable_fn)(const secp256k1_context *, secp256k1_ecdsa_recoverable_signature *, const unsigned char *, const unsigned char *, secp256k1_nonce_function, const void *) = secp256k1_ecdsa_sign_recoverable;
    int (*recover_fn)(const secp256k1_context *, secp256k1_pubkey *, const secp256k1_ecdsa_recoverable_signature *, const unsigned char *) = secp256k1_ecdsa_recover;
    unsigned int calls;
    int recid_out;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_recovery_illegal_callback, &illegal_data);

    memset(&recoverable_sig, 0xA5, sizeof(recoverable_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(parse_compact_fn(ctx, &recoverable_sig, NULL, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recoverable_sig, zero_recoverable_sig, sizeof(recoverable_sig)) == 0);

    memset(&recoverable_sig, 0xA5, sizeof(recoverable_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(parse_compact_fn(ctx, &recoverable_sig, compact, -1) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recoverable_sig, zero_recoverable_sig, sizeof(recoverable_sig)) == 0);

    memset(&recoverable_sig, 0xA5, sizeof(recoverable_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(parse_compact_fn(ctx, &recoverable_sig, compact, 4) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recoverable_sig, zero_recoverable_sig, sizeof(recoverable_sig)) == 0);

    memset(output64, 0xA5, sizeof(output64));
    calls = illegal_data.calls;
    FUZZ_CHECK(serialize_compact_fn(ctx, output64, NULL, valid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(output64, zero64, sizeof(output64)) == 0);

    memset(output64, 0xA5, sizeof(output64));
    recid_out = 7;
    calls = illegal_data.calls;
    FUZZ_CHECK(serialize_compact_fn(ctx, output64, &recid_out, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(recid_out == 0);
    FUZZ_CHECK(memcmp(output64, zero64, sizeof(output64)) == 0);

    memset(&normal_sig, 0xA5, sizeof(normal_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(convert_fn(ctx, &normal_sig, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&normal_sig, zero_normal_sig, sizeof(normal_sig)) == 0);

    memset(&recoverable_sig, 0xA5, sizeof(recoverable_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(sign_recoverable_fn(ctx, &recoverable_sig, NULL, valid_seckey32, NULL, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recoverable_sig, zero_recoverable_sig, sizeof(recoverable_sig)) == 0);

    memset(&recoverable_sig, 0xA5, sizeof(recoverable_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(sign_recoverable_fn(ctx, &recoverable_sig, msg32, NULL, NULL, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recoverable_sig, zero_recoverable_sig, sizeof(recoverable_sig)) == 0);

    memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(recover_fn(ctx, &recovered_pubkey, valid_sig, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);

    memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(recover_fn(ctx, &recovered_pubkey, NULL, msg32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_recover_failure_cleanup(const secp256k1_context *ctx, const secp256k1_ecdsa_recoverable_signature *sig, const unsigned char *msg32) {
    secp256k1_pubkey recovered_pubkey;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };

    memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, sig, msg32) == 0);
    FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);
}

static void secp256k1_fuzz_check_sign_recoverable_failure_cleanup(const secp256k1_context *ctx, const unsigned char *msg32, const unsigned char *seckey32) {
    secp256k1_ecdsa_recoverable_signature sig;
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_recoverable_signature)] = { 0 };

    memset(&sig, 0xA5, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &sig, msg32, seckey32, NULL, NULL) == 0);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
}

static void secp256k1_fuzz_check_sign_recoverable_nonce_failure_cleanup(const secp256k1_context *ctx, const unsigned char *msg32, const unsigned char *valid_seckey32) {
    secp256k1_fuzz_recovery_nonce_data nonce_data;
    secp256k1_ecdsa_recoverable_signature sig;
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_recoverable_signature)] = { 0 };

    nonce_data.self = &nonce_data;
    nonce_data.expected_msg32 = msg32;
    nonce_data.expected_key32 = valid_seckey32;
    nonce_data.calls = 0;
    memset(&sig, 0xA5, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &sig, msg32, valid_seckey32, secp256k1_fuzz_recovery_nonce_fail, &nonce_data) == 0);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
}

static void secp256k1_fuzz_check_sign_recoverable_retry_failure_cleanup(const secp256k1_context *ctx, const unsigned char *msg32, const unsigned char *valid_seckey32) {
    secp256k1_fuzz_recovery_nonce_data nonce_data;
    secp256k1_ecdsa_recoverable_signature sig;
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_recoverable_signature)] = { 0 };

    nonce_data.self = &nonce_data;
    nonce_data.expected_msg32 = msg32;
    nonce_data.expected_key32 = valid_seckey32;
    nonce_data.calls = 0;
    memset(&sig, 0x5A, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &sig, msg32, valid_seckey32, secp256k1_fuzz_recovery_nonce_retry_fail, &nonce_data) == 0);
    FUZZ_CHECK(nonce_data.calls == 3);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
}

static void secp256k1_fuzz_check_recoverable_high_s(const secp256k1_context *ctx, const secp256k1_ecdsa_recoverable_signature *sig, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_ecdsa_recoverable_signature high_sig;
    secp256k1_ecdsa_signature high_normal_sig;
    secp256k1_ecdsa_signature normalized_sig;
    secp256k1_pubkey recovered_pubkey;
    unsigned char compact[64];
    unsigned char low_compact[64];
    unsigned char normalized_compact[64];
    int recid;

    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, low_compact, &recid, sig) == 1);
    FUZZ_CHECK(recid >= 0 && recid <= 3);
    memcpy(compact, low_compact, sizeof(compact));
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, compact + 32) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &high_sig, compact, recid ^ 1) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &high_sig, msg32) == 1);
    secp256k1_fuzz_check_recovery_equation(ctx, &high_sig, msg32, &recovered_pubkey);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, pubkey, &recovered_pubkey) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &high_normal_sig, &high_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &high_normal_sig, msg32, pubkey) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &high_normal_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normalized_compact, &normalized_sig) == 1);
    FUZZ_CHECK(memcmp(normalized_compact, low_compact, sizeof(normalized_compact)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normalized_sig, msg32, pubkey) == 1);
}

static void secp256k1_fuzz_check_recoverable_valid_nonce_retry(const secp256k1_context *ctx) {
    secp256k1_fuzz_recovery_nonce_data nonce_data;
    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_ecdsa_signature normal_sig;
    secp256k1_pubkey generator;
    secp256k1_pubkey recovered_pubkey;
    unsigned char generator_uncompressed[65];
    unsigned char zero_s_message[32];
    unsigned char compact[64];
    unsigned char zero32[32] = { 0 };
    size_t generator_len = sizeof(generator_uncompressed);
    int recid;

    /* For d = k = 1, choose z = -x(G) so the first valid nonce reaches the
     * ECDSA equation's s == 0 rejection path. The callback then supplies k=2,
     * making the recovery-ID retry observable instead of probabilistic. */
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &generator, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, generator_uncompressed, &generator_len, &generator, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(generator_len == sizeof(generator_uncompressed));
    memcpy(zero_s_message, generator_uncompressed + 1, sizeof(zero_s_message));
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, zero_s_message) == 1);
    FUZZ_CHECK(secp256k1_ec_seckey_verify(ctx, zero_s_message) == 1);

    nonce_data.self = &nonce_data;
    nonce_data.expected_msg32 = zero_s_message;
    nonce_data.expected_key32 = secp256k1_fuzz_scalar_one;
    nonce_data.calls = 0;
    memset(&sig, 0xA5, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &sig, zero_s_message, secp256k1_fuzz_scalar_one, secp256k1_fuzz_recovery_nonce_s_zero_then_two, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls == 2);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &sig) == 1);
    FUZZ_CHECK(recid >= 0 && recid <= 3);
    FUZZ_CHECK(memcmp(compact, zero32, sizeof(zero32)) != 0);
    FUZZ_CHECK(memcmp(compact + 32, zero32, sizeof(zero32)) != 0);
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &sig, zero_s_message) == 1);
    secp256k1_fuzz_check_recovery_equation(ctx, &sig, zero_s_message, &recovered_pubkey);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &generator, &recovered_pubkey) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normal_sig, zero_s_message, &generator) == 1);
}

static void secp256k1_fuzz_check_recovery_recid_overflow_boundary(const secp256k1_context *ctx, const unsigned char *msg32) {
    unsigned char compact[64];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_pubkey recovered_pubkey;
    int recid;

    memcpy(compact, secp256k1_fuzz_recovery_p_minus_order_plus_x_for_y_one, 32);
    memcpy(compact + 32, secp256k1_fuzz_scalar_one, 32);
    for (recid = 2; recid < 4; recid++) {
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &sig, compact, recid) == 1);
        memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
        FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &sig, msg32) == 0);
        FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);
    }
}

static void secp256k1_fuzz_check_recovery_invalid_x_coordinate(secp256k1_context *ctx) {
    unsigned char compact[64] = { 0 };
    unsigned char zero_msg32[32] = { 0 };
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_pubkey recovered_pubkey;
    secp256k1_fuzz_recovery_illegal_data illegal_data;
    int (*recover_fn)(const secp256k1_context *, secp256k1_pubkey *, const secp256k1_ecdsa_recoverable_signature *, const unsigned char *) = secp256k1_ecdsa_recover;
    unsigned int calls;
    int recid;

    /* x = 5 is below the scalar order, but x^3 + 7 = 132 is not a quadratic
     * residue modulo the secp256k1 field prime. Test both Y-parity branches. */
    compact[31] = 5;
    compact[63] = 1;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_recovery_illegal_callback, &illegal_data);
    for (recid = 0; recid < 2; recid++) {
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &sig, compact, recid) == 1);
        memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
        calls = illegal_data.calls;
        FUZZ_CHECK(recover_fn(ctx, &recovered_pubkey, &sig, zero_msg32) == 0);
        FUZZ_CHECK(illegal_data.calls == calls);
        FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);
    }
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_recovery_zero_s_rejection(const secp256k1_context *ctx) {
    unsigned char compact[64] = { 0 };
    unsigned char msg32[32] = { 0 };
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_pubkey recovered_pubkey;

    /* r = 4 is a valid recovery x-coordinate; a nonzero message makes an
     * incorrectly accepted s = 0 produce a non-infinity result. */
    compact[31] = 4;
    msg32[31] = 1;
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &sig, compact, 0) == 1);
    memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &sig, msg32) == 0);
    FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);
}

static void secp256k1_fuzz_check_recovery_message_reduction(const secp256k1_context *ctx) {
    secp256k1_fuzz_recovery_nonce_data nonce_data;
    secp256k1_ecdsa_recoverable_signature reduced_sig;
    secp256k1_ecdsa_recoverable_signature overflowing_sig;
    secp256k1_pubkey reduced_pubkey;
    secp256k1_pubkey overflowing_pubkey;
    unsigned char overflowing_msg32[32];
    unsigned char reduced_compact[64];
    unsigned char overflowing_compact[64];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    int reduced_recid;
    int overflowing_recid;

    /* The recovery API consumes a 32-byte message hash as a scalar modulo n.
     * Use n + 1 so this contract is tested independently of random chance. */
    memcpy(overflowing_msg32, secp256k1_fuzz_scalar_order, sizeof(overflowing_msg32));
    overflowing_msg32[31]++;

    nonce_data.self = &nonce_data;
    nonce_data.expected_key32 = secp256k1_fuzz_scalar_one;
    nonce_data.expected_msg32 = overflowing_msg32;
    nonce_data.calls = 0;
    memset(&overflowing_sig, 0xA5, sizeof(overflowing_sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &overflowing_sig, overflowing_msg32, secp256k1_fuzz_scalar_one, secp256k1_fuzz_recovery_nonce_one, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls == 1);

    nonce_data.expected_msg32 = secp256k1_fuzz_scalar_one;
    nonce_data.calls = 0;
    memset(&reduced_sig, 0xA5, sizeof(reduced_sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &reduced_sig, secp256k1_fuzz_scalar_one, secp256k1_fuzz_scalar_one, secp256k1_fuzz_recovery_nonce_one, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls == 1);

    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, overflowing_compact, &overflowing_recid, &overflowing_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, reduced_compact, &reduced_recid, &reduced_sig) == 1);
    FUZZ_CHECK(overflowing_recid == reduced_recid);
    FUZZ_CHECK(memcmp(overflowing_compact, reduced_compact, sizeof(reduced_compact)) == 0);

    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &overflowing_pubkey, &overflowing_sig, overflowing_msg32) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &reduced_pubkey, &reduced_sig, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &overflowing_pubkey, &reduced_pubkey) == 0);
    FUZZ_CHECK(memcmp(&overflowing_pubkey, zero_pubkey, sizeof(zero_pubkey)) != 0);
    secp256k1_fuzz_check_recovery_equation(ctx, &overflowing_sig, overflowing_msg32, &overflowing_pubkey);
}

static void secp256k1_fuzz_check_recovery_r_plus_order_equation(const secp256k1_context *ctx) {
    static const unsigned char msg32[32] = {
        'T', 'h', 'i', 's', ' ', 'i', 's', ' ',
        'a', ' ', 'v', 'e', 'r', 'y', ' ', 's',
        'e', 'c', 'r', 'e', 't', ' ', 'm', 'e',
        's', 's', 'a', 'g', 'e', '.', '.', '.'
    };
    unsigned char compact[64] = { 0 };
    secp256k1_ecdsa_recoverable_signature sig;
    secp256k1_pubkey recovered_pubkey;
    int recid;

    /* (r,s) = (4,4) is a deterministic vector whose recid 2/3 paths use r+n. */
    compact[31] = 4;
    compact[63] = 4;
    for (recid = 2; recid < 4; recid++) {
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &sig, compact, recid) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &sig, msg32) == 1);
        secp256k1_fuzz_check_recovery_equation(ctx, &sig, msg32, &recovered_pubkey);
    }
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_RECOVERY
    static const unsigned char zero_s_rejection_trigger[] = "recovery zero s rejection\n";
    static const unsigned char message_reduction_trigger[] = "recovery message order reduction\n";
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 91);
    secp256k1_fuzz_recovery_nonce_data nonce_data;
    unsigned char seckey[32];
    unsigned char msg32[32];
    unsigned char compact[64];
    unsigned char checked_compact[64];
    unsigned char checked_default_compact[64];
    unsigned char retry_compact[64];
    unsigned char normal_compact[64];
    unsigned char zero_compact[64] = { 0 };
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    unsigned char sig64[64];
    secp256k1_pubkey pubkey;
    secp256k1_pubkey recovered_pubkey;
    secp256k1_ecdsa_signature normal_sig;
    secp256k1_ecdsa_signature normalized_sig;
    secp256k1_ecdsa_recoverable_signature recoverable_sig;
    secp256k1_ecdsa_recoverable_signature checked_recoverable_sig;
    secp256k1_ecdsa_recoverable_signature checked_default_recoverable_sig;
    secp256k1_ecdsa_recoverable_signature retry_recoverable_sig;
    secp256k1_ecdsa_recoverable_signature reparsed_sig;
    int recid;
    int checked_recid;
    int checked_default_recid;
    int retry_recid;
    int alt_recid;
    int parsed;

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 97);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 101);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    secp256k1_fuzz_check_recovery_generator_vector(ctx, input, size);
    secp256k1_fuzz_check_static_context_recovery(ctx, input, size);
    secp256k1_fuzz_check_sign_recoverable_input_output_alias(ctx, input, size);

    secp256k1_fuzz_recovery_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_recovery_sha256_compression);
    secp256k1_fuzz_recovery_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &recoverable_sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_recovery_sha256_compression_calls != 0);
    secp256k1_fuzz_recovery_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &checked_recoverable_sig, msg32, seckey, secp256k1_nonce_function_rfc6979, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_recovery_sha256_compression_calls != 0);
    secp256k1_fuzz_recovery_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &checked_default_recoverable_sig, msg32, seckey, secp256k1_nonce_function_default, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_recovery_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &recoverable_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, checked_compact, &checked_recid, &checked_recoverable_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, checked_default_compact, &checked_default_recid, &checked_default_recoverable_sig) == 1);
    FUZZ_CHECK(recid == checked_recid);
    FUZZ_CHECK(memcmp(compact, checked_compact, sizeof(compact)) == 0);
    FUZZ_CHECK(recid == checked_default_recid);
    FUZZ_CHECK(memcmp(compact, checked_default_compact, sizeof(compact)) == 0);
    secp256k1_context_set_sha256_compression(ctx, NULL);
    secp256k1_fuzz_check_recoverable_valid_nonce_retry(ctx);
    nonce_data.self = &nonce_data;
    nonce_data.expected_msg32 = msg32;
    nonce_data.expected_key32 = seckey;
    nonce_data.calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &retry_recoverable_sig, msg32, seckey, secp256k1_fuzz_recovery_nonce_retry, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls >= 4);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, retry_compact, &retry_recid, &retry_recoverable_sig) == 1);
    FUZZ_CHECK(retry_recid == recid);
    FUZZ_CHECK(memcmp(retry_compact, compact, sizeof(retry_compact)) == 0);
    FUZZ_CHECK(recid >= 0 && recid <= 3);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, compact, recid) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &reparsed_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normal_compact, &normal_sig) == 1);
    FUZZ_CHECK(memcmp(compact, normal_compact, sizeof(compact)) == 0);
    secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &normal_sig);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normalized_sig, msg32, &pubkey) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &reparsed_sig, msg32) == 1);
    secp256k1_fuzz_check_recovery_equation(ctx, &reparsed_sig, msg32, &recovered_pubkey);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &recovered_pubkey) == 0);
    secp256k1_fuzz_check_recoverable_signature_state_barrier(ctx, &reparsed_sig, msg32);
    secp256k1_fuzz_check_recovery_illegal_failure_cleanup(ctx, &reparsed_sig, compact, msg32, seckey);
    secp256k1_fuzz_check_recoverable_high_s(ctx, &reparsed_sig, msg32, &pubkey);
    secp256k1_fuzz_check_recovery_recid_overflow_boundary(ctx, msg32);
    if (size == sizeof("recovery-invalid-x-coordinate\n") - 1
        && memcmp(input, "recovery-invalid-x-coordinate\n", sizeof("recovery-invalid-x-coordinate\n") - 1) == 0) {
        secp256k1_fuzz_check_recovery_invalid_x_coordinate(ctx);
    }
    if (size == sizeof(zero_s_rejection_trigger) - 1
        && memcmp(input, zero_s_rejection_trigger, sizeof(zero_s_rejection_trigger) - 1) == 0) {
        secp256k1_fuzz_check_recovery_zero_s_rejection(ctx);
    }
    if (size == sizeof(message_reduction_trigger) - 1
        && memcmp(input, message_reduction_trigger, sizeof(message_reduction_trigger) - 1) == 0) {
        secp256k1_fuzz_check_recovery_message_reduction(ctx);
    }
    secp256k1_fuzz_check_recovery_r_plus_order_equation(ctx);

    secp256k1_fuzz_check_sign_recoverable_failure_cleanup(ctx, msg32, secp256k1_fuzz_scalar_zero);
    secp256k1_fuzz_check_sign_recoverable_failure_cleanup(ctx, msg32, secp256k1_fuzz_scalar_order);
    secp256k1_fuzz_check_sign_recoverable_nonce_failure_cleanup(ctx, msg32, seckey);
    secp256k1_fuzz_check_sign_recoverable_retry_failure_cleanup(ctx, msg32, seckey);

    for (alt_recid = 0; alt_recid < 4; alt_recid++) {
        if (alt_recid == recid) {
            continue;
        }
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, compact, alt_recid) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &reparsed_sig) == 1);
        secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &normal_sig);
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normalized_sig, msg32, &pubkey) == 1);
        if (secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &reparsed_sig, msg32)) {
            secp256k1_fuzz_check_recovery_equation(ctx, &reparsed_sig, msg32, &recovered_pubkey);
            FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &recovered_pubkey) != 0);
        }
    }

    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, zero_compact, 0) == 1);
    secp256k1_fuzz_check_recover_failure_cleanup(ctx, &reparsed_sig, msg32);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, zero_compact, 0);
    memcpy(sig64, secp256k1_fuzz_scalar_order_minus_one, 32);
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_order_minus_one, 32);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, sig64, recid);
    memcpy(sig64, secp256k1_fuzz_scalar_order, 32);
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_zero, 32);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, sig64, recid);
    memcpy(sig64, secp256k1_fuzz_scalar_zero, 32);
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_order, 32);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, sig64, recid);
    secp256k1_fuzz_derive(sig64, sizeof(sig64), input, size, 113);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, sig64, recid);

    if (size >= 64) {
        recid = secp256k1_fuzz_byte(input, size, 109) & 3;
        secp256k1_fuzz_check_recoverable_parse_compact(ctx, input, recid);
        parsed = secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, input, recid);
        if (parsed) {
            unsigned char compact2[64];
            int recid2;
            FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &reparsed_sig) == 1);
            FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &recoverable_sig, compact, recid) == 1);
            FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact2, &recid2, &recoverable_sig) == 1);
            FUZZ_CHECK(recid == recid2);
            FUZZ_CHECK(memcmp(compact, compact2, sizeof(compact)) == 0);
            FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &reparsed_sig) == 1);
            FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normal_compact, &normal_sig) == 1);
            FUZZ_CHECK(memcmp(compact, normal_compact, sizeof(compact)) == 0);
            secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &normal_sig);
            memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
            if (secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &reparsed_sig, msg32)) {
                FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normalized_sig, msg32, &recovered_pubkey) == 1);
                secp256k1_fuzz_check_recovery_equation(ctx, &reparsed_sig, msg32, &recovered_pubkey);
            } else {
                FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);
            }
        }
    }

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
