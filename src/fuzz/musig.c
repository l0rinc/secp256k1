/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "sha256_reference.h"
#include "../hash_impl.h"
#include "../field_impl.h"
#include "../int128_impl.h"

#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_MUSIG)
#define SECP256K1_FUZZ_MUSIG_MAX_SIGNERS 16
typedef int (*secp256k1_fuzz_musig_partial_sig_agg_fn)(const secp256k1_context *, unsigned char *, const secp256k1_musig_session *, const secp256k1_musig_partial_sig * const*, size_t);
typedef int (*secp256k1_fuzz_musig_partial_sig_verify_fn)(const secp256k1_context *, const secp256k1_musig_partial_sig *, const secp256k1_musig_pubnonce *, const secp256k1_pubkey *, const secp256k1_musig_keyagg_cache *, const secp256k1_musig_session *);
typedef int (*secp256k1_fuzz_musig_nonce_process_fn)(const secp256k1_context *, secp256k1_musig_session *, const secp256k1_musig_aggnonce *, const unsigned char *, const secp256k1_musig_keyagg_cache *);
typedef int (*secp256k1_fuzz_musig_partial_sign_fn)(const secp256k1_context *, secp256k1_musig_partial_sig *, secp256k1_musig_secnonce *, const secp256k1_keypair *, const secp256k1_musig_keyagg_cache *, const secp256k1_musig_session *);
typedef int (*secp256k1_fuzz_musig_nonce_gen_fn)(const secp256k1_context *, secp256k1_musig_secnonce *, secp256k1_musig_pubnonce *, unsigned char *, const unsigned char *, const secp256k1_pubkey *, const unsigned char *, const secp256k1_musig_keyagg_cache *, const unsigned char *);

static size_t secp256k1_fuzz_musig_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_musig_keyagglist_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_musig_keyaggcoef_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_musig_noncecoef_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_musig_challenge_sha256_compression_calls = 0;
static int secp256k1_fuzz_musig_force_zero_sha256_state = 0;

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
    if (secp256k1_fuzz_musig_force_zero_sha256_state) {
        memset(state, 0, 8 * sizeof(*state));
    }
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

static void secp256k1_fuzz_musig_scalar_negate(unsigned char out32[32], const unsigned char in32[32]) {
    unsigned int borrow = 0;
    size_t i;

    if (memcmp(in32, secp256k1_fuzz_scalar_zero, sizeof(secp256k1_fuzz_scalar_zero)) == 0) {
        memset(out32, 0, 32);
        return;
    }
    for (i = 32; i-- > 0;) {
        unsigned int minuend = secp256k1_fuzz_scalar_order[i];
        unsigned int subtrahend = (unsigned int)in32[i] + borrow;
        out32[i] = (unsigned char)(minuend - subtrahend);
        borrow = minuend < subtrahend;
    }
    FUZZ_CHECK(borrow == 0);
}

static void secp256k1_fuzz_musig_scalar_add_mod_order(unsigned char out32[32], const unsigned char addend32[32]) {
    unsigned char sum33[33] = { 0 };
    unsigned char order33[33] = { 0 };
    unsigned int carry = 0;
    size_t i;

    memcpy(order33 + 1, secp256k1_fuzz_scalar_order, sizeof(secp256k1_fuzz_scalar_order));
    for (i = 32; i-- > 0;) {
        unsigned int sum = (unsigned int)out32[i] + addend32[i] + carry;
        sum33[i + 1] = (unsigned char)sum;
        carry = sum >> 8;
    }
    sum33[0] = (unsigned char)carry;
    if (memcmp(sum33, order33, sizeof(sum33)) >= 0) {
        unsigned int borrow = 0;
        for (i = sizeof(sum33); i-- > 0;) {
            unsigned int minuend = sum33[i];
            unsigned int subtrahend = (unsigned int)order33[i] + borrow;
            sum33[i] = (unsigned char)(minuend - subtrahend);
            borrow = minuend < subtrahend;
        }
        FUZZ_CHECK(borrow == 0);
    }
    memcpy(out32, sum33 + 1, 32);
}

static void secp256k1_fuzz_musig_tagged_hash_reference(unsigned char out32[32], const unsigned char *tag, size_t taglen, const unsigned char *msg, size_t msglen);

static void secp256k1_fuzz_musig_check_noncecoef_reference(const secp256k1_context *ctx, const secp256k1_musig_aggnonce *aggnonce, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *msg32, const secp256k1_musig_session *session) {
    static const unsigned char noncecoef_tag[] = "MuSig/noncecoef";
    secp256k1_pubkey aggregate_pubkey;
    unsigned char aggnonce66[66];
    unsigned char aggregate_serialized[65];
    unsigned char aggregate_x32[32];
    unsigned char noncecoef_input[66 + 32 + 32];
    unsigned char noncecoef_hash[32];
    unsigned char expected_noncecoef[32];
    size_t aggregate_serialized_len = sizeof(aggregate_serialized);

    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, aggnonce66, aggnonce) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &aggregate_pubkey, keyagg_cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, aggregate_serialized, &aggregate_serialized_len, &aggregate_pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(aggregate_serialized_len == sizeof(aggregate_serialized));
    memcpy(aggregate_x32, aggregate_serialized + 1, sizeof(aggregate_x32));

    memcpy(noncecoef_input, aggnonce66, sizeof(aggnonce66));
    memcpy(noncecoef_input + sizeof(aggnonce66), aggregate_x32, sizeof(aggregate_x32));
    memcpy(noncecoef_input + sizeof(aggnonce66) + sizeof(aggregate_x32), msg32, 32);
    secp256k1_fuzz_musig_tagged_hash_reference(noncecoef_hash, noncecoef_tag, sizeof(noncecoef_tag) - 1, noncecoef_input, sizeof(noncecoef_input));

    secp256k1_fuzz_musig_reduce_scalar(expected_noncecoef, noncecoef_hash);
    FUZZ_CHECK(memcmp(expected_noncecoef, session->data + 37, sizeof(expected_noncecoef)) == 0);
}

static void secp256k1_fuzz_musig_tagged_hash_reference(unsigned char out32[32], const unsigned char *tag, size_t taglen, const unsigned char *msg, size_t msglen) {
    unsigned char taghash[32];
    unsigned char *transcript;
    size_t transcript_len = 2 * sizeof(taghash);

    FUZZ_CHECK(msg != NULL || msglen == 0);
    FUZZ_CHECK(msglen <= SIZE_MAX - transcript_len);
    transcript_len += msglen;
    transcript = (unsigned char *)malloc(transcript_len);
    FUZZ_CHECK(transcript != NULL);

    secp256k1_fuzz_sha256_standalone(taghash, tag, taglen);
    memcpy(transcript, taghash, sizeof(taghash));
    memcpy(transcript + sizeof(taghash), taghash, sizeof(taghash));
    if (msglen != 0) {
        memcpy(transcript + 2 * sizeof(taghash), msg, msglen);
    }
    secp256k1_fuzz_sha256_standalone(out32, transcript, transcript_len);
    memset(taghash, 0, sizeof(taghash));
    memset(transcript, 0, transcript_len);
    free(transcript);
}

/* Recompute one KeyAgg coefficient from the public key list instead of reading
 * the coefficient-producing cache or calling the MuSig implementation. */
static void secp256k1_fuzz_musig_keyagg_coefficient_reference(const secp256k1_context *ctx, unsigned char coefficient32[32], const secp256k1_pubkey * const* pubkeys, size_t n_pubkeys, size_t target_index) {
    static const unsigned char keyagg_list_tag[] = "KeyAgg list";
    static const unsigned char keyagg_coef_tag[] = "KeyAgg coefficient";
    unsigned char serialized[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS * 33];
    unsigned char pks_hash[32];
    unsigned char coefficient_input[65];
    unsigned char coefficient_hash[32];
    size_t serialized_len;
    size_t second_index = n_pubkeys;
    size_t i;

    FUZZ_CHECK(n_pubkeys > 0);
    FUZZ_CHECK(n_pubkeys <= SECP256K1_FUZZ_MUSIG_MAX_SIGNERS);
    FUZZ_CHECK(target_index < n_pubkeys);
    for (i = 0; i < n_pubkeys; i++) {
        serialized_len = 33;
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized + 33 * i, &serialized_len, pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == 33);
    }
    secp256k1_fuzz_musig_tagged_hash_reference(pks_hash, keyagg_list_tag, sizeof(keyagg_list_tag) - 1, serialized, 33 * n_pubkeys);

    /* Match the production cache's first distinct key selection. */
    for (i = 1; i < n_pubkeys; i++) {
        if (memcmp(pubkeys[0], pubkeys[i], sizeof(*pubkeys[0])) != 0) {
            second_index = i;
            break;
        }
    }
    if (second_index < n_pubkeys && memcmp(serialized + 33 * target_index, serialized + 33 * second_index, 33) == 0) {
        memcpy(coefficient32, secp256k1_fuzz_scalar_one, 32);
    } else {
        memcpy(coefficient_input, pks_hash, 32);
        memcpy(coefficient_input + 32, serialized + 33 * target_index, 33);
        secp256k1_fuzz_musig_tagged_hash_reference(coefficient_hash, keyagg_coef_tag, sizeof(keyagg_coef_tag) - 1, coefficient_input, sizeof(coefficient_input));
        secp256k1_fuzz_musig_reduce_scalar(coefficient32, coefficient_hash);
    }
}

/* Check a partial signature against the public MuSig equation. This deliberately
 * avoids secp256k1_musig_partial_sig_verify: using the same verifier as the
 * signer would let a shared equation change pass unnoticed. */
static int secp256k1_fuzz_musig_partial_sig_equation(const secp256k1_context *ctx, const secp256k1_musig_partial_sig *partial_sig, const secp256k1_musig_pubnonce *pubnonce, const secp256k1_pubkey *pubkey, const secp256k1_pubkey * const* pubkeys, size_t n_pubkeys, size_t signer_index, const secp256k1_musig_keyagg_cache *keyagg_cache, const secp256k1_musig_session *session, const unsigned char *msg32) {
    static const unsigned char challenge_tag[] = "BIP0340/challenge";
    unsigned char partial_sig32[32];
    unsigned char pubnonce66[66];
    unsigned char b32[32];
    unsigned char coefficient32[32];
    unsigned char challenge_input[96];
    unsigned char challenge_hash[32];
    unsigned char challenge32[32];
    unsigned char aggregate33[33];
    unsigned char final_nonce_x32[32];
    unsigned char aggregate_x32[32];
    unsigned char zero32[32] = { 0 };
    secp256k1_pubkey nonce0;
    secp256k1_pubkey nonce1;
    secp256k1_pubkey scaled_nonce1;
    secp256k1_pubkey signer_point;
    secp256k1_pubkey response_point;
    secp256k1_pubkey aggregate;
    secp256k1_pubkey expected_point;
    const secp256k1_pubkey *terms[3];
    size_t aggregate_len = sizeof(aggregate33);
    size_t term_count = 0;
    int rhs_infinity = 0;

    FUZZ_CHECK(pubkey != NULL);
    FUZZ_CHECK(pubkeys != NULL);
    FUZZ_CHECK(keyagg_cache != NULL);
    FUZZ_CHECK(session != NULL);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(session->data[4] <= 1);
    FUZZ_CHECK(keyagg_cache->data[164] <= 1);
    FUZZ_CHECK(secp256k1_musig_partial_sig_serialize(ctx, partial_sig32, partial_sig) == 1);
    FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, pubnonce66, pubnonce) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &nonce0, pubnonce66, 33) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &nonce1, pubnonce66 + 33, 33) == 1);

    /* Recompute e from the serialized final nonce, aggregate X coordinate, and
     * message. The stored challenge is checked rather than trusted. */
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &aggregate, keyagg_cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, aggregate33, &aggregate_len, &aggregate, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(aggregate_len == sizeof(aggregate33));
    memcpy(final_nonce_x32, session->data + 5, sizeof(final_nonce_x32));
    memcpy(aggregate_x32, aggregate33 + 1, sizeof(aggregate_x32));
    memcpy(challenge_input, final_nonce_x32, sizeof(final_nonce_x32));
    memcpy(challenge_input + 32, aggregate_x32, sizeof(aggregate_x32));
    memcpy(challenge_input + 64, msg32, 32);
    secp256k1_fuzz_musig_tagged_hash_reference(challenge_hash, challenge_tag, sizeof(challenge_tag) - 1, challenge_input, sizeof(challenge_input));
    secp256k1_fuzz_musig_reduce_scalar(challenge32, challenge_hash);
    FUZZ_CHECK(memcmp(challenge32, session->data + 69, sizeof(challenge32)) == 0);
    memcpy(b32, session->data + 37, sizeof(b32));
    secp256k1_fuzz_musig_keyagg_coefficient_reference(ctx, coefficient32, pubkeys, n_pubkeys, signer_index);

    /* The final nonce parity negates both public nonce components. */
    if (session->data[4]) {
        FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &nonce0) == 1);
    }
    terms[term_count++] = &nonce0;
    if (memcmp(b32, zero32, sizeof(b32)) != 0) {
        scaled_nonce1 = nonce1;
        FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &scaled_nonce1, b32) == 1);
        if (session->data[4]) {
            FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &scaled_nonce1) == 1);
        }
        terms[term_count++] = &scaled_nonce1;
    }

    /* The aggregate-cache parity adjusts the signer's public key before the
     * challenge and KeyAgg coefficient are applied. */
    if (memcmp(coefficient32, zero32, sizeof(coefficient32)) != 0
            && memcmp(challenge32, zero32, sizeof(challenge32)) != 0) {
        signer_point = *pubkey;
        if ((aggregate33[0] == 0x03) != (keyagg_cache->data[164] != 0)) {
            FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &signer_point) == 1);
        }
        FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &signer_point, coefficient32) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &signer_point, challenge32) == 1);
        terms[term_count++] = &signer_point;
    }

    FUZZ_CHECK(term_count <= sizeof(terms) / sizeof(terms[0]));
    if (term_count == 1) {
        expected_point = *terms[0];
    } else {
        rhs_infinity = !secp256k1_ec_pubkey_combine(ctx, &expected_point, terms, term_count);
    }

    if (memcmp(partial_sig32, zero32, sizeof(partial_sig32)) == 0) {
        return rhs_infinity;
    }
    if (rhs_infinity) {
        return 0;
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &response_point, partial_sig32) == 1);
    return secp256k1_ec_pubkey_cmp(ctx, &response_point, &expected_point) == 0;
}

