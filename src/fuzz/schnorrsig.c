/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"

#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_SCHNORRSIG)
static size_t secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls = 0;

static const uint32_t secp256k1_fuzz_schnorrsig_nonce_midstate[8] = {
    0x46615b35ul, 0xf4bfbff7ul, 0x9f8dc671ul, 0x83627ab3ul,
    0x60217180ul, 0x57358661ul, 0x21a29e54ul, 0x68b07b4cul
};
static const uint32_t secp256k1_fuzz_schnorrsig_aux_midstate[8] = {
    0x24dd3219ul, 0x4eba7e70ul, 0xca0fabb9ul, 0x0fa3166dul,
    0x3afbe4b1ul, 0x4c44df97ul, 0x4aac2739ul, 0x249e850aul
};
static const uint32_t secp256k1_fuzz_schnorrsig_challenge_midstate[8] = {
    0x9cecba11ul, 0x23925381ul, 0x11679112ul, 0xd1627e0ful,
    0x97c87550ul, 0x003cc765ul, 0x90f61164ul, 0x33e9b66aul
};

static void secp256k1_fuzz_schnorrsig_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    if (memcmp(state, secp256k1_fuzz_schnorrsig_nonce_midstate, sizeof(secp256k1_fuzz_schnorrsig_nonce_midstate)) == 0 ||
        memcmp(state, secp256k1_fuzz_schnorrsig_aux_midstate, sizeof(secp256k1_fuzz_schnorrsig_aux_midstate)) == 0) {
        secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls++;
    }
    if (memcmp(state, secp256k1_fuzz_schnorrsig_challenge_midstate, sizeof(secp256k1_fuzz_schnorrsig_challenge_midstate)) == 0) {
        secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls++;
    }
    secp256k1_sha256_transform(state, blocks64, n_blocks);
}

typedef struct {
    const void *self;
    const unsigned char *aux32;
    int calls;
} secp256k1_fuzz_schnorrsig_nonce_data;

typedef struct {
    const void *self;
    const unsigned char *nonce32;
    int calls;
} secp256k1_fuzz_schnorrsig_fixed_nonce_data;

static int secp256k1_fuzz_schnorrsig_nonce(unsigned char *nonce32, const unsigned char *msg, size_t msglen, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo, size_t algolen, void *data) {
    static const unsigned char expected_algo[13] = {'B', 'I', 'P', '0', '3', '4', '0', '/', 'n', 'o', 'n', 'c', 'e'};
    secp256k1_fuzz_schnorrsig_nonce_data *nonce_data = (secp256k1_fuzz_schnorrsig_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg != NULL || msglen == 0);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(xonly_pk32 != NULL);
    FUZZ_CHECK(algo != NULL);
    FUZZ_CHECK(algolen == sizeof(expected_algo));
    FUZZ_CHECK(memcmp(algo, expected_algo, sizeof(expected_algo)) == 0);
    nonce_data->calls++;
    return secp256k1_nonce_function_bip340(nonce32, msg, msglen, key32, xonly_pk32, algo, algolen, (void *)nonce_data->aux32);
}

static int secp256k1_fuzz_schnorrsig_nonce_fail(unsigned char *nonce32, const unsigned char *msg, size_t msglen, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo, size_t algolen, void *data) {
    static const unsigned char expected_algo[13] = {'B', 'I', 'P', '0', '3', '4', '0', '/', 'n', 'o', 'n', 'c', 'e'};
    secp256k1_fuzz_schnorrsig_nonce_data *nonce_data = (secp256k1_fuzz_schnorrsig_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg != NULL || msglen == 0);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(xonly_pk32 != NULL);
    FUZZ_CHECK(algo != NULL);
    FUZZ_CHECK(algolen == sizeof(expected_algo));
    FUZZ_CHECK(memcmp(algo, expected_algo, sizeof(expected_algo)) == 0);
    nonce_data->calls++;
    memset(nonce32, 0xA5, 32);
    return 0;
}

static int secp256k1_fuzz_schnorrsig_nonce_zero(unsigned char *nonce32, const unsigned char *msg, size_t msglen, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo, size_t algolen, void *data) {
    static const unsigned char expected_algo[13] = {'B', 'I', 'P', '0', '3', '4', '0', '/', 'n', 'o', 'n', 'c', 'e'};
    secp256k1_fuzz_schnorrsig_nonce_data *nonce_data = (secp256k1_fuzz_schnorrsig_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg != NULL || msglen == 0);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(xonly_pk32 != NULL);
    FUZZ_CHECK(algo != NULL);
    FUZZ_CHECK(algolen == sizeof(expected_algo));
    FUZZ_CHECK(memcmp(algo, expected_algo, sizeof(expected_algo)) == 0);
    nonce_data->calls++;
    memset(nonce32, 0, 32);
    return 1;
}

