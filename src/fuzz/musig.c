/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"

#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_MUSIG)
static size_t secp256k1_fuzz_musig_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_musig_keyagglist_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_musig_keyaggcoef_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_musig_noncecoef_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_musig_challenge_sha256_compression_calls = 0;

static const uint32_t secp256k1_fuzz_musig_keyagglist_midstate[8] = {
    0xb399d5e0ul, 0xc8fff302ul, 0x6badac71ul, 0x07c5b7f1ul,
    0x9701e2eful, 0x2a72ecf8ul, 0x201a4c7bul, 0xab148a38ul
};
static const uint32_t secp256k1_fuzz_musig_keyaggcoef_midstate[8] = {
    0x6ef02c5aul, 0x06a480deul, 0x1f298665ul, 0x1d1134f2ul,
    0x56a0b063ul, 0x52da4147ul, 0xf280d9d4ul, 0x4484be15ul
};
static const uint32_t secp256k1_fuzz_musig_noncecoef_midstate[8] = {
    0x2c7d5a45ul, 0x06bf7e53ul, 0x89be68a6ul, 0x971254c0ul,
    0x60ac12d2ul, 0x72846dcdul, 0x6c81212ful, 0xde7a2500ul
};
static const uint32_t secp256k1_fuzz_musig_challenge_midstate[8] = {
    0x9cecba11ul, 0x23925381ul, 0x11679112ul, 0xd1627e0ful,
    0x97c87550ul, 0x003cc765ul, 0x90f61164ul, 0x33e9b66aul
};

static void secp256k1_fuzz_musig_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    secp256k1_fuzz_musig_sha256_compression_calls += n_blocks;
    if (memcmp(state, secp256k1_fuzz_musig_keyagglist_midstate, sizeof(secp256k1_fuzz_musig_keyagglist_midstate)) == 0) {
        secp256k1_fuzz_musig_keyagglist_sha256_compression_calls++;
    }
    if (memcmp(state, secp256k1_fuzz_musig_keyaggcoef_midstate, sizeof(secp256k1_fuzz_musig_keyaggcoef_midstate)) == 0) {
        secp256k1_fuzz_musig_keyaggcoef_sha256_compression_calls++;
    }
    if (memcmp(state, secp256k1_fuzz_musig_noncecoef_midstate, sizeof(secp256k1_fuzz_musig_noncecoef_midstate)) == 0) {
        secp256k1_fuzz_musig_noncecoef_sha256_compression_calls++;
    }
    if (memcmp(state, secp256k1_fuzz_musig_challenge_midstate, sizeof(secp256k1_fuzz_musig_challenge_midstate)) == 0) {
        secp256k1_fuzz_musig_challenge_sha256_compression_calls++;
    }
    secp256k1_sha256_transform(state, blocks64, n_blocks);
}

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_musig_illegal_data;

static void secp256k1_fuzz_musig_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_musig_illegal_data *illegal_data = (secp256k1_fuzz_musig_illegal_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(illegal_data != NULL);
    FUZZ_CHECK(illegal_data->self == illegal_data);
    illegal_data->calls++;
}

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
    unsigned char zero_nonce[sizeof(secp256k1_musig_aggnonce)] = { 0 };
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
    } else {
        FUZZ_CHECK(memcmp(&aggnonce, zero_nonce, sizeof(aggnonce)) == 0);
    }
}

