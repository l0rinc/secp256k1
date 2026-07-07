/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

typedef struct {
    const void *self;
    const unsigned char *extra32;
    unsigned int calls;
} secp256k1_fuzz_ecdsa_nonce_data;

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
        secp256k1_fuzz_check_pubkey_roundtrip(ctx, &tweaked_pubkey);
    } else {
        FUZZ_CHECK(memcmp(tweaked_seckey, secp256k1_fuzz_scalar_zero, sizeof(tweaked_seckey)) == 0);
        FUZZ_CHECK(memcmp(&tweaked_pubkey, zero_pubkey, sizeof(tweaked_pubkey)) == 0);
    }
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

static void secp256k1_fuzz_check_pubkey_create_failure(const secp256k1_context *ctx, const unsigned char *seckey32) {
    secp256k1_pubkey pubkey;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };

    memset(&pubkey, 0xA5, sizeof(pubkey));
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey32) == 0);
    FUZZ_CHECK(memcmp(&pubkey, zero_pubkey, sizeof(pubkey)) == 0);
}

static int secp256k1_fuzz_scalar32_in_order(const unsigned char *input32) {
    return memcmp(input32, secp256k1_fuzz_scalar_order, 32) < 0;
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
    unsigned char compact[64];
    unsigned char roundtrip_der[72];
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_signature)] = { 0 };
    size_t roundtrip_der_len = sizeof(roundtrip_der);
    int parsed_der;

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

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
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
    unsigned char msg32[32];
    const secp256k1_pubkey *sorted_pubkeys[4];
    const secp256k1_pubkey *permuted_pubkeys[4];
    size_t i;

    secp256k1_fuzz_check_pubkey_parse(ctx, input, 0);
    secp256k1_fuzz_check_pubkey_parse(ctx, input, size);
    secp256k1_fuzz_check_pubkey_create_failure(ctx, secp256k1_fuzz_scalar_zero);
    secp256k1_fuzz_check_pubkey_create_failure(ctx, secp256k1_fuzz_scalar_order);

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 11);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 17);
    secp256k1_fuzz_derive(nonce_extra32, sizeof(nonce_extra32), input, size, 43);
    nonce_data.self = &nonce_data;
    nonce_data.extra32 = nonce_extra32;
    nonce_data.calls = 0;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &pubkey);

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

    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &sig, msg32, &pubkey) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, &sig) == 0);
    secp256k1_fuzz_check_signature_roundtrip(ctx, &sig);
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &parsed_sig, msg32, seckey, NULL, nonce_extra32) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &checked_sig, msg32, seckey, secp256k1_fuzz_ecdsa_nonce, &nonce_data) == 1);
    FUZZ_CHECK(nonce_data.calls >= 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, sig_extra64, &parsed_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, sig_checked64, &checked_sig) == 1);
    FUZZ_CHECK(memcmp(sig_extra64, sig_checked64, sizeof(sig_extra64)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &checked_sig, msg32, &pubkey) == 1);

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
    secp256k1_fuzz_derive(sig64, sizeof(sig64), input, size, 41);
    secp256k1_fuzz_check_signature_parse_der_boundary(ctx, sig64, sig64 + 32, msg32, &pubkey);
    secp256k1_fuzz_check_signature_parse_der_input(ctx, input, size, msg32, &pubkey);

    secp256k1_context_destroy(ctx);
    return 0;
}