static int secp256k1_fuzz_schnorrsig_nonce_fixed(unsigned char *nonce32, const unsigned char *msg, size_t msglen, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo, size_t algolen, void *data) {
    static const unsigned char expected_algo[13] = {'B', 'I', 'P', '0', '3', '4', '0', '/', 'n', 'o', 'n', 'c', 'e'};
    secp256k1_fuzz_schnorrsig_fixed_nonce_data *nonce_data = (secp256k1_fuzz_schnorrsig_fixed_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(nonce_data->nonce32 != NULL);
    FUZZ_CHECK(msg != NULL || msglen == 0);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(xonly_pk32 != NULL);
    FUZZ_CHECK(algo != NULL);
    FUZZ_CHECK(algolen == sizeof(expected_algo));
    FUZZ_CHECK(memcmp(algo, expected_algo, sizeof(expected_algo)) == 0);
    nonce_data->calls++;
    memcpy(nonce32, nonce_data->nonce32, 32);
    return 1;
}

static void secp256k1_fuzz_check_schnorrsig_sign_failure_cleanup(const secp256k1_context *ctx, const unsigned char *msg, size_t msglen, const secp256k1_keypair *keypair, const unsigned char *aux32) {
    secp256k1_fuzz_schnorrsig_nonce_data nonce_data;
    secp256k1_schnorrsig_extraparams extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    unsigned char sig64[64];
    unsigned char zero64[64] = { 0 };

    nonce_data.self = &nonce_data;
    nonce_data.aux32 = aux32;
    nonce_data.calls = 0;
    extraparams.noncefp = secp256k1_fuzz_schnorrsig_nonce_fail;
    extraparams.ndata = &nonce_data;
    memset(sig64, 0xA5, sizeof(sig64));
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64, msg, msglen, keypair, &extraparams) == 0);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    nonce_data.calls = 0;
    extraparams.noncefp = secp256k1_fuzz_schnorrsig_nonce_zero;
    memset(sig64, 0xA5, sizeof(sig64));
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64, msg, msglen, keypair, &extraparams) == 0);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);
}

