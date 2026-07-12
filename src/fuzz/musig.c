/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"
#include "../field_impl.h"
#include "../int128_impl.h"

#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_MUSIG)
typedef int (*secp256k1_fuzz_musig_partial_sig_agg_fn)(const secp256k1_context *, unsigned char *, const secp256k1_musig_session *, const secp256k1_musig_partial_sig * const*, size_t);

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

static void secp256k1_fuzz_musig_reduce_scalar(unsigned char out[32], const unsigned char in[32]) {
    size_t i;

    memcpy(out, in, 32);
    if (memcmp(out, secp256k1_fuzz_scalar_order, 32) >= 0) {
        unsigned int borrow = 0;
        for (i = 32; i-- > 0;) {
            unsigned int minuend = out[i];
            unsigned int subtrahend = secp256k1_fuzz_scalar_order[i] + borrow;
            out[i] = (unsigned char)(minuend - subtrahend);
            borrow = minuend < subtrahend;
        }
    }
}

static void secp256k1_fuzz_musig_check_noncecoef_reference(const secp256k1_context *ctx, const secp256k1_musig_aggnonce *aggnonce, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *msg32, const secp256k1_musig_session *session) {
    static const unsigned char noncecoef_tag[] = "MuSig/noncecoef";
    secp256k1_hash_ctx hash_ctx;
    secp256k1_sha256 sha;
    secp256k1_pubkey aggregate_pubkey;
    unsigned char aggnonce66[66];
    unsigned char aggregate_serialized[65];
    unsigned char aggregate_x32[32];
    unsigned char taghash[32];
    unsigned char noncecoef_hash[32];
    unsigned char expected_noncecoef[32];
    size_t aggregate_serialized_len = sizeof(aggregate_serialized);

    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, aggnonce66, aggnonce) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &aggregate_pubkey, keyagg_cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, aggregate_serialized, &aggregate_serialized_len, &aggregate_pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(aggregate_serialized_len == sizeof(aggregate_serialized));
    memcpy(aggregate_x32, aggregate_serialized + 1, sizeof(aggregate_x32));

    /* Recompute TaggedHash with the generic SHA256 interface so the expected
     * coefficient does not share MuSig's fixed-midstate implementation. */
    secp256k1_hash_ctx_init(&hash_ctx);
    secp256k1_sha256_initialize(&sha);
    secp256k1_sha256_write(&hash_ctx, &sha, noncecoef_tag, sizeof(noncecoef_tag) - 1);
    secp256k1_sha256_finalize(&hash_ctx, &sha, taghash);
    secp256k1_sha256_initialize(&sha);
    secp256k1_sha256_write(&hash_ctx, &sha, taghash, sizeof(taghash));
    secp256k1_sha256_write(&hash_ctx, &sha, taghash, sizeof(taghash));
    secp256k1_sha256_write(&hash_ctx, &sha, aggnonce66, sizeof(aggnonce66));
    secp256k1_sha256_write(&hash_ctx, &sha, aggregate_x32, sizeof(aggregate_x32));
    secp256k1_sha256_write(&hash_ctx, &sha, msg32, 32);
    secp256k1_sha256_finalize(&hash_ctx, &sha, noncecoef_hash);
    secp256k1_sha256_clear(&sha);

    secp256k1_fuzz_musig_reduce_scalar(expected_noncecoef, noncecoef_hash);
    FUZZ_CHECK(memcmp(expected_noncecoef, session->data + 37, sizeof(expected_noncecoef)) == 0);
}

static void secp256k1_fuzz_musig_tagged_hash_reference(unsigned char out32[32], const unsigned char *tag, size_t taglen, const unsigned char *msg, size_t msglen) {
    secp256k1_hash_ctx hash_ctx;
    secp256k1_sha256 sha;
    unsigned char taghash[32];

    secp256k1_hash_ctx_init(&hash_ctx);
    secp256k1_sha256_initialize(&sha);
    secp256k1_sha256_write(&hash_ctx, &sha, tag, taglen);
    secp256k1_sha256_finalize(&hash_ctx, &sha, taghash);
    secp256k1_sha256_initialize(&sha);
    secp256k1_sha256_write(&hash_ctx, &sha, taghash, sizeof(taghash));
    secp256k1_sha256_write(&hash_ctx, &sha, taghash, sizeof(taghash));
    secp256k1_sha256_write(&hash_ctx, &sha, msg, msglen);
    secp256k1_sha256_finalize(&hash_ctx, &sha, out32);
    secp256k1_sha256_clear(&sha);
}

static int secp256k1_fuzz_musig_nonce_reference(const secp256k1_context *ctx, unsigned char expected_k64[64], unsigned char expected_pubnonce66[66], const unsigned char *session_secrand32, const unsigned char *seckey32, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    static const unsigned char aux_tag[] = "MuSig/aux";
    static const unsigned char nonce_tag[] = "MuSig/nonce";
    unsigned char rand32[32];
    unsigned char aux_hash[32];
    unsigned char nonce_hash[32];
    unsigned char pk33[33];
    unsigned char aggregate_serialized[65];
    unsigned char aggregate_x32[32];
    unsigned char nonce_input[177];
    size_t pk_len = sizeof(pk33);
    size_t aggregate_len = sizeof(aggregate_serialized);
    size_t nonce_input_len;
    size_t i;

    FUZZ_CHECK(session_secrand32 != NULL);
    FUZZ_CHECK(pubkey != NULL);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, pk33, &pk_len, pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(pk_len == sizeof(pk33));

    if (seckey32 != NULL) {
        secp256k1_fuzz_musig_tagged_hash_reference(aux_hash, aux_tag, sizeof(aux_tag) - 1, session_secrand32, 32);
        for (i = 0; i < sizeof(rand32); i++) {
            rand32[i] = (unsigned char)(aux_hash[i] ^ seckey32[i]);
        }
    } else {
        memcpy(rand32, session_secrand32, sizeof(rand32));
    }

    nonce_input_len = 0;
    memcpy(nonce_input + nonce_input_len, rand32, sizeof(rand32));
    nonce_input_len += sizeof(rand32);
    nonce_input[nonce_input_len++] = (unsigned char)sizeof(pk33);
    memcpy(nonce_input + nonce_input_len, pk33, sizeof(pk33));
    nonce_input_len += sizeof(pk33);
    if (keyagg_cache != NULL) {
        secp256k1_pubkey aggregate_pubkey;
        FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &aggregate_pubkey, keyagg_cache) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, aggregate_serialized, &aggregate_len, &aggregate_pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
        FUZZ_CHECK(aggregate_len == sizeof(aggregate_serialized));
        memcpy(aggregate_x32, aggregate_serialized + 1, sizeof(aggregate_x32));
        nonce_input[nonce_input_len++] = (unsigned char)sizeof(aggregate_x32);
        memcpy(nonce_input + nonce_input_len, aggregate_x32, sizeof(aggregate_x32));
        nonce_input_len += sizeof(aggregate_x32);
    } else {
        nonce_input[nonce_input_len++] = 0;
    }
    nonce_input[nonce_input_len++] = (unsigned char)(msg32 != NULL);
    if (msg32 != NULL) {
        memset(nonce_input + nonce_input_len, 0, 7);
        nonce_input_len += 7;
        nonce_input[nonce_input_len++] = 32;
        memcpy(nonce_input + nonce_input_len, msg32, 32);
        nonce_input_len += 32;
    }
    memset(nonce_input + nonce_input_len, 0, 3);
    nonce_input_len += 3;
    nonce_input[nonce_input_len++] = (unsigned char)(extra_input32 != NULL ? 32 : 0);
    if (extra_input32 != NULL) {
        memcpy(nonce_input + nonce_input_len, extra_input32, 32);
        nonce_input_len += 32;
    }

    for (i = 0; i < 2; i++) {
        nonce_input[nonce_input_len] = (unsigned char)i;
        secp256k1_fuzz_musig_tagged_hash_reference(nonce_hash, nonce_tag, sizeof(nonce_tag) - 1, nonce_input, nonce_input_len + 1);
        secp256k1_fuzz_musig_reduce_scalar(expected_k64 + 32 * i, nonce_hash);
    }
    if (memcmp(expected_k64, secp256k1_fuzz_scalar_zero, 32) == 0 || memcmp(expected_k64 + 32, secp256k1_fuzz_scalar_zero, 32) == 0) {
        memset(expected_k64, 0, 64);
        memset(expected_pubnonce66, 0, 66);
        secp256k1_memclear_explicit(rand32, sizeof(rand32));
        secp256k1_memclear_explicit(aux_hash, sizeof(aux_hash));
        secp256k1_memclear_explicit(nonce_hash, sizeof(nonce_hash));
        secp256k1_memclear_explicit(nonce_input, sizeof(nonce_input));
        return 0;
    }

    for (i = 0; i < 2; i++) {
        secp256k1_pubkey nonce_pubkey;
        size_t nonce_len = 33;
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &nonce_pubkey, expected_k64 + 32 * i) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, expected_pubnonce66 + 33 * i, &nonce_len, &nonce_pubkey, SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(nonce_len == 33);
    }
    secp256k1_memclear_explicit(rand32, sizeof(rand32));
    secp256k1_memclear_explicit(aux_hash, sizeof(aux_hash));
    secp256k1_memclear_explicit(nonce_hash, sizeof(nonce_hash));
    secp256k1_memclear_explicit(nonce_input, sizeof(nonce_input));
    return 1;
}

