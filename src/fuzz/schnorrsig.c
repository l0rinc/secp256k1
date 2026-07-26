/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "sha256_reference.h"
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
    const unsigned char *expected_key32;
    const unsigned char *expected_xonly_pk32;
    const unsigned char *expected_msg;
    size_t expected_msglen;
    int calls;
} secp256k1_fuzz_schnorrsig_nonce_data;

typedef struct {
    const void *self;
    const unsigned char *nonce32;
    int calls;
} secp256k1_fuzz_schnorrsig_fixed_nonce_data;

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_schnorrsig_illegal_data;

typedef int (*secp256k1_fuzz_schnorrsig_sign_custom_fn)(const secp256k1_context *ctx, unsigned char *sig64, const unsigned char *msg, size_t msglen, const secp256k1_keypair *keypair, secp256k1_schnorrsig_extraparams *extraparams);

static void secp256k1_fuzz_schnorrsig_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_schnorrsig_illegal_data *illegal_data = (secp256k1_fuzz_schnorrsig_illegal_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(illegal_data != NULL);
    FUZZ_CHECK(illegal_data->self == illegal_data);
    illegal_data->calls++;
}

static int secp256k1_fuzz_schnorrsig_nonce(unsigned char *nonce32, const unsigned char *msg, size_t msglen, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo, size_t algolen, void *data) {
    static const unsigned char expected_algo[13] = {'B', 'I', 'P', '0', '3', '4', '0', '/', 'n', 'o', 'n', 'c', 'e'};
    secp256k1_fuzz_schnorrsig_nonce_data *nonce_data = (secp256k1_fuzz_schnorrsig_nonce_data *)data;

    FUZZ_CHECK(nonce_data != NULL);
    FUZZ_CHECK(nonce_data->self == nonce_data);
    FUZZ_CHECK(msg != NULL || msglen == 0);
    FUZZ_CHECK(key32 != NULL);
    FUZZ_CHECK(xonly_pk32 != NULL);
    if (nonce_data->expected_key32 != NULL) {
        FUZZ_CHECK(memcmp(key32, nonce_data->expected_key32, 32) == 0);
    }
    if (nonce_data->expected_xonly_pk32 != NULL) {
        FUZZ_CHECK(memcmp(xonly_pk32, nonce_data->expected_xonly_pk32, 32) == 0);
    }
    if (nonce_data->expected_msg != NULL) {
        FUZZ_CHECK(msglen == nonce_data->expected_msglen);
        FUZZ_CHECK(memcmp(msg, nonce_data->expected_msg, msglen) == 0);
    }
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
    if (nonce_data->expected_key32 != NULL) {
        FUZZ_CHECK(memcmp(key32, nonce_data->expected_key32, 32) == 0);
    }
    if (nonce_data->expected_xonly_pk32 != NULL) {
        FUZZ_CHECK(memcmp(xonly_pk32, nonce_data->expected_xonly_pk32, 32) == 0);
    }
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
    if (nonce_data->expected_key32 != NULL) {
        FUZZ_CHECK(memcmp(key32, nonce_data->expected_key32, 32) == 0);
    }
    if (nonce_data->expected_xonly_pk32 != NULL) {
        FUZZ_CHECK(memcmp(xonly_pk32, nonce_data->expected_xonly_pk32, 32) == 0);
    }
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

static void secp256k1_fuzz_schnorrsig_tagged_hash_reference(unsigned char out32[32], const unsigned char *tag, size_t taglen, const unsigned char *part1, size_t part1_len, const unsigned char *part2, size_t part2_len, const unsigned char *part3, size_t part3_len) {
    unsigned char taghash[32];
    unsigned char *transcript;
    size_t transcript_len = 2 * sizeof(taghash);
    size_t offset;

    FUZZ_CHECK(part1_len <= SIZE_MAX - transcript_len);
    transcript_len += part1_len;
    FUZZ_CHECK(part2_len <= SIZE_MAX - transcript_len);
    transcript_len += part2_len;
    FUZZ_CHECK(part3_len <= SIZE_MAX - transcript_len);
    transcript_len += part3_len;
    transcript = (unsigned char *)malloc(transcript_len);
    FUZZ_CHECK(transcript != NULL);

    secp256k1_fuzz_sha256_standalone(taghash, tag, taglen);
    memcpy(transcript, taghash, sizeof(taghash));
    memcpy(transcript + sizeof(taghash), taghash, sizeof(taghash));
    offset = 2 * sizeof(taghash);
    if (part1_len != 0) {
        memcpy(transcript + offset, part1, part1_len);
        offset += part1_len;
    }
    if (part2_len != 0) {
        memcpy(transcript + offset, part2, part2_len);
        offset += part2_len;
    }
    if (part3_len != 0) {
        memcpy(transcript + offset, part3, part3_len);
    }
    secp256k1_fuzz_sha256_standalone(out32, transcript, transcript_len);
    memset(taghash, 0, sizeof(taghash));
    memset(transcript, 0, transcript_len);
    free(transcript);
}

static void secp256k1_fuzz_check_schnorrsig_nonce_reference(const unsigned char *msg, size_t msglen, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *aux32) {
    static const unsigned char aux_tag[] = "BIP0340/aux";
    static const unsigned char nonce_tag[] = "BIP0340/nonce";
    unsigned char zero32[32] = { 0 };
    unsigned char masked_key[32];
    unsigned char aux_hash[32];
    unsigned char expected_nonce[32];
    unsigned char actual_nonce[32];
    const unsigned char *aux_input = aux32 == NULL ? zero32 : aux32;
    size_t i;

    secp256k1_fuzz_schnorrsig_tagged_hash_reference(aux_hash, aux_tag, sizeof(aux_tag) - 1, aux_input, sizeof(zero32), NULL, 0, NULL, 0);
    for (i = 0; i < sizeof(masked_key); i++) {
        masked_key[i] = (unsigned char)(aux_hash[i] ^ key32[i]);
    }
    secp256k1_fuzz_schnorrsig_tagged_hash_reference(expected_nonce, nonce_tag, sizeof(nonce_tag) - 1, masked_key, sizeof(masked_key), xonly_pk32, 32, msg, msglen);
    FUZZ_CHECK(secp256k1_nonce_function_bip340(actual_nonce, msg, msglen, key32, xonly_pk32, nonce_tag, sizeof(nonce_tag) - 1, (void *)aux32) == 1);
    FUZZ_CHECK(memcmp(actual_nonce, expected_nonce, sizeof(actual_nonce)) == 0);
}

static void secp256k1_fuzz_check_schnorrsig_custom_nonce_tag(const unsigned char *msg, size_t msglen, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *algo, size_t algolen, const unsigned char *aux32) {
    static const unsigned char aux_tag[] = "BIP0340/aux";
    unsigned char zero32[32] = { 0 };
    unsigned char masked_key[32];
    unsigned char aux_hash[32];
    unsigned char expected_nonce[32];
    unsigned char actual_nonce[32];
    const unsigned char *aux_input = aux32 == NULL ? zero32 : aux32;
    size_t i;

    /* The public nonce callback accepts arbitrary algorithm tags. Keep this
     * transcript independent from the optimized BIP0340/nonce branch. */
    secp256k1_fuzz_schnorrsig_tagged_hash_reference(aux_hash, aux_tag, sizeof(aux_tag) - 1, aux_input, sizeof(zero32), NULL, 0, NULL, 0);
    for (i = 0; i < sizeof(masked_key); i++) {
        masked_key[i] = (unsigned char)(aux_hash[i] ^ key32[i]);
    }
    secp256k1_fuzz_schnorrsig_tagged_hash_reference(expected_nonce, algo, algolen, masked_key, sizeof(masked_key), xonly_pk32, 32, msg, msglen);
    FUZZ_CHECK(secp256k1_nonce_function_bip340(actual_nonce, msg, msglen, key32, xonly_pk32, algo, algolen, (void *)aux32) == 1);
    FUZZ_CHECK(memcmp(actual_nonce, expected_nonce, sizeof(actual_nonce)) == 0);
}

static void secp256k1_fuzz_check_schnorrsig_custom_nonce_tag_boundaries(const unsigned char *input, size_t size, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *aux32) {
    static const unsigned char trigger[] = "schnorrsig-custom-nonce-tag-boundaries\n";
    static const unsigned char boundary_algo[16] = {
        0x42, 0x49, 0x50, 0x30, 0x33, 0x34, 0x30, 0x2f,
        0x6e, 0x6f, 0x6e, 0x63, 0x65, 0x42, 0x59, 0x54
    };
    size_t algolen;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    /* The optimized BIP-340 tag is selected only at length 13. Exercise both
     * sides of that dispatch and the short-tag lengths accepted by the API. */
    for (algolen = 0; algolen <= sizeof(boundary_algo); algolen++) {
        secp256k1_fuzz_check_schnorrsig_custom_nonce_tag(input, size, key32, xonly_pk32, boundary_algo, algolen, aux32);
        secp256k1_fuzz_check_schnorrsig_custom_nonce_tag(input, size, key32, xonly_pk32, boundary_algo, algolen, NULL);
    }
}

static void secp256k1_fuzz_check_schnorrsig_sign_failure_cleanup(const secp256k1_context *ctx, const unsigned char *msg, size_t msglen, const secp256k1_keypair *keypair, const unsigned char *aux32) {
    secp256k1_fuzz_schnorrsig_nonce_data nonce_data;
    secp256k1_schnorrsig_extraparams extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    unsigned char sig64[64];
    unsigned char zero64[64] = { 0 };

    nonce_data.self = &nonce_data;
    nonce_data.aux32 = aux32;
    nonce_data.expected_key32 = NULL;
    nonce_data.expected_xonly_pk32 = NULL;
    nonce_data.expected_msg = NULL;
    nonce_data.expected_msglen = 0;
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

static void secp256k1_fuzz_check_schnorrsig_sign_precondition_cleanup(secp256k1_context *ctx, const unsigned char *valid_msg32, const secp256k1_keypair *valid_keypair, const unsigned char *aux32) {
    secp256k1_fuzz_schnorrsig_illegal_data illegal_data;
    secp256k1_fuzz_schnorrsig_nonce_data nonce_data;
    secp256k1_schnorrsig_extraparams extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    secp256k1_fuzz_schnorrsig_sign_custom_fn sign_custom = secp256k1_schnorrsig_sign_custom;
    const secp256k1_keypair *null_keypair;
    unsigned char sig64[64];
    unsigned char zero64[64] = { 0 };
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    nonce_data.self = &nonce_data;
    nonce_data.aux32 = aux32;
    nonce_data.expected_key32 = NULL;
    nonce_data.expected_xonly_pk32 = NULL;
    nonce_data.expected_msg = NULL;
    nonce_data.expected_msglen = 0;
    nonce_data.calls = 0;
    extraparams.noncefp = secp256k1_fuzz_schnorrsig_nonce_fail;
    extraparams.ndata = &nonce_data;
    null_keypair = NULL;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_schnorrsig_illegal_callback, &illegal_data);

    memset(sig64, 0xA5, sizeof(sig64));
    calls = illegal_data.calls;
    nonce_data.calls = 0;
    FUZZ_CHECK(sign_custom(ctx, sig64, NULL, 1, valid_keypair, &extraparams) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(nonce_data.calls == 0);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    memset(sig64, 0xA5, sizeof(sig64));
    calls = illegal_data.calls;
    nonce_data.calls = 0;
    FUZZ_CHECK(sign_custom(ctx, sig64, valid_msg32, 32, null_keypair, &extraparams) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(nonce_data.calls == 0);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_schnorrsig_deprecated_sign(const secp256k1_context *ctx, const secp256k1_keypair *keypair) {
    static const unsigned char msg32[32] = { 0 };
    static const unsigned char aux32[32] = {
        0x42, 0x17, 0xA5, 0x6C, 0x39, 0xD0, 0x8E, 0xF1,
        0x24, 0xB7, 0x5D, 0x03, 0x99, 0xCE, 0x71, 0x48,
        0xDA, 0x2F, 0x86, 0x10, 0x5A, 0xE4, 0xBC, 0x67,
        0x13, 0xF8, 0x40, 0x9B, 0x2D, 0x75, 0xC6, 0xAE
    };
    unsigned char sign32_sig64[64];
    unsigned char deprecated_sig64[64];

    /* Keep the deprecated entry point tied to sign32, including aux_rand32. */
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sign32_sig64, msg32, keypair, aux32) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_sign(ctx, deprecated_sig64, msg32, keypair, aux32) == 1);
    FUZZ_CHECK(memcmp(sign32_sig64, deprecated_sig64, sizeof(sign32_sig64)) == 0);
}

static void secp256k1_fuzz_check_schnorrsig_empty_message(const secp256k1_context *ctx, const secp256k1_keypair *keypair, const secp256k1_xonly_pubkey *xonly) {
    static const unsigned char empty_message = 0;
    unsigned char null_message_sig[64];
    unsigned char nonnull_message_sig[64];

    /* The API deliberately permits NULL only for a zero-length message. The
     * two legal representations must describe the same signing transcript. */
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, null_message_sig, NULL, 0, keypair, NULL) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, nonnull_message_sig, &empty_message, 0, keypair, NULL) == 1);
    FUZZ_CHECK(memcmp(null_message_sig, nonnull_message_sig, sizeof(null_message_sig)) == 0);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, null_message_sig, NULL, 0, xonly) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, nonnull_message_sig, &empty_message, 0, xonly) == 1);
}