/* Verify parseable but externally supplied partial signatures without
 * delegating the expected result to the production verifier. */
static void secp256k1_fuzz_check_musig_arbitrary_partial_sig(const secp256k1_context *ctx, const unsigned char *input, size_t size, const secp256k1_musig_pubnonce *pubnonce, const secp256k1_pubkey *pubkey, const secp256k1_pubkey * const* pubkeys, size_t n_pubkeys, size_t signer_index, const secp256k1_musig_keyagg_cache *keyagg_cache, const secp256k1_musig_session *session, const unsigned char *msg32) {
    unsigned char arbitrary32[32];
    unsigned char samples[4][32];
    const unsigned char *sample_ptrs[4];
    secp256k1_musig_partial_sig partial_sig;
    size_t i;

    memcpy(samples[0], secp256k1_fuzz_scalar_zero, sizeof(samples[0]));
    memcpy(samples[1], secp256k1_fuzz_scalar_one, sizeof(samples[1]));
    memcpy(samples[2], secp256k1_fuzz_scalar_order_minus_one, sizeof(samples[2]));
    secp256k1_fuzz_derive(arbitrary32, sizeof(arbitrary32), input, size, 617);
    secp256k1_fuzz_musig_reduce_scalar(samples[3], arbitrary32);
    for (i = 0; i < sizeof(sample_ptrs) / sizeof(sample_ptrs[0]); i++) {
        sample_ptrs[i] = samples[i];
    }

    for (i = 0; i < sizeof(sample_ptrs) / sizeof(sample_ptrs[0]); i++) {
        int expected;
        int actual;

        FUZZ_CHECK(secp256k1_musig_partial_sig_parse(ctx, &partial_sig, sample_ptrs[i]) == 1);
        expected = secp256k1_fuzz_musig_partial_sig_equation(ctx, &partial_sig, pubnonce, pubkey, pubkeys, n_pubkeys, signer_index, keyagg_cache, session, msg32);
        actual = secp256k1_musig_partial_sig_verify(ctx, &partial_sig, pubnonce, pubkey, keyagg_cache, session);
        FUZZ_CHECK(actual == expected);
    }
}

/* Check the final MuSig signature with the BIP340 equation instead of calling
 * the Schnorr verifier. This catches an aggregation error even when the
 * signing and verification paths share an implementation detail. */
static void secp256k1_fuzz_check_musig_final_sig_equation(const secp256k1_context *ctx, const unsigned char *sig64, const secp256k1_musig_session *session, const secp256k1_xonly_pubkey *agg_pk, const unsigned char *msg32) {
    static const unsigned char challenge_tag[] = "BIP0340/challenge";
    unsigned char aggregate_x32[32];
    unsigned char final_nonce_x32[32];
    unsigned char challenge_input[96];
    unsigned char challenge_hash[32];
    unsigned char challenge32[32];
    unsigned char nonce33[33];
    unsigned char aggregate33[33];
    unsigned char zero32[32] = { 0 };
    secp256k1_pubkey nonce_point;
    secp256k1_pubkey aggregate_point;
    secp256k1_pubkey challenge_point;
    secp256k1_pubkey response_point;
    secp256k1_pubkey expected_point;
    const secp256k1_pubkey *terms[2];
    int challenge_nonzero;
    int response_nonzero;
    size_t term_count = 0;

    FUZZ_CHECK(sig64 != NULL);
    FUZZ_CHECK(session != NULL);
    FUZZ_CHECK(agg_pk != NULL);
    FUZZ_CHECK(msg32 != NULL);
    FUZZ_CHECK(session->data[4] <= 1);
    FUZZ_CHECK(memcmp(sig64, session->data + 5, sizeof(final_nonce_x32)) == 0);

    /* nonce_process records the parity of the pre-adjustment effective nonce;
     * partial_sign negates it when needed, so the final BIP340 R is even. */
    nonce33[0] = 0x02;
    memcpy(nonce33 + 1, sig64, sizeof(final_nonce_x32));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &nonce_point, nonce33, sizeof(nonce33)) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, aggregate_x32, agg_pk) == 1);
    aggregate33[0] = 0x02;
    memcpy(aggregate33 + 1, aggregate_x32, sizeof(aggregate_x32));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &aggregate_point, aggregate33, sizeof(aggregate33)) == 1);

    memcpy(final_nonce_x32, sig64, sizeof(final_nonce_x32));
    memcpy(challenge_input, final_nonce_x32, sizeof(final_nonce_x32));
    memcpy(challenge_input + 32, aggregate_x32, sizeof(aggregate_x32));
    memcpy(challenge_input + 64, msg32, 32);
    secp256k1_fuzz_musig_tagged_hash_reference(challenge_hash, challenge_tag, sizeof(challenge_tag) - 1, challenge_input, sizeof(challenge_input));
    secp256k1_fuzz_musig_reduce_scalar(challenge32, challenge_hash);
    FUZZ_CHECK(memcmp(challenge32, session->data + 69, sizeof(challenge32)) == 0);

    challenge_nonzero = memcmp(challenge32, zero32, sizeof(challenge32)) != 0;
    response_nonzero = memcmp(sig64 + 32, zero32, sizeof(zero32)) != 0;
    if (challenge_nonzero) {
        challenge_point = aggregate_point;
        FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &challenge_point, challenge32) == 1);
        terms[term_count++] = &challenge_point;
    }
    terms[term_count++] = &nonce_point;

    if (response_nonzero) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &response_point, sig64 + 32) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected_point, terms, term_count) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &response_point, &expected_point) == 0);
    } else {
        /* A zero response represents infinity; the right-hand side must also
         * be infinity. The public combine API reports that case as failure. */
        FUZZ_CHECK(challenge_nonzero);
        FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected_point, terms, term_count) == 0);
    }
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

static void secp256k1_fuzz_check_musig_single_keyagg_reference(const secp256k1_context *ctx) {
    static const unsigned char keyagg_list_tag[] = "KeyAgg list";
    static const unsigned char keyagg_coef_tag[] = "KeyAgg coefficient";
    unsigned char serialized[33];
    unsigned char pks_hash[32];
    unsigned char coefficient_input[65];
    unsigned char coefficient_hash[32];
    unsigned char coefficient[32];
    size_t serialized_len = sizeof(serialized);
    secp256k1_pubkey pubkey;
    secp256k1_pubkey expected_full;
    secp256k1_pubkey actual_full;
    secp256k1_xonly_pubkey expected_xonly;
    secp256k1_xonly_pubkey actual_xonly;
    secp256k1_musig_keyagg_cache cache;
    const secp256k1_pubkey *pubkey_ptrs[1];

    /* With one key there is no second distinct key, so the coefficient is still
     * KeyAggCoeff(pk_hash, pk), rather than the identity scalar. */
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized, &serialized_len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(serialized_len == sizeof(serialized));
    secp256k1_fuzz_musig_tagged_hash_reference(pks_hash, keyagg_list_tag, sizeof(keyagg_list_tag) - 1, serialized, sizeof(serialized));
    memcpy(coefficient_input, pks_hash, sizeof(pks_hash));
    memcpy(coefficient_input + sizeof(pks_hash), serialized, sizeof(serialized));
    secp256k1_fuzz_musig_tagged_hash_reference(coefficient_hash, keyagg_coef_tag, sizeof(keyagg_coef_tag) - 1, coefficient_input, sizeof(coefficient_input));
    secp256k1_fuzz_musig_reduce_scalar(coefficient, coefficient_hash);
    FUZZ_CHECK(memcmp(coefficient, secp256k1_fuzz_scalar_zero, sizeof(coefficient)) != 0);
    FUZZ_CHECK(memcmp(coefficient, secp256k1_fuzz_scalar_one, sizeof(coefficient)) != 0);

    expected_full = pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &expected_full, coefficient) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &expected_xonly, NULL, &expected_full) == 1);
    pubkey_ptrs[0] = &pubkey;
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly, &cache, pubkey_ptrs, 1) == 1);
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

static void secp256k1_fuzz_check_musig_four_keyagg_reference(const secp256k1_context *ctx) {
    static const unsigned char keyagg_list_tag[] = "KeyAgg list";
    static const unsigned char keyagg_coef_tag[] = "KeyAgg coefficient";
    unsigned char scalar_two[32] = { 0 };
    unsigned char scalar_three[32] = { 0 };
    unsigned char scalar_four[32] = { 0 };
    unsigned char serialized[4][33];
    unsigned char keyagg_hash[32];
    unsigned char coefficient_input[65];
    unsigned char coefficient_hash[32];
    unsigned char coefficient[32];
    const unsigned char *seckeys[4];
    secp256k1_pubkey pubkeys[4];
    secp256k1_pubkey scaled[4];
    secp256k1_pubkey expected_full;
    secp256k1_pubkey actual_full;
    secp256k1_xonly_pubkey expected_xonly;
    secp256k1_xonly_pubkey actual_xonly;
    secp256k1_musig_keyagg_cache cache;
    const secp256k1_pubkey *pubkey_ptrs[4];
    const secp256k1_pubkey *terms[4];
    size_t serialized_len;
    size_t second_index = 4;
    size_t i;

    scalar_two[31] = 2;
    scalar_three[31] = 3;
    scalar_four[31] = 4;
    seckeys[0] = secp256k1_fuzz_scalar_one;
    seckeys[1] = scalar_two;
    seckeys[2] = scalar_three;
    seckeys[3] = scalar_four;

    for (i = 0; i < 4; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckeys[i]) == 1);
        pubkey_ptrs[i] = &pubkeys[i];
        serialized_len = sizeof(serialized[i]);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized[i], &serialized_len, &pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == sizeof(serialized[i]));
    }

    /* Exercise the arbitrary-list callback at index three while keeping the
     * KeyAgg transcript and weighted point equation independent of MuSig. */
    secp256k1_fuzz_musig_tagged_hash_reference(keyagg_hash, keyagg_list_tag, sizeof(keyagg_list_tag) - 1, &serialized[0][0], sizeof(serialized));
    for (i = 1; i < 4; i++) {
        if (memcmp(serialized[0], serialized[i], sizeof(serialized[0])) != 0) {
            second_index = i;
            break;
        }
    }
    FUZZ_CHECK(second_index != 4);
    for (i = 0; i < 4; i++) {
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
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected_full, terms, 4) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &expected_xonly, NULL, &expected_full) == 1);

    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly, &cache, pubkey_ptrs, 4) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &actual_full, &cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &actual_full, &expected_full) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &actual_xonly, &expected_xonly) == 0);
    FUZZ_CHECK(memcmp(cache.data + 132, keyagg_hash, sizeof(keyagg_hash)) == 0);
}

/* Exercise the first key-count boundary beyond the stateful seven-signer
 * fixtures. Keep the transcript and weighted point sum independent of the
 * MuSig aggregation callback so a skipped eighth term cannot agree with a
 * shorter production-derived check. */