static void secp256k1_fuzz_check_musig_partial_sig_serialize_failure_cleanup(secp256k1_context *ctx, const secp256k1_musig_partial_sig *valid_sig) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    unsigned char serialized[32];
    unsigned char zero32[32] = { 0 };
    secp256k1_musig_partial_sig invalid_sig = *valid_sig;

    invalid_sig.data[0] ^= 1u;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    memset(serialized, 0xA5, sizeof(serialized));
    FUZZ_CHECK(secp256k1_musig_partial_sig_serialize(ctx, serialized, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(serialized, zero32, sizeof(serialized)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_partial_sig_parse(secp256k1_context *ctx, const unsigned char *input32) {
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
        secp256k1_fuzz_check_musig_partial_sig_serialize_failure_cleanup(ctx, &sig);
    } else {
        FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
    }
}

static void secp256k1_fuzz_check_musig_nonce_agg_failure_cleanup(secp256k1_context *ctx, const secp256k1_musig_pubnonce * const *pubnonce_ptrs, size_t n_pubnonces) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    unsigned char zero_aggnonce[sizeof(secp256k1_musig_aggnonce)] = { 0 };
    secp256k1_musig_pubnonce invalid_pubnonce;
    secp256k1_musig_aggnonce aggnonce;
    const secp256k1_musig_pubnonce *invalid_pubnonce_ptrs[2];
    size_t i;

    FUZZ_CHECK(n_pubnonces > 0);
    FUZZ_CHECK(n_pubnonces <= 2);

    for (i = 0; i < n_pubnonces; i++) {
        invalid_pubnonce_ptrs[i] = pubnonce_ptrs[i];
    }
    invalid_pubnonce = *pubnonce_ptrs[0];
    invalid_pubnonce.data[0] ^= 1u;
    invalid_pubnonce_ptrs[0] = &invalid_pubnonce;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(&aggnonce, 0xA5, sizeof(aggnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, invalid_pubnonce_ptrs, n_pubnonces) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&aggnonce, zero_aggnonce, sizeof(aggnonce)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_nonce_agg(secp256k1_context *ctx, const unsigned char *nonce_a66, const unsigned char *nonce_b66) {
    unsigned char serialized_a66[66];
    unsigned char serialized_ab66[66];
    unsigned char serialized_ba66[66];
    unsigned char normalized_a66[66];
    secp256k1_musig_pubnonce pubnonce_a;
    secp256k1_musig_pubnonce pubnonce_b;
    const secp256k1_musig_pubnonce *pubnonce_ptrs[2];
    secp256k1_musig_aggnonce aggnonce_a;
    secp256k1_musig_aggnonce aggnonce_ab;
    secp256k1_musig_aggnonce aggnonce_ba;

    FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &pubnonce_a, nonce_a66) == 1);
    FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &pubnonce_b, nonce_b66) == 1);

    pubnonce_ptrs[0] = &pubnonce_a;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce_a, pubnonce_ptrs, 1) == 1);
    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, normalized_a66, &pubnonce_a) == 1);
    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized_a66, &aggnonce_a) == 1);
    FUZZ_CHECK(memcmp(serialized_a66, normalized_a66, sizeof(serialized_a66)) == 0);

    pubnonce_ptrs[0] = &pubnonce_a;
    pubnonce_ptrs[1] = &pubnonce_b;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce_ab, pubnonce_ptrs, 2) == 1);
    pubnonce_ptrs[0] = &pubnonce_b;
    pubnonce_ptrs[1] = &pubnonce_a;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce_ba, pubnonce_ptrs, 2) == 1);
    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized_ab66, &aggnonce_ab) == 1);
    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized_ba66, &aggnonce_ba) == 1);
    FUZZ_CHECK(memcmp(serialized_ab66, serialized_ba66, sizeof(serialized_ab66)) == 0);
    secp256k1_fuzz_check_musig_nonce_agg_failure_cleanup(ctx, pubnonce_ptrs, 2);
}

static void secp256k1_fuzz_check_musig_nonce_process_failure_cleanup(secp256k1_context *ctx, const secp256k1_musig_aggnonce *aggnonce, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    unsigned char zero_session[sizeof(secp256k1_musig_session)] = { 0 };
    secp256k1_musig_aggnonce invalid_aggnonce = *aggnonce;
    secp256k1_musig_session session;

    invalid_aggnonce.data[0] ^= 1u;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(&session, 0xA5, sizeof(session));
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &invalid_aggnonce, msg32, keyagg_cache) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&session, zero_session, sizeof(session)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_negate_musig_pubnonce_parts(const secp256k1_context *ctx, unsigned char *negated66, const unsigned char *nonce66) {
    secp256k1_pubkey pubkey;
    size_t serialized_len;
    int i;

    for (i = 0; i < 2; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, nonce66 + 33*i, 33) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &pubkey) == 1);
        serialized_len = 33;
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, negated66 + 33*i, &serialized_len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == 33);
    }
}