static void secp256k1_fuzz_check_schnorrsig_extraparams_magic(secp256k1_context *ctx, const unsigned char *msg32, const secp256k1_keypair *keypair) {
    static const unsigned char expected_magic[4] = SECP256K1_SCHNORRSIG_EXTRAPARAMS_MAGIC;
    secp256k1_fuzz_schnorrsig_illegal_data illegal_data;
    secp256k1_schnorrsig_extraparams extraparams;
    unsigned char sig64[64];
    unsigned char zero64[64] = { 0 };
    unsigned int calls;
    size_t i;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_schnorrsig_illegal_callback, &illegal_data);
    for (i = 0; i < sizeof(extraparams.magic); i++) {
        memset(&extraparams, 0, sizeof(extraparams));
        memcpy(extraparams.magic, expected_magic, sizeof(extraparams.magic));
        extraparams.magic[i] ^= 1u;
        memset(sig64, 0xA5, sizeof(sig64));
        calls = illegal_data.calls;
        FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64, msg32, 32, keypair, &extraparams) == 0);
        FUZZ_CHECK(illegal_data.calls == calls + 1);
        FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);
    }
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_bip340_nonce_failure_cleanup(const unsigned char *msg, size_t msglen, const unsigned char *key32, const unsigned char *xonly_pk32, const unsigned char *aux32) {
    static const unsigned char expected_algo[13] = {'B', 'I', 'P', '0', '3', '4', '0', '/', 'n', 'o', 'n', 'c', 'e'};
    unsigned char nonce32[32];
    unsigned char zero32[32] = { 0 };

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_bip340(NULL, msg, msglen, key32, xonly_pk32, expected_algo, sizeof(expected_algo), (void *)aux32) == 0);

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_bip340(nonce32, NULL, msglen == 0 ? 1 : msglen, key32, xonly_pk32, expected_algo, sizeof(expected_algo), (void *)aux32) == 0);
    FUZZ_CHECK(memcmp(nonce32, zero32, sizeof(nonce32)) == 0);

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_bip340(nonce32, msg, msglen, NULL, xonly_pk32, expected_algo, sizeof(expected_algo), (void *)aux32) == 0);
    FUZZ_CHECK(memcmp(nonce32, zero32, sizeof(nonce32)) == 0);

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_bip340(nonce32, msg, msglen, key32, NULL, expected_algo, sizeof(expected_algo), (void *)aux32) == 0);
    FUZZ_CHECK(memcmp(nonce32, zero32, sizeof(nonce32)) == 0);

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_bip340(nonce32, msg, msglen, key32, xonly_pk32, NULL, 0, (void *)aux32) == 0);
    FUZZ_CHECK(memcmp(nonce32, zero32, sizeof(nonce32)) == 0);

#if SIZE_MAX > 0xffffffff
    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_bip340(nonce32, msg, (size_t)SECP256K1_SHA256_MAX_SIZE - 128, key32, xonly_pk32, expected_algo, sizeof(expected_algo), (void *)aux32) == 0);
    FUZZ_CHECK(memcmp(nonce32, zero32, sizeof(nonce32)) == 0);

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_bip340(nonce32, msg, msglen, key32, xonly_pk32, expected_algo, (size_t)SECP256K1_SHA256_MAX_SIZE, (void *)aux32) == 0);
    FUZZ_CHECK(memcmp(nonce32, zero32, sizeof(nonce32)) == 0);
#endif

    memset(nonce32, 0xA5, sizeof(nonce32));
    FUZZ_CHECK(secp256k1_nonce_function_bip340(nonce32, NULL, 0, key32, xonly_pk32, expected_algo, sizeof(expected_algo), (void *)aux32) == 1);
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

