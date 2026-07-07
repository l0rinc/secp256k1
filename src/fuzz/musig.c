/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_MUSIG)
static int secp256k1_fuzz_musig_ge_ext_parse(const secp256k1_context *ctx, unsigned char *serialized33, const unsigned char *input33) {
    unsigned char zero33[33] = { 0 };
    secp256k1_pubkey pubkey;
    size_t serialized_len = 33;

    if (memcmp(input33, zero33, sizeof(zero33)) == 0) {
        memset(serialized33, 0, 33);
        return 1;
    }
    if (!secp256k1_ec_pubkey_parse(ctx, &pubkey, input33, 33)) {
        return 0;
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized33, &serialized_len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(serialized_len == 33);
    return 1;
}

static int secp256k1_fuzz_musig_pubnonce_part_parse(const secp256k1_context *ctx, unsigned char *serialized33, const unsigned char *input33) {
    secp256k1_pubkey pubkey;
    size_t serialized_len = 33;

    if (!secp256k1_ec_pubkey_parse(ctx, &pubkey, input33, 33)) {
        return 0;
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized33, &serialized_len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(serialized_len == 33);
    return 1;
}

static void secp256k1_fuzz_check_musig_pubnonce_parse(const secp256k1_context *ctx, const unsigned char *input66) {
    unsigned char expected66[66];
    unsigned char serialized66[66];
    unsigned char zero_nonce[sizeof(secp256k1_musig_pubnonce)] = { 0 };
    secp256k1_musig_pubnonce pubnonce;
    secp256k1_musig_pubnonce reparsed;
    int expected_ret;
    int parse_ret;

    expected_ret = secp256k1_fuzz_musig_pubnonce_part_parse(ctx, expected66, input66);
    expected_ret &= secp256k1_fuzz_musig_pubnonce_part_parse(ctx, expected66 + 33, input66 + 33);

    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    parse_ret = secp256k1_musig_pubnonce_parse(ctx, &pubnonce, input66);
    FUZZ_CHECK(parse_ret == expected_ret);
    if (parse_ret) {
        FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized66, &pubnonce) == 1);
        FUZZ_CHECK(memcmp(serialized66, expected66, sizeof(serialized66)) == 0);
        FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &reparsed, serialized66) == 1);
        FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized66, &reparsed) == 1);
        FUZZ_CHECK(memcmp(serialized66, expected66, sizeof(serialized66)) == 0);
    } else {
        FUZZ_CHECK(memcmp(&pubnonce, zero_nonce, sizeof(pubnonce)) == 0);
    }
}

static void secp256k1_fuzz_check_musig_aggnonce_parse(const secp256k1_context *ctx, const unsigned char *input66) {
    unsigned char expected66[66];
    unsigned char serialized66[66];
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_aggnonce reparsed;
    int expected_ret;
    int parse_ret;

    expected_ret = secp256k1_fuzz_musig_ge_ext_parse(ctx, expected66, input66);
    expected_ret &= secp256k1_fuzz_musig_ge_ext_parse(ctx, expected66 + 33, input66 + 33);

    memset(&aggnonce, 0xA5, sizeof(aggnonce));
    parse_ret = secp256k1_musig_aggnonce_parse(ctx, &aggnonce, input66);
    FUZZ_CHECK(parse_ret == expected_ret);
    if (parse_ret) {
        FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized66, &aggnonce) == 1);
        FUZZ_CHECK(memcmp(serialized66, expected66, sizeof(serialized66)) == 0);
        FUZZ_CHECK(secp256k1_musig_aggnonce_parse(ctx, &reparsed, serialized66) == 1);
        FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized66, &reparsed) == 1);
        FUZZ_CHECK(memcmp(serialized66, expected66, sizeof(serialized66)) == 0);
    }
}