static void secp256k1_fuzz_check_musig_nonce_agg_inverse(const secp256k1_context *ctx, const unsigned char *nonce66) {
    unsigned char negated66[66];
    unsigned char serialized66[66];
    unsigned char zero66[66] = { 0 };
    secp256k1_musig_pubnonce pubnonce;
    secp256k1_musig_pubnonce negated_pubnonce;
    const secp256k1_musig_pubnonce *pubnonce_ptrs[2];
    secp256k1_musig_aggnonce aggnonce;

    secp256k1_fuzz_negate_musig_pubnonce_parts(ctx, negated66, nonce66);
    FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &pubnonce, nonce66) == 1);
    FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &negated_pubnonce, negated66) == 1);

    pubnonce_ptrs[0] = &pubnonce;
    pubnonce_ptrs[1] = &negated_pubnonce;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 2) == 1);
    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized66, &aggnonce) == 1);
    FUZZ_CHECK(memcmp(serialized66, zero66, sizeof(serialized66)) == 0);

    pubnonce_ptrs[0] = &negated_pubnonce;
    pubnonce_ptrs[1] = &pubnonce;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 2) == 1);
    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized66, &aggnonce) == 1);
    FUZZ_CHECK(memcmp(serialized66, zero66, sizeof(serialized66)) == 0);
}

static void secp256k1_fuzz_check_musig_keyagg_hash_routing(secp256k1_context *ctx, const secp256k1_pubkey * const*pubkeys, size_t n_pubkeys, const secp256k1_xonly_pubkey *expected_agg_pk, const secp256k1_musig_keyagg_cache *expected_cache) {
    secp256k1_xonly_pubkey routed_agg_pk;
    secp256k1_musig_keyagg_cache routed_cache;
    secp256k1_pubkey expected_full;
    secp256k1_pubkey routed_full;

    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &expected_full, expected_cache) == 1);

    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_musig_sha256_compression);
    secp256k1_fuzz_musig_sha256_compression_calls = 0;
    secp256k1_fuzz_musig_keyagglist_sha256_compression_calls = 0;
    secp256k1_fuzz_musig_keyaggcoef_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &routed_agg_pk, &routed_cache, pubkeys, n_pubkeys) == 1);
    FUZZ_CHECK(secp256k1_fuzz_musig_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_musig_keyagglist_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_musig_keyaggcoef_sha256_compression_calls != 0);
    secp256k1_context_set_sha256_compression(ctx, NULL);

    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &routed_agg_pk, expected_agg_pk) == 0);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &routed_full, &routed_cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &routed_full, &expected_full) == 0);
}

typedef int (*secp256k1_fuzz_musig_tweak_func)(const secp256k1_context *ctx, secp256k1_pubkey *output_pubkey, secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *tweak32);

static void secp256k1_fuzz_check_musig_tweak_overflow_rollback(const secp256k1_context *ctx, secp256k1_fuzz_musig_tweak_func tweak_func, const secp256k1_musig_keyagg_cache *cache) {
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_musig_keyagg_cache failed_cache;
    secp256k1_pubkey failed_output;

    failed_cache = *cache;
    memset(&failed_output, 0xA5, sizeof(failed_output));
    FUZZ_CHECK(tweak_func(ctx, &failed_output, &failed_cache, secp256k1_fuzz_scalar_order) == 0);
    FUZZ_CHECK(memcmp(&failed_output, zero_pubkey, sizeof(failed_output)) == 0);
    FUZZ_CHECK(memcmp(&failed_cache, cache, sizeof(failed_cache)) == 0);

    failed_cache = *cache;
    FUZZ_CHECK(tweak_func(ctx, NULL, &failed_cache, secp256k1_fuzz_scalar_order) == 0);
    FUZZ_CHECK(memcmp(&failed_cache, cache, sizeof(failed_cache)) == 0);
}