static void secp256k1_fuzz_check_musig_keyagg_reference(const secp256k1_context *ctx) {
    static const unsigned char keyagg_list_tag[] = "KeyAgg list";
    static const unsigned char keyagg_coef_tag[] = "KeyAgg coefficient";
    unsigned char scalar_two[32] = { 0 };
    unsigned char serialized[66];
    unsigned char coefficient_input[65];
    unsigned char coefficient_hash[32];
    unsigned char coefficient[32];
    size_t serialized_len;
    secp256k1_pubkey pubkey_one;
    secp256k1_pubkey pubkey_two;
    secp256k1_pubkey scaled_one;
    secp256k1_pubkey expected_full;
    secp256k1_pubkey actual_full;
    secp256k1_xonly_pubkey expected_xonly;
    secp256k1_xonly_pubkey actual_xonly;
    secp256k1_musig_keyagg_cache cache;
    const secp256k1_pubkey *pubkey_ptrs[2];
    const secp256k1_pubkey *expected_ptrs[2];

    scalar_two[31] = 2;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_one, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_two, scalar_two) == 1);
    pubkey_ptrs[0] = &pubkey_one;
    pubkey_ptrs[1] = &pubkey_two;
    serialized_len = 33;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized, &serialized_len, &pubkey_one, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(serialized_len == 33);
    serialized_len = 33;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized + 33, &serialized_len, &pubkey_two, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(serialized_len == 33);
    FUZZ_CHECK(secp256k1_tagged_sha256(ctx, coefficient_hash, keyagg_list_tag, sizeof(keyagg_list_tag) - 1, serialized, sizeof(serialized)) == 1);

    memcpy(coefficient_input, coefficient_hash, 32);
    memcpy(coefficient_input + 32, serialized, 33);
    FUZZ_CHECK(secp256k1_tagged_sha256(ctx, coefficient_hash, keyagg_coef_tag, sizeof(keyagg_coef_tag) - 1, coefficient_input, sizeof(coefficient_input)) == 1);
    secp256k1_fuzz_musig_reduce_scalar(coefficient, coefficient_hash);
    FUZZ_CHECK(memcmp(coefficient, secp256k1_fuzz_scalar_zero, sizeof(coefficient)) != 0);

    scaled_one = pubkey_one;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &scaled_one, coefficient) == 1);
    expected_ptrs[0] = &scaled_one;
    expected_ptrs[1] = &pubkey_two;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected_full, expected_ptrs, 2) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &expected_xonly, NULL, &expected_full) == 1);

    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly, &cache, pubkey_ptrs, 2) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &actual_full, &cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &actual_full, &expected_full) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &actual_xonly, &expected_xonly) == 0);
}

static void secp256k1_fuzz_check_musig_three_keyagg_reference(const secp256k1_context *ctx) {
    static const unsigned char keyagg_list_tag[] = "KeyAgg list";
    static const unsigned char keyagg_coef_tag[] = "KeyAgg coefficient";
    unsigned char scalar_two[32] = { 0 };
    unsigned char scalar_three[32] = { 0 };
    unsigned char serialized[3][33];
    unsigned char keyagg_hash[32];
    unsigned char coefficient_input[65];
    unsigned char coefficient_hash[32];
    unsigned char coefficient[32];
    secp256k1_pubkey pubkeys[3];
    secp256k1_pubkey scaled[3];
    secp256k1_pubkey expected_full;
    secp256k1_pubkey actual_full;
    secp256k1_xonly_pubkey expected_xonly;
    secp256k1_xonly_pubkey actual_xonly;
    secp256k1_musig_keyagg_cache cache;
    const secp256k1_pubkey *pubkey_ptrs[3];
    const secp256k1_pubkey *terms[3];
    size_t serialized_len;
    size_t i;
    size_t second_index = 0;

    scalar_two[31] = 2;
    scalar_three[31] = 3;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[0], secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[1], scalar_two) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[2], scalar_three) == 1);
    for (i = 0; i < 3; i++) {
        pubkey_ptrs[i] = &pubkeys[i];
        serialized_len = sizeof(serialized[i]);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized[i], &serialized_len, &pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == sizeof(serialized[i]));
    }

    /* Compute the three-key coefficient rule without MuSig's fixed midstates.
     * The first distinct key after pk[0] receives coefficient one; every other
     * key is hashed, which is the branch a two-key reference cannot exercise. */
    secp256k1_fuzz_musig_tagged_hash_reference(keyagg_hash, keyagg_list_tag, sizeof(keyagg_list_tag) - 1, &serialized[0][0], sizeof(serialized));
    for (i = 1; i < 3; i++) {
        if (memcmp(serialized[0], serialized[i], sizeof(serialized[0])) != 0) {
            second_index = i;
            break;
        }
    }
    FUZZ_CHECK(second_index != 0);
    for (i = 0; i < 3; i++) {
        if (i == second_index) {
            memcpy(coefficient, secp256k1_fuzz_scalar_one, sizeof(coefficient));
        } else {
            memcpy(coefficient_input, keyagg_hash, sizeof(keyagg_hash));
            memcpy(coefficient_input + sizeof(keyagg_hash), serialized[i], sizeof(serialized[i]));
            secp256k1_fuzz_musig_tagged_hash_reference(coefficient_hash, keyagg_coef_tag, sizeof(keyagg_coef_tag) - 1, coefficient_input, sizeof(coefficient_input));
            secp256k1_fuzz_musig_reduce_scalar(coefficient, coefficient_hash);
            FUZZ_CHECK(memcmp(coefficient, secp256k1_fuzz_scalar_zero, sizeof(coefficient)) != 0);
        }
        scaled[i] = pubkeys[i];
        if (memcmp(coefficient, secp256k1_fuzz_scalar_one, sizeof(coefficient)) != 0) {
            FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &scaled[i], coefficient) == 1);
        }
        terms[i] = &scaled[i];
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected_full, terms, 3) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &expected_xonly, NULL, &expected_full) == 1);

    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly, &cache, pubkey_ptrs, 3) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &actual_full, &cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &actual_full, &expected_full) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &actual_xonly, &expected_xonly) == 0);
    FUZZ_CHECK(memcmp(cache.data + 132, keyagg_hash, sizeof(keyagg_hash)) == 0);
}