static void secp256k1_fuzz_check_musig_eight_keyagg_reference(const secp256k1_context *ctx) {
    static const unsigned char keyagg_list_tag[] = "KeyAgg list";
    static const size_t n_pubkeys = 8;
    unsigned char seckeys[8][32] = { 0 };
    secp256k1_pubkey pubkeys[8];
    secp256k1_pubkey scaled[8];
    secp256k1_pubkey expected_full;
    secp256k1_pubkey actual_full;
    secp256k1_xonly_pubkey expected_xonly;
    secp256k1_xonly_pubkey actual_xonly;
    secp256k1_xonly_pubkey actual_xonly_no_cache;
    secp256k1_musig_keyagg_cache cache;
    const secp256k1_pubkey *pubkey_ptrs[8];
    const secp256k1_pubkey *terms[8];
    unsigned char serialized[8 * 33];
    unsigned char keyagg_hash[32];
    unsigned char coefficient[32];
    size_t serialized_len;
    size_t i;

    for (i = 0; i < n_pubkeys; i++) {
        seckeys[i][31] = (unsigned char)(i + 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckeys[i]) == 1);
        pubkey_ptrs[i] = &pubkeys[i];
        serialized_len = 33;
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized + 33 * i, &serialized_len, &pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == 33);
    }

    secp256k1_fuzz_musig_tagged_hash_reference(keyagg_hash, keyagg_list_tag, sizeof(keyagg_list_tag) - 1, serialized, sizeof(serialized));
    for (i = 0; i < n_pubkeys; i++) {
        secp256k1_fuzz_musig_keyagg_coefficient_reference(ctx, coefficient, pubkey_ptrs, n_pubkeys, i);
        FUZZ_CHECK(memcmp(coefficient, secp256k1_fuzz_scalar_zero, sizeof(coefficient)) != 0);
        scaled[i] = pubkeys[i];
        if (memcmp(coefficient, secp256k1_fuzz_scalar_one, sizeof(coefficient)) != 0) {
            FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &scaled[i], coefficient) == 1);
        }
        terms[i] = &scaled[i];
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected_full, terms, n_pubkeys) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &expected_xonly, NULL, &expected_full) == 1);

    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly, &cache, pubkey_ptrs, n_pubkeys) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly_no_cache, NULL, pubkey_ptrs, n_pubkeys) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &actual_full, &cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &actual_full, &expected_full) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &actual_xonly, &expected_xonly) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &actual_xonly_no_cache, &expected_xonly) == 0);
    FUZZ_CHECK(memcmp(cache.data + 132, keyagg_hash, sizeof(keyagg_hash)) == 0);
}

/* Exercise an arbitrary list well beyond the stateful eight-signer fixtures.
 * Keep the transcript, coefficient hashes, and weighted point sum independent
 * of MuSig's ecmult_multi callback so a truncated 16-key list cannot agree
 * with a production-derived check. */
static void secp256k1_fuzz_check_musig_sixteen_keyagg_reference(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "long MuSig key aggregation\n";
    static const unsigned char keyagg_list_tag[] = "KeyAgg list";
    static const unsigned char keyagg_coef_tag[] = "KeyAgg coefficient";
    enum { N_PUBKEYS = 16 };
    unsigned char seckeys[N_PUBKEYS][32] = { { 0 } };
    unsigned char serialized[N_PUBKEYS * 33];
    unsigned char keyagg_hash[32];
    unsigned char coefficient_input[65];
    unsigned char coefficient_hash[32];
    unsigned char coefficient[32];
    secp256k1_pubkey pubkeys[N_PUBKEYS];
    secp256k1_pubkey scaled[N_PUBKEYS];
    secp256k1_pubkey expected_full;
    secp256k1_pubkey actual_full;
    secp256k1_xonly_pubkey expected_xonly;
    secp256k1_xonly_pubkey actual_xonly;
    secp256k1_xonly_pubkey actual_xonly_no_cache;
    secp256k1_musig_keyagg_cache cache;
    const secp256k1_pubkey *pubkey_ptrs[N_PUBKEYS];
    const secp256k1_pubkey *terms[N_PUBKEYS];
    size_t second_index = N_PUBKEYS;
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    for (i = 0; i < N_PUBKEYS; i++) {
        size_t serialized_len = 33;
        seckeys[i][31] = (unsigned char)(i + 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckeys[i]) == 1);
        pubkey_ptrs[i] = &pubkeys[i];
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized + 33 * i, &serialized_len, &pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == 33);
    }

    secp256k1_fuzz_musig_tagged_hash_reference(keyagg_hash, keyagg_list_tag, sizeof(keyagg_list_tag) - 1, serialized, sizeof(serialized));
    for (i = 1; i < N_PUBKEYS; i++) {
        if (memcmp(serialized, serialized + 33 * i, 33) != 0) {
            second_index = i;
            break;
        }
    }
    FUZZ_CHECK(second_index < N_PUBKEYS);

    for (i = 0; i < N_PUBKEYS; i++) {
        if (i == second_index) {
            memcpy(coefficient, secp256k1_fuzz_scalar_one, sizeof(coefficient));
        } else {
            memcpy(coefficient_input, keyagg_hash, sizeof(keyagg_hash));
            memcpy(coefficient_input + sizeof(keyagg_hash), serialized + 33 * i, 33);
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

    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected_full, terms, N_PUBKEYS) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &expected_xonly, NULL, &expected_full) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly, &cache, pubkey_ptrs, N_PUBKEYS) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly_no_cache, NULL, pubkey_ptrs, N_PUBKEYS) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &actual_full, &cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &actual_full, &expected_full) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &actual_xonly, &expected_xonly) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &actual_xonly_no_cache, &expected_xonly) == 0);
    FUZZ_CHECK(memcmp(cache.data + 132, keyagg_hash, sizeof(keyagg_hash)) == 0);
}

/* Exercise the first-distinct-key rule with valid duplicate public keys. The
 * expected coefficients are derived from the canonical serialized list so
 * this check does not inherit MuSig's opaque-structure comparison. */
static void secp256k1_fuzz_check_musig_duplicate_keyagg_reference(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "duplicate MuSig key aggregation\n";
    static const unsigned char keyagg_list_tag[] = "KeyAgg list";
    static const unsigned char keyagg_coef_tag[] = "KeyAgg coefficient";
    static const unsigned char key_indices[] = {
        1, 1, 2, 3, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
    };
    enum { N_PUBKEYS = 16 };
    unsigned char seckeys[N_PUBKEYS][32] = { { 0 } };
    unsigned char serialized[N_PUBKEYS * 33];
    unsigned char keyagg_hash[32];
    unsigned char coefficient_input[65];
    unsigned char coefficient_hash[32];
    unsigned char coefficient[32];
    secp256k1_pubkey pubkeys[N_PUBKEYS];
    secp256k1_pubkey scaled[N_PUBKEYS];
    secp256k1_pubkey expected_full;
    secp256k1_pubkey actual_full;
    secp256k1_xonly_pubkey expected_xonly;
    secp256k1_xonly_pubkey actual_xonly;
    secp256k1_xonly_pubkey actual_xonly_no_cache;
    secp256k1_musig_keyagg_cache cache;
    const secp256k1_pubkey *pubkey_ptrs[N_PUBKEYS];
    const secp256k1_pubkey *terms[N_PUBKEYS];
    size_t second_index = N_PUBKEYS;
    size_t i;

    STATIC_ASSERT(sizeof(key_indices) == N_PUBKEYS);
    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    for (i = 0; i < N_PUBKEYS; i++) {
        size_t serialized_len = 33;
        seckeys[i][31] = key_indices[i];
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckeys[i]) == 1);
        pubkey_ptrs[i] = &pubkeys[i];
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized + 33 * i, &serialized_len, &pubkeys[i], SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == 33);
    }

    secp256k1_fuzz_musig_tagged_hash_reference(keyagg_hash, keyagg_list_tag, sizeof(keyagg_list_tag) - 1, serialized, sizeof(serialized));
    for (i = 1; i < N_PUBKEYS; i++) {
        if (memcmp(serialized, serialized + 33 * i, 33) != 0) {
            second_index = i;
            break;
        }
    }
    FUZZ_CHECK(second_index == 2);

    for (i = 0; i < N_PUBKEYS; i++) {
        if (memcmp(serialized + 33 * i, serialized + 33 * second_index, 33) == 0) {
            memcpy(coefficient, secp256k1_fuzz_scalar_one, sizeof(coefficient));
        } else {
            memcpy(coefficient_input, keyagg_hash, sizeof(keyagg_hash));
            memcpy(coefficient_input + sizeof(keyagg_hash), serialized + 33 * i, 33);
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

    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected_full, terms, N_PUBKEYS) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &expected_xonly, NULL, &expected_full) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly, &cache, pubkey_ptrs, N_PUBKEYS) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &actual_xonly_no_cache, NULL, pubkey_ptrs, N_PUBKEYS) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &actual_full, &cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &actual_full, &expected_full) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &actual_xonly, &expected_xonly) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &actual_xonly_no_cache, &expected_xonly) == 0);
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
    const secp256k1_pubkey *pubkey_ptrs[2];
    size_t i;

    for (i = 0; i < 2; i++) {
        FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce[i], &pubnonce[i], (uint64_t)(i + 1), &keypairs[i], msg32, cache, NULL) == 1);
        pubnonce_ptrs[i] = &pubnonce[i];
        partial_sig_ptrs[i] = &partial_sig[i];
        pubkey_ptrs[i] = &pubkeys[i];
    }
    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 2) == 1);
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, cache) == 1);
    secp256k1_fuzz_musig_check_noncecoef_reference(ctx, &aggnonce, cache, msg32, &session);
    for (i = 0; i < 2; i++) {
        FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig[i], &secnonce[i], &keypairs[i], cache, &session) == 1);
        FUZZ_CHECK(secp256k1_fuzz_musig_partial_sig_equation(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], pubkey_ptrs, 2, i, cache, &session, msg32) == 1);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], cache, &session) == 1);
    }
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sig_ptrs, 2) == 1);
    secp256k1_fuzz_check_musig_final_sig_equation(ctx, sig64, &session, agg_xonly, msg32);
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