static void secp256k1_fuzz_check_schnorrsig_nonce_overflow(const secp256k1_context *ctx, const unsigned char *msg, size_t msglen, const secp256k1_keypair *keypair, const secp256k1_xonly_pubkey *xonly) {
    static const unsigned char overflow_nonce32[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static const unsigned char reduced_nonce32[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x45, 0x51, 0x23, 0x19, 0x50, 0xB7, 0x5F, 0xC4,
        0x40, 0x2D, 0xA1, 0x73, 0x2F, 0xC9, 0xBE, 0xBE
    };
    secp256k1_fuzz_schnorrsig_fixed_nonce_data nonce_data;
    secp256k1_schnorrsig_extraparams extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    unsigned char sig64_overflow[64];
    unsigned char sig64_reduced[64];

    extraparams.noncefp = secp256k1_fuzz_schnorrsig_nonce_fixed;
    extraparams.ndata = &nonce_data;
    nonce_data.self = &nonce_data;

    nonce_data.nonce32 = overflow_nonce32;
    nonce_data.calls = 0;
    memset(sig64_overflow, 0xA5, sizeof(sig64_overflow));
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_overflow, msg, msglen, keypair, &extraparams) == 1);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_overflow, msg, msglen, xonly) == 1);

    nonce_data.nonce32 = reduced_nonce32;
    nonce_data.calls = 0;
    memset(sig64_reduced, 0x5A, sizeof(sig64_reduced));
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_reduced, msg, msglen, keypair, &extraparams) == 1);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_reduced, msg, msglen, xonly) == 1);
    FUZZ_CHECK(memcmp(sig64_overflow, sig64_reduced, sizeof(sig64_overflow)) == 0);
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_SCHNORRSIG)
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 121);
    secp256k1_fuzz_schnorrsig_nonce_data nonce_data;
    unsigned char seckey[32];
    unsigned char aux32[32];
    unsigned char msg32[32];
    unsigned char sig64[64];
    unsigned char sig64_custom[64];
    unsigned char sig64_checked[64];
    unsigned char sig64_explicit_bip340[64];
    unsigned char sig64_null_aux[64];
    unsigned char sig64_zero_aux[64];
    unsigned char sig64_zero_aux_custom[64];
    unsigned char sig64_bad[64];
    unsigned char other_seckey[32];
    unsigned char negated_seckey[32];
    unsigned char msg32_bad[32];
    unsigned char zero_aux32[32] = { 0 };
    secp256k1_keypair keypair;
    secp256k1_keypair other_keypair;
    secp256k1_keypair negated_keypair;
    secp256k1_xonly_pubkey xonly;
    secp256k1_xonly_pubkey other_xonly;
    secp256k1_xonly_pubkey negated_xonly;
    secp256k1_schnorrsig_extraparams extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    secp256k1_schnorrsig_extraparams null_extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    secp256k1_schnorrsig_extraparams zero_aux_extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    secp256k1_schnorrsig_extraparams checked_extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    secp256k1_schnorrsig_extraparams explicit_bip340_extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    size_t msglen;
    size_t wrong_msglen;
    size_t flip_index;
    unsigned char flip_mask;
    unsigned char sig64_negated[64];
    int parity;
    int negated_parity;

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 127);
    secp256k1_fuzz_derive(aux32, sizeof(aux32), input, size, 131);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 137);
    msglen = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 139) % (size + 1));
    nonce_data.self = &nonce_data;
    nonce_data.aux32 = aux32;
    nonce_data.calls = 0;
    checked_extraparams.noncefp = secp256k1_fuzz_schnorrsig_nonce;
    checked_extraparams.ndata = &nonce_data;

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly, &parity, &keypair) == 1);
    memcpy(negated_seckey, seckey, sizeof(negated_seckey));
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, negated_seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &negated_keypair, negated_seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &negated_xonly, &negated_parity, &negated_keypair) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &negated_xonly) == 0);
    FUZZ_CHECK(parity != negated_parity);
    secp256k1_fuzz_valid_seckey32(ctx, other_seckey, input, size, 149);
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &other_keypair, other_seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &other_xonly, NULL, &other_keypair) == 1);
    secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_schnorrsig_sha256_compression);
    secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls = 0;
    secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sig64, msg32, &keypair, aux32) == 1);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sig64_negated, msg32, &negated_keypair, aux32) == 1);
    FUZZ_CHECK(memcmp(sig64, sig64_negated, sizeof(sig64)) == 0);
    secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), &xonly) == 1);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_checked, msg32, sizeof(msg32), &keypair, &checked_extraparams) == 1);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(memcmp(sig64_checked, sig64, sizeof(sig64_checked)) == 0);
    explicit_bip340_extraparams.noncefp = secp256k1_nonce_function_bip340;
    explicit_bip340_extraparams.ndata = aux32;
    secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls = 0;
    secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_explicit_bip340, msg32, sizeof(msg32), &keypair, &explicit_bip340_extraparams) == 1);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls != 0);
    FUZZ_CHECK(memcmp(sig64_explicit_bip340, sig64, sizeof(sig64_explicit_bip340)) == 0);
    secp256k1_context_set_sha256_compression(ctx, NULL);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32) - 1, &xonly) == 0);
    if (secp256k1_xonly_pubkey_cmp(ctx, &xonly, &other_xonly) != 0) {
        FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), &other_xonly) == 0);
    }

    memcpy(sig64_bad, sig64, sizeof(sig64_bad));
    memset(sig64_bad + 32, 0xFF, 32);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_bad, msg32, sizeof(msg32), &xonly) == 0);

    flip_index = (size_t)(secp256k1_fuzz_byte(input, size, 141) & 63u);
    flip_mask = (unsigned char)(secp256k1_fuzz_byte(input, size, 143) | 1u);
    memcpy(sig64_bad, sig64, sizeof(sig64_bad));
    sig64_bad[flip_index] ^= flip_mask;
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_bad, msg32, sizeof(msg32), &xonly) == 0);

    memcpy(msg32_bad, msg32, sizeof(msg32_bad));
    msg32_bad[flip_index & 31u] ^= flip_mask;
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32_bad, sizeof(msg32_bad), &xonly) == 0);

    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sig64_null_aux, msg32, &keypair, NULL) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sig64_zero_aux, msg32, &keypair, zero_aux32) == 1);
    FUZZ_CHECK(memcmp(sig64_null_aux, sig64_zero_aux, sizeof(sig64_null_aux)) == 0);
    zero_aux_extraparams.ndata = zero_aux32;
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_zero_aux_custom, msg32, sizeof(msg32), &keypair, &zero_aux_extraparams) == 1);
    FUZZ_CHECK(memcmp(sig64_zero_aux, sig64_zero_aux_custom, sizeof(sig64_zero_aux)) == 0);
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_custom, msg32, sizeof(msg32), &keypair, &null_extraparams) == 1);
    FUZZ_CHECK(memcmp(sig64_null_aux, sig64_custom, sizeof(sig64_null_aux)) == 0);

    extraparams.ndata = aux32;
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_custom, input, msglen, &keypair, &extraparams) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_custom, input, msglen, &xonly) == 1);
    secp256k1_fuzz_check_schnorrsig_nonce_overflow(ctx, input, msglen, &keypair, &xonly);
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_explicit_bip340, input, msglen, &keypair, &explicit_bip340_extraparams) == 1);
    FUZZ_CHECK(memcmp(sig64_explicit_bip340, sig64_custom, sizeof(sig64_explicit_bip340)) == 0);
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_checked, input, msglen, &keypair, &checked_extraparams) == 1);
    FUZZ_CHECK(nonce_data.calls == 2);
    FUZZ_CHECK(memcmp(sig64_checked, sig64_custom, sizeof(sig64_checked)) == 0);
    secp256k1_fuzz_check_schnorrsig_sign_failure_cleanup(ctx, input, msglen, &keypair, aux32);
    wrong_msglen = msglen == 0 ? 1 : msglen - 1;
    FUZZ_CHECK(wrong_msglen != msglen);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_custom, input, wrong_msglen, &xonly) == 0);
    if (secp256k1_xonly_pubkey_cmp(ctx, &xonly, &other_xonly) != 0) {
        FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_custom, input, msglen, &other_xonly) == 0);
    }

    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_custom, msg32, sizeof(msg32), &keypair, &extraparams) == 1);
    FUZZ_CHECK(memcmp(sig64, sig64_custom, sizeof(sig64)) == 0);

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