static void secp256k1_fuzz_check_musig_tweaked_sign_case(const secp256k1_context *ctx, const secp256k1_keypair *keypairs, const secp256k1_pubkey *pubkeys, const secp256k1_musig_keyagg_cache *cache, const secp256k1_xonly_pubkey *agg_xonly) {
    unsigned char msg32[32] = { 0 };
    unsigned char sig64[64];
    secp256k1_musig_secnonce secnonce[2];
    secp256k1_musig_pubnonce pubnonce[2];
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_session session;
    secp256k1_musig_partial_sig partial_sig[2];
    const secp256k1_musig_pubnonce *pubnonce_ptrs[2];
    const secp256k1_musig_partial_sig *partial_sig_ptrs[2];
    size_t i;

    for (i = 0; i < 2; i++) {
        FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce[i], &pubnonce[i], (uint64_t)(i + 1), &keypairs[i], msg32, cache, NULL) == 1);
        pubnonce_ptrs[i] = &pubnonce[i];
        partial_sig_ptrs[i] = &partial_sig[i];
    }
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 2) == 1);
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, cache) == 1);
    secp256k1_fuzz_musig_check_noncecoef_reference(ctx, &aggnonce, cache, msg32, &session);
    for (i = 0; i < 2; i++) {
        FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig[i], &secnonce[i], &keypairs[i], cache, &session) == 1);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], cache, &session) == 1);
    }
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sig_ptrs, 2) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), agg_xonly) == 1);
}

static void secp256k1_fuzz_check_musig_tweaked_signing(const secp256k1_context *ctx) {
    unsigned char scalar_two[32] = { 0 };
    unsigned char tweak[32] = { 0 };
    secp256k1_keypair keypairs[2];
    secp256k1_pubkey pubkeys[2];
    secp256k1_xonly_pubkey base_xonly;
    secp256k1_xonly_pubkey tweaked_xonly;
    secp256k1_musig_keyagg_cache base_cache;
    secp256k1_musig_keyagg_cache tweaked_cache;
    secp256k1_pubkey tweaked_full;
    const secp256k1_pubkey *pubkey_ptrs[2];
    unsigned int parity_seen[2] = { 0, 0 };
    unsigned int parity_tested[2] = { 0, 0 };
    unsigned int counter;

    scalar_two[31] = 2;
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypairs[0], secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypairs[1], scalar_two) == 1);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &pubkeys[0], &keypairs[0]) == 1);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &pubkeys[1], &keypairs[1]) == 1);
    pubkey_ptrs[0] = &pubkeys[0];
    pubkey_ptrs[1] = &pubkeys[1];
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &base_xonly, &base_cache, pubkey_ptrs, 2) == 1);

    /* Small plain tweaks are deterministic and provide both final-key parity
     * branches without depending on the fuzzer input distribution. */
    for (counter = 1; counter <= 32; counter++) {
        int parity;
        memset(tweak, 0, sizeof(tweak));
        tweak[31] = (unsigned char)counter;
        tweaked_cache = base_cache;
        FUZZ_CHECK(secp256k1_musig_pubkey_ec_tweak_add(ctx, &tweaked_full, &tweaked_cache, tweak) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &tweaked_xonly, &parity, &tweaked_full) == 1);
        FUZZ_CHECK(parity == 0 || parity == 1);
        parity_seen[parity] = 1;
        if (!parity_tested[parity]) {
            secp256k1_fuzz_check_musig_tweaked_sign_case(ctx, keypairs, pubkeys, &tweaked_cache, &tweaked_xonly);
            parity_tested[parity] = 1;
        }
    }
    FUZZ_CHECK(parity_seen[0] != 0);
    FUZZ_CHECK(parity_seen[1] != 0);
    FUZZ_CHECK(parity_tested[0] != 0);
    FUZZ_CHECK(parity_tested[1] != 0);
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
    secp256k1_musig_partial_sig overflow_sig = *valid_sig;

    invalid_sig.data[0] ^= 1u;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    memset(serialized, 0xA5, sizeof(serialized));
    FUZZ_CHECK(secp256k1_musig_partial_sig_serialize(ctx, serialized, &invalid_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(serialized, zero32, sizeof(serialized)) == 0);

    memset(overflow_sig.data + 4, 0xFF, 32);
    memset(serialized, 0x5A, sizeof(serialized));
    FUZZ_CHECK(secp256k1_musig_partial_sig_serialize(ctx, serialized, &overflow_sig) == 0);
    FUZZ_CHECK(illegal_data.calls == 2);
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

    /* A malformed suffix must not be ignored after valid public nonces have
     * already been accumulated. */
    if (n_pubnonces > 1) {
        for (i = 0; i < n_pubnonces; i++) {
            invalid_pubnonce_ptrs[i] = pubnonce_ptrs[i];
        }
        invalid_pubnonce = *pubnonce_ptrs[n_pubnonces - 1];
        invalid_pubnonce.data[0] ^= 1u;
        invalid_pubnonce_ptrs[n_pubnonces - 1] = &invalid_pubnonce;
        memset(&aggnonce, 0x5A, sizeof(aggnonce));
        FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, invalid_pubnonce_ptrs, n_pubnonces) == 0);
        FUZZ_CHECK(illegal_data.calls == 2);
        FUZZ_CHECK(memcmp(&aggnonce, zero_aggnonce, sizeof(aggnonce)) == 0);

        for (i = 0; i < n_pubnonces; i++) {
            invalid_pubnonce_ptrs[i] = pubnonce_ptrs[i];
        }
        invalid_pubnonce = *pubnonce_ptrs[n_pubnonces - 1];
        memset(invalid_pubnonce.data + 4, 0, 64);
        invalid_pubnonce.data[4] = 1;
        invalid_pubnonce_ptrs[n_pubnonces - 1] = &invalid_pubnonce;
        memset(&aggnonce, 0x96, sizeof(aggnonce));
        FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, invalid_pubnonce_ptrs, n_pubnonces) == 0);
        FUZZ_CHECK(illegal_data.calls == 3);
        FUZZ_CHECK(memcmp(&aggnonce, zero_aggnonce, sizeof(aggnonce)) == 0);
    }
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

static void secp256k1_fuzz_negate_musig_pubnonce_part(const secp256k1_context *ctx, unsigned char *negated66, const unsigned char *nonce66, size_t part) {
    secp256k1_pubkey pubkey;
    size_t serialized_len;

    FUZZ_CHECK(part < 2);
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &pubkey, nonce66 + 33*part, 33) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &pubkey) == 1);
    serialized_len = 33;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, negated66 + 33*part, &serialized_len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(serialized_len == 33);
}