static void secp256k1_fuzz_schnorrsig_reduce_scalar(unsigned char *out32, const unsigned char *input32) {
    size_t i;
    unsigned int borrow = 0;

    memcpy(out32, input32, 32);
    if (memcmp(out32, secp256k1_fuzz_scalar_order, 32) < 0) {
        return;
    }
    for (i = 32; i > 0; i--) {
        unsigned int a = out32[i - 1];
        unsigned int b = secp256k1_fuzz_scalar_order[i - 1] + borrow;
        out32[i - 1] = (unsigned char)(a - b);
        borrow = a < b;
    }
    FUZZ_CHECK(borrow == 0);
}

/* Keep the BIP340 challenge and response equation independent from the
 * optimized challenge path used by the signer and verifier. */
static void secp256k1_fuzz_check_schnorrsig_fixed_nonce_equation(const secp256k1_context *ctx, const unsigned char *msg32, const secp256k1_keypair *keypair) {
    static const unsigned char challenge_tag[] = "BIP0340/challenge";
    static const unsigned char nonce32[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    };
    secp256k1_fuzz_schnorrsig_fixed_nonce_data nonce_data;
    secp256k1_schnorrsig_extraparams extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    secp256k1_pubkey nonce_pubkey;
    secp256k1_xonly_pubkey xonly;
    unsigned char seckey[32];
    unsigned char xonly32[32];
    unsigned char nonce_serialized[33];
    unsigned char challenge_input[96];
    unsigned char challenge32[32];
    unsigned char reduced_challenge32[32];
    unsigned char expected_sig64[64];
    unsigned char actual_sig64[64];
    size_t nonce_serialized_len = sizeof(nonce_serialized);
    int parity;

    FUZZ_CHECK(secp256k1_keypair_sec(ctx, seckey, keypair) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly, &parity, keypair) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly32, &xonly) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &nonce_pubkey, nonce32) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, nonce_serialized, &nonce_serialized_len, &nonce_pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(nonce_serialized_len == sizeof(nonce_serialized));
    FUZZ_CHECK(nonce_serialized[0] == SECP256K1_TAG_PUBKEY_EVEN);

    memcpy(challenge_input, nonce_serialized + 1, 32);
    memcpy(challenge_input + 32, xonly32, 32);
    memcpy(challenge_input + 64, msg32, 32);
    FUZZ_CHECK(secp256k1_tagged_sha256(ctx, challenge32, challenge_tag, sizeof(challenge_tag) - 1, challenge_input, sizeof(challenge_input)) == 1);
    secp256k1_fuzz_schnorrsig_reduce_scalar(reduced_challenge32, challenge32);
    if (memcmp(reduced_challenge32, secp256k1_fuzz_scalar_zero, 32) == 0) {
        memcpy(expected_sig64 + 32, nonce32, 32);
    } else {
        if (parity) {
            FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, seckey) == 1);
        }
        FUZZ_CHECK(secp256k1_ec_seckey_tweak_mul(ctx, seckey, reduced_challenge32) == 1);
        FUZZ_CHECK(secp256k1_ec_seckey_tweak_add(ctx, seckey, nonce32) == 1);
        memcpy(expected_sig64 + 32, seckey, 32);
    }
    memcpy(expected_sig64, nonce_serialized + 1, 32);

    nonce_data.self = &nonce_data;
    nonce_data.nonce32 = nonce32;
    nonce_data.calls = 0;
    extraparams.noncefp = secp256k1_fuzz_schnorrsig_nonce_fixed;
    extraparams.ndata = &nonce_data;
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, actual_sig64, msg32, 32, keypair, &extraparams) == 1);
    FUZZ_CHECK(nonce_data.calls == 1);
    FUZZ_CHECK(memcmp(actual_sig64, expected_sig64, sizeof(actual_sig64)) == 0);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, actual_sig64, msg32, 32, &xonly) == 1);
    secp256k1_memclear_explicit(seckey, sizeof(seckey));
}

/* Verify an arbitrary BIP340 signature without calling the library verifier.
 * The public point operations are independent of the verifier's internal
 * ecmult path; the candidate R must have even Y and x == r. */
static int secp256k1_fuzz_schnorrsig_verify_reference(const secp256k1_context *ctx, const unsigned char *sig64, const unsigned char *msg, size_t msglen, const unsigned char *xonly32) {
    static const unsigned char challenge_tag[] = "BIP0340/challenge";
    static const unsigned char field_p[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
    };
    unsigned char challenge32[32];
    unsigned char reduced_challenge32[32];
    unsigned char r33[33];
    unsigned char pk33[33];
    unsigned char expected_serialized[65];
    secp256k1_pubkey r_pubkey;
    secp256k1_pubkey pk_pubkey;
    secp256k1_pubkey s_pubkey = { 0 };
    secp256k1_pubkey e_pubkey = { 0 };
    secp256k1_pubkey negated_e_pubkey = { 0 };
    secp256k1_pubkey expected_pubkey;
    const secp256k1_pubkey *terms[2];
    size_t expected_len = sizeof(expected_serialized);
    int have_s;
    int have_e;

    if (memcmp(sig64, field_p, 32) >= 0 || memcmp(sig64 + 32, secp256k1_fuzz_scalar_order, 32) >= 0) {
        return 0;
    }

    r33[0] = SECP256K1_TAG_PUBKEY_EVEN;
    memcpy(r33 + 1, sig64, 32);
    if (!secp256k1_ec_pubkey_parse(ctx, &r_pubkey, r33, sizeof(r33))) {
        return 0;
    }
    pk33[0] = SECP256K1_TAG_PUBKEY_EVEN;
    memcpy(pk33 + 1, xonly32, 32);
    if (!secp256k1_ec_pubkey_parse(ctx, &pk_pubkey, pk33, sizeof(pk33))) {
        return 0;
    }

    secp256k1_fuzz_schnorrsig_tagged_hash_reference(challenge32, challenge_tag, sizeof(challenge_tag) - 1, sig64, 32, xonly32, 32, msg, msglen);
    secp256k1_fuzz_schnorrsig_reduce_scalar(reduced_challenge32, challenge32);
    have_s = memcmp(sig64 + 32, secp256k1_fuzz_scalar_zero, 32) != 0;
    have_e = memcmp(reduced_challenge32, secp256k1_fuzz_scalar_zero, 32) != 0;
    if (have_s) {
        if (!secp256k1_ec_pubkey_create(ctx, &s_pubkey, sig64 + 32)) {
            return 0;
        }
    }
    if (have_e) {
        e_pubkey = pk_pubkey;
        if (!secp256k1_ec_pubkey_tweak_mul(ctx, &e_pubkey, reduced_challenge32)) {
            return 0;
        }
        negated_e_pubkey = e_pubkey;
        if (!secp256k1_ec_pubkey_negate(ctx, &negated_e_pubkey)) {
            return 0;
        }
    }
    if (have_s && have_e) {
        terms[0] = &s_pubkey;
        terms[1] = &negated_e_pubkey;
        if (!secp256k1_ec_pubkey_combine(ctx, &expected_pubkey, terms, 2)) {
            return 0;
        }
    } else if (have_s) {
        expected_pubkey = s_pubkey;
    } else if (have_e) {
        expected_pubkey = negated_e_pubkey;
    } else {
        return 0;
    }
    if (!secp256k1_ec_pubkey_serialize(ctx, expected_serialized, &expected_len, &expected_pubkey, SECP256K1_EC_UNCOMPRESSED)) {
        return 0;
    }
    return expected_len == sizeof(expected_serialized)
        && (expected_serialized[64] & 1u) == 0
        && memcmp(expected_serialized + 1, sig64, 32) == 0;
}

static void secp256k1_fuzz_check_schnorrsig_verify_reference(const secp256k1_context *ctx, const unsigned char *sig64, const unsigned char *msg, size_t msglen, const unsigned char *xonly32, const secp256k1_xonly_pubkey *xonly) {
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_verify_reference(ctx, sig64, msg, msglen, xonly32)
               == secp256k1_schnorrsig_verify(ctx, sig64, msg, msglen, xonly));
}

/* Model the part of Bitcoin Core's Tapscript signature path after
 * SignatureHashSchnorr has produced the 32-byte message hash. Core accepts a
 * 64-byte default signature or a 65-byte signature with a valid, non-default
 * hash type, strips the latter byte, reparses the raw x-only key, and then
 * calls the fixed-size Schnorr verifier. */
static int secp256k1_fuzz_core_schnorr_hash_type_valid(unsigned char hashtype) {
    return hashtype != 0 && (hashtype <= 3 || (hashtype >= 0x81 && hashtype <= 0x83));
}