static uint64_t secp256k1_fuzz_musig_nonzero_counter(const unsigned char *input, size_t size) {
    unsigned char counter_bytes[8];
    uint64_t counter = 0;
    size_t i;

    secp256k1_fuzz_derive(counter_bytes, sizeof(counter_bytes), input, size, 199);
    for (i = 0; i < sizeof(counter_bytes); i++) {
        counter = (counter << 8) | (uint64_t)counter_bytes[i];
    }
    return counter == 0 ? 1 : counter;
}

static void secp256k1_fuzz_musig_counter_to_secrand(unsigned char session_secrand32[32], uint64_t counter) {
    memset(session_secrand32, 0, 32);
    session_secrand32[0] = (unsigned char)(counter >> 56);
    session_secrand32[1] = (unsigned char)(counter >> 48);
    session_secrand32[2] = (unsigned char)(counter >> 40);
    session_secrand32[3] = (unsigned char)(counter >> 32);
    session_secrand32[4] = (unsigned char)(counter >> 24);
    session_secrand32[5] = (unsigned char)(counter >> 16);
    session_secrand32[6] = (unsigned char)(counter >> 8);
    session_secrand32[7] = (unsigned char)counter;
}

static void secp256k1_fuzz_check_musig_nonce_gen_counter(secp256k1_context *ctx, const unsigned char *input, size_t size, const unsigned char *seckey, const secp256k1_keypair *keypair, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    unsigned char counter_secrand[32];
    unsigned char zero32[32] = { 0 };
    unsigned char serialized_counter[66];
    unsigned char serialized_explicit[66];
    uint64_t counter = secp256k1_fuzz_musig_nonzero_counter(input, size);
    size_t hash_calls;
    int counter_ret;
    secp256k1_musig_secnonce secnonce_counter;
    secp256k1_musig_secnonce secnonce_explicit;
    secp256k1_musig_pubnonce pubnonce_counter;
    secp256k1_musig_pubnonce pubnonce_explicit;

    secp256k1_fuzz_musig_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_musig_sha256_compression);
    counter_ret = secp256k1_musig_nonce_gen_counter(ctx, &secnonce_counter, &pubnonce_counter, counter, keypair, msg32, keyagg_cache, extra_input32);
    hash_calls = secp256k1_fuzz_musig_sha256_compression_calls;
    secp256k1_context_set_sha256_compression(ctx, NULL);
    FUZZ_CHECK(counter_ret == 1);
    FUZZ_CHECK(hash_calls != 0);

    secp256k1_fuzz_musig_counter_to_secrand(counter_secrand, counter);
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce_explicit, &pubnonce_explicit, counter_secrand, seckey, pubkey, msg32, keyagg_cache, extra_input32) == 1);
    FUZZ_CHECK(memcmp(counter_secrand, zero32, sizeof(counter_secrand)) == 0);
    FUZZ_CHECK(memcmp(secnonce_counter.data, secnonce_explicit.data, sizeof(secnonce_counter.data)) == 0);
    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized_counter, &pubnonce_counter) == 1);
    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized_explicit, &pubnonce_explicit) == 1);
    FUZZ_CHECK(memcmp(serialized_counter, serialized_explicit, sizeof(serialized_counter)) == 0);
}

static void secp256k1_fuzz_check_musig_zero_counter_sign(secp256k1_context *ctx, const secp256k1_keypair *keypair, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const secp256k1_xonly_pubkey *agg_pk) {
    unsigned char sig64[64];
    unsigned char zero132[132] = { 0 };
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    const secp256k1_musig_pubnonce *pubnonce_ptrs[1];
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_session session;
    secp256k1_musig_partial_sig partial_sig;
    const secp256k1_musig_partial_sig *partial_sig_ptrs[1];

    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, 0, keypair, msg32, keyagg_cache, NULL) == 1);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) != 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) != 0);

    pubnonce_ptrs[0] = &pubnonce;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 1) == 1);
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, keyagg_cache) == 1);
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, keypair, keyagg_cache, &session) == 1);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig, &pubnonce, pubkey, keyagg_cache, &session) == 1);

    partial_sig_ptrs[0] = &partial_sig;
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sig_ptrs, 1) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, 32, agg_pk) == 1);
}