static void secp256k1_fuzz_negate_musig_pubnonce_parts(const secp256k1_context *ctx, unsigned char *negated66, const unsigned char *nonce66) {
    memcpy(negated66, nonce66, 66);
    secp256k1_fuzz_negate_musig_pubnonce_part(ctx, negated66, nonce66, 0);
    secp256k1_fuzz_negate_musig_pubnonce_part(ctx, negated66, nonce66, 1);
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

static void secp256k1_fuzz_check_musig_noncanonical_duplicate(secp256k1_context *ctx) {
    static const unsigned char point_x_one[33] = {
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01
    };
    static const unsigned char field_p_plus_one[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
    };
    secp256k1_pubkey canonical_pk;
    secp256k1_pubkey noncanonical_pk;
    secp256k1_xonly_pubkey valid_agg;
    secp256k1_fe noncanonical_x;
    secp256k1_fe_storage x_storage;
    const secp256k1_pubkey *noncanonical_ptrs[2];
    const secp256k1_pubkey *cache_pubkey_ptrs[1];
    size_t serialized_len;
    unsigned char noncanonical_serialized[33];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    unsigned char zero_agg[sizeof(secp256k1_xonly_pubkey)] = { 0 };
    unsigned char zero_cache[sizeof(secp256k1_musig_keyagg_cache)] = { 0 };
    unsigned char zero_secnonce[sizeof(secp256k1_musig_secnonce)] = { 0 };
    unsigned char zero_pubnonce[sizeof(secp256k1_musig_pubnonce)] = { 0 };
    secp256k1_xonly_pubkey failed_agg;
    secp256k1_musig_keyagg_cache failed_cache;
    secp256k1_musig_keyagg_cache valid_cache;
    secp256k1_musig_keyagg_cache invalid_cache;
    secp256k1_musig_keyagg_cache cache_before;
    secp256k1_pubkey cache_output;
    secp256k1_musig_secnonce failed_secnonce;
    secp256k1_musig_pubnonce failed_pubnonce;
    unsigned char nonce_rand[32];
    unsigned char nonce_rand_before[32];
    secp256k1_fuzz_musig_illegal_data illegal_data;
    unsigned int calls;

    STATIC_ASSERT(sizeof(secp256k1_fe_storage) == 32);
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &canonical_pk, point_x_one, sizeof(point_x_one)) == 1);
    noncanonical_pk = canonical_pk;
    secp256k1_fe_set_b32_mod(&noncanonical_x, field_p_plus_one);
    secp256k1_fe_impl_to_storage(&x_storage, &noncanonical_x);
    memcpy(noncanonical_pk.data, &x_storage, sizeof(x_storage));

    FUZZ_CHECK(secp256k1_memcmp_var(&canonical_pk, &noncanonical_pk, sizeof(canonical_pk)) != 0);

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    serialized_len = sizeof(noncanonical_serialized);
    memset(noncanonical_serialized, 0xA5, sizeof(noncanonical_serialized));
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, noncanonical_serialized, &serialized_len, &noncanonical_pk, SECP256K1_EC_COMPRESSED) == 0);
    FUZZ_CHECK(serialized_len == 0);
    FUZZ_CHECK(memcmp(noncanonical_serialized, zero_pubkey, sizeof(noncanonical_serialized)) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);

    noncanonical_ptrs[0] = &canonical_pk;
    noncanonical_ptrs[1] = &noncanonical_pk;
    memset(&failed_agg, 0xA5, sizeof(failed_agg));
    memset(&failed_cache, 0xA5, sizeof(failed_cache));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &failed_agg, &failed_cache, noncanonical_ptrs, 2) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&failed_agg, zero_agg, sizeof(failed_agg)) == 0);
    FUZZ_CHECK(memcmp(&failed_cache, zero_cache, sizeof(failed_cache)) == 0);

    /* Keep the metadata from a real API-created cache. Use the fixed x = 1
     * point for the aggregate so that its p + 1 representation fits in the
     * 256-bit storage buffer while preserving a valid curve point. */
    cache_pubkey_ptrs[0] = &canonical_pk;
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &valid_agg, &valid_cache, cache_pubkey_ptrs, 1) == 1);
    memcpy(valid_cache.data + 4, canonical_pk.data, sizeof(canonical_pk.data));
    secp256k1_fe_set_b32_mod(&noncanonical_x, field_p_plus_one);
    secp256k1_fe_impl_to_storage(&x_storage, &noncanonical_x);
    invalid_cache = valid_cache;
    memcpy(invalid_cache.data + 4, &x_storage, sizeof(x_storage));
    FUZZ_CHECK(secp256k1_memcmp_var(&valid_cache, &invalid_cache, sizeof(valid_cache)) != 0);
    cache_before = invalid_cache;

    memset(&cache_output, 0xA5, sizeof(cache_output));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &cache_output, &invalid_cache) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&cache_output, zero_pubkey, sizeof(cache_output)) == 0);
    FUZZ_CHECK(memcmp(&invalid_cache, &cache_before, sizeof(invalid_cache)) == 0);

    memset(nonce_rand, 0x42, sizeof(nonce_rand));
    memcpy(nonce_rand_before, nonce_rand, sizeof(nonce_rand));
    memset(&failed_secnonce, 0xA5, sizeof(failed_secnonce));
    memset(&failed_pubnonce, 0xA5, sizeof(failed_pubnonce));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &failed_secnonce, &failed_pubnonce, nonce_rand, NULL, &canonical_pk, NULL, &invalid_cache, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&failed_secnonce, zero_secnonce, sizeof(failed_secnonce)) == 0);
    FUZZ_CHECK(memcmp(&failed_pubnonce, zero_pubnonce, sizeof(failed_pubnonce)) == 0);
    FUZZ_CHECK(memcmp(nonce_rand, nonce_rand_before, sizeof(nonce_rand)) == 0);
    FUZZ_CHECK(memcmp(&invalid_cache, &cache_before, sizeof(invalid_cache)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_pubkey_agg_failure_cleanup(secp256k1_context *ctx, const secp256k1_xonly_pubkey *valid_agg_pk, const secp256k1_musig_keyagg_cache *valid_cache) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_pubkey invalid_pubkey;
    const secp256k1_pubkey *invalid_pubkey_ptrs[1];
    secp256k1_xonly_pubkey failed_agg_pk;
    secp256k1_musig_keyagg_cache failed_cache;
    unsigned char zero_agg_pk[sizeof(failed_agg_pk)] = { 0 };
    unsigned char zero_cache[sizeof(failed_cache)] = { 0 };

    memset(&invalid_pubkey, 0, sizeof(invalid_pubkey));
    invalid_pubkey_ptrs[0] = &invalid_pubkey;
    failed_agg_pk = *valid_agg_pk;
    failed_cache = *valid_cache;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &failed_agg_pk, &failed_cache, invalid_pubkey_ptrs, 1) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&failed_agg_pk, zero_agg_pk, sizeof(failed_agg_pk)) == 0);
    FUZZ_CHECK(memcmp(&failed_cache, zero_cache, sizeof(failed_cache)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_pubkey_agg_success(const secp256k1_context *ctx, const secp256k1_xonly_pubkey *agg_pk, const secp256k1_musig_keyagg_cache *keyagg_cache) {
    unsigned char xonly32[32];
    unsigned char compressed33[33];
    secp256k1_pubkey full_agg_pk;
    size_t compressed_len = sizeof(compressed33);

    if (agg_pk != NULL) {
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly32, agg_pk) == 1);
    }
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &full_agg_pk, keyagg_cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, compressed33, &compressed_len, &full_agg_pk, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(compressed_len == sizeof(compressed33));
}