static int secp256k1_fuzz_core_tapscript_schnorr_verify(const secp256k1_context *ctx, const unsigned char *sig, size_t siglen, const unsigned char *xonly32, const unsigned char *msg32) {
    secp256k1_xonly_pubkey xonly;

    if (siglen != 64 && siglen != 65) {
        return 0;
    }
    if (siglen == 65 && !secp256k1_fuzz_core_schnorr_hash_type_valid(sig[64])) {
        return 0;
    }
    if (!secp256k1_xonly_pubkey_parse(ctx, &xonly, xonly32)) {
        return 0;
    }
    return secp256k1_schnorrsig_verify(ctx, sig, msg32, 32, &xonly);
}

/* Model EvalChecksigTapscript's outer sequencing from Bitcoin Core. An empty
 * signature is a non-aborting failed check and therefore must not reach the
 * Schnorr parser. A nonempty signature consumes validation weight before the
 * key-size dispatch, and a 32-byte key invokes the inner checker. Other
 * nonempty key sizes are upgradeable and pass through when the discourage flag
 * is not set. Return 1 for continued script execution and 0 for script abort;
 * success_out is the signature-check result when execution continues. */
static int secp256k1_fuzz_core_tapscript_eval(const secp256k1_context *ctx, const unsigned char *sig, size_t siglen, const unsigned char *pubkey, size_t pubkeylen, const unsigned char *msg32, int validation_weight_available, int *success_out) {
    int success = siglen != 0;

    if (success && !validation_weight_available) {
        return 0;
    }
    if (pubkeylen == 0) {
        return 0;
    }
    if (pubkeylen == 32) {
        if (success && !secp256k1_fuzz_core_tapscript_schnorr_verify(ctx, sig, siglen, pubkey, msg32)) {
            return 0;
        }
    }
    *success_out = success;
    return 1;
}

static void secp256k1_fuzz_check_core_tapscript_schnorr_composition(const secp256k1_context *ctx, const unsigned char *input, size_t size, const unsigned char *sig64, const unsigned char *xonly32, const unsigned char *msg32) {
    static const unsigned char trigger[] = "core-tapscript-schnorr-composition\n";
    static const unsigned char field_p[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
    };
    static const unsigned char invalid_hash_types[] = { 0x00, 0x04, 0x80, 0x84 };
    const secp256k1_context *contexts[2];
    unsigned char sig65[65];
    unsigned char invalid_sig64[64];
    unsigned char unknown_pubkey[33] = { 0 };
    unsigned char serialized_xonly[32];
    unsigned char zero_xonly[sizeof(secp256k1_xonly_pubkey)] = { 0 };
    secp256k1_xonly_pubkey parsed_xonly;
    int success;
    int reference;
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    contexts[0] = ctx;
    contexts[1] = secp256k1_context_static;
    memcpy(sig65, sig64, sizeof(sig64[0]) * 64);
    sig65[64] = 0x01;
    memcpy(invalid_sig64, sig64, sizeof(invalid_sig64));
    memset(invalid_sig64 + 32, 0xFF, 32);

    reference = secp256k1_fuzz_schnorrsig_verify_reference(ctx, sig64, msg32, 32, xonly32);
    FUZZ_CHECK(reference == 1);
    for (i = 0; i < sizeof(contexts) / sizeof(contexts[0]); i++) {
        FUZZ_CHECK(secp256k1_fuzz_core_tapscript_schnorr_verify(contexts[i], sig64, 64, xonly32, msg32) == reference);
        FUZZ_CHECK(secp256k1_fuzz_core_tapscript_schnorr_verify(contexts[i], sig65, sizeof(sig65), xonly32, msg32) == reference);

        FUZZ_CHECK(secp256k1_xonly_pubkey_parse(contexts[i], &parsed_xonly, xonly32) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(contexts[i], serialized_xonly, &parsed_xonly) == 1);
        FUZZ_CHECK(memcmp(serialized_xonly, xonly32, sizeof(serialized_xonly)) == 0);
    }

    /* BIP341/BIP342 witness bytes can carry a field overflow as the raw
     * output key. Core must reject it before the Schnorr equation is used. */
    for (i = 0; i < sizeof(contexts) / sizeof(contexts[0]); i++) {
        memset(&parsed_xonly, 0xA5, sizeof(parsed_xonly));
        FUZZ_CHECK(secp256k1_xonly_pubkey_parse(contexts[i], &parsed_xonly, field_p) == 0);
        FUZZ_CHECK(memcmp(&parsed_xonly, zero_xonly, sizeof(parsed_xonly)) == 0);
        FUZZ_CHECK(secp256k1_fuzz_core_tapscript_schnorr_verify(contexts[i], sig64, 64, field_p, msg32) == 0);
    }
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_verify_reference(ctx, sig64, msg32, 32, field_p) == 0);

    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_schnorr_verify(ctx, sig64, 63, xonly32, msg32) == 0);
    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_schnorr_verify(ctx, sig64, 66, xonly32, msg32) == 0);
    for (i = 0; i < sizeof(invalid_hash_types); i++) {
        sig65[64] = invalid_hash_types[i];
        FUZZ_CHECK(secp256k1_fuzz_core_tapscript_schnorr_verify(ctx, sig65, sizeof(sig65), xonly32, msg32) == 0);
    }

    /* The outer Tapscript rule is intentionally different from calling the
     * library verifier directly: an empty signature does not parse its key,
     * while a nonempty signature does. This distinction is consensus-visible
     * for malformed 32-byte witness keys. */
    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_eval(ctx, NULL, 0, xonly32, 32, msg32, 1, &success) == 1);
    FUZZ_CHECK(success == 0);
    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_eval(ctx, NULL, 0, field_p, 32, msg32, 1, &success) == 1);
    FUZZ_CHECK(success == 0);
    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_eval(ctx, NULL, 0, NULL, 0, msg32, 1, &success) == 0);
    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_eval(ctx, sig64, 64, NULL, 0, msg32, 1, &success) == 0);
    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_eval(ctx, invalid_sig64, sizeof(invalid_sig64), xonly32, 32, msg32, 1, &success) == 0);
    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_eval(ctx, sig64, 64, xonly32, 32, msg32, 0, &success) == 0);
    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_eval(ctx, sig64, 64, unknown_pubkey, sizeof(unknown_pubkey), msg32, 1, &success) == 1);
    FUZZ_CHECK(success == 1);
    FUZZ_CHECK(secp256k1_fuzz_core_tapscript_eval(ctx, NULL, 0, unknown_pubkey, sizeof(unknown_pubkey), msg32, 1, &success) == 1);
    FUZZ_CHECK(success == 0);
}

/* Model Bitcoin Core's CKey::SignSchnorr composition after it has computed
 * the 32-byte SignatureHashSchnorr. The merkle_root pointer has three
 * observable states in Core: NULL means no tweak, a pointer to a null uint256
 * means H_TapTweak(xonly), and a non-null root means
 * H_TapTweak(xonly || merkle_root). */