static void secp256k1_fuzz_check_musig_partial_sig_parse(const secp256k1_context *ctx, const unsigned char *input32) {
    unsigned char serialized[32];
    unsigned char zero_sig[sizeof(secp256k1_musig_partial_sig)] = { 0 };
    secp256k1_musig_partial_sig sig;
    secp256k1_musig_partial_sig reparsed;

    memset(&sig, 0xA5, sizeof(sig));
    if (secp256k1_musig_partial_sig_parse(ctx, &sig, input32)) {
        FUZZ_CHECK(secp256k1_musig_partial_sig_serialize(ctx, serialized, &sig) == 1);
        FUZZ_CHECK(memcmp(serialized, input32, sizeof(serialized)) == 0);
        FUZZ_CHECK(secp256k1_musig_partial_sig_parse(ctx, &reparsed, serialized) == 1);
        FUZZ_CHECK(memcmp(&sig, &reparsed, sizeof(sig)) == 0);
    } else {
        FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
    }
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_MUSIG)
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 151);
    unsigned char seckey[3][32];
    unsigned char tweak[32];
    unsigned char agg_xonly32[32];
    unsigned char tweaked_xonly32[32];
    unsigned char nonce66[66];
    unsigned char zero66[66] = { 0 };
    unsigned char valid_aggnonce66[66];
    unsigned char mixed_aggnonce66[66];
    unsigned char mixed_pubnonce66[66];
    unsigned char sig32[32];
    unsigned char ones32[32];
    secp256k1_pubkey pubkeys[3];
    const secp256k1_pubkey *pubkey_ptrs[3];
    secp256k1_pubkey agg_full;
    secp256k1_pubkey cache_full;
    secp256k1_pubkey expected_tweaked_full;
    secp256k1_pubkey musig_tweaked_full;
    secp256k1_pubkey one_expected_full;
    secp256k1_pubkey one_tweaked_full;
    secp256k1_pubkey zero_tweaked_full;
    secp256k1_xonly_pubkey agg_xonly;
    secp256k1_xonly_pubkey agg_xonly_from_full;
    secp256k1_xonly_pubkey tweaked_xonly;
    secp256k1_musig_keyagg_cache cache;
    secp256k1_musig_keyagg_cache tweak_cache;
    secp256k1_musig_keyagg_cache tweak_cache_no_output;
    secp256k1_musig_keyagg_cache one_tweak_cache;
    secp256k1_musig_keyagg_cache zero_tweak_cache;
    size_t n_pubkeys;
    size_t i;
    int parity;
    int ret;
    int ret_no_output;
    size_t aggnonce_part_len;

    n_pubkeys = (size_t)((secp256k1_fuzz_byte(input, size, 157) % 3u) + 1u);
    for (i = 0; i < n_pubkeys; i++) {
        secp256k1_fuzz_valid_seckey32(ctx, seckey[i], input, size, 163u + (unsigned int)i);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckey[i]) == 1);
        pubkey_ptrs[i] = &pubkeys[i];
    }
    secp256k1_fuzz_scalar32(tweak, input, size, 173);
    aggnonce_part_len = 33;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, valid_aggnonce66, &aggnonce_part_len, &pubkeys[0], SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(aggnonce_part_len == 33);
    aggnonce_part_len = 33;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, valid_aggnonce66 + 33, &aggnonce_part_len, &pubkeys[n_pubkeys - 1], SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(aggnonce_part_len == 33);
    memcpy(mixed_aggnonce66, zero66, sizeof(mixed_aggnonce66));
    memcpy(mixed_aggnonce66 + 33, valid_aggnonce66 + 33, 33);
    memcpy(mixed_pubnonce66, valid_aggnonce66, 33);
    memset(mixed_pubnonce66 + 33, 0, 33);
    memset(ones32, 0xFF, sizeof(ones32));

    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_xonly, &cache, pubkey_ptrs, n_pubkeys) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &agg_full, &cache) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &agg_xonly_from_full, NULL, &agg_full) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &agg_xonly, &agg_xonly_from_full) == 0);

    zero_tweak_cache = cache;
    FUZZ_CHECK(secp256k1_musig_pubkey_ec_tweak_add(ctx, &zero_tweaked_full, &zero_tweak_cache, secp256k1_fuzz_scalar_zero) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &zero_tweaked_full, &agg_full) == 0);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_full, &zero_tweak_cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &cache_full, &zero_tweaked_full) == 0);
    zero_tweak_cache = cache;
    FUZZ_CHECK(secp256k1_musig_pubkey_ec_tweak_add(ctx, NULL, &zero_tweak_cache, secp256k1_fuzz_scalar_zero) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_full, &zero_tweak_cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &cache_full, &agg_full) == 0);

    one_tweak_cache = cache;
    ret = secp256k1_musig_pubkey_ec_tweak_add(ctx, &one_tweaked_full, &one_tweak_cache, secp256k1_fuzz_scalar_one);
    one_expected_full = agg_full;
    ret_no_output = secp256k1_ec_pubkey_tweak_add(ctx, &one_expected_full, secp256k1_fuzz_scalar_one);
    FUZZ_CHECK(ret == ret_no_output);
    if (ret) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &one_tweaked_full, &one_expected_full) == 0);
        FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_full, &one_tweak_cache) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &cache_full, &one_tweaked_full) == 0);
        one_tweak_cache = cache;
        FUZZ_CHECK(secp256k1_musig_pubkey_ec_tweak_add(ctx, NULL, &one_tweak_cache, secp256k1_fuzz_scalar_one) == 1);
        FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_full, &one_tweak_cache) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &cache_full, &one_tweaked_full) == 0);
    }

    tweak_cache = cache;
    ret = secp256k1_musig_pubkey_ec_tweak_add(ctx, &musig_tweaked_full, &tweak_cache, tweak);
    tweak_cache_no_output = cache;
    ret_no_output = secp256k1_musig_pubkey_ec_tweak_add(ctx, NULL, &tweak_cache_no_output, tweak);
    FUZZ_CHECK(ret == ret_no_output);
    if (ret) {
        expected_tweaked_full = agg_full;
        FUZZ_CHECK(secp256k1_ec_pubkey_tweak_add(ctx, &expected_tweaked_full, tweak) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &musig_tweaked_full, &expected_tweaked_full) == 0);
        FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_full, &tweak_cache) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &cache_full, &musig_tweaked_full) == 0);
        FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_full, &tweak_cache_no_output) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &cache_full, &musig_tweaked_full) == 0);
    }

    tweak_cache = cache;
    ret = secp256k1_musig_pubkey_xonly_tweak_add(ctx, &musig_tweaked_full, &tweak_cache, tweak);
    tweak_cache_no_output = cache;
    ret_no_output = secp256k1_musig_pubkey_xonly_tweak_add(ctx, NULL, &tweak_cache_no_output, tweak);
    FUZZ_CHECK(ret == ret_no_output);
    if (ret) {
        FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &tweaked_xonly, &parity, &musig_tweaked_full) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, agg_xonly32, &agg_xonly) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, tweaked_xonly32, &tweaked_xonly) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, tweaked_xonly32, parity, &agg_xonly, tweak) == 1);
        FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_full, &tweak_cache) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &cache_full, &musig_tweaked_full) == 0);
        FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_full, &tweak_cache_no_output) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &cache_full, &musig_tweaked_full) == 0);
        FUZZ_CHECK(secp256k1_xonly_pubkey_parse(ctx, &agg_xonly_from_full, agg_xonly32) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &agg_xonly, &agg_xonly_from_full) == 0);
    }

    secp256k1_fuzz_derive(nonce66, sizeof(nonce66), input, size, 181);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, zero66);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, valid_aggnonce66);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, mixed_aggnonce66);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, mixed_pubnonce66);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, nonce66);
    secp256k1_fuzz_derive(nonce66, sizeof(nonce66), input, size, 191);
    secp256k1_fuzz_check_musig_aggnonce_parse(ctx, zero66);
    secp256k1_fuzz_check_musig_aggnonce_parse(ctx, valid_aggnonce66);
    secp256k1_fuzz_check_musig_aggnonce_parse(ctx, mixed_aggnonce66);
    secp256k1_fuzz_check_musig_aggnonce_parse(ctx, nonce66);
    secp256k1_fuzz_derive(sig32, sizeof(sig32), input, size, 193);
    secp256k1_fuzz_check_musig_partial_sig_parse(ctx, secp256k1_fuzz_scalar_zero);
    secp256k1_fuzz_check_musig_partial_sig_parse(ctx, secp256k1_fuzz_scalar_order_minus_one);
    secp256k1_fuzz_check_musig_partial_sig_parse(ctx, secp256k1_fuzz_scalar_order);
    secp256k1_fuzz_check_musig_partial_sig_parse(ctx, ones32);
    secp256k1_fuzz_check_musig_partial_sig_parse(ctx, sig32);

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