static void secp256k1_fuzz_check_musig_keyagg_cache_curve_barrier(secp256k1_context *ctx, const secp256k1_pubkey *valid_pubkey, const secp256k1_musig_keyagg_cache *valid_cache) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_pubkey invalid_pubkey;
    secp256k1_pubkey output_pubkey;
    secp256k1_musig_keyagg_cache invalid_cache;
    secp256k1_musig_keyagg_cache cache_before;
    unsigned char serialized[33];
    unsigned char zero_pubkey[sizeof(output_pubkey)] = { 0 };
    size_t serialized_len;
    unsigned int calls;

    /* The second 32 bytes of the platform-native group storage hold Y. */
    invalid_pubkey = *valid_pubkey;
    invalid_pubkey.data[32] ^= 1u;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    serialized_len = sizeof(serialized);
    memset(serialized, 0xA5, sizeof(serialized));
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized, &serialized_len, &invalid_pubkey, SECP256K1_EC_COMPRESSED) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);

    invalid_cache = *valid_cache;
    memcpy(invalid_cache.data + 4, invalid_pubkey.data, sizeof(invalid_pubkey.data));
    cache_before = invalid_cache;

    memset(&output_pubkey, 0xA5, sizeof(output_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &output_pubkey, &invalid_cache) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&output_pubkey, zero_pubkey, sizeof(output_pubkey)) == 0);
    FUZZ_CHECK(memcmp(&invalid_cache, &cache_before, sizeof(invalid_cache)) == 0);

    memset(&output_pubkey, 0x5A, sizeof(output_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_pubkey_ec_tweak_add(ctx, &output_pubkey, &invalid_cache, secp256k1_fuzz_scalar_zero) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&output_pubkey, zero_pubkey, sizeof(output_pubkey)) == 0);
    FUZZ_CHECK(memcmp(&invalid_cache, &cache_before, sizeof(invalid_cache)) == 0);

    memset(&output_pubkey, 0x3C, sizeof(output_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_pubkey_xonly_tweak_add(ctx, &output_pubkey, &invalid_cache, secp256k1_fuzz_scalar_zero) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&output_pubkey, zero_pubkey, sizeof(output_pubkey)) == 0);
    FUZZ_CHECK(memcmp(&invalid_cache, &cache_before, sizeof(invalid_cache)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_keyagg_cache_semantic_barrier(secp256k1_context *ctx, const secp256k1_musig_keyagg_cache *valid_cache) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_keyagg_cache invalid_cache;
    secp256k1_musig_keyagg_cache cache_before;
    secp256k1_pubkey output_pubkey;
    unsigned char zero_tweak[32] = { 0 };
    unsigned char zero_pubkey[sizeof(output_pubkey)] = { 0 };
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    invalid_cache = *valid_cache;
    invalid_cache.data[164] = 2;
    cache_before = invalid_cache;
    memset(&output_pubkey, 0xA5, sizeof(output_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_pubkey_xonly_tweak_add(ctx, &output_pubkey, &invalid_cache, zero_tweak) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&output_pubkey, zero_pubkey, sizeof(output_pubkey)) == 0);
    FUZZ_CHECK(memcmp(&invalid_cache, &cache_before, sizeof(invalid_cache)) == 0);

    invalid_cache = *valid_cache;
    memset(invalid_cache.data + 165, 0xFF, 32);
    cache_before = invalid_cache;
    memset(&output_pubkey, 0x5A, sizeof(output_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_pubkey_ec_tweak_add(ctx, &output_pubkey, &invalid_cache, zero_tweak) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&output_pubkey, zero_pubkey, sizeof(output_pubkey)) == 0);
    FUZZ_CHECK(memcmp(&invalid_cache, &cache_before, sizeof(invalid_cache)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
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
    unsigned char expected_k64[64];
    unsigned char expected_pubnonce66[66];
    unsigned char zero32[32] = { 0 };
    unsigned char zero132[132] = { 0 };
    unsigned char serialized_counter[66];
    unsigned char serialized_explicit[66];
    uint64_t counter = secp256k1_fuzz_musig_nonzero_counter(input, size);
    size_t hash_calls;
    int counter_ret;
    int explicit_ret;
    secp256k1_musig_secnonce secnonce_counter;
    secp256k1_musig_secnonce secnonce_explicit;
    secp256k1_musig_pubnonce pubnonce_counter;
    secp256k1_musig_pubnonce pubnonce_explicit;

    secp256k1_fuzz_musig_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_musig_sha256_compression);
    counter_ret = secp256k1_musig_nonce_gen_counter(ctx, &secnonce_counter, &pubnonce_counter, counter, keypair, msg32, keyagg_cache, extra_input32);
    hash_calls = secp256k1_fuzz_musig_sha256_compression_calls;
    secp256k1_context_set_sha256_compression(ctx, NULL);
    FUZZ_CHECK(hash_calls != 0);

    secp256k1_fuzz_musig_counter_to_secrand(counter_secrand, counter);
    FUZZ_CHECK(secp256k1_fuzz_musig_nonce_reference(ctx, expected_k64, expected_pubnonce66, counter_secrand, seckey, pubkey, msg32, keyagg_cache, extra_input32) == counter_ret);
    explicit_ret = secp256k1_musig_nonce_gen(ctx, &secnonce_explicit, &pubnonce_explicit, counter_secrand, seckey, pubkey, msg32, keyagg_cache, extra_input32);
    FUZZ_CHECK(explicit_ret == counter_ret);
    if (counter_ret == 0) {
        FUZZ_CHECK(memcmp(&secnonce_counter, zero132, sizeof(secnonce_counter)) == 0);
        FUZZ_CHECK(memcmp(&pubnonce_counter, zero132, sizeof(pubnonce_counter)) == 0);
        FUZZ_CHECK(memcmp(&secnonce_explicit, zero132, sizeof(secnonce_explicit)) == 0);
        FUZZ_CHECK(memcmp(&pubnonce_explicit, zero132, sizeof(pubnonce_explicit)) == 0);
        return;
    }
    FUZZ_CHECK(memcmp(counter_secrand, zero32, sizeof(counter_secrand)) == 0);
    FUZZ_CHECK(memcmp(secnonce_counter.data, secnonce_explicit.data, sizeof(secnonce_counter.data)) == 0);
    FUZZ_CHECK(memcmp(secnonce_counter.data + 4, expected_k64, sizeof(expected_k64)) == 0);
    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized_counter, &pubnonce_counter) == 1);
    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized_explicit, &pubnonce_explicit) == 1);
    FUZZ_CHECK(memcmp(serialized_counter, serialized_explicit, sizeof(serialized_counter)) == 0);
    FUZZ_CHECK(memcmp(serialized_counter, expected_pubnonce66, sizeof(serialized_counter)) == 0);
}

static void secp256k1_fuzz_check_musig_nonce_scalar_barrier(secp256k1_context *ctx) {
    unsigned char zero32[32] = { 0 };
    unsigned char serialized66[66];
    secp256k1_keypair keypair;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, 2, &keypair, NULL, NULL, NULL) == 1);
    FUZZ_CHECK(memcmp(secnonce.data + 4, zero32, sizeof(zero32)) != 0);
    FUZZ_CHECK(memcmp(secnonce.data + 36, zero32, sizeof(zero32)) != 0);
    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized66, &pubnonce) == 1);
}

static void secp256k1_fuzz_check_musig_partial_sign_nonce_parity(secp256k1_context *ctx) {
    unsigned char msg32[32] = { 0 };
    unsigned char sig64[64];
    secp256k1_keypair keypair;
    secp256k1_pubkey pubkey;
    secp256k1_xonly_pubkey agg_pk;
    secp256k1_musig_keyagg_cache keyagg_cache;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_session session;
    secp256k1_musig_partial_sig partial_sig;
    const secp256k1_pubkey *pubkey_ptrs[1];
    const secp256k1_musig_pubnonce *pubnonce_ptrs[1];
    const secp256k1_musig_partial_sig *partial_sig_ptrs[1];
    unsigned int parity_seen[2] = { 0, 0 };
    uint64_t counter;

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &pubkey, &keypair) == 1);
    pubkey_ptrs[0] = &pubkey;
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_pk, &keyagg_cache, pubkey_ptrs, 1) == 1);
    pubnonce_ptrs[0] = &pubnonce;
    partial_sig_ptrs[0] = &partial_sig;

    /* Keep both final-nonce parity branches live. The signature check makes
     * the parity choice a functional postcondition, not just a byte check. */
    for (counter = 0; counter < 16; counter++) {
        FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, counter, &keypair, msg32, &keyagg_cache, NULL) == 1);
        FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 1) == 1);
        FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, &keyagg_cache) == 1);
        FUZZ_CHECK(session.data[4] <= 1);
        parity_seen[session.data[4]] = 1;
        FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, &keypair, &keyagg_cache, &session) == 1);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig, &pubnonce, &pubkey, &keyagg_cache, &session) == 1);
        FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sig_ptrs, 1) == 1);
        FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), &agg_pk) == 1);
    }
    FUZZ_CHECK(parity_seen[0] != 0);
    FUZZ_CHECK(parity_seen[1] != 0);
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

static void secp256k1_fuzz_check_musig_infinity_nonce_process(secp256k1_context *ctx, const secp256k1_keypair *keypair, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const secp256k1_xonly_pubkey *agg_pk) {
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
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, 32, agg_pk) == 0);
    FUZZ_CHECK(memcmp(sig64, gen_xonly32, sizeof(gen_xonly32)) == 0);
}