static void secp256k1_fuzz_check_musig_infinity_nonce_process(secp256k1_context *ctx, const secp256k1_keypair *keypair, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache) {
    unsigned char sig64[64];
    unsigned char serialized66[66];
    unsigned char negated66[66];
    unsigned char gen_xonly32[32];
    unsigned char zero66[66] = { 0 };
    unsigned char zero132[132] = { 0 };
    secp256k1_pubkey gen_pubkey;
    secp256k1_xonly_pubkey gen_xonly;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    secp256k1_musig_pubnonce negated_pubnonce;
    const secp256k1_musig_pubnonce *pubnonce_ptrs[2];
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_session session;
    secp256k1_musig_partial_sig partial_sig;
    const secp256k1_musig_partial_sig *partial_sig_ptrs[1];

    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, 1, keypair, msg32, keyagg_cache, NULL) == 1);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) != 0);

    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized66, &pubnonce) == 1);
    secp256k1_fuzz_negate_musig_pubnonce_parts(ctx, negated66, serialized66);
    FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &negated_pubnonce, negated66) == 1);

    pubnonce_ptrs[0] = &pubnonce;
    pubnonce_ptrs[1] = &negated_pubnonce;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 2) == 1);
    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized66, &aggnonce) == 1);
    FUZZ_CHECK(memcmp(serialized66, zero66, sizeof(serialized66)) == 0);

    secp256k1_fuzz_musig_noncecoef_sha256_compression_calls = 0;
    secp256k1_fuzz_musig_challenge_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_musig_sha256_compression);
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, keyagg_cache) == 1);
    FUZZ_CHECK(secp256k1_fuzz_musig_noncecoef_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_musig_challenge_sha256_compression_calls != 0);
    secp256k1_context_set_sha256_compression(ctx, NULL);

    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, keypair, keyagg_cache, &session) == 1);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig, &pubnonce, pubkey, keyagg_cache, &session) == 1);

    partial_sig_ptrs[0] = &partial_sig;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &gen_pubkey, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &gen_xonly, NULL, &gen_pubkey) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, gen_xonly32, &gen_xonly) == 1);
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sig_ptrs, 1) == 1);
    FUZZ_CHECK(memcmp(sig64, gen_xonly32, sizeof(gen_xonly32)) == 0);
}