static void secp256k1_fuzz_check_core_taproot_signing_composition(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "core-taproot-signing-composition\n";
    static const unsigned char seckey32[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B
    };
    static const unsigned char msg32[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };
    static const unsigned char zero_aux32[32] = { 0 };
    static const unsigned char zero_root32[32] = { 0 };
    static const unsigned char script_root32[32] = {
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42
    };
    static const unsigned char base_xonly32[32] = {
        0x77, 0x4A, 0xE7, 0xF8, 0x58, 0xA9, 0x41, 0x1E,
        0x5E, 0xF4, 0x24, 0x6B, 0x70, 0xC6, 0x5A, 0xAC,
        0x56, 0x49, 0x98, 0x0B, 0xE5, 0xC1, 0x78, 0x91,
        0xBB, 0xEC, 0x17, 0x89, 0x5D, 0xA0, 0x08, 0xCB
    };
    static const unsigned char expected_tweaks[2][32] = {
        {
            0x4A, 0xBC, 0x8F, 0xBF, 0x22, 0x43, 0xDF, 0x3A,
            0x8B, 0x8E, 0xB7, 0x6E, 0x42, 0x88, 0x03, 0xAF,
            0xAD, 0x00, 0x17, 0xA1, 0x1A, 0x88, 0xBB, 0x6D,
            0xCD, 0xCA, 0xB3, 0x12, 0xAB, 0x25, 0xF0, 0x37
        },
        {
            0x56, 0xD0, 0x07, 0xF7, 0x62, 0xFE, 0x8E, 0xAA,
            0xF1, 0x88, 0xD8, 0xA1, 0x8D, 0xEF, 0x9F, 0x5F,
            0x9E, 0x45, 0xAF, 0xE2, 0x27, 0x97, 0x91, 0xA2,
            0xAE, 0xF9, 0xD5, 0xC8, 0xEE, 0xE4, 0xD2, 0x13
        }
    };
    static const unsigned char expected_xonly32[3][32] = {
        {
            0x77, 0x4A, 0xE7, 0xF8, 0x58, 0xA9, 0x41, 0x1E,
            0x5E, 0xF4, 0x24, 0x6B, 0x70, 0xC6, 0x5A, 0xAC,
            0x56, 0x49, 0x98, 0x0B, 0xE5, 0xC1, 0x78, 0x91,
            0xBB, 0xEC, 0x17, 0x89, 0x5D, 0xA0, 0x08, 0xCB
        },
        {
            0x31, 0x14, 0xEE, 0x06, 0x01, 0x5C, 0x28, 0xEF,
            0xC7, 0x0B, 0x86, 0x7B, 0x6A, 0xA3, 0xD2, 0xAA,
            0xAF, 0xAD, 0xF3, 0x7D, 0x07, 0xC1, 0xA1, 0x08,
            0x73, 0xD0, 0x33, 0x01, 0xF4, 0x1F, 0x00, 0xBC
        },
        {
            0x36, 0x06, 0x01, 0x33, 0x7B, 0xBD, 0x8E, 0xA0,
            0x3F, 0xF4, 0x8F, 0x24, 0x18, 0x76, 0x31, 0x1C,
            0x0C, 0x80, 0x29, 0xAF, 0x40, 0xB8, 0x51, 0xE7,
            0xD0, 0x6D, 0xCB, 0x69, 0x34, 0xA1, 0xB0, 0x28
        }
    };
    static const unsigned char expected_sig64[3][64] = {
        {
            0xDC, 0xD5, 0x6E, 0x38, 0x86, 0xE6, 0x4C, 0xBC,
            0xB1, 0xEC, 0xBD, 0x7A, 0x47, 0x9D, 0x5E, 0xB5,
            0xAD, 0x1F, 0x86, 0xCE, 0x4E, 0x49, 0xE9, 0xB8,
            0x0C, 0xFA, 0xD8, 0xE9, 0x98, 0x10, 0x7B, 0x31,
            0xAE, 0x1A, 0x91, 0xF2, 0x39, 0xD6, 0x23, 0x13,
            0x39, 0x39, 0x2A, 0x63, 0x56, 0x9B, 0x05, 0xC1,
            0x1F, 0x0C, 0xB4, 0x99, 0x57, 0xF7, 0x9C, 0xDB,
            0xC0, 0xE0, 0x63, 0x0A, 0xB9, 0xB2, 0x54, 0x5D
        },
        {
            0xAC, 0x58, 0x1D, 0xFF, 0x36, 0xEA, 0xE6, 0x2D,
            0x64, 0x1E, 0x8E, 0x81, 0x7D, 0x85, 0x5A, 0x46,
            0x25, 0xCF, 0x05, 0x1D, 0x73, 0x99, 0x84, 0x4F,
            0x0A, 0xFF, 0x9F, 0x34, 0x71, 0x9E, 0x96, 0x67,
            0x76, 0x65, 0xA6, 0x90, 0x48, 0x7D, 0x96, 0xD1,
            0x46, 0xD0, 0xCC, 0xB8, 0x13, 0x6E, 0x4E, 0xCA,
            0xB0, 0xCB, 0xFB, 0xA9, 0x86, 0x79, 0x5E, 0x47,
            0x49, 0x2E, 0x37, 0xE7, 0x97, 0xD1, 0xF5, 0xD9
        },
        {
            0xBD, 0xE5, 0xD0, 0xCE, 0x08, 0x39, 0x14, 0xD7,
            0x76, 0xB2, 0x76, 0x33, 0x21, 0xE0, 0x27, 0x98,
            0xA5, 0x94, 0x68, 0x7A, 0x64, 0x67, 0xF7, 0xC8,
            0x75, 0x6E, 0x4C, 0xCC, 0x5F, 0x79, 0x82, 0x7F,
            0x99, 0x92, 0xA3, 0x3D, 0xD9, 0x47, 0xB0, 0x7E,
            0xD9, 0x0A, 0x0C, 0xBA, 0x48, 0x5A, 0xE0, 0xCB,
            0x28, 0xF4, 0xF3, 0x4A, 0xE8, 0x3E, 0x16, 0x11,
            0xCC, 0x23, 0xB2, 0x51, 0xB3, 0xAD, 0x8F, 0xBC
        }
    };
    static const unsigned char tap_tweak_tag[] = "TapTweak";
    const unsigned char *merkle_roots[3] = { NULL, zero_root32, script_root32 };
    unsigned char base_xonly_actual[32];
    unsigned char tweak32[32];
    unsigned char xonly32[32];
    unsigned char sig64[64];
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey xonly;
    int base_parity;
    int output_parity;
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey32) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(secp256k1_context_static, &xonly, &base_parity, &keypair) == 1);
    FUZZ_CHECK(base_parity == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(secp256k1_context_static, base_xonly_actual, &xonly) == 1);
    FUZZ_CHECK(memcmp(base_xonly_actual, base_xonly32, sizeof(base_xonly_actual)) == 0);

    for (i = 0; i < sizeof(merkle_roots) / sizeof(merkle_roots[0]); i++) {
        const unsigned char *merkle_root = merkle_roots[i];

        FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey32) == 1);
        if (merkle_root != NULL) {
            secp256k1_fuzz_schnorrsig_tagged_hash_reference(
                tweak32, tap_tweak_tag, sizeof(tap_tweak_tag) - 1,
                base_xonly32, sizeof(base_xonly32),
                i == 1 ? NULL : merkle_root, i == 1 ? 0 : sizeof(script_root32),
                NULL, 0);
            FUZZ_CHECK(memcmp(tweak32, expected_tweaks[i - 1], sizeof(tweak32)) == 0);
            /* Match Core's KeyPair::KeyPair call. The no-table consistency
             * fallback in d3c88265 preserves this valid static-context path. */
            FUZZ_CHECK(secp256k1_keypair_xonly_tweak_add(secp256k1_context_static, &keypair, tweak32) == 1);
        }

        FUZZ_CHECK(secp256k1_keypair_xonly_pub(secp256k1_context_static, &xonly, &output_parity, &keypair) == 1);
        FUZZ_CHECK(output_parity == (i == 0));
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(secp256k1_context_static, xonly32, &xonly) == 1);
        FUZZ_CHECK(memcmp(xonly32, expected_xonly32[i], sizeof(xonly32)) == 0);

        memset(sig64, 0xA5, sizeof(sig64));
        FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sig64, msg32, &keypair, zero_aux32) == 1);
        FUZZ_CHECK(memcmp(sig64, expected_sig64[i], sizeof(sig64)) == 0);
        FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), &xonly) == 1);
        FUZZ_CHECK(secp256k1_schnorrsig_verify(secp256k1_context_static, sig64, msg32, sizeof(msg32), &xonly) == 1);
    }
}

/* BIP340 rejects the identity as the reconstructed nonce. Use P = G and set
 * s to the challenge so the verifier must reach that explicit rejection. */
static void secp256k1_fuzz_check_schnorrsig_infinity_rejection(const secp256k1_context *ctx) {
    static const unsigned char seckey32[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    };
    static const unsigned char msg32[32] = { 0 };
    static const unsigned char challenge_tag[] = "BIP0340/challenge";
    unsigned char xonly32[32];
    unsigned char challenge32[32];
    unsigned char sig64[64];
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey xonly;

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey32) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly, NULL, &keypair) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly32, &xonly) == 1);

    /* The x-coordinate is valid wire data; the reconstructed point is not. */
    memcpy(sig64, xonly32, sizeof(xonly32));
    secp256k1_fuzz_schnorrsig_tagged_hash_reference(challenge32, challenge_tag, sizeof(challenge_tag) - 1, sig64, 32, xonly32, 32, msg32, sizeof(msg32));
    secp256k1_fuzz_schnorrsig_reduce_scalar(sig64 + 32, challenge32);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), &xonly) == 0);
}

/* Use P = R = G and s = e + 1 to make the BIP340 equation reduce to
 * s*G - e*P = G. The challenge and scalar addition are byte-level references,
 * so the expected validity does not depend on the signing path or production
 * point arithmetic. */