static void secp256k1_fuzz_check_musig_partial_sig_agg_failure_cleanup(secp256k1_context *ctx, const secp256k1_musig_session *session, const secp256k1_musig_partial_sig * const *partial_sig_ptrs, size_t n_sigs) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_fuzz_musig_partial_sig_agg_fn partial_sig_agg = secp256k1_musig_partial_sig_agg;
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

    /* The cleanup must happen before the NULL-session argument check. */
    memset(sig64, 0xA5, sizeof(sig64));
    FUZZ_CHECK(partial_sig_agg(ctx, sig64, NULL, partial_sig_ptrs, n_sigs) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    memset(sig64, 0xA5, sizeof(sig64));
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, session, invalid_partial_sig_ptrs, n_sigs) == 0);
    FUZZ_CHECK(illegal_data.calls == 2);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    invalid_partial_sig = *partial_sig_ptrs[0];
    memset(invalid_partial_sig.data + 4, 0xFF, 32);
    invalid_partial_sig_ptrs[0] = &invalid_partial_sig;
    memset(sig64, 0x5A, sizeof(sig64));
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, session, invalid_partial_sig_ptrs, n_sigs) == 0);
    FUZZ_CHECK(illegal_data.calls == 3);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    /* A malformed suffix must not be ignored after valid partial signatures
     * have already been accumulated. */
    if (n_sigs > 1) {
        for (i = 0; i < n_sigs; i++) {
            invalid_partial_sig_ptrs[i] = partial_sig_ptrs[i];
        }
        invalid_partial_sig = *partial_sig_ptrs[n_sigs - 1];
        invalid_partial_sig.data[0] ^= 1u;
        invalid_partial_sig_ptrs[n_sigs - 1] = &invalid_partial_sig;
        memset(sig64, 0x3C, sizeof(sig64));
        FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, session, invalid_partial_sig_ptrs, n_sigs) == 0);
        FUZZ_CHECK(illegal_data.calls == 4);
        FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

        for (i = 0; i < n_sigs; i++) {
            invalid_partial_sig_ptrs[i] = partial_sig_ptrs[i];
        }
        invalid_partial_sig = *partial_sig_ptrs[n_sigs - 1];
        memset(invalid_partial_sig.data + 4, 0xFF, 32);
        invalid_partial_sig_ptrs[n_sigs - 1] = &invalid_partial_sig;
        memset(sig64, 0x96, sizeof(sig64));
        FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, session, invalid_partial_sig_ptrs, n_sigs) == 0);
        FUZZ_CHECK(illegal_data.calls == 5);
        FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);
    }
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_session_state_barrier(secp256k1_context *ctx, const secp256k1_musig_session *valid_session, const secp256k1_musig_partial_sig * const *partial_sig_ptrs, size_t n_sigs) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_session invalid_session;
    unsigned char sig64[64];
    unsigned char zero64[64] = { 0 };
    unsigned int calls;

    FUZZ_CHECK(n_sigs > 0);
    FUZZ_CHECK(n_sigs <= 3);

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    invalid_session = *valid_session;
    invalid_session.data[4] = 2;
    memset(sig64, 0xA5, sizeof(sig64));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &invalid_session, partial_sig_ptrs, n_sigs) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    invalid_session = *valid_session;
    memset(invalid_session.data + 5, 0xFF, 32);
    memset(sig64, 0x5A, sizeof(sig64));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &invalid_session, partial_sig_ptrs, n_sigs) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    invalid_session = *valid_session;
    memset(invalid_session.data + 37, 0xFF, 32);
    memset(sig64, 0x3C, sizeof(sig64));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &invalid_session, partial_sig_ptrs, n_sigs) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    invalid_session = *valid_session;
    memset(invalid_session.data + 69, 0xFF, 32);
    memset(sig64, 0x96, sizeof(sig64));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &invalid_session, partial_sig_ptrs, n_sigs) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    invalid_session = *valid_session;
    memset(invalid_session.data + 101, 0xFF, 32);
    memset(sig64, 0xC3, sizeof(sig64));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &invalid_session, partial_sig_ptrs, n_sigs) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_partial_sign_failure_cleanup(secp256k1_context *ctx, const secp256k1_musig_secnonce *valid_secnonce, const secp256k1_keypair *keypair, const secp256k1_musig_keyagg_cache *valid_cache, const secp256k1_musig_session *session) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_keyagg_cache invalid_cache = *valid_cache;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_partial_sig partial_sig;
    unsigned char zero_sig[sizeof(partial_sig)] = { 0 };

    invalid_cache.data[0] ^= 1u;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    secnonce = *valid_secnonce;
    memset(&partial_sig, 0xA5, sizeof(partial_sig));
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, keypair, &invalid_cache, session) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&partial_sig, zero_sig, sizeof(partial_sig)) == 0);

    secnonce = *valid_secnonce;
    secnonce.data[0] ^= 1u;
    memset(&partial_sig, 0x5A, sizeof(partial_sig));
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, keypair, valid_cache, session) == 0);
    FUZZ_CHECK(illegal_data.calls == 2);
    FUZZ_CHECK(memcmp(&partial_sig, zero_sig, sizeof(partial_sig)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_sign_roundtrip(secp256k1_context *ctx, const unsigned char *input, size_t size, unsigned char seckey[3][32], const secp256k1_keypair *keypairs, const secp256k1_pubkey *pubkeys, size_t n_pubkeys, const unsigned char *msg32) {
    static const unsigned char scalar_two[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };
    unsigned char session_rand[3][32];
    unsigned char pubnonce_ser[66];
    unsigned char wrong_pubnonce_ser[66];
    unsigned char sig64[64];
    unsigned char sig64_replay[64];
    unsigned char zero132[132] = { 0 };
    unsigned char expected_k64[64];
    unsigned char expected_pubnonce66[66];
    secp256k1_musig_secnonce secnonce[3];
    secp256k1_musig_pubnonce pubnonce[3];
    secp256k1_musig_pubnonce wrong_pubnonce;
    secp256k1_pubkey wrong_pubkey;
    const secp256k1_musig_pubnonce *pubnonce_ptrs[3];
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_session session;
    secp256k1_musig_session session_replay;
    secp256k1_musig_session wrong_cache_session;
    secp256k1_musig_partial_sig partial_sig[3];
    const secp256k1_musig_partial_sig *partial_sig_ptrs[3];
    const secp256k1_pubkey *pubkey_ptrs[3];
    secp256k1_musig_keyagg_cache keyagg_cache;
    secp256k1_musig_keyagg_cache wrong_cache;
    secp256k1_xonly_pubkey agg_pk;
    secp256k1_xonly_pubkey wrong_cache_agg_pk;
    secp256k1_pubkey wrong_cache_pubkey;
    const secp256k1_pubkey *wrong_cache_pubkey_ptrs[1];
    size_t n_signers = n_pubkeys < 2 ? 2 : n_pubkeys;
    size_t i;

    for (i = 0; i < n_signers; i++) {
        pubkey_ptrs[i] = &pubkeys[i];
    }
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_pk, &keyagg_cache, pubkey_ptrs, n_signers) == 1);
    wrong_cache_pubkey_ptrs[0] = &wrong_cache_pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &wrong_cache_pubkey, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &wrong_cache_agg_pk, &wrong_cache, wrong_cache_pubkey_ptrs, 1) == 1);
    if (secp256k1_xonly_pubkey_cmp(ctx, &wrong_cache_agg_pk, &agg_pk) == 0) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &wrong_cache_pubkey, secp256k1_fuzz_scalar_order_minus_one) == 1);
        FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &wrong_cache_agg_pk, &wrong_cache, wrong_cache_pubkey_ptrs, 1) == 1);
    }
    if (secp256k1_xonly_pubkey_cmp(ctx, &wrong_cache_agg_pk, &agg_pk) == 0) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &wrong_cache_pubkey, scalar_two) == 1);
        FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &wrong_cache_agg_pk, &wrong_cache, wrong_cache_pubkey_ptrs, 1) == 1);
    }
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &wrong_cache_agg_pk, &agg_pk) != 0);
    secp256k1_fuzz_musig_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_musig_sha256_compression);
    for (i = 0; i < n_signers; i++) {
        secp256k1_fuzz_derive(session_rand[i], sizeof(session_rand[i]), input, size, 211u + (unsigned int)i);
        if (memcmp(session_rand[i], secp256k1_fuzz_scalar_zero, sizeof(session_rand[i])) == 0) {
            memcpy(session_rand[i], secp256k1_fuzz_scalar_one, sizeof(session_rand[i]));
        }
        FUZZ_CHECK(secp256k1_fuzz_musig_nonce_reference(ctx, expected_k64, expected_pubnonce66, session_rand[i], seckey[i], &pubkeys[i], msg32, &keyagg_cache, NULL) == 1);
        secp256k1_fuzz_musig_sha256_compression_calls = 0;
        FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce[i], &pubnonce[i], session_rand[i], seckey[i], &pubkeys[i], msg32, &keyagg_cache, NULL) == 1);
        FUZZ_CHECK(secp256k1_fuzz_musig_sha256_compression_calls != 0);
        FUZZ_CHECK(memcmp(session_rand[i], secp256k1_fuzz_scalar_zero, sizeof(session_rand[i])) == 0);
        FUZZ_CHECK(memcmp(secnonce[i].data + 4, expected_k64, sizeof(expected_k64)) == 0);
        FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, pubnonce_ser, &pubnonce[i]) == 1);
        FUZZ_CHECK(memcmp(pubnonce_ser, expected_pubnonce66, sizeof(pubnonce_ser)) == 0);
        pubnonce_ptrs[i] = &pubnonce[i];
    }
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, n_signers) == 1);
    secp256k1_fuzz_check_musig_nonce_process_failure_cleanup(ctx, &aggnonce, msg32, &keyagg_cache);
    secp256k1_fuzz_musig_noncecoef_sha256_compression_calls = 0;
    secp256k1_fuzz_musig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, &keyagg_cache) == 1);
    secp256k1_fuzz_musig_check_noncecoef_reference(ctx, &aggnonce, &keyagg_cache, msg32, &session);
    FUZZ_CHECK(secp256k1_fuzz_musig_noncecoef_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_musig_challenge_sha256_compression_calls != 0);
    secp256k1_fuzz_musig_noncecoef_sha256_compression_calls = 0;
    secp256k1_fuzz_musig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session_replay, &aggnonce, msg32, &keyagg_cache) == 1);
    FUZZ_CHECK(secp256k1_fuzz_musig_noncecoef_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_musig_challenge_sha256_compression_calls != 0);
    secp256k1_context_set_sha256_compression(ctx, NULL);
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &wrong_cache_session, &aggnonce, msg32, &wrong_cache) == 1);
    secp256k1_fuzz_check_musig_partial_sign_failure_cleanup(ctx, &secnonce[0], &keypairs[0], &keyagg_cache, &session);
    for (i = 0; i < n_signers; i++) {
        FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig[i], &secnonce[i], &keypairs[i], &keyagg_cache, &session) == 1);
        FUZZ_CHECK(memcmp(secnonce[i].data, zero132, sizeof(secnonce[i].data)) == 0);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], &keyagg_cache, &session) == 1);
        /* A session and cache from different key aggregations must not be
         * interchangeable, even when both opaque objects are valid. */
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], &wrong_cache, &session) == 0);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], &keyagg_cache, &wrong_cache_session) == 0);
        /* The verifier must bind a partial signature to the signer's key from
         * key aggregation, not merely to any valid curve point. */
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &wrong_pubkey, secp256k1_fuzz_scalar_one) == 1);
        if (secp256k1_ec_pubkey_cmp(ctx, &wrong_pubkey, &pubkeys[i]) == 0) {
            FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &wrong_pubkey, secp256k1_fuzz_scalar_order_minus_one) == 1);
        }
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &wrong_pubkey, &pubkeys[i]) != 0);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &pubnonce[i], &wrong_pubkey, &keyagg_cache, &session) == 0);
        /* Negating only R1 changes the effective nonce by -2R1. Since R1 is a
         * nonzero generator multiple, this is a valid but different nonce. */
        FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, pubnonce_ser, &pubnonce[i]) == 1);
        memcpy(wrong_pubnonce_ser, pubnonce_ser, sizeof(wrong_pubnonce_ser));
        secp256k1_fuzz_negate_musig_pubnonce_part(ctx, wrong_pubnonce_ser, pubnonce_ser, 0);
        FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &wrong_pubnonce, wrong_pubnonce_ser) == 1);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &wrong_pubnonce, &pubkeys[i], &keyagg_cache, &session) == 0);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], &keyagg_cache, &session_replay) == 1);
        partial_sig_ptrs[i] = &partial_sig[i];
    }
    secp256k1_fuzz_check_musig_session_state_barrier(ctx, &session, partial_sig_ptrs, n_signers);
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