static void secp256k1_fuzz_check_musig_partial_sig_agg_failure_cleanup(secp256k1_context *ctx, const secp256k1_musig_session *session, const secp256k1_musig_partial_sig * const *partial_sig_ptrs, size_t n_sigs) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    unsigned char sig64[64];
    unsigned char zero64[64] = { 0 };
    secp256k1_musig_partial_sig invalid_partial_sig;
    const secp256k1_musig_partial_sig *invalid_partial_sig_ptrs[3];
    size_t i;

    FUZZ_CHECK(n_sigs > 0);
    FUZZ_CHECK(n_sigs <= 3);

    for (i = 0; i < n_sigs; i++) {
        invalid_partial_sig_ptrs[i] = partial_sig_ptrs[i];
    }
    invalid_partial_sig = *partial_sig_ptrs[0];
    invalid_partial_sig.data[0] ^= 1u;
    invalid_partial_sig_ptrs[0] = &invalid_partial_sig;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(sig64, 0xA5, sizeof(sig64));
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, session, invalid_partial_sig_ptrs, n_sigs) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_sign_roundtrip(secp256k1_context *ctx, const unsigned char *input, size_t size, unsigned char seckey[3][32], const secp256k1_keypair *keypairs, const secp256k1_pubkey *pubkeys, size_t n_pubkeys, const unsigned char *msg32) {
    unsigned char session_rand[3][32];
    unsigned char sig64[64];
    unsigned char sig64_replay[64];
    unsigned char zero132[132] = { 0 };
    secp256k1_musig_secnonce secnonce[3];
    secp256k1_musig_pubnonce pubnonce[3];
    const secp256k1_musig_pubnonce *pubnonce_ptrs[3];
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_session session;
    secp256k1_musig_session session_replay;
    secp256k1_musig_partial_sig partial_sig[3];
    const secp256k1_musig_partial_sig *partial_sig_ptrs[3];
    const secp256k1_pubkey *pubkey_ptrs[3];
    secp256k1_musig_keyagg_cache keyagg_cache;
    secp256k1_xonly_pubkey agg_pk;
    size_t n_signers = n_pubkeys < 2 ? 2 : n_pubkeys;
    size_t i;

    for (i = 0; i < n_signers; i++) {
        pubkey_ptrs[i] = &pubkeys[i];
    }
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_pk, &keyagg_cache, pubkey_ptrs, n_signers) == 1);
    secp256k1_fuzz_musig_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_musig_sha256_compression);
    for (i = 0; i < n_signers; i++) {
        secp256k1_fuzz_derive(session_rand[i], sizeof(session_rand[i]), input, size, 211u + (unsigned int)i);
        if (memcmp(session_rand[i], secp256k1_fuzz_scalar_zero, sizeof(session_rand[i])) == 0) {
            memcpy(session_rand[i], secp256k1_fuzz_scalar_one, sizeof(session_rand[i]));
        }
        secp256k1_fuzz_musig_sha256_compression_calls = 0;
        FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce[i], &pubnonce[i], session_rand[i], seckey[i], &pubkeys[i], msg32, &keyagg_cache, NULL) == 1);
        FUZZ_CHECK(secp256k1_fuzz_musig_sha256_compression_calls != 0);
        FUZZ_CHECK(memcmp(session_rand[i], secp256k1_fuzz_scalar_zero, sizeof(session_rand[i])) == 0);
        pubnonce_ptrs[i] = &pubnonce[i];
    }
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, n_signers) == 1);
    secp256k1_fuzz_check_musig_nonce_process_failure_cleanup(ctx, &aggnonce, msg32, &keyagg_cache);
    secp256k1_fuzz_musig_noncecoef_sha256_compression_calls = 0;
    secp256k1_fuzz_musig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, &keyagg_cache) == 1);
    FUZZ_CHECK(secp256k1_fuzz_musig_noncecoef_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_musig_challenge_sha256_compression_calls != 0);
    secp256k1_fuzz_musig_noncecoef_sha256_compression_calls = 0;
    secp256k1_fuzz_musig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session_replay, &aggnonce, msg32, &keyagg_cache) == 1);
    FUZZ_CHECK(secp256k1_fuzz_musig_noncecoef_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_musig_challenge_sha256_compression_calls != 0);
    secp256k1_context_set_sha256_compression(ctx, NULL);
    for (i = 0; i < n_signers; i++) {
        FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig[i], &secnonce[i], &keypairs[i], &keyagg_cache, &session) == 1);
        FUZZ_CHECK(memcmp(secnonce[i].data, zero132, sizeof(secnonce[i].data)) == 0);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], &keyagg_cache, &session) == 1);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], &keyagg_cache, &session_replay) == 1);
        partial_sig_ptrs[i] = &partial_sig[i];
    }
    memset(sig64, 0xA5, sizeof(sig64));
    memset(sig64_replay, 0x5A, sizeof(sig64_replay));
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sig_ptrs, n_signers) == 1);
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64_replay, &session_replay, partial_sig_ptrs, n_signers) == 1);
    FUZZ_CHECK(memcmp(sig64, sig64_replay, sizeof(sig64)) == 0);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, 32, &agg_pk) == 1);
    secp256k1_fuzz_check_musig_partial_sig_agg_failure_cleanup(ctx, &session, partial_sig_ptrs, n_signers);
}