static void secp256k1_fuzz_check_schnorrsig_generator_equation(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "schnorrsig-generator-equation\n";
    static const unsigned char generator_x[32] = {
        0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC,
        0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B, 0x07,
        0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9,
        0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
    };
    static const unsigned char challenge_tag[] = "BIP0340/challenge";
    static const unsigned char msg32[32] = { 0 };
    unsigned char challenge32[32];
    unsigned char reduced_challenge32[32];
    unsigned char sig64[64];
    unsigned char serialized_xonly[32];
#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
    unsigned char zero_sig64[64] = { 0 };
    secp256k1_keypair keypair;
    unsigned int calls;
#endif
    const secp256k1_context *contexts[2];
    secp256k1_xonly_pubkey xonly[2];
    size_t i;
    unsigned int carry = 1;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    contexts[0] = ctx;
    contexts[1] = secp256k1_context_static;
    memcpy(sig64, generator_x, sizeof(generator_x));
    secp256k1_fuzz_schnorrsig_tagged_hash_reference(challenge32, challenge_tag, sizeof(challenge_tag) - 1, generator_x, sizeof(generator_x), generator_x, sizeof(generator_x), msg32, sizeof(msg32));
    secp256k1_fuzz_schnorrsig_reduce_scalar(reduced_challenge32, challenge32);
    memcpy(sig64 + 32, reduced_challenge32, sizeof(reduced_challenge32));
    for (i = sizeof(reduced_challenge32); i != 0; i--) {
        unsigned int sum = (unsigned int)sig64[32 + i - 1] + carry;
        sig64[32 + i - 1] = (unsigned char)sum;
        carry = sum >> 8;
    }
    FUZZ_CHECK(carry == 0);
    secp256k1_fuzz_schnorrsig_reduce_scalar(sig64 + 32, sig64 + 32);
    for (i = 0; i < sizeof(contexts) / sizeof(contexts[0]); i++) {
        FUZZ_CHECK(secp256k1_xonly_pubkey_parse(contexts[i], &xonly[i], generator_x) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(contexts[i], serialized_xonly, &xonly[i]) == 1);
        FUZZ_CHECK(memcmp(serialized_xonly, generator_x, sizeof(serialized_xonly)) == 0);
        FUZZ_CHECK(secp256k1_schnorrsig_verify(contexts[i], sig64, msg32, sizeof(msg32), &xonly[i]) == 1);
    }

#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
    /* Verification is a public-data path. Schnorr signing still requires
     * generator precomputation and must reject the static singleton without
     * leaving stale signature bytes behind. */
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, secp256k1_fuzz_scalar_one) == 1);
    calls = secp256k1_fuzz_default_illegal_calls;
    memset(sig64, 0xA5, sizeof(sig64));
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(secp256k1_context_static, sig64, msg32, &keypair, NULL) == 0);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
    FUZZ_CHECK(memcmp(sig64, zero_sig64, sizeof(sig64)) == 0);

    memset(sig64, 0x5A, sizeof(sig64));
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(secp256k1_context_static, sig64, msg32, sizeof(msg32), &keypair, NULL) == 0);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
    FUZZ_CHECK(memcmp(sig64, zero_sig64, sizeof(sig64)) == 0);
#endif
}

/* Check a generated signature against BIP340's public point equation without
 * calling the library verifier. This catches shared challenge/signing changes
 * that would otherwise make signing and verification agree on the same error. */
static void secp256k1_fuzz_check_schnorrsig_signature_equation(const secp256k1_context *ctx, const unsigned char *sig64, const unsigned char *msg, size_t msglen, const unsigned char *xonly32) {
    static const unsigned char challenge_tag[] = "BIP0340/challenge";
    unsigned char challenge32[32];
    unsigned char reduced_challenge32[32];
    unsigned char r33[33];
    unsigned char pk33[33];
    secp256k1_pubkey r_pubkey;
    secp256k1_pubkey pk_pubkey;
    secp256k1_pubkey s_pubkey = { 0 };
    secp256k1_pubkey e_pubkey = { 0 };
    secp256k1_pubkey negated_e_pubkey = { 0 };
    secp256k1_pubkey expected_pubkey;
    const secp256k1_pubkey *terms[2];
    int have_s;
    int have_e;

    secp256k1_fuzz_schnorrsig_tagged_hash_reference(challenge32, challenge_tag, sizeof(challenge_tag) - 1, sig64, 32, xonly32, 32, msg, msglen);
    secp256k1_fuzz_schnorrsig_reduce_scalar(reduced_challenge32, challenge32);

    r33[0] = SECP256K1_TAG_PUBKEY_EVEN;
    memcpy(r33 + 1, sig64, 32);
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &r_pubkey, r33, sizeof(r33)) == 1);
    pk33[0] = SECP256K1_TAG_PUBKEY_EVEN;
    memcpy(pk33 + 1, xonly32, 32);
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &pk_pubkey, pk33, sizeof(pk33)) == 1);

    have_s = memcmp(sig64 + 32, secp256k1_fuzz_scalar_zero, 32) != 0;
    have_e = memcmp(reduced_challenge32, secp256k1_fuzz_scalar_zero, 32) != 0;
    if (have_s) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &s_pubkey, sig64 + 32) == 1);
    }
    if (have_e) {
        e_pubkey = pk_pubkey;
        FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &e_pubkey, reduced_challenge32) == 1);
        negated_e_pubkey = e_pubkey;
        FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &negated_e_pubkey) == 1);
    }

    if (have_s && have_e) {
        terms[0] = &s_pubkey;
        terms[1] = &negated_e_pubkey;
        FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &expected_pubkey, terms, 2) == 1);
    } else if (have_s) {
        expected_pubkey = s_pubkey;
    } else {
        FUZZ_CHECK(have_e);
        expected_pubkey = negated_e_pubkey;
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &expected_pubkey, &r_pubkey) == 0);
}

/* BIP340 rejects a signature when its reconstructed nonce has odd Y, even if
 * the scalar equation is otherwise valid. The normal signer always negates an
 * odd nonce, so construct the pre-normalization case directly. */
static void secp256k1_fuzz_check_schnorrsig_odd_nonce_rejection(const secp256k1_context *ctx) {
    static const unsigned char msg32[32] = { 0 };
    static const unsigned char challenge_tag[] = "BIP0340/challenge";
    unsigned char seckey32[32] = { 0 };
    unsigned char nonce32[32] = { 0 };
    unsigned char pubkey33[33];
    unsigned char xonly32[32];
    unsigned char nonce_serialized[33];
    unsigned char challenge_input[96];
    unsigned char challenge32[32];
    unsigned char reduced_challenge32[32];
    unsigned char sig64[64];
    unsigned char response_pubkey_serialized[65];
    secp256k1_pubkey pubkey;
    secp256k1_pubkey nonce_pubkey;
    secp256k1_pubkey response_pubkey;
    secp256k1_pubkey combined_pubkey;
    secp256k1_pubkey challenge_pubkey;
    secp256k1_pubkey negated_challenge_pubkey;
    const secp256k1_pubkey *response_terms[2];
    secp256k1_xonly_pubkey xonly;
    size_t nonce_serialized_len;
    size_t response_pubkey_serialized_len;
    int nonce;
    int found = 0;

    seckey32[31] = 1;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey32) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &xonly, NULL, &pubkey) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly32, &xonly) == 1);
    pubkey33[0] = SECP256K1_TAG_PUBKEY_EVEN;
    memcpy(pubkey33 + 1, xonly32, sizeof(xonly32));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &challenge_pubkey, pubkey33, sizeof(pubkey33)) == 1);

    for (nonce = 1; nonce <= 32 && !found; nonce++) {
        memset(nonce32, 0, sizeof(nonce32));
        nonce32[31] = (unsigned char)nonce;
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &nonce_pubkey, nonce32) == 1);
        nonce_serialized_len = sizeof(nonce_serialized);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, nonce_serialized, &nonce_serialized_len, &nonce_pubkey, SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(nonce_serialized_len == sizeof(nonce_serialized));
        if (nonce_serialized[0] != SECP256K1_TAG_PUBKEY_ODD) {
            continue;
        }

        memcpy(challenge_input, nonce_serialized + 1, 32);
        memcpy(challenge_input + 32, xonly32, 32);
        memcpy(challenge_input + 64, msg32, 32);
        FUZZ_CHECK(secp256k1_tagged_sha256(ctx, challenge32, challenge_tag, sizeof(challenge_tag) - 1, challenge_input, sizeof(challenge_input)) == 1);
        secp256k1_fuzz_schnorrsig_reduce_scalar(reduced_challenge32, challenge32);

        memcpy(sig64, nonce_serialized + 1, 32);
        if (memcmp(reduced_challenge32, secp256k1_fuzz_scalar_zero, 32) == 0) {
            memcpy(sig64 + 32, nonce32, 32);
        } else {
            memcpy(sig64 + 32, seckey32, 32);
            FUZZ_CHECK(secp256k1_ec_seckey_tweak_mul(ctx, sig64 + 32, reduced_challenge32) == 1);
            FUZZ_CHECK(secp256k1_ec_seckey_tweak_add(ctx, sig64 + 32, nonce32) == 1);
        }

        /* Independently reconstruct R = sG - eP through public API paths. */
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &response_pubkey, sig64 + 32) == 1);
        if (memcmp(reduced_challenge32, secp256k1_fuzz_scalar_zero, 32) != 0) {
            FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &challenge_pubkey, reduced_challenge32) == 1);
        }
        negated_challenge_pubkey = challenge_pubkey;
        FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &negated_challenge_pubkey) == 1);
        response_terms[0] = &response_pubkey;
        response_terms[1] = &negated_challenge_pubkey;
        FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined_pubkey, response_terms, 2) == 1);
        response_pubkey_serialized_len = sizeof(response_pubkey_serialized);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, response_pubkey_serialized, &response_pubkey_serialized_len, &combined_pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
        FUZZ_CHECK(response_pubkey_serialized_len == sizeof(response_pubkey_serialized));
        FUZZ_CHECK(response_pubkey_serialized[0] == SECP256K1_TAG_PUBKEY_UNCOMPRESSED);
        FUZZ_CHECK(memcmp(response_pubkey_serialized + 1, sig64, 32) == 0);
        FUZZ_CHECK((response_pubkey_serialized[64] & 1u) != 0);
        FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), &xonly) == 0);
        found = 1;
    }
    FUZZ_CHECK(found);
}