static void secp256k1_fuzz_check_musig_nonce_gen_invalid_pubkey_cleanup(secp256k1_context *ctx, const secp256k1_pubkey *valid_pubkey, const unsigned char *valid_seckey, const unsigned char *msg32, const unsigned char *session_rand32) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_pubkey invalid_pubkey = *valid_pubkey;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    unsigned char session_rand[32];
    unsigned char zero132[132] = { 0 };

    /* This is a public output, so the severity is stale-state propagation,
     * not disclosure of the secret nonce. Still require failed calls not to
     * leave an apparently usable nonce object behind. */
    invalid_pubkey.data[0] ^= 1u;
    memcpy(session_rand, session_rand32, sizeof(session_rand));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_rand, valid_seckey, &invalid_pubkey, msg32, NULL, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&secnonce, zero132, sizeof(secnonce)) == 0);
    FUZZ_CHECK(memcmp(&pubnonce, zero132, sizeof(pubnonce)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_nonce_gen_counter_failure_cleanup(secp256k1_context *ctx, const secp256k1_keypair *keypair, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_keyagg_cache invalid_keyagg_cache = *keyagg_cache;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    unsigned char zero132[132] = { 0 };

    /* The public nonce is not secret, but leaving it live after a failed call
     * can still propagate an object whose corresponding secret nonce is dead. */
    invalid_keyagg_cache.data[0] ^= 1u;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, 0, keypair, msg32, &invalid_keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_keypair_consistency(secp256k1_context *ctx, const secp256k1_keypair *keypair, const secp256k1_pubkey *other_pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    static const unsigned char scalar_two[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_keypair mismatched_keypair;
    secp256k1_pubkey original_pubkey;
    secp256k1_pubkey alternate_pubkey;
    const secp256k1_pubkey *mismatched_pubkey = other_pubkey;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    unsigned char zero132[132] = { 0 };

    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &original_pubkey, keypair) == 1);
    if (secp256k1_ec_pubkey_cmp(ctx, &original_pubkey, mismatched_pubkey) == 0) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &alternate_pubkey, secp256k1_fuzz_scalar_one) == 1);
        mismatched_pubkey = &alternate_pubkey;
        if (secp256k1_ec_pubkey_cmp(ctx, &original_pubkey, mismatched_pubkey) == 0) {
            FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &alternate_pubkey, scalar_two) == 1);
        }
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &original_pubkey, mismatched_pubkey) != 0);
    }

    mismatched_keypair = *keypair;
    memcpy(mismatched_keypair.data + 32, mismatched_pubkey->data, sizeof(mismatched_keypair.data) - 32);
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, 0, &mismatched_keypair, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&secnonce, zero132, sizeof(secnonce)) == 0);
    FUZZ_CHECK(memcmp(&pubnonce, zero132, sizeof(pubnonce)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_noncanonical_nonce_storage(secp256k1_context *ctx, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache) {
    static const unsigned char field_p_plus_one[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
    };
    secp256k1_fuzz_musig_illegal_data illegal_data;
    unsigned char point_x_one[33] = { 0 };
    unsigned char nonce_input[66] = { 0 };
    unsigned char serialized66[66];
    unsigned char zero66[66] = { 0 };
    unsigned char zero132[132] = { 0 };
    unsigned char zero133[133] = { 0 };
    secp256k1_musig_pubnonce canonical_pubnonce;
    secp256k1_musig_pubnonce invalid_pubnonce;
    secp256k1_musig_aggnonce canonical_aggnonce;
    secp256k1_musig_aggnonce invalid_aggnonce;
    secp256k1_musig_aggnonce failed_aggnonce;
    secp256k1_musig_session failed_session;
    const secp256k1_musig_pubnonce *nonce_ptrs[1];
    secp256k1_fe noncanonical_x;
    secp256k1_fe_storage x_storage;
    unsigned int calls;

    STATIC_ASSERT(sizeof(secp256k1_fe_storage) == 32);
    point_x_one[0] = 0x02;
    point_x_one[32] = 0x01;
    memcpy(nonce_input, point_x_one, sizeof(point_x_one));
    memcpy(nonce_input + 33, point_x_one, sizeof(point_x_one));
    FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &canonical_pubnonce, nonce_input) == 1);
    secp256k1_fe_set_b32_mod(&noncanonical_x, field_p_plus_one);
    secp256k1_fe_impl_to_storage(&x_storage, &noncanonical_x);

    invalid_pubnonce = canonical_pubnonce;
    memcpy(invalid_pubnonce.data + 4, &x_storage, sizeof(x_storage));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(serialized66, 0xA5, sizeof(serialized66));
    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized66, &invalid_pubnonce) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(serialized66, zero66, sizeof(serialized66)) == 0);

    nonce_ptrs[0] = &invalid_pubnonce;
    memset(&failed_aggnonce, 0xA5, sizeof(failed_aggnonce));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &failed_aggnonce, nonce_ptrs, 1) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&failed_aggnonce, zero132, sizeof(failed_aggnonce)) == 0);

    nonce_ptrs[0] = &canonical_pubnonce;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &canonical_aggnonce, nonce_ptrs, 1) == 1);
    invalid_aggnonce = canonical_aggnonce;
    memcpy(invalid_aggnonce.data + 4, &x_storage, sizeof(x_storage));
    memset(serialized66, 0xA5, sizeof(serialized66));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized66, &invalid_aggnonce) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(serialized66, zero66, sizeof(serialized66)) == 0);

    memset(&failed_session, 0xA5, sizeof(failed_session));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &failed_session, &invalid_aggnonce, msg32, keyagg_cache) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&failed_session, zero133, sizeof(failed_session)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_opaque_nonce_barriers(secp256k1_context *ctx, const secp256k1_keypair *keypair, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_secnonce invalid_signer_secnonce;
    secp256k1_musig_secnonce invalid_scalar_secnonce;
    secp256k1_musig_pubnonce pubnonce;
    secp256k1_musig_pubnonce invalid_point_pubnonce;
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_aggnonce invalid_point_aggnonce;
    secp256k1_musig_session session;
    secp256k1_musig_partial_sig partial_sig;
    secp256k1_pubkey keypair_pubkey;
    secp256k1_pubkey alternate_pubkey;
    const secp256k1_musig_pubnonce *pubnonce_ptrs[1];
    unsigned char serialized66[66];
    unsigned char zero66[66] = { 0 };
    unsigned char zero132[132] = { 0 };
    unsigned char zero133[133] = { 0 };
    unsigned int calls;

    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &keypair_pubkey, keypair) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &alternate_pubkey, secp256k1_fuzz_scalar_one) == 1);
    if (secp256k1_ec_pubkey_cmp(ctx, &keypair_pubkey, &alternate_pubkey) == 0) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &alternate_pubkey, secp256k1_fuzz_scalar_order_minus_one) == 1);
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &keypair_pubkey, &alternate_pubkey) != 0);

    FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, 0, keypair, msg32, keyagg_cache, extra_input32) == 1);
    pubnonce_ptrs[0] = &pubnonce;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 1) == 1);
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, keyagg_cache) == 1);

    invalid_point_pubnonce = pubnonce;
    memset(invalid_point_pubnonce.data + 4, 0, 64);
    invalid_point_pubnonce.data[4] = 1;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    memset(serialized66, 0xA5, sizeof(serialized66));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized66, &invalid_point_pubnonce) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(serialized66, zero66, sizeof(serialized66)) == 0);

    pubnonce_ptrs[0] = &invalid_point_pubnonce;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 1) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(aggnonce.data, zero132, sizeof(aggnonce.data)) == 0);

    invalid_point_aggnonce = aggnonce;
    /* Recreate a valid aggregate after the failed call, then corrupt only its
     * first raw point while preserving the aggregate magic. */
    pubnonce_ptrs[0] = &pubnonce;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &invalid_point_aggnonce, pubnonce_ptrs, 1) == 1);
    memset(invalid_point_aggnonce.data + 4, 0, 64);
    invalid_point_aggnonce.data[4] = 1;

    memset(serialized66, 0xA5, sizeof(serialized66));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, serialized66, &invalid_point_aggnonce) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(serialized66, zero66, sizeof(serialized66)) == 0);

    memset(&session, 0xA5, sizeof(session));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &invalid_point_aggnonce, msg32, keyagg_cache) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&session, zero133, sizeof(session)) == 0);

    pubnonce_ptrs[0] = &pubnonce;
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 1) == 1);
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, keyagg_cache) == 1);

    /* Keep both secret scalars intact and replace only the embedded signer key.
     * A secnonce generated for one public key must not sign for another. */
    invalid_signer_secnonce = secnonce;
    memcpy(invalid_signer_secnonce.data + 4 + 2 * 32, alternate_pubkey.data, sizeof(alternate_pubkey.data));
    memset(&partial_sig, 0xA5, sizeof(partial_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &invalid_signer_secnonce, keypair, keyagg_cache, &session) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&partial_sig, zero132, sizeof(partial_sig)) == 0);
    FUZZ_CHECK(memcmp(&invalid_signer_secnonce, zero132, sizeof(invalid_signer_secnonce)) == 0);

    invalid_scalar_secnonce = secnonce;
    memcpy(invalid_scalar_secnonce.data + 4, secp256k1_fuzz_scalar_order, 32);
    memset(&partial_sig, 0xA5, sizeof(partial_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &invalid_scalar_secnonce, keypair, keyagg_cache, &session) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&partial_sig, zero132, sizeof(partial_sig)) == 0);
    FUZZ_CHECK(memcmp(&invalid_scalar_secnonce, zero132, sizeof(invalid_scalar_secnonce)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
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
    secp256k1_fuzz_check_musig_noncanonical_duplicate(ctx);
    secp256k1_fuzz_check_musig_keyagg_reference(ctx);
    secp256k1_fuzz_check_musig_three_keyagg_reference(ctx);
    secp256k1_fuzz_check_musig_tweaked_signing(ctx);
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
    secp256k1_fuzz_check_musig_pubkey_agg_success(ctx, &agg_xonly, &cache);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &single_agg_xonly, &single_cache, pubkey_ptrs, 1) == 1);
    secp256k1_fuzz_check_musig_pubkey_agg_success(ctx, &single_agg_xonly, &single_cache);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, NULL, &cache_no_output, pubkey_ptrs, n_pubkeys) == 1);
    secp256k1_fuzz_check_musig_pubkey_agg_success(ctx, NULL, &cache_no_output);
    secp256k1_fuzz_check_musig_pubkey_agg_failure_cleanup(ctx, &agg_xonly, &cache);
    secp256k1_fuzz_check_musig_keyagg_cache_curve_barrier(ctx, &pubkeys[0], &cache);
    secp256k1_fuzz_check_musig_keyagg_cache_semantic_barrier(ctx, &cache);
    secp256k1_fuzz_check_musig_keyagg_hash_routing(ctx, pubkey_ptrs, n_pubkeys, &agg_xonly, &cache);
    secp256k1_fuzz_check_musig_keypair_consistency(ctx, &keypairs[0], &pubkeys[1], tweak, &cache, session_rand);
    secp256k1_fuzz_check_musig_noncanonical_nonce_storage(ctx, tweak, &cache);
    secp256k1_fuzz_check_musig_opaque_nonce_barriers(ctx, &keypairs[0], tweak, &cache, session_rand);
    secp256k1_fuzz_check_musig_nonce_gen_counter(ctx, input, size, seckey[0], &keypairs[0], &pubkeys[0], tweak, &cache, session_rand);
    secp256k1_fuzz_check_musig_nonce_gen_counter_failure_cleanup(ctx, &keypairs[0], tweak, &cache, session_rand);
    secp256k1_fuzz_check_musig_zero_counter_sign(ctx, &keypairs[0], &pubkeys[0], tweak, &single_cache, &single_agg_xonly);
    secp256k1_fuzz_check_musig_nonce_scalar_barrier(ctx);
    secp256k1_fuzz_check_musig_partial_sign_nonce_parity(ctx);
    secp256k1_fuzz_check_musig_infinity_nonce_process(ctx, &keypairs[0], &pubkeys[0], tweak, &single_cache, &single_agg_xonly);
    secp256k1_fuzz_check_musig_sign_roundtrip(ctx, input, size, seckey, keypairs, pubkeys, n_pubkeys, tweak);
    secp256k1_fuzz_check_musig_nonce_gen_failure_cleanup(ctx, &pubkeys[0], seckey[0], tweak, &cache, tweak, session_rand);
    secp256k1_fuzz_check_musig_nonce_gen_invalid_pubkey_cleanup(ctx, &pubkeys[0], seckey[0], tweak, session_rand);
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