static void secp256k1_fuzz_check_musig_nonce_gen_failure_cleanup(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *valid_seckey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32, const unsigned char *session_rand32) {
    unsigned char session_rand[32];
    unsigned char zero132[132] = { 0 };
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;

    memcpy(session_rand, session_rand32, sizeof(session_rand));
    if (memcmp(session_rand, secp256k1_fuzz_scalar_zero, sizeof(session_rand)) == 0) {
        memcpy(session_rand, secp256k1_fuzz_scalar_one, sizeof(session_rand));
    }
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_rand, secp256k1_fuzz_scalar_zero, pubkey, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);

    memcpy(session_rand, session_rand32, sizeof(session_rand));
    if (memcmp(session_rand, secp256k1_fuzz_scalar_zero, sizeof(session_rand)) == 0) {
        memcpy(session_rand, secp256k1_fuzz_scalar_one, sizeof(session_rand));
    }
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_rand, secp256k1_fuzz_scalar_order, pubkey, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);

    memset(session_rand, 0, sizeof(session_rand));
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_rand, valid_seckey, pubkey, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);
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
    unsigned char fixed_pubnonce66[66];
    unsigned char invalid_aggnonce66[66];
    unsigned char mixed_aggnonce66[66];
    unsigned char mixed_pubnonce66[66];
    unsigned char sig32[32];
    unsigned char ones32[32];
    unsigned char session_rand[32];
    secp256k1_pubkey pubkeys[3];
    secp256k1_pubkey fixed_pubkeys[2];
    secp256k1_keypair keypairs[3];
    const secp256k1_pubkey *pubkey_ptrs[3];
    secp256k1_pubkey agg_full;
    secp256k1_pubkey cache_full;
    secp256k1_pubkey expected_tweaked_full;
    secp256k1_pubkey musig_tweaked_full;
    secp256k1_pubkey one_expected_full;
    secp256k1_pubkey one_tweaked_full;
    secp256k1_pubkey zero_tweaked_full;
    secp256k1_xonly_pubkey agg_xonly;
    secp256k1_xonly_pubkey single_agg_xonly;
    secp256k1_xonly_pubkey agg_xonly_from_full;
    secp256k1_xonly_pubkey tweaked_xonly;
    secp256k1_musig_keyagg_cache cache;
    secp256k1_musig_keyagg_cache single_cache;
    secp256k1_musig_keyagg_cache cache_no_output;
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
    for (i = 0; i < 3; i++) {
        secp256k1_fuzz_valid_seckey32(ctx, seckey[i], input, size, 163u + (unsigned int)i);
        FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypairs[i], seckey[i]) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckey[i]) == 1);
        pubkey_ptrs[i] = &pubkeys[i];
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &fixed_pubkeys[0], secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &fixed_pubkeys[1], secp256k1_fuzz_scalar_order_minus_one) == 1);
    secp256k1_fuzz_scalar32(tweak, input, size, 173);
    secp256k1_fuzz_derive(session_rand, sizeof(session_rand), input, size, 197);
    if (memcmp(session_rand, secp256k1_fuzz_scalar_zero, sizeof(session_rand)) == 0) {
        memcpy(session_rand, secp256k1_fuzz_scalar_one, sizeof(session_rand));
    }
    aggnonce_part_len = 33;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, valid_aggnonce66, &aggnonce_part_len, &pubkeys[0], SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(aggnonce_part_len == 33);
    aggnonce_part_len = 33;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, valid_aggnonce66 + 33, &aggnonce_part_len, &pubkeys[n_pubkeys - 1], SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(aggnonce_part_len == 33);
    aggnonce_part_len = 33;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, fixed_pubnonce66, &aggnonce_part_len, &fixed_pubkeys[0], SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(aggnonce_part_len == 33);
    aggnonce_part_len = 33;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, fixed_pubnonce66 + 33, &aggnonce_part_len, &fixed_pubkeys[1], SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(aggnonce_part_len == 33);
    memcpy(mixed_aggnonce66, zero66, sizeof(mixed_aggnonce66));
    memcpy(mixed_aggnonce66 + 33, valid_aggnonce66 + 33, 33);
    memcpy(invalid_aggnonce66, valid_aggnonce66, sizeof(invalid_aggnonce66));
    invalid_aggnonce66[0] = 0x05;
    memcpy(mixed_pubnonce66, valid_aggnonce66, 33);
    memset(mixed_pubnonce66 + 33, 0, 33);
    memset(ones32, 0xFF, sizeof(ones32));

    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_xonly, &cache, pubkey_ptrs, n_pubkeys) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &single_agg_xonly, &single_cache, pubkey_ptrs, 1) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, NULL, &cache_no_output, pubkey_ptrs, n_pubkeys) == 1);
    secp256k1_fuzz_check_musig_keyagg_hash_routing(ctx, pubkey_ptrs, n_pubkeys, &agg_xonly, &cache);
    secp256k1_fuzz_check_musig_nonce_gen_counter(ctx, input, size, seckey[0], &keypairs[0], &pubkeys[0], tweak, &cache, session_rand);
    secp256k1_fuzz_check_musig_zero_counter_sign(ctx, &keypairs[0], &pubkeys[0], tweak, &single_cache, &single_agg_xonly);
    secp256k1_fuzz_check_musig_infinity_nonce_process(ctx, &keypairs[0], &pubkeys[0], tweak, &single_cache);
    secp256k1_fuzz_check_musig_sign_roundtrip(ctx, input, size, seckey, keypairs, pubkeys, n_pubkeys, tweak);
    secp256k1_fuzz_check_musig_nonce_gen_failure_cleanup(ctx, &pubkeys[0], seckey[0], tweak, &cache, tweak, session_rand);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &agg_full, &cache) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_full, &cache_no_output) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &cache_full, &agg_full) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &agg_xonly_from_full, NULL, &agg_full) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &agg_xonly, &agg_xonly_from_full) == 0);
    secp256k1_fuzz_check_musig_tweak_overflow_rollback(ctx, secp256k1_musig_pubkey_ec_tweak_add, &cache);
    secp256k1_fuzz_check_musig_tweak_overflow_rollback(ctx, secp256k1_musig_pubkey_xonly_tweak_add, &cache);

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
        secp256k1_fuzz_check_musig_tweak_overflow_rollback(ctx, secp256k1_musig_pubkey_ec_tweak_add, &one_tweak_cache);
        secp256k1_fuzz_check_musig_tweak_overflow_rollback(ctx, secp256k1_musig_pubkey_xonly_tweak_add, &one_tweak_cache);
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
        secp256k1_fuzz_check_musig_tweak_overflow_rollback(ctx, secp256k1_musig_pubkey_ec_tweak_add, &tweak_cache);
        secp256k1_fuzz_check_musig_tweak_overflow_rollback(ctx, secp256k1_musig_pubkey_xonly_tweak_add, &tweak_cache);
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
        secp256k1_fuzz_check_musig_tweak_overflow_rollback(ctx, secp256k1_musig_pubkey_ec_tweak_add, &tweak_cache);
        secp256k1_fuzz_check_musig_tweak_overflow_rollback(ctx, secp256k1_musig_pubkey_xonly_tweak_add, &tweak_cache);
    }

    secp256k1_fuzz_derive(nonce66, sizeof(nonce66), input, size, 181);
    secp256k1_fuzz_check_musig_nonce_agg(ctx, valid_aggnonce66, fixed_pubnonce66);
    secp256k1_fuzz_check_musig_nonce_agg_inverse(ctx, valid_aggnonce66);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, zero66);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, valid_aggnonce66);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, mixed_aggnonce66);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, mixed_pubnonce66);
    secp256k1_fuzz_check_musig_pubnonce_parse(ctx, nonce66);
    secp256k1_fuzz_derive(nonce66, sizeof(nonce66), input, size, 191);
    secp256k1_fuzz_check_musig_aggnonce_parse(ctx, zero66);
    secp256k1_fuzz_check_musig_aggnonce_parse(ctx, valid_aggnonce66);
    secp256k1_fuzz_check_musig_aggnonce_parse(ctx, invalid_aggnonce66);
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