static void secp256k1_fuzz_check_schnorrsig_s_order_boundary(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "schnorrsig s order boundary\n";
    static const unsigned char generator_x[32] = {
        0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC,
        0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B, 0x07,
        0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9,
        0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
    };
    static const unsigned char msg32[32] = { 0 };
    const secp256k1_context *contexts[2];
    secp256k1_xonly_pubkey xonly;
    unsigned char serialized_xonly[32];
    unsigned char sig64[64];
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    memcpy(sig64, generator_x, sizeof(generator_x));
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_order, 32);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_verify_reference(ctx, sig64, msg32, sizeof(msg32), generator_x) == 0);

    contexts[0] = ctx;
    contexts[1] = secp256k1_context_static;
    for (i = 0; i < sizeof(contexts) / sizeof(contexts[0]); i++) {
        FUZZ_CHECK(secp256k1_xonly_pubkey_parse(contexts[i], &xonly, generator_x) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(contexts[i], serialized_xonly, &xonly) == 1);
        FUZZ_CHECK(memcmp(serialized_xonly, generator_x, sizeof(serialized_xonly)) == 0);
        FUZZ_CHECK(secp256k1_schnorrsig_verify(contexts[i], sig64, msg32, sizeof(msg32), &xonly) == 0);
    }
}

static void secp256k1_fuzz_check_schnorrsig_rx_overflow(const secp256k1_context *ctx, const unsigned char *sig64, const unsigned char *msg, size_t msglen, const secp256k1_xonly_pubkey *xonly) {
    static const unsigned char field_p[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
    };
    static const unsigned char field_p_plus_one[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
    };
    unsigned char sig64_overflow[64];

    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg, msglen, xonly) == 1);

    memcpy(sig64_overflow, sig64, sizeof(sig64_overflow));
    memcpy(sig64_overflow, field_p, sizeof(field_p));
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_overflow, msg, msglen, xonly) == 0);

    memcpy(sig64_overflow, sig64, sizeof(sig64_overflow));
    memcpy(sig64_overflow, field_p_plus_one, sizeof(field_p_plus_one));
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_overflow, msg, msglen, xonly) == 0);
}

static void secp256k1_fuzz_check_schnorrsig_invalid_pubkey_verify(secp256k1_context *ctx, const unsigned char *sig64, const unsigned char *msg, size_t msglen) {
    secp256k1_fuzz_schnorrsig_illegal_data illegal_data;
    secp256k1_xonly_pubkey invalid_xonly;

    memset(&invalid_xonly, 0, sizeof(invalid_xonly));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_schnorrsig_illegal_callback, &illegal_data);
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_schnorrsig_sha256_compression);

    secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg, msglen, &invalid_xonly) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls == 0);

    secp256k1_context_set_sha256_compression(ctx, NULL);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_schnorrsig_keypair_consistency(secp256k1_context *ctx, const unsigned char *msg32, const secp256k1_keypair *keypair, const secp256k1_keypair *other_keypair, const unsigned char *aux32) {
    static const unsigned char scalar_two[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };
    static const unsigned char scalar_three[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03
    };
    const unsigned char *alternate_seckeys[3] = { secp256k1_fuzz_scalar_one, scalar_two, scalar_three };
    secp256k1_fuzz_schnorrsig_illegal_data illegal_data;
    secp256k1_keypair mismatched_keypair;
    secp256k1_pubkey original_pubkey;
    secp256k1_pubkey other_pubkey;
    secp256k1_pubkey alternate_pubkey;
    const secp256k1_pubkey *mismatched_pubkey = &other_pubkey;
    unsigned char sig64[64];
    unsigned char zero64[64] = { 0 };
    unsigned int calls;
    size_t i;

    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &original_pubkey, keypair) == 1);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &other_pubkey, other_keypair) == 1);
    if (secp256k1_ec_pubkey_cmp(ctx, &original_pubkey, mismatched_pubkey) == 0) {
        mismatched_pubkey = &alternate_pubkey;
        for (i = 0; i < sizeof(alternate_seckeys) / sizeof(alternate_seckeys[0]); i++) {
            FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &alternate_pubkey, alternate_seckeys[i]) == 1);
            if (secp256k1_ec_pubkey_cmp(ctx, &original_pubkey, mismatched_pubkey) != 0) {
                break;
            }
        }
        FUZZ_CHECK(i < sizeof(alternate_seckeys) / sizeof(alternate_seckeys[0]));
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &original_pubkey, mismatched_pubkey) != 0);
    }

    mismatched_keypair = *keypair;
    memcpy(mismatched_keypair.data + 32, mismatched_pubkey->data, sizeof(mismatched_keypair.data) - 32);
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_schnorrsig_illegal_callback, &illegal_data);

    calls = illegal_data.calls;
    memset(sig64, 0xA5, sizeof(sig64));
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sig64, msg32, &mismatched_keypair, aux32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    calls = illegal_data.calls;
    memset(sig64, 0xA5, sizeof(sig64));
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64, msg32, 32, &mismatched_keypair, NULL) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(sig64, zero64, sizeof(sig64)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_schnorrsig_impossible_msglen(secp256k1_context *ctx, const unsigned char *sig64, const unsigned char *msg, const secp256k1_keypair *keypair, const secp256k1_xonly_pubkey *xonly, const unsigned char *aux32) {
#if SIZE_MAX > 0xffffffff
    secp256k1_fuzz_schnorrsig_illegal_data illegal_data;
    secp256k1_fuzz_schnorrsig_nonce_data nonce_data;
    secp256k1_schnorrsig_extraparams extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    unsigned char oversized_sig64[64];
    unsigned char zero64[64] = { 0 };
    unsigned int calls;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    nonce_data.self = &nonce_data;
    nonce_data.aux32 = aux32;
    nonce_data.expected_key32 = NULL;
    nonce_data.expected_xonly_pk32 = NULL;
    nonce_data.expected_msg = NULL;
    nonce_data.expected_msglen = 0;
    nonce_data.calls = 0;
    extraparams.noncefp = secp256k1_fuzz_schnorrsig_nonce;
    extraparams.ndata = &nonce_data;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_schnorrsig_illegal_callback, &illegal_data);
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_schnorrsig_sha256_compression);

    memset(oversized_sig64, 0xA5, sizeof(oversized_sig64));
    calls = illegal_data.calls;
    secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls = 0;
    secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, oversized_sig64, msg, (size_t)SECP256K1_SHA256_MAX_SIZE - 128, keypair, &extraparams) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(nonce_data.calls == 0);
    FUZZ_CHECK(memcmp(oversized_sig64, zero64, sizeof(oversized_sig64)) == 0);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls == 0);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls == 0);

    calls = illegal_data.calls;
    secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg, (size_t)SECP256K1_SHA256_MAX_SIZE - 128, xonly) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls == 0);

    secp256k1_context_set_sha256_compression(ctx, NULL);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
#else
    (void)ctx;
    (void)sig64;
    (void)msg;
    (void)keypair;
    (void)xonly;
    (void)aux32;
