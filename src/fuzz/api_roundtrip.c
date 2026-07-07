/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

static void secp256k1_fuzz_check_tweak_add(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *seckey, const unsigned char *tweak32) {
    unsigned char tweaked_seckey[32];
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
    }
}

static void secp256k1_fuzz_check_tweak_mul(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *seckey, const unsigned char *tweak32) {
    unsigned char tweaked_seckey[32];
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

static int secp256k1_fuzz_scalar32_in_order(const unsigned char *input32) {
    return memcmp(input32, secp256k1_fuzz_scalar_order, 32) < 0;
}

static void secp256k1_fuzz_check_signature_parse_compact(const secp256k1_context *ctx, const unsigned char *input64, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char compact[64];
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
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, pubkey) == 0);
    }
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 1);
    secp256k1_pubkey pubkey;
    secp256k1_pubkey combine_pubkey;
    secp256k1_pubkey pubkey_neg;
    secp256k1_pubkey pubkey_neg_from_seckey;
    secp256k1_pubkey parsed_pubkey;
    secp256k1_pubkey sort_pubkeys[4];
    secp256k1_ecdsa_signature sig;
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char seckey[32];
    unsigned char combine_seckey[32];
    unsigned char seckey_neg[32];
    unsigned char sort_seckey[32];
    unsigned char tweak32[32];
    unsigned char zero_compact[64] = { 0 };
    unsigned char sig64[64];
    unsigned char msg32[32];
    const secp256k1_pubkey *sorted_pubkeys[4];
    const secp256k1_pubkey *permuted_pubkeys[4];
    int parsed_der;
    size_t i;

    if (secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, input, size)) {
        secp256k1_fuzz_check_pubkey_roundtrip(ctx, &parsed_pubkey);
    }

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 11);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 17);
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

    parsed_der = secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, input, size);
    secp256k1_fuzz_check_signature_roundtrip(ctx, &parsed_sig);
    if (!parsed_der) {
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, &pubkey) == 0);
    }

    secp256k1_context_destroy(ctx);
    return 0;
}