static void secp256k1_fuzz_check_musig_xonly_tweaked_signing(const secp256k1_context *ctx) {
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

    /* X-only tweaks normalize the input aggregate before adding the tweak.
     * Complete a signing round for the zero tweak and both output parities so
     * parity_acc is checked by the signing equations, not just serialization. */
    for (counter = 0; counter <= 32; counter++) {
        int parity;
        memset(tweak, 0, sizeof(tweak));
        tweak[31] = (unsigned char)counter;
        tweaked_cache = base_cache;
        FUZZ_CHECK(secp256k1_musig_pubkey_xonly_tweak_add(ctx, &tweaked_full, &tweaked_cache, tweak) == 1);
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

static void secp256k1_fuzz_check_musig_partial_sig_verify_invalid_state(secp256k1_context *ctx, const secp256k1_musig_partial_sig *valid_partial_sig, const secp256k1_musig_pubnonce *valid_pubnonce, const secp256k1_pubkey *valid_pubkey, const secp256k1_musig_keyagg_cache *valid_cache, const secp256k1_musig_session *valid_session) {
    secp256k1_fuzz_musig_partial_sig_verify_fn partial_sig_verify = secp256k1_musig_partial_sig_verify;
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_partial_sig invalid_partial_sig;
    secp256k1_musig_pubnonce invalid_pubnonce;
    secp256k1_pubkey invalid_pubkey;
    secp256k1_musig_keyagg_cache invalid_cache;
    secp256k1_musig_session invalid_session;
    secp256k1_musig_partial_sig partial_sig_before = *valid_partial_sig;
    secp256k1_musig_pubnonce pubnonce_before = *valid_pubnonce;
    secp256k1_pubkey pubkey_before = *valid_pubkey;
    secp256k1_musig_keyagg_cache cache_before = *valid_cache;
    secp256k1_musig_session session_before = *valid_session;
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    /* Preserve the magic while overflowing the scalar so this reaches the
     * verifier's partial-signature load barrier rather than its magic check. */
    invalid_partial_sig = *valid_partial_sig;
    memset(invalid_partial_sig.data + 4, 0xFF, sizeof(invalid_partial_sig.data) - 4);
    calls = illegal_data.calls;
    FUZZ_CHECK(partial_sig_verify(ctx, &invalid_partial_sig, valid_pubnonce, valid_pubkey, valid_cache, valid_session) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    invalid_pubnonce = *valid_pubnonce;
    invalid_pubnonce.data[0] ^= 1u;
    calls = illegal_data.calls;
    FUZZ_CHECK(partial_sig_verify(ctx, valid_partial_sig, &invalid_pubnonce, valid_pubkey, valid_cache, valid_session) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    invalid_pubkey = *valid_pubkey;
    memset(invalid_pubkey.data, 0, sizeof(invalid_pubkey.data));
    calls = illegal_data.calls;
    FUZZ_CHECK(partial_sig_verify(ctx, valid_partial_sig, valid_pubnonce, &invalid_pubkey, valid_cache, valid_session) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    invalid_cache = *valid_cache;
    invalid_cache.data[0] ^= 1u;
    calls = illegal_data.calls;
    FUZZ_CHECK(partial_sig_verify(ctx, valid_partial_sig, valid_pubnonce, valid_pubkey, &invalid_cache, valid_session) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    invalid_session = *valid_session;
    invalid_session.data[4] = 2;
    calls = illegal_data.calls;
    FUZZ_CHECK(partial_sig_verify(ctx, valid_partial_sig, valid_pubnonce, valid_pubkey, valid_cache, &invalid_session) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    FUZZ_CHECK(memcmp(valid_partial_sig, &partial_sig_before, sizeof(partial_sig_before)) == 0);
    FUZZ_CHECK(memcmp(valid_pubnonce, &pubnonce_before, sizeof(pubnonce_before)) == 0);
    FUZZ_CHECK(memcmp(valid_pubkey, &pubkey_before, sizeof(pubkey_before)) == 0);
    FUZZ_CHECK(memcmp(valid_cache, &cache_before, sizeof(cache_before)) == 0);
    FUZZ_CHECK(memcmp(valid_session, &session_before, sizeof(session_before)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
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

static void secp256k1_fuzz_check_musig_effective_nonce(const secp256k1_context *ctx, const unsigned char *aggnonce66, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache) {
    unsigned char expected33[33];
    unsigned char b32[32];
    unsigned char zero33[33] = { 0 };
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_session session;
    secp256k1_pubkey point[2];
    secp256k1_pubkey scaled;
    secp256k1_pubkey expected;
    secp256k1_pubkey generator;
    const secp256k1_pubkey *points[2];
    int point_infinity[2];
    int effective_infinity;
    size_t expected_len = sizeof(expected33);
    size_t i;

    FUZZ_CHECK(secp256k1_musig_aggnonce_parse(ctx, &aggnonce, aggnonce66) == 1);
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, &aggnonce, msg32, keyagg_cache) == 1);
    secp256k1_fuzz_musig_check_noncecoef_reference(ctx, &aggnonce, keyagg_cache, msg32, &session);
    memcpy(b32, session.data + 37, sizeof(b32));

    for (i = 0; i < 2; i++) {
        point_infinity[i] = memcmp(aggnonce66 + 33 * i, zero33, sizeof(zero33)) == 0;
        if (!point_infinity[i]) {
            FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &point[i], aggnonce66 + 33 * i, 33) == 1);
        }
    }

    /* Independently model nonce_pts[0] + b*nonce_pts[1] through the public
     * point operations. The aggregate nonce format permits either component
     * to be infinity, so exercise each branch instead of only the all-infinity
     * fallback. */
    effective_infinity = 0;
    if (point_infinity[1] || memcmp(b32, secp256k1_fuzz_scalar_zero, sizeof(b32)) == 0) {
        if (point_infinity[0]) {
            effective_infinity = 1;
        } else {
            expected = point[0];
        }
    } else {
        scaled = point[1];
        FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &scaled, b32) == 1);
        if (point_infinity[0]) {
            expected = scaled;
        } else {
            points[0] = &point[0];
            points[1] = &scaled;
            effective_infinity = !secp256k1_ec_pubkey_combine(ctx, &expected, points, 2);
        }
    }

    if (effective_infinity) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &generator, secp256k1_fuzz_scalar_one) == 1);
        expected = generator;
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, expected33, &expected_len, &expected, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(expected_len == sizeof(expected33));
    FUZZ_CHECK(session.data[4] == (unsigned char)(expected33[0] == 0x03));
    FUZZ_CHECK(memcmp(session.data + 5, expected33 + 1, 32) == 0);
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

static void secp256k1_fuzz_check_musig_nonce_agg_long(const secp256k1_context *ctx, const unsigned char *input, size_t size, const unsigned char *nonce66) {
    static const unsigned char trigger[] = "long MuSig nonce aggregation\n";
    static const unsigned char sixteen32[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
    };
    enum { N_PUBNONCES = 16 };
    unsigned char expected66[66];
    unsigned char actual66[66];
    secp256k1_musig_pubnonce pubnonce;
    secp256k1_musig_aggnonce aggnonce;
    const secp256k1_musig_pubnonce *pubnonce_ptrs[N_PUBNONCES];
    secp256k1_pubkey component;
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    /* Repeating one valid public nonce is deliberately a loop-stress case,
     * not a claim that a MuSig signer set may reuse one nonce. Compute each
     * expected component as 16*P through the public-key tweak API, which is a
     * separate path from the aggregation loop. */
    FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &pubnonce, nonce66) == 1);
    for (i = 0; i < N_PUBNONCES; i++) {
        pubnonce_ptrs[i] = &pubnonce;
    }
    for (i = 0; i < 2; i++) {
        size_t serialized_len = 33;
        FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &component, nonce66 + 33 * i, 33) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &component, sixteen32) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, expected66 + 33 * i, &serialized_len, &component, SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(serialized_len == 33);
    }

    FUZZ_CHECK(secp256k1_musig_nonce_agg(ctx, &aggnonce, pubnonce_ptrs, N_PUBNONCES) == 1);
    FUZZ_CHECK(secp256k1_musig_aggnonce_serialize(ctx, actual66, &aggnonce) == 1);
    FUZZ_CHECK(memcmp(actual66, expected66, sizeof(actual66)) == 0);
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

static void secp256k1_fuzz_check_musig_nonce_process_cache_failure_cleanup(secp256k1_context *ctx, const secp256k1_musig_aggnonce *aggnonce, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *valid_cache) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    unsigned char zero_session[sizeof(secp256k1_musig_session)] = { 0 };
    secp256k1_musig_keyagg_cache invalid_cache = *valid_cache;
    secp256k1_musig_keyagg_cache cache_before;
    secp256k1_musig_session session;

    /* Keep the aggregate nonce valid so this isolates the cache-load failure
     * after nonce_process has initialized its output. */
    invalid_cache.data[0] ^= 1u;
    cache_before = invalid_cache;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(&session, 0xA5, sizeof(session));
    FUZZ_CHECK(secp256k1_musig_nonce_process(ctx, &session, aggnonce, msg32, &invalid_cache) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&session, zero_session, sizeof(session)) == 0);
    FUZZ_CHECK(memcmp(&invalid_cache, &cache_before, sizeof(invalid_cache)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_nonce_process_null_input_cleanup(secp256k1_context *ctx, const secp256k1_musig_aggnonce *aggnonce, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache) {
    struct {
        const secp256k1_musig_aggnonce *aggnonce;
        const unsigned char *msg32;
        const secp256k1_musig_keyagg_cache *keyagg_cache;
    } invalid_cases[3];
    secp256k1_fuzz_musig_nonce_process_fn nonce_process = secp256k1_musig_nonce_process;
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_session valid_session;
    secp256k1_musig_session session;
    secp256k1_musig_aggnonce aggnonce_before = *aggnonce;
    secp256k1_musig_keyagg_cache cache_before = *keyagg_cache;
    unsigned char zero_session[sizeof(session)] = { 0 };
    size_t i;

    invalid_cases[0].aggnonce = NULL;
    invalid_cases[0].msg32 = msg32;
    invalid_cases[0].keyagg_cache = keyagg_cache;
    invalid_cases[1].aggnonce = aggnonce;
    invalid_cases[1].msg32 = NULL;
    invalid_cases[1].keyagg_cache = keyagg_cache;
    invalid_cases[2].aggnonce = aggnonce;
    invalid_cases[2].msg32 = msg32;
    invalid_cases[2].keyagg_cache = NULL;

    /* Start each invalid call from a real session so this catches stale
     * transcript reuse, not only a missing write into an already-empty slot. */
    FUZZ_CHECK(nonce_process(ctx, &valid_session, aggnonce, msg32, keyagg_cache) == 1);
    FUZZ_CHECK(memcmp(&valid_session, zero_session, sizeof(valid_session)) != 0);

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    for (i = 0; i < sizeof(invalid_cases) / sizeof(invalid_cases[0]); i++) {
        session = valid_session;
        FUZZ_CHECK(nonce_process(ctx, &session, invalid_cases[i].aggnonce, invalid_cases[i].msg32, invalid_cases[i].keyagg_cache) == 0);
        FUZZ_CHECK(illegal_data.calls == i + 1);
        FUZZ_CHECK(memcmp(&session, zero_session, sizeof(session)) == 0);
        FUZZ_CHECK(memcmp(aggnonce, &aggnonce_before, sizeof(aggnonce_before)) == 0);
        FUZZ_CHECK(memcmp(keyagg_cache, &cache_before, sizeof(cache_before)) == 0);
    }
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

static void secp256k1_fuzz_check_musig_empty_aggregation(secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const secp256k1_musig_pubnonce *pubnonce) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    int (*pubkey_agg)(const secp256k1_context *, secp256k1_xonly_pubkey *, secp256k1_musig_keyagg_cache *, const secp256k1_pubkey * const*, size_t) = secp256k1_musig_pubkey_agg;
    int (*nonce_agg)(const secp256k1_context *, secp256k1_musig_aggnonce *, const secp256k1_musig_pubnonce * const*, size_t) = secp256k1_musig_nonce_agg;
    const secp256k1_pubkey *pubkey_ptrs[1];
    const secp256k1_musig_pubnonce *pubnonce_ptrs[1];
    secp256k1_xonly_pubkey agg_pk;
    secp256k1_musig_keyagg_cache keyagg_cache;
    secp256k1_musig_aggnonce aggnonce;
    unsigned char zero_agg[sizeof(agg_pk)] = { 0 };
    unsigned char zero_cache[sizeof(keyagg_cache)] = { 0 };
    unsigned char zero_aggnonce[sizeof(aggnonce)] = { 0 };
    unsigned int calls;

    pubkey_ptrs[0] = pubkey;
    pubnonce_ptrs[0] = pubnonce;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    /* A zero-sized list must be rejected before any list element is loaded. */
    memset(&agg_pk, 0xA5, sizeof(agg_pk));
    memset(&keyagg_cache, 0x5A, sizeof(keyagg_cache));
    calls = illegal_data.calls;
    FUZZ_CHECK(pubkey_agg(ctx, &agg_pk, &keyagg_cache, pubkey_ptrs, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&agg_pk, zero_agg, sizeof(agg_pk)) == 0);
    FUZZ_CHECK(memcmp(&keyagg_cache, zero_cache, sizeof(keyagg_cache)) == 0);

    /* Keep the zero-sized case independent of the array pointer as well. */
    memset(&agg_pk, 0x96, sizeof(agg_pk));
    memset(&keyagg_cache, 0x69, sizeof(keyagg_cache));
    calls = illegal_data.calls;
    FUZZ_CHECK(pubkey_agg(ctx, &agg_pk, &keyagg_cache, NULL, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&agg_pk, zero_agg, sizeof(agg_pk)) == 0);
    FUZZ_CHECK(memcmp(&keyagg_cache, zero_cache, sizeof(keyagg_cache)) == 0);

    memset(&aggnonce, 0xA5, sizeof(aggnonce));
    calls = illegal_data.calls;
    FUZZ_CHECK(nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&aggnonce, zero_aggnonce, sizeof(aggnonce)) == 0);

    memset(&aggnonce, 0x5A, sizeof(aggnonce));
    calls = illegal_data.calls;
    FUZZ_CHECK(nonce_agg(ctx, &aggnonce, NULL, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&aggnonce, zero_aggnonce, sizeof(aggnonce)) == 0);

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

    /* The second 64-byte group storage starts after the aggregate point. It
     * has its own curve-validation contract even when the aggregate point is
     * valid; use x = 1, y = 0 to make that distinction explicit. */
    invalid_cache = *valid_cache;
    memset(invalid_cache.data + 68, 0, 64);
    invalid_cache.data[68] = 1;
    cache_before = invalid_cache;

    memset(&output_pubkey, 0x96, sizeof(output_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &output_pubkey, &invalid_cache) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&output_pubkey, zero_pubkey, sizeof(output_pubkey)) == 0);
    FUZZ_CHECK(memcmp(&invalid_cache, &cache_before, sizeof(invalid_cache)) == 0);

    memset(&output_pubkey, 0x69, sizeof(output_pubkey));
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

static void secp256k1_fuzz_check_musig_tweak_infinity_rollback(secp256k1_context *ctx, const secp256k1_pubkey *generator_pubkey) {
    const secp256k1_pubkey *pubkeys[1];
    const secp256k1_fuzz_musig_tweak_func tweak_funcs[2] = {
        secp256k1_musig_pubkey_ec_tweak_add,
        secp256k1_musig_pubkey_xonly_tweak_add
    };
    unsigned char coefficient[32];
    unsigned char cancel_tweaks[2][32];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    secp256k1_musig_keyagg_cache cache;
    secp256k1_musig_keyagg_cache cache_before;
    secp256k1_musig_keyagg_cache failed_cache;
    secp256k1_pubkey aggregate_pubkey;
    secp256k1_pubkey expected_pubkey;
    secp256k1_pubkey failed_output;
    secp256k1_xonly_pubkey aggregate_xonly;
    secp256k1_fuzz_musig_illegal_data illegal_data;
    int aggregate_parity;
    unsigned int calls;
    size_t i;

    FUZZ_CHECK(generator_pubkey != NULL);
    pubkeys[0] = generator_pubkey;
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, NULL, &cache, pubkeys, 1) == 1);
    secp256k1_fuzz_musig_keyagg_coefficient_reference(ctx, coefficient, pubkeys, 1, 0);
    FUZZ_CHECK(memcmp(coefficient, secp256k1_fuzz_scalar_zero, sizeof(coefficient)) != 0);
    expected_pubkey = *generator_pubkey;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &expected_pubkey, coefficient) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_get(ctx, &aggregate_pubkey, &cache) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &aggregate_pubkey, &expected_pubkey) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &aggregate_xonly, &aggregate_parity, &aggregate_pubkey) == 1);
    FUZZ_CHECK(aggregate_parity == 0 || aggregate_parity == 1);
    secp256k1_fuzz_musig_scalar_negate(cancel_tweaks[0], coefficient);
    if (aggregate_parity) {
        memcpy(cancel_tweaks[1], coefficient, sizeof(cancel_tweaks[1]));
    } else {
        memcpy(cancel_tweaks[1], cancel_tweaks[0], sizeof(cancel_tweaks[1]));
    }
    cache_before = cache;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    for (i = 0; i < sizeof(tweak_funcs) / sizeof(tweak_funcs[0]); i++) {
        failed_cache = cache_before;
        memset(&failed_output, 0xA5, sizeof(failed_output));
        calls = illegal_data.calls;
        FUZZ_CHECK(tweak_funcs[i](ctx, &failed_output, &failed_cache, cancel_tweaks[i]) == 0);
        FUZZ_CHECK(illegal_data.calls == calls);
        FUZZ_CHECK(memcmp(&failed_output, zero_pubkey, sizeof(failed_output)) == 0);
        FUZZ_CHECK(memcmp(&failed_cache, &cache_before, sizeof(failed_cache)) == 0);

        failed_cache = cache_before;
        calls = illegal_data.calls;
        FUZZ_CHECK(tweak_funcs[i](ctx, NULL, &failed_cache, cancel_tweaks[i]) == 0);
        FUZZ_CHECK(illegal_data.calls == calls);
        FUZZ_CHECK(memcmp(&failed_cache, &cache_before, sizeof(failed_cache)) == 0);
    }
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
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