#endif
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
    unsigned char custom_algo[32];
    unsigned char zero_aux32[32] = { 0 };
    unsigned char xonly32[32];
    unsigned char other_xonly32[32];
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
    size_t custom_algolen;
    size_t wrong_msglen;
    size_t flip_index;
    unsigned char flip_mask;
    unsigned char sig64_negated[64];
    int parity;
    int negated_parity;
    unsigned char normalized_seckey[32];

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 127);
    secp256k1_fuzz_derive(aux32, sizeof(aux32), input, size, 131);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 137);
    msglen = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 139) % (size + 1));
    secp256k1_fuzz_derive(custom_algo, sizeof(custom_algo), input, size, 151);
    custom_algolen = 14 + (secp256k1_fuzz_byte(input, size, 157) % (sizeof(custom_algo) - 13));
    nonce_data.self = &nonce_data;
    nonce_data.aux32 = aux32;
    nonce_data.expected_key32 = NULL;
    nonce_data.expected_xonly_pk32 = NULL;
    nonce_data.expected_msg = NULL;
    nonce_data.expected_msglen = 0;
    nonce_data.calls = 0;
    checked_extraparams.noncefp = secp256k1_fuzz_schnorrsig_nonce;
    checked_extraparams.ndata = &nonce_data;

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly, &parity, &keypair) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly32, &xonly) == 1);
    secp256k1_fuzz_check_schnorrsig_empty_message(ctx, &keypair, &xonly);
    memcpy(normalized_seckey, seckey, sizeof(normalized_seckey));
    if (parity) {
        FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, normalized_seckey) == 1);
    }
    nonce_data.expected_key32 = normalized_seckey;
    nonce_data.expected_xonly_pk32 = xonly32;
    nonce_data.expected_msg = msg32;
    nonce_data.expected_msglen = sizeof(msg32);
    secp256k1_fuzz_check_schnorrsig_nonce_reference(input, msglen, seckey, xonly32, aux32);
    secp256k1_fuzz_check_schnorrsig_nonce_reference(input, msglen, seckey, xonly32, NULL);
    secp256k1_fuzz_check_schnorrsig_custom_nonce_tag(input, msglen, seckey, xonly32, custom_algo, custom_algolen, aux32);
    secp256k1_fuzz_check_schnorrsig_custom_nonce_tag(input, msglen, seckey, xonly32, custom_algo, custom_algolen, NULL);
    secp256k1_fuzz_check_schnorrsig_custom_nonce_tag_boundaries(input, size, seckey, xonly32, aux32);
    secp256k1_fuzz_check_bip340_nonce_failure_cleanup(input, msglen, seckey, xonly32, aux32);
    memcpy(negated_seckey, seckey, sizeof(negated_seckey));
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, negated_seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &negated_keypair, negated_seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &negated_xonly, &negated_parity, &negated_keypair) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &negated_xonly) == 0);
    FUZZ_CHECK(parity != negated_parity);
    secp256k1_fuzz_valid_seckey32(ctx, other_seckey, input, size, 149);
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &other_keypair, other_seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &other_xonly, NULL, &other_keypair) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, other_xonly32, &other_xonly) == 1);
    secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_schnorrsig_sha256_compression);
    secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls = 0;
    secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sig64, msg32, &keypair, aux32) == 1);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_nonce_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls != 0);
    secp256k1_fuzz_check_schnorrsig_deprecated_sign(ctx, &keypair);
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sig64_negated, msg32, &negated_keypair, aux32) == 1);
    FUZZ_CHECK(memcmp(sig64, sig64_negated, sizeof(sig64)) == 0);
    secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), &xonly) == 1);
    secp256k1_fuzz_check_schnorrsig_verify_reference(ctx, sig64, msg32, sizeof(msg32), xonly32, &xonly);
    FUZZ_CHECK(secp256k1_fuzz_schnorrsig_challenge_sha256_compression_calls != 0);
    secp256k1_fuzz_check_schnorrsig_signature_equation(ctx, sig64, msg32, sizeof(msg32), xonly32);
    secp256k1_fuzz_check_schnorrsig_rx_overflow(ctx, sig64, msg32, sizeof(msg32), &xonly);
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
    secp256k1_fuzz_check_schnorrsig_fixed_nonce_equation(ctx, msg32, &keypair);
    secp256k1_fuzz_check_schnorrsig_odd_nonce_rejection(ctx);
    if (size == sizeof("schnorrsig-infinity-rejection\n") - 1
        && memcmp(input, "schnorrsig-infinity-rejection\n", sizeof("schnorrsig-infinity-rejection\n") - 1) == 0) {
        secp256k1_fuzz_check_schnorrsig_infinity_rejection(ctx);
    }
    secp256k1_fuzz_check_schnorrsig_generator_equation(ctx, input, size);
    secp256k1_fuzz_check_core_tapscript_schnorr_composition(ctx, input, size, sig64, xonly32, msg32);
    secp256k1_fuzz_check_core_taproot_signing_composition(ctx, input, size);
    secp256k1_fuzz_check_schnorrsig_s_order_boundary(ctx, input, size);
    secp256k1_fuzz_check_schnorrsig_invalid_pubkey_verify(ctx, sig64, msg32, sizeof(msg32));
    secp256k1_fuzz_check_schnorrsig_extraparams_magic(ctx, msg32, &keypair);
    secp256k1_fuzz_check_schnorrsig_keypair_consistency(ctx, msg32, &keypair, &other_keypair, aux32);
    secp256k1_fuzz_check_schnorrsig_impossible_msglen(ctx, sig64, input, &keypair, &xonly, aux32);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32) - 1, &xonly) == 0);
    if (secp256k1_xonly_pubkey_cmp(ctx, &xonly, &other_xonly) != 0) {
        FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), &other_xonly) == 0);
        secp256k1_fuzz_check_schnorrsig_verify_reference(ctx, sig64, msg32, sizeof(msg32), other_xonly32, &other_xonly);
    }

    memcpy(sig64_bad, sig64, sizeof(sig64_bad));
    memset(sig64_bad + 32, 0xFF, 32);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_bad, msg32, sizeof(msg32), &xonly) == 0);
    secp256k1_fuzz_check_schnorrsig_verify_reference(ctx, sig64_bad, msg32, sizeof(msg32), xonly32, &xonly);

    flip_index = (size_t)(secp256k1_fuzz_byte(input, size, 141) & 63u);
    flip_mask = (unsigned char)(secp256k1_fuzz_byte(input, size, 143) | 1u);
    memcpy(sig64_bad, sig64, sizeof(sig64_bad));
    sig64_bad[flip_index] ^= flip_mask;
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_bad, msg32, sizeof(msg32), &xonly) == 0);
    secp256k1_fuzz_check_schnorrsig_verify_reference(ctx, sig64_bad, msg32, sizeof(msg32), xonly32, &xonly);

    memcpy(msg32_bad, msg32, sizeof(msg32_bad));
    msg32_bad[flip_index & 31u] ^= flip_mask;
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32_bad, sizeof(msg32_bad), &xonly) == 0);
    secp256k1_fuzz_check_schnorrsig_verify_reference(ctx, sig64, msg32_bad, sizeof(msg32_bad), xonly32, &xonly);

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
    secp256k1_fuzz_check_schnorrsig_verify_reference(ctx, sig64_custom, input, msglen, xonly32, &xonly);
    secp256k1_fuzz_check_schnorrsig_signature_equation(ctx, sig64_custom, input, msglen, xonly32);
    secp256k1_fuzz_check_schnorrsig_rx_overflow(ctx, sig64_custom, input, msglen, &xonly);
    secp256k1_fuzz_check_schnorrsig_nonce_overflow(ctx, input, msglen, &keypair, &xonly);
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_explicit_bip340, input, msglen, &keypair, &explicit_bip340_extraparams) == 1);
    FUZZ_CHECK(memcmp(sig64_explicit_bip340, sig64_custom, sizeof(sig64_explicit_bip340)) == 0);
    nonce_data.expected_msg = input;
    nonce_data.expected_msglen = msglen;
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_checked, input, msglen, &keypair, &checked_extraparams) == 1);
    FUZZ_CHECK(nonce_data.calls == 2);
    FUZZ_CHECK(memcmp(sig64_checked, sig64_custom, sizeof(sig64_checked)) == 0);
    secp256k1_fuzz_check_schnorrsig_sign_failure_cleanup(ctx, input, msglen, &keypair, aux32);
    secp256k1_fuzz_check_schnorrsig_sign_precondition_cleanup(ctx, msg32, &keypair, aux32);
    wrong_msglen = msglen == 0 ? 1 : msglen - 1;
    FUZZ_CHECK(wrong_msglen != msglen);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_custom, input, wrong_msglen, &xonly) == 0);
    secp256k1_fuzz_check_schnorrsig_verify_reference(ctx, sig64_custom, input, wrong_msglen, xonly32, &xonly);
    if (secp256k1_xonly_pubkey_cmp(ctx, &xonly, &other_xonly) != 0) {
        FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_custom, input, msglen, &other_xonly) == 0);
        secp256k1_fuzz_check_schnorrsig_verify_reference(ctx, sig64_custom, input, msglen, other_xonly32, &other_xonly);
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