static void secp256k1_fuzz_check_musig_nonce_gen_counter_optional_inputs(const secp256k1_context *ctx, const unsigned char *seckey, const secp256k1_keypair *keypair, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    unsigned char counter_secrand[32];
    unsigned char expected_k64[64];
    unsigned char expected_pubnonce66[66];
    unsigned char zero32[32] = { 0 };
    unsigned char zero132[132] = { 0 };
    unsigned char serialized_counter[66];
    unsigned char serialized_explicit[66];
    const unsigned char *case_msg32;
    const secp256k1_musig_keyagg_cache *case_keyagg_cache;
    const unsigned char *case_extra_input32;
    uint64_t counter;
    unsigned int combination;
    int counter_ret;
    int explicit_ret;
    secp256k1_musig_secnonce secnonce_counter;
    secp256k1_musig_secnonce secnonce_explicit;
    secp256k1_musig_pubnonce pubnonce_counter;
    secp256k1_musig_pubnonce pubnonce_explicit;

    /* The deterministic tests cover these optional arguments individually,
     * but the counter wrapper has its own keypair-loading and byte-counter
     * path. Compare every combination against both independent paths. */
    for (combination = 0; combination < 8; combination++) {
        case_msg32 = (combination & 1u) != 0 ? msg32 : NULL;
        case_keyagg_cache = (combination & 2u) != 0 ? keyagg_cache : NULL;
        case_extra_input32 = (combination & 4u) != 0 ? extra_input32 : NULL;
        counter = UINT64_MAX - (uint64_t)combination;
        secp256k1_fuzz_musig_counter_to_secrand(counter_secrand, counter);
        memset(&secnonce_counter, 0xA5, sizeof(secnonce_counter));
        memset(&pubnonce_counter, 0xA5, sizeof(pubnonce_counter));
        counter_ret = secp256k1_musig_nonce_gen_counter(ctx, &secnonce_counter, &pubnonce_counter, counter, keypair, case_msg32, case_keyagg_cache, case_extra_input32);
        FUZZ_CHECK(secp256k1_fuzz_musig_nonce_reference(ctx, expected_k64, expected_pubnonce66, counter_secrand, seckey, pubkey, case_msg32, case_keyagg_cache, case_extra_input32) == counter_ret);

        memset(&secnonce_explicit, 0xA5, sizeof(secnonce_explicit));
        memset(&pubnonce_explicit, 0xA5, sizeof(pubnonce_explicit));
        explicit_ret = secp256k1_musig_nonce_gen(ctx, &secnonce_explicit, &pubnonce_explicit, counter_secrand, seckey, pubkey, case_msg32, case_keyagg_cache, case_extra_input32);
        FUZZ_CHECK(explicit_ret == counter_ret);
        if (counter_ret == 0) {
            FUZZ_CHECK(memcmp(&secnonce_counter, zero132, sizeof(secnonce_counter)) == 0);
            FUZZ_CHECK(memcmp(&pubnonce_counter, zero132, sizeof(pubnonce_counter)) == 0);
            FUZZ_CHECK(memcmp(&secnonce_explicit, zero132, sizeof(secnonce_explicit)) == 0);
            FUZZ_CHECK(memcmp(&pubnonce_explicit, zero132, sizeof(pubnonce_explicit)) == 0);
            continue;
        }
        FUZZ_CHECK(memcmp(counter_secrand, zero32, sizeof(counter_secrand)) == 0);
        FUZZ_CHECK(memcmp(secnonce_counter.data, secnonce_explicit.data, sizeof(secnonce_counter.data)) == 0);
        FUZZ_CHECK(memcmp(secnonce_counter.data + 4, expected_k64, sizeof(expected_k64)) == 0);
        FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized_counter, &pubnonce_counter) == 1);
        FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized_explicit, &pubnonce_explicit) == 1);
        FUZZ_CHECK(memcmp(serialized_counter, serialized_explicit, sizeof(serialized_counter)) == 0);
        FUZZ_CHECK(memcmp(serialized_counter, expected_pubnonce66, sizeof(serialized_counter)) == 0);
    }
}

static void secp256k1_fuzz_check_musig_nonce_gen_optional_seckey(const secp256k1_context *ctx, const unsigned char *input, size_t size, const unsigned char *seckey, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    unsigned char session_secrand[32];
    unsigned char session_secrand_before[32];
    unsigned char expected_k64[64];
    unsigned char expected_pubnonce66[66];
    unsigned char serialized_pubnonce66[66];
    unsigned char zero32[32] = { 0 };
    unsigned char zero132[132] = { 0 };
    const unsigned char *case_seckey;
    const unsigned char *case_msg32;
    const secp256k1_musig_keyagg_cache *case_keyagg_cache;
    const unsigned char *case_extra_input32;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    unsigned int combination;
    int expected_ret;
    int actual_ret;

    /* The public API permits seckey to be omitted while retaining the signer
     * public key. Cover that transcript branch independently of the counter
     * wrapper, which necessarily has a keypair and therefore cannot reach it. */
    for (combination = 0; combination < 16; combination++) {
        case_seckey = (combination & 1u) != 0 ? seckey : NULL;
        case_msg32 = (combination & 2u) != 0 ? msg32 : NULL;
        case_keyagg_cache = (combination & 4u) != 0 ? keyagg_cache : NULL;
        case_extra_input32 = (combination & 8u) != 0 ? extra_input32 : NULL;
        secp256k1_fuzz_derive(session_secrand, sizeof(session_secrand), input, size, 401u + combination);
        if (memcmp(session_secrand, zero32, sizeof(session_secrand)) == 0) {
            session_secrand[31] = (unsigned char)(combination + 1u);
        }
        memcpy(session_secrand_before, session_secrand, sizeof(session_secrand));
        memset(&secnonce, 0xA5, sizeof(secnonce));
        memset(&pubnonce, 0x5A, sizeof(pubnonce));
        expected_ret = secp256k1_fuzz_musig_nonce_reference(ctx, expected_k64, expected_pubnonce66, session_secrand_before, case_seckey, pubkey, case_msg32, case_keyagg_cache, case_extra_input32);
        actual_ret = secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_secrand, case_seckey, pubkey, case_msg32, case_keyagg_cache, case_extra_input32);
        FUZZ_CHECK(actual_ret == expected_ret);
        if (expected_ret == 0) {
            FUZZ_CHECK(memcmp(&secnonce, zero132, sizeof(secnonce)) == 0);
            FUZZ_CHECK(memcmp(&pubnonce, zero132, sizeof(pubnonce)) == 0);
            FUZZ_CHECK(memcmp(session_secrand, session_secrand_before, sizeof(session_secrand)) == 0);
            continue;
        }
        FUZZ_CHECK(memcmp(session_secrand, zero32, sizeof(session_secrand)) == 0);
        FUZZ_CHECK(memcmp(secnonce.data + 4, expected_k64, sizeof(expected_k64)) == 0);
        FUZZ_CHECK(secp256k1_musig_pubnonce_serialize(ctx, serialized_pubnonce66, &pubnonce) == 1);
        FUZZ_CHECK(memcmp(serialized_pubnonce66, expected_pubnonce66, sizeof(serialized_pubnonce66)) == 0);
    }
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
        FUZZ_CHECK(secp256k1_fuzz_musig_partial_sig_equation(ctx, &partial_sig, &pubnonce, &pubkey, pubkey_ptrs, 1, 0, &keyagg_cache, &session, msg32) == 1);
        FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig, &pubnonce, &pubkey, &keyagg_cache, &session) == 1);
        FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sig_ptrs, 1) == 1);
        secp256k1_fuzz_check_musig_final_sig_equation(ctx, sig64, &session, &agg_pk, msg32);
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
    {
        const secp256k1_pubkey *pubkey_ptrs[1];
        pubkey_ptrs[0] = pubkey;
        FUZZ_CHECK(secp256k1_fuzz_musig_partial_sig_equation(ctx, &partial_sig, &pubnonce, pubkey, pubkey_ptrs, 1, 0, keyagg_cache, &session, msg32) == 1);
    }
    FUZZ_CHECK(secp256k1_musig_partial_sig_verify(ctx, &partial_sig, &pubnonce, pubkey, keyagg_cache, &session) == 1);

    partial_sig_ptrs[0] = &partial_sig;
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sig_ptrs, 1) == 1);
    secp256k1_fuzz_check_musig_final_sig_equation(ctx, sig64, &session, agg_pk, msg32);
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
    {
        const secp256k1_pubkey *pubkey_ptrs[1];
        pubkey_ptrs[0] = pubkey;
        FUZZ_CHECK(secp256k1_fuzz_musig_partial_sig_equation(ctx, &partial_sig, &pubnonce, pubkey, pubkey_ptrs, 1, 0, keyagg_cache, &session, msg32) == 1);
    }
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
    const secp256k1_musig_partial_sig *invalid_partial_sig_ptrs[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS];
    size_t i;

    FUZZ_CHECK(n_sigs > 0);
    FUZZ_CHECK(n_sigs <= SECP256K1_FUZZ_MUSIG_MAX_SIGNERS);

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

static void secp256k1_fuzz_check_musig_aggregation_null_member_cleanup(secp256k1_context *ctx, const secp256k1_musig_pubnonce *pubnonce_a, const secp256k1_musig_pubnonce *pubnonce_b, const secp256k1_musig_partial_sig *partial_sig_a, const secp256k1_musig_partial_sig *partial_sig_b, const secp256k1_musig_session *session) {
    int (*nonce_agg)(const secp256k1_context *, secp256k1_musig_aggnonce *, const secp256k1_musig_pubnonce * const*, size_t) = secp256k1_musig_nonce_agg;
    secp256k1_fuzz_musig_partial_sig_agg_fn partial_sig_agg = secp256k1_musig_partial_sig_agg;
    secp256k1_fuzz_musig_illegal_data illegal_data;
    const secp256k1_musig_pubnonce *pubnonce_ptrs[2];
    const secp256k1_musig_partial_sig *partial_sig_ptrs[2];
    secp256k1_musig_pubnonce pubnonce_a_before = *pubnonce_a;
    secp256k1_musig_pubnonce pubnonce_b_before = *pubnonce_b;
    secp256k1_musig_partial_sig partial_sig_a_before = *partial_sig_a;
    secp256k1_musig_partial_sig partial_sig_b_before = *partial_sig_b;
    secp256k1_musig_aggnonce aggnonce;
    unsigned char sig64[64];
    unsigned char zero_aggnonce[sizeof(aggnonce)] = { 0 };
    unsigned char zero_sig64[sizeof(sig64)] = { 0 };

    /* The first case reaches a valid member before the NULL pointer; the
     * second makes the precondition independent of the prefix. */
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    pubnonce_ptrs[0] = pubnonce_a;
    pubnonce_ptrs[1] = NULL;
    memset(&aggnonce, 0xA5, sizeof(aggnonce));
    FUZZ_CHECK(nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 2) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&aggnonce, zero_aggnonce, sizeof(aggnonce)) == 0);

    pubnonce_ptrs[0] = NULL;
    pubnonce_ptrs[1] = pubnonce_b;
    memset(&aggnonce, 0x5A, sizeof(aggnonce));
    FUZZ_CHECK(nonce_agg(ctx, &aggnonce, pubnonce_ptrs, 2) == 0);
    FUZZ_CHECK(illegal_data.calls == 2);
    FUZZ_CHECK(memcmp(&aggnonce, zero_aggnonce, sizeof(aggnonce)) == 0);

    partial_sig_ptrs[0] = partial_sig_a;
    partial_sig_ptrs[1] = NULL;
    memset(sig64, 0x3C, sizeof(sig64));
    FUZZ_CHECK(partial_sig_agg(ctx, sig64, session, partial_sig_ptrs, 2) == 0);
    FUZZ_CHECK(illegal_data.calls == 3);
    FUZZ_CHECK(memcmp(sig64, zero_sig64, sizeof(sig64)) == 0);

    partial_sig_ptrs[0] = NULL;
    partial_sig_ptrs[1] = partial_sig_b;
    memset(sig64, 0x96, sizeof(sig64));
    FUZZ_CHECK(partial_sig_agg(ctx, sig64, session, partial_sig_ptrs, 2) == 0);
    FUZZ_CHECK(illegal_data.calls == 4);
    FUZZ_CHECK(memcmp(sig64, zero_sig64, sizeof(sig64)) == 0);

    FUZZ_CHECK(memcmp(pubnonce_a, &pubnonce_a_before, sizeof(pubnonce_a_before)) == 0);
    FUZZ_CHECK(memcmp(pubnonce_b, &pubnonce_b_before, sizeof(pubnonce_b_before)) == 0);
    FUZZ_CHECK(memcmp(partial_sig_a, &partial_sig_a_before, sizeof(partial_sig_a_before)) == 0);
    FUZZ_CHECK(memcmp(partial_sig_b, &partial_sig_b_before, sizeof(partial_sig_b_before)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_empty_partial_sig_aggregation(secp256k1_context *ctx, const secp256k1_musig_session *session, const secp256k1_musig_partial_sig * const *partial_sig_ptrs) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_fuzz_musig_partial_sig_agg_fn partial_sig_agg = secp256k1_musig_partial_sig_agg;
    unsigned char sig64[64];
    unsigned char zero64[64] = { 0 };
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    /* The list length is a contract, even when the array pointer is valid. */
    memset(sig64, 0xA5, sizeof(sig64));
    FUZZ_CHECK(partial_sig_agg(ctx, sig64, session, partial_sig_ptrs, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    /* Keep the zero-length case independent of the array pointer. */
    memset(sig64, 0x5A, sizeof(sig64));
    calls = illegal_data.calls;
    FUZZ_CHECK(partial_sig_agg(ctx, sig64, session, NULL, 0) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_partial_sig_agg_long(const secp256k1_context *ctx, const unsigned char *input, size_t size, const secp256k1_musig_session *session) {
    static const unsigned char trigger[] = "long MuSig partial signature aggregation\n";
    enum { N_SIGS = 16 };
    secp256k1_musig_partial_sig boundary_sig;
    const secp256k1_musig_partial_sig *partial_sig_ptrs[N_SIGS];
    unsigned char partial_sig32[32];
    unsigned char expected_sig64[64];
    unsigned char actual_sig64[64];
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    /* The API aggregates scalar encodings and does not verify signer identity
     * or the partial-signature equation. Repeating this parseable boundary
     * scalar stresses the count loop and forces modular wraparound, while
     * the expected result comes from independent byte arithmetic. */
    FUZZ_CHECK(secp256k1_musig_partial_sig_parse(ctx, &boundary_sig, secp256k1_fuzz_scalar_order_minus_one) == 1);
    FUZZ_CHECK(secp256k1_musig_partial_sig_serialize(ctx, partial_sig32, &boundary_sig) == 1);
    memcpy(expected_sig64, session->data + 5, 32);
    memcpy(expected_sig64 + 32, session->data + 101, 32);
    for (i = 0; i < N_SIGS; i++) {
        partial_sig_ptrs[i] = &boundary_sig;
        secp256k1_fuzz_musig_scalar_add_mod_order(expected_sig64 + 32, partial_sig32);
    }

    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, actual_sig64, session, partial_sig_ptrs, N_SIGS) == 1);
    FUZZ_CHECK(memcmp(actual_sig64, expected_sig64, sizeof(actual_sig64)) == 0);
}

static void secp256k1_fuzz_check_musig_session_state_barrier(secp256k1_context *ctx, const secp256k1_musig_session *valid_session, const secp256k1_musig_partial_sig * const *partial_sig_ptrs, size_t n_sigs) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_session invalid_session;
    unsigned char sig64[64];
    unsigned char zero64[64] = { 0 };
    unsigned int calls;

    FUZZ_CHECK(n_sigs > 0);
    FUZZ_CHECK(n_sigs <= SECP256K1_FUZZ_MUSIG_MAX_SIGNERS);

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
    unsigned char zero_secnonce[sizeof(secnonce)] = { 0 };

    invalid_cache.data[0] ^= 1u;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    secnonce = *valid_secnonce;
    memset(&partial_sig, 0xA5, sizeof(partial_sig));
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, keypair, &invalid_cache, session) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&partial_sig, zero_sig, sizeof(partial_sig)) == 0);
    FUZZ_CHECK(memcmp(&secnonce, zero_secnonce, sizeof(secnonce)) == 0);

    secnonce = *valid_secnonce;
    secnonce.data[0] ^= 1u;
    memset(&partial_sig, 0x5A, sizeof(partial_sig));
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, keypair, valid_cache, session) == 0);
    FUZZ_CHECK(illegal_data.calls == 2);
    FUZZ_CHECK(memcmp(&partial_sig, zero_sig, sizeof(partial_sig)) == 0);
    FUZZ_CHECK(memcmp(&secnonce, zero_secnonce, sizeof(secnonce)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_partial_sign_null_argument_cleanup(secp256k1_context *ctx, const secp256k1_musig_secnonce *valid_secnonce, const secp256k1_keypair *valid_keypair, const secp256k1_musig_keyagg_cache *valid_cache, const secp256k1_musig_session *valid_session) {
    struct {
        const secp256k1_keypair *keypair;
        const secp256k1_musig_keyagg_cache *keyagg_cache;
        const secp256k1_musig_session *session;
    } invalid_cases[3];
    secp256k1_fuzz_musig_partial_sign_fn partial_sign = secp256k1_musig_partial_sign;
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_partial_sig partial_sig;
    secp256k1_musig_secnonce secnonce;
    secp256k1_keypair keypair_before = *valid_keypair;
    secp256k1_musig_keyagg_cache cache_before = *valid_cache;
    secp256k1_musig_session session_before = *valid_session;
    unsigned char zero_partial_sig[sizeof(partial_sig)] = { 0 };
    unsigned char zero_secnonce[sizeof(secnonce)] = { 0 };
    size_t i;

    invalid_cases[0].keypair = NULL;
    invalid_cases[0].keyagg_cache = valid_cache;
    invalid_cases[0].session = valid_session;
    invalid_cases[1].keypair = valid_keypair;
    invalid_cases[1].keyagg_cache = NULL;
    invalid_cases[1].session = valid_session;
    invalid_cases[2].keypair = valid_keypair;
    invalid_cases[2].keyagg_cache = valid_cache;
    invalid_cases[2].session = NULL;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    for (i = 0; i < sizeof(invalid_cases) / sizeof(invalid_cases[0]); i++) {
        memset(&partial_sig, 0xA5, sizeof(partial_sig));
        secnonce = *valid_secnonce;
        FUZZ_CHECK(partial_sign(ctx, &partial_sig, &secnonce, invalid_cases[i].keypair, invalid_cases[i].keyagg_cache, invalid_cases[i].session) == 0);
        FUZZ_CHECK(illegal_data.calls == i + 1);
        FUZZ_CHECK(memcmp(&partial_sig, zero_partial_sig, sizeof(partial_sig)) == 0);
        FUZZ_CHECK(memcmp(&secnonce, zero_secnonce, sizeof(secnonce)) == 0);
        FUZZ_CHECK(memcmp(valid_keypair, &keypair_before, sizeof(keypair_before)) == 0);
        FUZZ_CHECK(memcmp(valid_cache, &cache_before, sizeof(cache_before)) == 0);
        FUZZ_CHECK(memcmp(valid_session, &session_before, sizeof(session_before)) == 0);
    }
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_partial_sign_null_output_cleanup(secp256k1_context *ctx, const secp256k1_musig_secnonce *valid_secnonce, const secp256k1_keypair *valid_keypair, const secp256k1_musig_keyagg_cache *valid_cache, const secp256k1_musig_session *valid_session) {
    secp256k1_fuzz_musig_partial_sign_fn partial_sign = secp256k1_musig_partial_sign;
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_secnonce secnonce;
    secp256k1_keypair keypair_before = *valid_keypair;
    secp256k1_musig_keyagg_cache cache_before = *valid_cache;
    secp256k1_musig_session session_before = *valid_session;
    unsigned char zero_secnonce[sizeof(secnonce)] = { 0 };

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    secnonce = *valid_secnonce;
    FUZZ_CHECK(partial_sign(ctx, NULL, &secnonce, valid_keypair, valid_cache, valid_session) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&secnonce, zero_secnonce, sizeof(secnonce)) == 0);
    FUZZ_CHECK(memcmp(valid_keypair, &keypair_before, sizeof(keypair_before)) == 0);
    FUZZ_CHECK(memcmp(valid_cache, &cache_before, sizeof(cache_before)) == 0);
    FUZZ_CHECK(memcmp(valid_session, &session_before, sizeof(session_before)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_partial_sign_opaque_state_cleanup(secp256k1_context *ctx, const secp256k1_musig_secnonce *valid_secnonce, const secp256k1_keypair *valid_keypair, const secp256k1_musig_keyagg_cache *valid_cache, const secp256k1_musig_session *valid_session) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_keypair invalid_keypair;
    secp256k1_keypair generator_keypair;
    secp256k1_pubkey generator_pubkey;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_partial_sig partial_sig;
    secp256k1_musig_session invalid_session;
    secp256k1_keypair keypair_before;
    secp256k1_keypair valid_keypair_before;
    secp256k1_musig_keyagg_cache cache_before;
    secp256k1_musig_session session_before;
    secp256k1_musig_session invalid_session_before;
    unsigned char zero_partial_sig[sizeof(partial_sig)] = { 0 };
    unsigned char zero_secnonce[sizeof(secnonce)] = { 0 };
    unsigned int calls;

    /* Make keypair_load's documented dummy point equal the secnonce point so
     * a mutation that ignores the failed load reaches the signing code. */
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &generator_keypair, secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &generator_pubkey, &generator_keypair) == 1);
    invalid_keypair = *valid_keypair;
    memset(invalid_keypair.data + 32, 0, sizeof(invalid_keypair.data) - 32);
    secnonce = *valid_secnonce;
    memcpy(secnonce.data + 68, generator_pubkey.data, sizeof(generator_pubkey.data));
    keypair_before = invalid_keypair;
    valid_keypair_before = *valid_keypair;
    cache_before = *valid_cache;
    session_before = *valid_session;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    memset(&partial_sig, 0xA5, sizeof(partial_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, &invalid_keypair, valid_cache, valid_session) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&partial_sig, zero_partial_sig, sizeof(partial_sig)) == 0);
    FUZZ_CHECK(memcmp(&secnonce, zero_secnonce, sizeof(secnonce)) == 0);
    FUZZ_CHECK(memcmp(&invalid_keypair, &keypair_before, sizeof(invalid_keypair)) == 0);
    FUZZ_CHECK(memcmp(valid_keypair, &valid_keypair_before, sizeof(valid_keypair_before)) == 0);
    FUZZ_CHECK(memcmp(valid_cache, &cache_before, sizeof(cache_before)) == 0);
    FUZZ_CHECK(memcmp(valid_session, &session_before, sizeof(session_before)) == 0);

    invalid_session = *valid_session;
    invalid_session.data[4] = 2;
    invalid_session_before = invalid_session;
    secnonce = *valid_secnonce;
    memset(&partial_sig, 0x5A, sizeof(partial_sig));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &secnonce, valid_keypair, valid_cache, &invalid_session) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&partial_sig, zero_partial_sig, sizeof(partial_sig)) == 0);
    FUZZ_CHECK(memcmp(&secnonce, zero_secnonce, sizeof(secnonce)) == 0);
    FUZZ_CHECK(memcmp(valid_keypair, &valid_keypair_before, sizeof(valid_keypair_before)) == 0);
    FUZZ_CHECK(memcmp(valid_cache, &cache_before, sizeof(cache_before)) == 0);
    FUZZ_CHECK(memcmp(&invalid_session, &invalid_session_before, sizeof(invalid_session)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_secnonce_reuse(secp256k1_context *ctx, secp256k1_musig_secnonce *secnonce, const secp256k1_musig_keyagg_cache *keyagg_cache, const secp256k1_musig_session *session) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_keypair generator_keypair;
    secp256k1_musig_partial_sig replay_sig;
    unsigned char zero_sig[sizeof(replay_sig)] = { 0 };
    unsigned char zero_secnonce[sizeof(*secnonce)] = { 0 };

    /* A successful partial signature consumes the secret nonce. Exercise the
     * next call so a zeroed object cannot silently become signable again. The
     * generator keypair lets the mutation proof reach signing after it
     * replaces the consumed nonce's missing public point with G. */
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &generator_keypair, secp256k1_fuzz_scalar_one) == 1);
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(&replay_sig, 0xA5, sizeof(replay_sig));
    FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &replay_sig, secnonce, &generator_keypair, keyagg_cache, session) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&replay_sig, zero_sig, sizeof(replay_sig)) == 0);
    FUZZ_CHECK(memcmp(secnonce, zero_secnonce, sizeof(*secnonce)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_musig_sign_roundtrip(secp256k1_context *ctx, const unsigned char *input, size_t size, const unsigned char (*seckey)[32], const secp256k1_keypair *keypairs, const secp256k1_pubkey *pubkeys, size_t n_pubkeys, const unsigned char *msg32) {
    static const unsigned char scalar_two[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };
    unsigned char session_rand[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS][32];
    unsigned char pubnonce_ser[66];
    unsigned char wrong_pubnonce_ser[66];
    unsigned char sig64[64];
    unsigned char sig64_replay[64];
    unsigned char zero132[132] = { 0 };
    unsigned char expected_k64[64];
    unsigned char expected_pubnonce66[66];
    secp256k1_musig_secnonce secnonce[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS];
    secp256k1_musig_pubnonce pubnonce[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS];
    secp256k1_musig_pubnonce wrong_pubnonce;
    secp256k1_pubkey wrong_pubkey;
    const secp256k1_musig_pubnonce *pubnonce_ptrs[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS];
    secp256k1_musig_aggnonce aggnonce;
    secp256k1_musig_session session;
    secp256k1_musig_session session_replay;
    secp256k1_musig_session wrong_cache_session;
    secp256k1_musig_partial_sig partial_sig[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS];
    const secp256k1_musig_partial_sig *partial_sig_ptrs[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS];
    const secp256k1_pubkey *pubkey_ptrs[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS];
    secp256k1_musig_keyagg_cache keyagg_cache;
    secp256k1_musig_keyagg_cache wrong_cache;
    secp256k1_xonly_pubkey agg_pk;
    secp256k1_xonly_pubkey wrong_cache_agg_pk;
    secp256k1_pubkey wrong_cache_pubkey;
    const secp256k1_pubkey *wrong_cache_pubkey_ptrs[1];
    size_t n_signers = n_pubkeys < 2 ? 2 : n_pubkeys;
    size_t i;

    FUZZ_CHECK(n_signers <= SECP256K1_FUZZ_MUSIG_MAX_SIGNERS);

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
    secp256k1_fuzz_check_musig_nonce_process_cache_failure_cleanup(ctx, &aggnonce, msg32, &keyagg_cache);
    secp256k1_fuzz_check_musig_nonce_process_null_input_cleanup(ctx, &aggnonce, msg32, &keyagg_cache);
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
    secp256k1_fuzz_check_musig_partial_sign_null_argument_cleanup(ctx, &secnonce[0], &keypairs[0], &keyagg_cache, &session);
    if (size == sizeof("partial-sign-opaque-state-cleanup\n") - 1
            && memcmp(input, "partial-sign-opaque-state-cleanup\n", sizeof("partial-sign-opaque-state-cleanup\n") - 1) == 0) {
        secp256k1_fuzz_check_musig_partial_sign_opaque_state_cleanup(ctx, &secnonce[0], &keypairs[0], &keyagg_cache, &session);
    }
    if (size == sizeof("partial-sign-null-output-cleanup\n") - 1
            && memcmp(input, "partial-sign-null-output-cleanup\n", sizeof("partial-sign-null-output-cleanup\n") - 1) == 0) {
        secp256k1_fuzz_check_musig_partial_sign_null_output_cleanup(ctx, &secnonce[0], &keypairs[0], &keyagg_cache, &session);
    }
    for (i = 0; i < n_signers; i++) {
        FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig[i], &secnonce[i], &keypairs[i], &keyagg_cache, &session) == 1);
        FUZZ_CHECK(memcmp(secnonce[i].data, zero132, sizeof(secnonce[i].data)) == 0);
        if (i == 0) {
            secp256k1_fuzz_check_musig_secnonce_reuse(ctx, &secnonce[i], &keyagg_cache, &session);
        }
        FUZZ_CHECK(secp256k1_fuzz_musig_partial_sig_equation(ctx, &partial_sig[i], &pubnonce[i], &pubkeys[i], pubkey_ptrs, n_signers, i, &keyagg_cache, &session, msg32) == 1);
        secp256k1_fuzz_check_musig_arbitrary_partial_sig(ctx, input, size, &pubnonce[i], &pubkeys[i], pubkey_ptrs, n_signers, i, &keyagg_cache, &session, msg32);
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
    secp256k1_fuzz_check_musig_empty_partial_sig_aggregation(ctx, &session, partial_sig_ptrs);
    secp256k1_fuzz_check_musig_partial_sig_agg_long(ctx, input, size, &session);
    memset(sig64, 0xA5, sizeof(sig64));
    memset(sig64_replay, 0x5A, sizeof(sig64_replay));
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64, &session, partial_sig_ptrs, n_signers) == 1);
    FUZZ_CHECK(secp256k1_musig_partial_sig_agg(ctx, sig64_replay, &session_replay, partial_sig_ptrs, n_signers) == 1);
    FUZZ_CHECK(memcmp(sig64, sig64_replay, sizeof(sig64)) == 0);
    secp256k1_fuzz_check_musig_final_sig_equation(ctx, sig64, &session, &agg_pk, msg32);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, 32, &agg_pk) == 1);
    secp256k1_fuzz_check_musig_partial_sig_agg_failure_cleanup(ctx, &session, partial_sig_ptrs, n_signers);
    if (size >= 80) {
        secp256k1_fuzz_check_musig_aggregation_null_member_cleanup(ctx, &pubnonce[0], &pubnonce[1], &partial_sig[0], &partial_sig[1], &session);
    }
    if (size == sizeof("partial-sig-verify-invalid-state\n") - 1
            && memcmp(input, "partial-sig-verify-invalid-state\n", sizeof("partial-sig-verify-invalid-state\n") - 1) == 0) {
        secp256k1_fuzz_check_musig_partial_sig_verify_invalid_state(ctx, &partial_sig[0], &pubnonce[0], &pubkeys[0], &keyagg_cache, &session);
    }
}

static void secp256k1_fuzz_check_musig_sixteen_sign_roundtrip(secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "sixteen MuSig sign roundtrip\n";
    static const unsigned char msg32[32] = {
        0x73, 0x69, 0x78, 0x74, 0x65, 0x65, 0x6e, 0x20,
        0x4d, 0x75, 0x53, 0x69, 0x67, 0x20, 0x73, 0x69,
        0x67, 0x6e, 0x20, 0x72, 0x6f, 0x75, 0x6e, 0x64,
        0x74, 0x72, 0x69, 0x70, 0x00, 0x00, 0x00, 0x01
    };
    unsigned char seckey[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS][32] = { { 0 } };
    secp256k1_keypair keypairs[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS];
    secp256k1_pubkey pubkeys[SECP256K1_FUZZ_MUSIG_MAX_SIGNERS];
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    for (i = 0; i < SECP256K1_FUZZ_MUSIG_MAX_SIGNERS; i++) {
        seckey[i][31] = (unsigned char)(i + 1);
        FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypairs[i], seckey[i]) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckey[i]) == 1);
    }
    secp256k1_fuzz_check_musig_sign_roundtrip(ctx, input, size, seckey, keypairs, pubkeys, SECP256K1_FUZZ_MUSIG_MAX_SIGNERS, msg32);
}

static void secp256k1_fuzz_check_musig_nonce_gen_failure_cleanup(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *valid_seckey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32, const unsigned char *session_rand32) {
    unsigned char session_rand[32];
    unsigned char session_rand_before[32];
    unsigned char zero132[132] = { 0 };
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;

    /* Only successful nonce generation consumes the caller's session random;
     * rejected inputs must leave it available for a corrected retry. */
    memcpy(session_rand, session_rand32, sizeof(session_rand));
    if (memcmp(session_rand, secp256k1_fuzz_scalar_zero, sizeof(session_rand)) == 0) {
        memcpy(session_rand, secp256k1_fuzz_scalar_one, sizeof(session_rand));
    }
    memcpy(session_rand_before, session_rand, sizeof(session_rand));
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_rand, secp256k1_fuzz_scalar_zero, pubkey, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(memcmp(session_rand, session_rand_before, sizeof(session_rand)) == 0);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);

    memcpy(session_rand, session_rand32, sizeof(session_rand));
    if (memcmp(session_rand, secp256k1_fuzz_scalar_zero, sizeof(session_rand)) == 0) {
        memcpy(session_rand, secp256k1_fuzz_scalar_one, sizeof(session_rand));
    }
    memcpy(session_rand_before, session_rand, sizeof(session_rand));
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_rand, secp256k1_fuzz_scalar_order, pubkey, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(memcmp(session_rand, session_rand_before, sizeof(session_rand)) == 0);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);

    memset(session_rand, 0, sizeof(session_rand));
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_rand, valid_seckey, pubkey, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);
}

static void secp256k1_fuzz_check_musig_nonce_gen_zero_scalar_failure(secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const unsigned char *seckey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32, const unsigned char *session_rand32) {
    unsigned char session_rand[32];
    unsigned char session_rand_before[32];
    unsigned char zero132[132] = { 0 };
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;

    /* A zero-derived scalar is cryptographically invalid but otherwise a
     * reachable return path. Force it through the pluggable hash hook so the
     * wrapper's retry and output-cleanup contracts are exercised deterministically. */
    memcpy(session_rand, session_rand32, sizeof(session_rand));
    if (memcmp(session_rand, secp256k1_fuzz_scalar_zero, sizeof(session_rand)) == 0) {
        memcpy(session_rand, secp256k1_fuzz_scalar_one, sizeof(session_rand));
    }
    memcpy(session_rand_before, session_rand, sizeof(session_rand));
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));

    secp256k1_fuzz_musig_force_zero_sha256_state = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_musig_sha256_compression);
    secp256k1_fuzz_musig_sha256_compression_calls = 0;
    secp256k1_fuzz_musig_force_zero_sha256_state = 1;
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_rand, seckey, pubkey, msg32, keyagg_cache, extra_input32) == 0);
    secp256k1_fuzz_musig_force_zero_sha256_state = 0;
    secp256k1_context_set_sha256_compression(ctx, NULL);

    FUZZ_CHECK(secp256k1_fuzz_musig_sha256_compression_calls != 0);
    FUZZ_CHECK(memcmp(session_rand, session_rand_before, sizeof(session_rand)) == 0);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);
}

static void secp256k1_fuzz_check_musig_nonce_gen_invalid_pubkey_cleanup(secp256k1_context *ctx, const secp256k1_pubkey *valid_pubkey, const unsigned char *valid_seckey, const unsigned char *msg32, const unsigned char *session_rand32) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_pubkey invalid_pubkey = *valid_pubkey;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    unsigned char session_rand[32];
    unsigned char session_rand_before[32];
    unsigned char zero132[132] = { 0 };

    /* This is a public output, so the severity is stale-state propagation,
     * not disclosure of the secret nonce. Still require failed calls not to
     * leave an apparently usable nonce object behind. */
    invalid_pubkey.data[0] ^= 1u;
    memcpy(session_rand, session_rand32, sizeof(session_rand));
    if (memcmp(session_rand, secp256k1_fuzz_scalar_zero, sizeof(session_rand)) == 0) {
        memcpy(session_rand, secp256k1_fuzz_scalar_one, sizeof(session_rand));
    }
    memcpy(session_rand_before, session_rand, sizeof(session_rand));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    FUZZ_CHECK(secp256k1_musig_nonce_gen(ctx, &secnonce, &pubnonce, session_rand, valid_seckey, &invalid_pubkey, msg32, NULL, NULL) == 0);
    FUZZ_CHECK(memcmp(session_rand, session_rand_before, sizeof(session_rand)) == 0);
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

static void secp256k1_fuzz_check_musig_nonce_gen_null_session_random(secp256k1_context *ctx, const unsigned char *seckey, const secp256k1_pubkey *pubkey, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    secp256k1_fuzz_musig_nonce_gen_fn nonce_gen = secp256k1_musig_nonce_gen;
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    unsigned char zero132[132] = { 0 };

    /* session_secrand32 is mandatory. The outputs still need to be invalidated
     * before that precondition is rejected, because callers may reuse buffers. */
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);
    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0x5A, sizeof(pubnonce));
    FUZZ_CHECK(nonce_gen(ctx, &secnonce, &pubnonce, NULL, seckey, pubkey, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&secnonce, zero132, sizeof(secnonce)) == 0);
    FUZZ_CHECK(memcmp(&pubnonce, zero132, sizeof(pubnonce)) == 0);
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

static void secp256k1_fuzz_check_musig_nonce_gen_counter_partial_keypair(secp256k1_context *ctx, const secp256k1_keypair *keypair, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32) {
    secp256k1_fuzz_musig_illegal_data illegal_data;
    secp256k1_keypair invalid_secret_keypair = *keypair;
    secp256k1_keypair invalid_public_keypair = *keypair;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    unsigned char zero132[132] = { 0 };
    unsigned int calls;

    /* Counter-nonce generation consumes a keypair, so neither independently
     * usable opaque half may be allowed to reach nonce derivation. */
    memset(invalid_secret_keypair.data, 0, 32);
    memset(invalid_public_keypair.data + 32, 0, sizeof(invalid_public_keypair.data) - 32);

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_musig_illegal_callback, &illegal_data);

    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, 0, &invalid_secret_keypair, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);

    memset(&secnonce, 0xA5, sizeof(secnonce));
    memset(&pubnonce, 0xA5, sizeof(pubnonce));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, 0, &invalid_public_keypair, msg32, keyagg_cache, extra_input32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(secnonce.data, zero132, sizeof(secnonce.data)) == 0);
    FUZZ_CHECK(memcmp(pubnonce.data, zero132, sizeof(pubnonce.data)) == 0);

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

static void secp256k1_fuzz_check_musig_opaque_nonce_barriers(secp256k1_context *ctx, const secp256k1_keypair *keypair, const unsigned char *msg32, const secp256k1_musig_keyagg_cache *keyagg_cache, const unsigned char *extra_input32, int check_zero_scalar) {
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

    if (check_zero_scalar) {
        /* Overflow rejection above does not reach the scalar-zero clauses in
         * secnonce_load. Exercise each secret scalar independently. */
        invalid_scalar_secnonce = secnonce;
        memset(invalid_scalar_secnonce.data + 4, 0, 32);
        memset(&partial_sig, 0xA5, sizeof(partial_sig));
        calls = illegal_data.calls;
        FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &invalid_scalar_secnonce, keypair, keyagg_cache, &session) == 0);
        FUZZ_CHECK(illegal_data.calls == calls + 1);
        FUZZ_CHECK(memcmp(&partial_sig, zero132, sizeof(partial_sig)) == 0);
        FUZZ_CHECK(memcmp(&invalid_scalar_secnonce, zero132, sizeof(invalid_scalar_secnonce)) == 0);

        invalid_scalar_secnonce = secnonce;
        memset(invalid_scalar_secnonce.data + 36, 0, 32);
        memset(&partial_sig, 0xA5, sizeof(partial_sig));
        calls = illegal_data.calls;
        FUZZ_CHECK(secp256k1_musig_partial_sign(ctx, &partial_sig, &invalid_scalar_secnonce, keypair, keyagg_cache, &session) == 0);
        FUZZ_CHECK(illegal_data.calls == calls + 1);
        FUZZ_CHECK(memcmp(&partial_sig, zero132, sizeof(partial_sig)) == 0);
        FUZZ_CHECK(memcmp(&invalid_scalar_secnonce, zero132, sizeof(invalid_scalar_secnonce)) == 0);
    }

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_MUSIG)
    static const unsigned char zero_scalar_secnonce_trigger[] = "MuSig zero secnonce scalar\n";
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 151);
    unsigned char seckey[8][32];
    unsigned char tweak[32];
    unsigned char agg_xonly32[32];
    unsigned char tweaked_xonly32[32];
    unsigned char nonce66[66];
    unsigned char zero66[66] = { 0 };
    unsigned char valid_aggnonce66[66];
    unsigned char fixed_pubnonce66[66];
    unsigned char invalid_aggnonce66[66];
    unsigned char mixed_aggnonce66[66];
    unsigned char reversed_mixed_aggnonce66[66];
    unsigned char mixed_pubnonce66[66];
    unsigned char sig32[32];
    unsigned char ones32[32];
    unsigned char session_rand[32];
    secp256k1_pubkey pubkeys[8];
    secp256k1_pubkey fixed_pubkeys[2];
    secp256k1_keypair keypairs[8];
    secp256k1_musig_pubnonce fixed_pubnonce;
    const secp256k1_pubkey *pubkey_ptrs[8];
    secp256k1_pubkey agg_full;
    secp256k1_pubkey cache_full;
    secp256k1_pubkey expected_tweaked_full;
    secp256k1_pubkey musig_tweaked_full;
    secp256k1_pubkey one_expected_full;
    secp256k1_pubkey one_tweaked_full;
    secp256k1_pubkey zero_tweaked_full;
    secp256k1_xonly_pubkey agg_xonly;
    secp256k1_xonly_pubkey single_agg_xonly;
    secp256k1_xonly_pubkey agg_xonly_no_cache;
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

    if (secp256k1_fuzz_byte(input, size, 157) == (unsigned char)'8') {
        n_pubkeys = 8;
    } else {
        n_pubkeys = (size_t)((secp256k1_fuzz_byte(input, size, 157) % 7u) + 1u);
    }
    for (i = 0; i < 8; i++) {
        secp256k1_fuzz_valid_seckey32(ctx, seckey[i], input, size, 163u + (unsigned int)i);
        FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypairs[i], seckey[i]) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkeys[i], seckey[i]) == 1);
        pubkey_ptrs[i] = &pubkeys[i];
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &fixed_pubkeys[0], secp256k1_fuzz_scalar_one) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &fixed_pubkeys[1], secp256k1_fuzz_scalar_order_minus_one) == 1);
    if (size == sizeof("musig-tweak-infinity-rollback\n") - 1
        && memcmp(input, "musig-tweak-infinity-rollback\n", sizeof("musig-tweak-infinity-rollback\n") - 1) == 0) {
        secp256k1_fuzz_check_musig_tweak_infinity_rollback(ctx, &fixed_pubkeys[0]);
    }
    secp256k1_fuzz_check_musig_noncanonical_duplicate(ctx);
    secp256k1_fuzz_check_musig_keyagg_reference(ctx);
    secp256k1_fuzz_check_musig_single_keyagg_reference(ctx);
    secp256k1_fuzz_check_musig_three_keyagg_reference(ctx);
    secp256k1_fuzz_check_musig_four_keyagg_reference(ctx);
    if (secp256k1_fuzz_byte(input, size, 157) == (unsigned char)'8') {
        secp256k1_fuzz_check_musig_eight_keyagg_reference(ctx);
    }
    secp256k1_fuzz_check_musig_sixteen_keyagg_reference(ctx, input, size);
    secp256k1_fuzz_check_musig_duplicate_keyagg_reference(ctx, input, size);
    secp256k1_fuzz_check_musig_sixteen_sign_roundtrip(ctx, input, size);
    secp256k1_fuzz_check_musig_tweaked_signing(ctx);
    secp256k1_fuzz_check_musig_xonly_tweaked_signing(ctx);
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
    FUZZ_CHECK(secp256k1_musig_pubnonce_parse(ctx, &fixed_pubnonce, fixed_pubnonce66) == 1);
    memcpy(mixed_aggnonce66, zero66, sizeof(mixed_aggnonce66));
    memcpy(mixed_aggnonce66 + 33, valid_aggnonce66 + 33, 33);
    memcpy(reversed_mixed_aggnonce66, zero66, sizeof(reversed_mixed_aggnonce66));
    memcpy(reversed_mixed_aggnonce66, valid_aggnonce66, 33);
    memcpy(invalid_aggnonce66, valid_aggnonce66, sizeof(invalid_aggnonce66));
    invalid_aggnonce66[0] = 0x05;
    memcpy(mixed_pubnonce66, valid_aggnonce66, 33);
    memset(mixed_pubnonce66 + 33, 0, 33);
    memset(ones32, 0xFF, sizeof(ones32));

    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_xonly, &cache, pubkey_ptrs, n_pubkeys) == 1);
    secp256k1_fuzz_check_musig_pubkey_agg_success(ctx, &agg_xonly, &cache);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &single_agg_xonly, &single_cache, pubkey_ptrs, 1) == 1);
    secp256k1_fuzz_check_musig_pubkey_agg_success(ctx, &single_agg_xonly, &single_cache);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_xonly_no_cache, NULL, pubkey_ptrs, n_pubkeys) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &agg_xonly_no_cache, &agg_xonly) == 0);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, NULL, &cache_no_output, pubkey_ptrs, n_pubkeys) == 1);
    FUZZ_CHECK(secp256k1_musig_pubkey_agg(ctx, NULL, NULL, pubkey_ptrs, n_pubkeys) == 1);
    secp256k1_fuzz_check_musig_pubkey_agg_success(ctx, NULL, &cache_no_output);
    secp256k1_fuzz_check_musig_pubkey_agg_failure_cleanup(ctx, &agg_xonly, &cache);
    secp256k1_fuzz_check_musig_empty_aggregation(ctx, &pubkeys[0], &fixed_pubnonce);
    secp256k1_fuzz_check_musig_keyagg_cache_curve_barrier(ctx, &pubkeys[0], &cache);
    secp256k1_fuzz_check_musig_keyagg_cache_semantic_barrier(ctx, &cache);
    secp256k1_fuzz_check_musig_keyagg_hash_routing(ctx, pubkey_ptrs, n_pubkeys, &agg_xonly, &cache);
    if (size == sizeof("partial keypair nonce counter invalid\n") - 1
        && memcmp(input, "partial keypair nonce counter invalid\n", sizeof("partial keypair nonce counter invalid\n") - 1) == 0) {
        secp256k1_fuzz_check_musig_nonce_gen_counter_partial_keypair(ctx, &keypairs[0], tweak, &cache, session_rand);
    }
    secp256k1_fuzz_check_musig_keypair_consistency(ctx, &keypairs[0], &pubkeys[1], tweak, &cache, session_rand);
    secp256k1_fuzz_check_musig_noncanonical_nonce_storage(ctx, tweak, &cache);
    secp256k1_fuzz_check_musig_opaque_nonce_barriers(ctx, &keypairs[0], tweak, &cache, session_rand,
        size == sizeof(zero_scalar_secnonce_trigger) - 1
        && memcmp(input, zero_scalar_secnonce_trigger, sizeof(zero_scalar_secnonce_trigger) - 1) == 0);
    secp256k1_fuzz_check_musig_nonce_gen_counter(ctx, input, size, seckey[0], &keypairs[0], &pubkeys[0], tweak, &cache, session_rand);
    secp256k1_fuzz_check_musig_nonce_gen_counter_optional_inputs(ctx, seckey[0], &keypairs[0], &pubkeys[0], tweak, &cache, session_rand);
    secp256k1_fuzz_check_musig_nonce_gen_optional_seckey(ctx, input, size, seckey[0], &pubkeys[0], tweak, &cache, session_rand);
    secp256k1_fuzz_check_musig_nonce_gen_counter_failure_cleanup(ctx, &keypairs[0], tweak, &cache, session_rand);
    if (size == sizeof("MuSig nonce null session random\n") - 1
        && memcmp(input, "MuSig nonce null session random\n", sizeof("MuSig nonce null session random\n") - 1) == 0) {
        secp256k1_fuzz_check_musig_nonce_gen_null_session_random(ctx, seckey[0], &pubkeys[0], tweak, &cache, tweak);
    }
    secp256k1_fuzz_check_musig_zero_counter_sign(ctx, &keypairs[0], &pubkeys[0], tweak, &single_cache, &single_agg_xonly);
    secp256k1_fuzz_check_musig_nonce_scalar_barrier(ctx);
    secp256k1_fuzz_check_musig_partial_sign_nonce_parity(ctx);
    secp256k1_fuzz_check_musig_infinity_nonce_process(ctx, &keypairs[0], &pubkeys[0], tweak, &single_cache, &single_agg_xonly);
    secp256k1_fuzz_check_musig_sign_roundtrip(ctx, input, size, seckey, keypairs, pubkeys, n_pubkeys, tweak);
    secp256k1_fuzz_check_musig_nonce_gen_failure_cleanup(ctx, &pubkeys[0], seckey[0], tweak, &cache, tweak, session_rand);
    secp256k1_fuzz_check_musig_nonce_gen_zero_scalar_failure(ctx, &pubkeys[0], seckey[0], tweak, &cache, tweak, session_rand);
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
    secp256k1_fuzz_check_musig_nonce_agg_long(ctx, input, size, valid_aggnonce66);
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
    secp256k1_fuzz_check_musig_effective_nonce(ctx, mixed_aggnonce66, tweak, &cache);
    secp256k1_fuzz_check_musig_effective_nonce(ctx, reversed_mixed_aggnonce66, tweak, &cache);
    secp256k1_fuzz_check_musig_effective_nonce(ctx, zero66, tweak, &cache);
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
