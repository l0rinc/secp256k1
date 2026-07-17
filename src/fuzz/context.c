/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"
#include "sha256_reference.h"
#include "secp256k1_preallocated.h"

static size_t secp256k1_fuzz_sha256_compression_calls = 0;
static size_t secp256k1_fuzz_sha256_compression_max_blocks = 0;

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_context_callback_data;

typedef int (*secp256k1_fuzz_tagged_sha256_fn)(
    const secp256k1_context *ctx,
    unsigned char *hash32,
    const unsigned char *tag,
    size_t taglen,
    const unsigned char *msg,
    size_t msglen
);

static void secp256k1_fuzz_context_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_context_callback_data *callback_data = (secp256k1_fuzz_context_callback_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(callback_data != NULL);
    FUZZ_CHECK(callback_data->self == callback_data);
    callback_data->calls++;
}

static void secp256k1_fuzz_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    secp256k1_fuzz_sha256_compression_calls += n_blocks;
    if (n_blocks > secp256k1_fuzz_sha256_compression_max_blocks) {
        secp256k1_fuzz_sha256_compression_max_blocks = n_blocks;
    }
    secp256k1_sha256_transform(state, blocks64, n_blocks);
}

static void secp256k1_fuzz_invalid_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    (void)state;
    (void)blocks64;
    (void)n_blocks;
}

static void secp256k1_fuzz_tagged_sha256_reference(unsigned char *hash32, const unsigned char *tag, size_t taglen, const unsigned char *msg, size_t msglen) {
    unsigned char taghash[32];
    unsigned char *tagged_msg;

    FUZZ_CHECK(msglen <= SIZE_MAX - 2 * sizeof(taghash));
    tagged_msg = (unsigned char *)malloc(2 * sizeof(taghash) + msglen);
    FUZZ_CHECK(tagged_msg != NULL);
    secp256k1_fuzz_sha256_standalone(taghash, tag, taglen);
    memcpy(tagged_msg, taghash, sizeof(taghash));
    memcpy(tagged_msg + sizeof(taghash), taghash, sizeof(taghash));
    if (msglen != 0) {
        memcpy(tagged_msg + 2 * sizeof(taghash), msg, msglen);
    }
    secp256k1_fuzz_sha256_standalone(hash32, tagged_msg, 2 * sizeof(taghash) + msglen);
    memset(taghash, 0, sizeof(taghash));
    memset(tagged_msg, 0, 2 * sizeof(taghash) + msglen);
    free(tagged_msg);
}

static void secp256k1_fuzz_check_tagged_sha256(const secp256k1_context *ctx, const secp256k1_context *clone, const unsigned char *tag, size_t taglen, const unsigned char *msg, size_t msglen) {
    unsigned char expected[32];
    unsigned char hash32[32];
    unsigned char hash32_clone[32];
    unsigned char hash32_static[32];

    secp256k1_fuzz_tagged_sha256_reference(expected, tag, taglen, msg, msglen);
    memset(hash32, 0xA5, sizeof(hash32));
    memset(hash32_clone, 0x5A, sizeof(hash32_clone));
    memset(hash32_static, 0x3C, sizeof(hash32_static));
    FUZZ_CHECK(secp256k1_tagged_sha256(ctx, hash32, tag, taglen, msg, msglen) == 1);
    FUZZ_CHECK(secp256k1_tagged_sha256(clone, hash32_clone, tag, taglen, msg, msglen) == 1);
    FUZZ_CHECK(secp256k1_tagged_sha256(secp256k1_context_static, hash32_static, tag, taglen, msg, msglen) == 1);
    FUZZ_CHECK(memcmp(hash32, expected, sizeof(hash32)) == 0);
    FUZZ_CHECK(memcmp(hash32_clone, expected, sizeof(hash32_clone)) == 0);
    FUZZ_CHECK(memcmp(hash32_static, expected, sizeof(hash32_static)) == 0);
}

static void secp256k1_fuzz_check_tagged_sha256_compression(const secp256k1_context *ctx, const unsigned char *tag, size_t taglen, const unsigned char *msg, size_t msglen, int expect_custom_compression) {
    unsigned char expected[32];
    unsigned char hash32[32];

    secp256k1_fuzz_tagged_sha256_reference(expected, tag, taglen, msg, msglen);
    memset(hash32, 0xA5, sizeof(hash32));
    secp256k1_fuzz_sha256_compression_calls = 0;
    secp256k1_fuzz_sha256_compression_max_blocks = 0;
    FUZZ_CHECK(secp256k1_tagged_sha256(ctx, hash32, tag, taglen, msg, msglen) == 1);
    FUZZ_CHECK(memcmp(hash32, expected, sizeof(hash32)) == 0);
    FUZZ_CHECK((secp256k1_fuzz_sha256_compression_calls != 0) == expect_custom_compression);
}

static void secp256k1_fuzz_check_sha256_reject_keeps_backend(secp256k1_context *ctx, const unsigned char *tag, size_t taglen, const unsigned char *msg, size_t msglen) {
    secp256k1_fuzz_context_callback_data callback_data;
    unsigned int calls;

    callback_data.self = &callback_data;
    callback_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_context_illegal_callback, &callback_data);

    calls = callback_data.calls;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_invalid_sha256_compression);
    FUZZ_CHECK(callback_data.calls == calls + 1);
    secp256k1_fuzz_check_tagged_sha256_compression(ctx, tag, taglen, msg, msglen, 1);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_tagged_sha256_impossible_lengths(secp256k1_context *ctx, const unsigned char *tag, const unsigned char *msg) {
#if SIZE_MAX > 0xffffffff
    secp256k1_fuzz_context_callback_data callback_data;
    unsigned char hash32[32];
    unsigned char zero32[32] = { 0 };
    unsigned int calls;

    callback_data.self = &callback_data;
    callback_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_context_illegal_callback, &callback_data);
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_sha256_compression);

    memset(hash32, 0xA5, sizeof(hash32));
    calls = callback_data.calls;
    secp256k1_fuzz_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_tagged_sha256(ctx, hash32, tag, (size_t)SECP256K1_SHA256_MAX_SIZE, msg, 0) == 0);
    FUZZ_CHECK(callback_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(hash32, zero32, sizeof(hash32)) == 0);
    FUZZ_CHECK(secp256k1_fuzz_sha256_compression_calls == 0);

    memset(hash32, 0x5A, sizeof(hash32));
    calls = callback_data.calls;
    secp256k1_fuzz_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_tagged_sha256(ctx, hash32, tag, 0, msg, (size_t)SECP256K1_SHA256_MAX_SIZE - 64) == 0);
    FUZZ_CHECK(callback_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(hash32, zero32, sizeof(hash32)) == 0);
    FUZZ_CHECK(secp256k1_fuzz_sha256_compression_calls == 0);

    secp256k1_context_set_sha256_compression(ctx, NULL);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
#else
    (void)ctx;
    (void)tag;
    (void)msg;
#endif
}

static void secp256k1_fuzz_check_tagged_sha256_null_inputs(secp256k1_context *ctx, const unsigned char *tag, const unsigned char *msg) {
    secp256k1_fuzz_context_callback_data callback_data;
    secp256k1_fuzz_tagged_sha256_fn tagged_sha256_fn = secp256k1_tagged_sha256;
    unsigned char hash32[32];
    unsigned char zero32[32] = { 0 };
    const unsigned char *null_input = NULL;
    unsigned int calls;

    callback_data.self = &callback_data;
    callback_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_context_illegal_callback, &callback_data);

    memset(hash32, 0xA5, sizeof(hash32));
    calls = callback_data.calls;
    FUZZ_CHECK(tagged_sha256_fn(ctx, hash32, null_input, 0, msg, 0) == 0);
    FUZZ_CHECK(callback_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(hash32, zero32, sizeof(hash32)) == 0);

    memset(hash32, 0x5A, sizeof(hash32));
    calls = callback_data.calls;
    FUZZ_CHECK(tagged_sha256_fn(ctx, hash32, tag, 0, null_input, 0) == 0);
    FUZZ_CHECK(callback_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(hash32, zero32, sizeof(hash32)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_context_ecdsa_equivalence(const secp256k1_context *ctx, const secp256k1_context *other, const unsigned char *msg32, const unsigned char *seckey) {
    secp256k1_pubkey pubkey;
    secp256k1_pubkey pubkey_other;
    secp256k1_ecdsa_signature sig;
    secp256k1_ecdsa_signature sig_other;
    unsigned char compact[64];
    unsigned char compact_other[64];

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(other, &pubkey_other, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &pubkey_other) == 0);

    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_sign(other, &sig_other, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(other, compact_other, &sig_other) == 1);
    FUZZ_CHECK(memcmp(compact, compact_other, sizeof(compact)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(secp256k1_context_static, &sig, msg32, &pubkey) == 1);
}

static void secp256k1_fuzz_check_sha256_secret_operations(const secp256k1_context *custom_ctx, const secp256k1_context *custom_clone, const secp256k1_context *custom_prealloc_clone, const unsigned char *input, size_t size, const unsigned char *seed32, const unsigned char *msg32, const unsigned char *seckey) {
    static const unsigned char trigger[] = "sha256 secret operations\n";
    const secp256k1_context *custom_contexts[3];
    secp256k1_context *default_ctx;
    secp256k1_pubkey default_pubkey;
    secp256k1_ecdsa_signature default_sig;
    unsigned char default_serialized[33];
    unsigned char default_compact[64];
    size_t default_serialized_len = sizeof(default_serialized);
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    custom_contexts[0] = custom_ctx;
    custom_contexts[1] = custom_clone;
    custom_contexts[2] = custom_prealloc_clone;

    /* A valid replacement compression function must preserve secret-dependent
     * API results, even though the context's blinding state is different. */
    default_ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    FUZZ_CHECK(default_ctx != NULL);
    FUZZ_CHECK(secp256k1_context_randomize(default_ctx, seed32) == 1);

    FUZZ_CHECK(secp256k1_ec_pubkey_create(default_ctx, &default_pubkey, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(default_ctx, default_serialized, &default_serialized_len, &default_pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(default_serialized_len == sizeof(default_serialized));
    FUZZ_CHECK(secp256k1_ecdsa_sign(default_ctx, &default_sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(default_ctx, default_compact, &default_sig) == 1);

#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_SCHNORRSIG)
    {
        secp256k1_keypair default_keypair;
        unsigned char default_schnorr[64];

        FUZZ_CHECK(secp256k1_keypair_create(default_ctx, &default_keypair, seckey) == 1);
        FUZZ_CHECK(secp256k1_schnorrsig_sign32(default_ctx, default_schnorr, msg32, &default_keypair, NULL) == 1);

        for (i = 0; i < sizeof(custom_contexts) / sizeof(custom_contexts[0]); ++i) {
            secp256k1_keypair custom_keypair;
            unsigned char custom_schnorr[64];
            size_t calls;

            calls = secp256k1_fuzz_sha256_compression_calls;
            FUZZ_CHECK(secp256k1_keypair_create(custom_contexts[i], &custom_keypair, seckey) == 1);
            FUZZ_CHECK(secp256k1_schnorrsig_sign32(custom_contexts[i], custom_schnorr, msg32, &custom_keypair, NULL) == 1);
            FUZZ_CHECK(secp256k1_fuzz_sha256_compression_calls > calls);
            FUZZ_CHECK(memcmp(custom_schnorr, default_schnorr, sizeof(custom_schnorr)) == 0);
        }
    }
#endif

    secp256k1_fuzz_sha256_compression_calls = 0;
    for (i = 0; i < sizeof(custom_contexts) / sizeof(custom_contexts[0]); ++i) {
        secp256k1_pubkey custom_pubkey;
        secp256k1_ecdsa_signature custom_sig;
        unsigned char custom_serialized[33];
        unsigned char custom_compact[64];
        size_t custom_serialized_len = sizeof(custom_serialized);
        size_t calls;

        calls = secp256k1_fuzz_sha256_compression_calls;
        FUZZ_CHECK(secp256k1_ec_pubkey_create(custom_contexts[i], &custom_pubkey, seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(custom_contexts[i], custom_serialized, &custom_serialized_len, &custom_pubkey, SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(custom_serialized_len == sizeof(custom_serialized));
        FUZZ_CHECK(memcmp(custom_serialized, default_serialized, sizeof(custom_serialized)) == 0);
        FUZZ_CHECK(secp256k1_ecdsa_sign(custom_contexts[i], &custom_sig, msg32, seckey, NULL, NULL) == 1);
        FUZZ_CHECK(secp256k1_fuzz_sha256_compression_calls > calls);
        FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(custom_contexts[i], custom_compact, &custom_sig) == 1);
        FUZZ_CHECK(memcmp(custom_compact, default_compact, sizeof(custom_compact)) == 0);
        FUZZ_CHECK(secp256k1_ecdsa_verify(secp256k1_context_static, &custom_sig, msg32, &custom_pubkey) == 1);
    }

    secp256k1_context_destroy(default_ctx);
}

static void secp256k1_fuzz_check_context_null_reset_signing(const secp256k1_context *ctx, const unsigned char *msg32, const unsigned char *seckey, const unsigned char *expected_compact) {
    secp256k1_ecdsa_signature reset_sig;
    unsigned char reset_compact[64];

    /* A NULL seed resets generator blinding to the initial state without
     * changing deterministic public API results. */
    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &reset_sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, reset_compact, &reset_sig) == 1);
    FUZZ_CHECK(memcmp(reset_compact, expected_compact, sizeof(reset_compact)) == 0);
}

#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_SCHNORRSIG)
static void secp256k1_fuzz_check_context_null_reset_schnorr(const secp256k1_context *ctx, const unsigned char *msg32, const unsigned char *seckey, const unsigned char *expected_sig64) {
    secp256k1_keypair keypair;
    unsigned char reset_sig64[64];

    /* Schnorr signing has its own x-only parity and response path; prove that
     * it observes the same documented reset invariant independently. */
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, reset_sig64, msg32, &keypair, NULL) == 1);
    FUZZ_CHECK(memcmp(reset_sig64, expected_sig64, sizeof(reset_sig64)) == 0);
}
#endif

static void secp256k1_fuzz_check_context_illegal_callback_clone(secp256k1_context *ctx, size_t prealloc_size) {
    secp256k1_fuzz_context_callback_data cloned_data;
    secp256k1_fuzz_context_callback_data original_data;
    secp256k1_context *callback_clone;
    secp256k1_context *callback_prealloc_clone;
    void *callback_prealloc_clone_mem;
    int (*seckey_verify_fn)(const secp256k1_context *, const unsigned char *) = secp256k1_ec_seckey_verify;

    cloned_data.self = &cloned_data;
    cloned_data.calls = 0;
    original_data.self = &original_data;
    original_data.calls = 0;

    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_context_illegal_callback, &cloned_data);
    callback_clone = secp256k1_context_clone(ctx);
    FUZZ_CHECK(callback_clone != NULL);
    callback_prealloc_clone_mem = malloc(prealloc_size);
    FUZZ_CHECK(callback_prealloc_clone_mem != NULL);
    callback_prealloc_clone = secp256k1_context_preallocated_clone(ctx, callback_prealloc_clone_mem);
    FUZZ_CHECK(callback_prealloc_clone != NULL);

    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_context_illegal_callback, &original_data);
    FUZZ_CHECK(seckey_verify_fn(callback_clone, NULL) == 0);
    FUZZ_CHECK(cloned_data.calls == 1);
    FUZZ_CHECK(original_data.calls == 0);

    FUZZ_CHECK(seckey_verify_fn(callback_prealloc_clone, NULL) == 0);
    FUZZ_CHECK(cloned_data.calls == 2);
    FUZZ_CHECK(original_data.calls == 0);

    FUZZ_CHECK(seckey_verify_fn(ctx, NULL) == 0);
    FUZZ_CHECK(cloned_data.calls == 2);
    FUZZ_CHECK(original_data.calls == 1);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
    secp256k1_context_set_illegal_callback(callback_clone, NULL, NULL);
    secp256k1_context_set_illegal_callback(callback_prealloc_clone, NULL, NULL);
    secp256k1_context_destroy(callback_clone);
    secp256k1_context_preallocated_destroy(callback_prealloc_clone);
    free(callback_prealloc_clone_mem);
}

static void secp256k1_fuzz_check_context_flag_matrix(const unsigned char *seed32, const unsigned char *msg32, const unsigned char *seckey) {
    static const unsigned int flags[] = {
        SECP256K1_CONTEXT_NONE,
        SECP256K1_CONTEXT_VERIFY,
        SECP256K1_CONTEXT_SIGN,
        SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY
    };
    unsigned char reference_pubkey[33];
    unsigned char reference_sig[64];
    size_t reference_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_NONE);
    size_t i;
    int have_reference = 0;

    /* Deprecated flags are documented aliases for the fully capable context.
     * Compare behavior through public serialization instead of inspecting the
     * implementation's randomized or callback-bearing state. */
    for (i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
        secp256k1_context *ctx;
        secp256k1_context *prealloc_ctx;
        secp256k1_context *clone;
        secp256k1_context *prealloc_clone;
        secp256k1_context *contexts[4];
        void *prealloc_mem;
        void *prealloc_clone_mem;
        size_t prealloc_size;
        size_t clone_size;
        size_t j;

        prealloc_size = secp256k1_context_preallocated_size(flags[i]);
        FUZZ_CHECK(prealloc_size == reference_size);
        ctx = secp256k1_context_create(flags[i]);
        FUZZ_CHECK(ctx != NULL);
        prealloc_mem = malloc(prealloc_size);
        FUZZ_CHECK(prealloc_mem != NULL);
        prealloc_ctx = secp256k1_context_preallocated_create(prealloc_mem, flags[i]);
        FUZZ_CHECK(prealloc_ctx != NULL);
        FUZZ_CHECK(secp256k1_context_randomize(ctx, seed32) == 1);
        FUZZ_CHECK(secp256k1_context_randomize(prealloc_ctx, seed32) == 1);

        clone_size = secp256k1_context_preallocated_clone_size(ctx);
        FUZZ_CHECK(clone_size == reference_size);
        FUZZ_CHECK(secp256k1_context_preallocated_clone_size(prealloc_ctx) == reference_size);
        clone = secp256k1_context_clone(ctx);
        FUZZ_CHECK(clone != NULL);
        prealloc_clone_mem = malloc(clone_size);
        FUZZ_CHECK(prealloc_clone_mem != NULL);
        prealloc_clone = secp256k1_context_preallocated_clone(ctx, prealloc_clone_mem);
        FUZZ_CHECK(prealloc_clone != NULL);

        contexts[0] = ctx;
        contexts[1] = prealloc_ctx;
        contexts[2] = clone;
        contexts[3] = prealloc_clone;
        for (j = 0; j < sizeof(contexts) / sizeof(contexts[0]); j++) {
            secp256k1_pubkey pubkey;
            unsigned char serialized[33];
            size_t serialized_len = sizeof(serialized);
            secp256k1_ecdsa_signature sig;
            unsigned char compact[64];

            FUZZ_CHECK(secp256k1_ec_pubkey_create(contexts[j], &pubkey, seckey) == 1);
            FUZZ_CHECK(secp256k1_ec_pubkey_serialize(contexts[j], serialized, &serialized_len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
            FUZZ_CHECK(serialized_len == sizeof(serialized));
            FUZZ_CHECK(secp256k1_ecdsa_sign(contexts[j], &sig, msg32, seckey, NULL, NULL) == 1);
            FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(contexts[j], compact, &sig) == 1);
            if (!have_reference) {
                memcpy(reference_pubkey, serialized, sizeof(reference_pubkey));
                memcpy(reference_sig, compact, sizeof(reference_sig));
                have_reference = 1;
            } else {
                FUZZ_CHECK(memcmp(serialized, reference_pubkey, sizeof(reference_pubkey)) == 0);
                FUZZ_CHECK(memcmp(compact, reference_sig, sizeof(reference_sig)) == 0);
            }
        }

        secp256k1_context_destroy(clone);
        secp256k1_context_preallocated_destroy(prealloc_clone);
        free(prealloc_clone_mem);
        secp256k1_context_preallocated_destroy(prealloc_ctx);
        free(prealloc_mem);
        secp256k1_context_destroy(ctx);
    }
}

static void secp256k1_fuzz_check_context_null_prealloc(void) {
#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
    secp256k1_context *(*create_fn)(void *, unsigned int) = secp256k1_context_preallocated_create;
    unsigned int calls = secp256k1_fuzz_default_illegal_calls;

    FUZZ_CHECK(create_fn(NULL, SECP256K1_CONTEXT_NONE) == NULL);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == calls + 1);
#endif
}

static void secp256k1_fuzz_check_context_invalid_flags(void) {
#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
    unsigned int calls = secp256k1_fuzz_default_illegal_calls;

    FUZZ_CHECK(secp256k1_context_preallocated_size(SECP256K1_FLAGS_TYPE_COMPRESSION) == 0);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == calls + 1);
#endif
}

static void secp256k1_fuzz_check_static_context_lifecycle(const unsigned char *seed32) {
#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
    /* The singleton is const in the public API. Cast only to exercise APIs
     * whose first contract check must reject it before any write. */
    secp256k1_context *static_ctx = (secp256k1_context *)secp256k1_context_static;
    unsigned int calls = secp256k1_fuzz_default_illegal_calls;
    secp256k1_context *clone;

    FUZZ_CHECK(secp256k1_context_randomize(static_ctx, seed32) == 0);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
    FUZZ_CHECK(secp256k1_context_randomize(static_ctx, NULL) == 0);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
    secp256k1_context_set_sha256_compression(static_ctx, NULL);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);

    clone = secp256k1_context_clone(static_ctx);
    FUZZ_CHECK(clone == NULL);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
    FUZZ_CHECK(secp256k1_context_preallocated_clone_size(static_ctx) == 0);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);

    secp256k1_context_set_illegal_callback(static_ctx, NULL, NULL);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
    secp256k1_context_set_error_callback(static_ctx, NULL, NULL);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
    secp256k1_context_destroy(static_ctx);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
    secp256k1_context_preallocated_destroy(static_ctx);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
#else
    (void)seed32;
#endif
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_context *clone;
    secp256k1_context *hash_clone;
    secp256k1_context *prealloc_ctx;
    secp256k1_context *prealloc_clone;
    secp256k1_context *prealloc_hash_clone;
    secp256k1_pubkey pubkey;
    secp256k1_pubkey pubkey_clone;
    secp256k1_ecdsa_signature sig;
    secp256k1_ecdsa_signature sig_clone;
    unsigned char seed32[32];
    unsigned char reset_seed32[32];
    unsigned char seckey[32];
    unsigned char msg32[32];
    unsigned char compact[64];
    unsigned char compact_clone[64];
#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_SCHNORRSIG)
    unsigned char schnorr_sig64[64];
#endif
    size_t taglen;
    size_t msglen;
    size_t tag_offset;
    size_t msg_offset;
    size_t prealloc_size;
    void *prealloc_mem;
    void *prealloc_clone_mem;
    void *prealloc_hash_clone_mem;

    secp256k1_fuzz_check_context_null_prealloc();
    secp256k1_fuzz_check_context_invalid_flags();
    FUZZ_CHECK(ctx != NULL);
    secp256k1_fuzz_derive(seed32, sizeof(seed32), input, size, 31);
    secp256k1_fuzz_derive(reset_seed32, sizeof(reset_seed32), input, size, 37);
    if (size == sizeof("static context lifecycle\n") - 1
            && memcmp(input, "static context lifecycle\n", sizeof("static context lifecycle\n") - 1) == 0) {
        secp256k1_fuzz_check_static_context_lifecycle(seed32);
    }
    FUZZ_CHECK(secp256k1_context_randomize(ctx, seed32) == 1);
    clone = secp256k1_context_clone(ctx);
    FUZZ_CHECK(clone != NULL);
    prealloc_size = secp256k1_context_preallocated_size(SECP256K1_CONTEXT_NONE);
    FUZZ_CHECK(prealloc_size != 0);
    FUZZ_CHECK(secp256k1_context_preallocated_clone_size(ctx) == prealloc_size);
    prealloc_mem = malloc(prealloc_size);
    FUZZ_CHECK(prealloc_mem != NULL);
    prealloc_ctx = secp256k1_context_preallocated_create(prealloc_mem, SECP256K1_CONTEXT_NONE);
    FUZZ_CHECK(prealloc_ctx != NULL);
    FUZZ_CHECK(secp256k1_context_randomize(prealloc_ctx, seed32) == 1);
    FUZZ_CHECK(secp256k1_context_preallocated_clone_size(prealloc_ctx) == prealloc_size);
    prealloc_clone_mem = malloc(prealloc_size);
    FUZZ_CHECK(prealloc_clone_mem != NULL);
    prealloc_clone = secp256k1_context_preallocated_clone(ctx, prealloc_clone_mem);
    FUZZ_CHECK(prealloc_clone != NULL);
    FUZZ_CHECK(secp256k1_context_randomize(ctx, reset_seed32) == 1);
    secp256k1_fuzz_check_context_illegal_callback_clone(ctx, prealloc_size);

    taglen = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 47) % (size + 1));
    msglen = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 53) % (size + 1));
    tag_offset = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 59) % (size - taglen + 1));
    msg_offset = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 61) % (size - msglen + 1));
    secp256k1_fuzz_check_tagged_sha256(ctx, clone, input, 0, input, 0);
    secp256k1_fuzz_check_tagged_sha256(ctx, clone, input, size, input, size);
    secp256k1_fuzz_check_tagged_sha256(ctx, clone, input + tag_offset, taglen, input + msg_offset, msglen);
    secp256k1_fuzz_check_tagged_sha256(prealloc_ctx, prealloc_clone, input + tag_offset, taglen, input + msg_offset, msglen);
    secp256k1_fuzz_check_tagged_sha256_impossible_lengths(ctx, input + tag_offset, input + msg_offset);
    secp256k1_fuzz_check_tagged_sha256_null_inputs(ctx, input, input);
    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 41);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 43);

    secp256k1_fuzz_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_sha256_compression);
    FUZZ_CHECK(secp256k1_fuzz_sha256_compression_calls != 0);
    hash_clone = secp256k1_context_clone(ctx);
    FUZZ_CHECK(hash_clone != NULL);
    prealloc_hash_clone_mem = malloc(prealloc_size);
    FUZZ_CHECK(prealloc_hash_clone_mem != NULL);
    prealloc_hash_clone = secp256k1_context_preallocated_clone(ctx, prealloc_hash_clone_mem);
    FUZZ_CHECK(prealloc_hash_clone != NULL);
    secp256k1_fuzz_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_context_randomize(ctx, seed32) == 1);
    FUZZ_CHECK(secp256k1_fuzz_sha256_compression_calls != 0);
    secp256k1_fuzz_check_tagged_sha256_compression(ctx, input, 0, input, 0, 1);
    secp256k1_fuzz_check_tagged_sha256_compression(hash_clone, input + tag_offset, taglen, input + msg_offset, msglen, 1);
    secp256k1_fuzz_check_tagged_sha256_compression(prealloc_hash_clone, input + tag_offset, taglen, input + msg_offset, msglen, 1);
    secp256k1_fuzz_check_sha256_reject_keeps_backend(ctx, input + tag_offset, taglen, input + msg_offset, msglen);
    secp256k1_fuzz_check_sha256_secret_operations(ctx, hash_clone, prealloc_hash_clone, input, size, seed32, msg32, seckey);
    secp256k1_context_set_sha256_compression(ctx, NULL);
    secp256k1_fuzz_check_tagged_sha256_compression(ctx, input + tag_offset, taglen, input + msg_offset, msglen, 0);
    secp256k1_fuzz_check_tagged_sha256_compression(hash_clone, input, size, input, size, 1);
    secp256k1_fuzz_check_tagged_sha256_compression(prealloc_hash_clone, input, size, input, size, 1);
    if (size >= 128) {
        FUZZ_CHECK(secp256k1_fuzz_sha256_compression_max_blocks > 1);
    }
    secp256k1_context_preallocated_destroy(prealloc_hash_clone);
    free(prealloc_hash_clone_mem);
    secp256k1_context_destroy(hash_clone);

    secp256k1_fuzz_check_context_flag_matrix(seed32, msg32, seckey);

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(clone, &pubkey_clone, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &pubkey_clone) == 0);

    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_sign(clone, &sig_clone, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(clone, compact_clone, &sig_clone) == 1);
    FUZZ_CHECK(memcmp(compact, compact_clone, sizeof(compact)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(secp256k1_context_static, &sig, msg32, &pubkey) == 1);
#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_SCHNORRSIG)
    {
        secp256k1_keypair keypair;
        FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey) == 1);
        FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, schnorr_sig64, msg32, &keypair, NULL) == 1);
    }
#endif
    secp256k1_fuzz_check_context_ecdsa_equivalence(ctx, prealloc_ctx, msg32, seckey);
    secp256k1_fuzz_check_context_ecdsa_equivalence(ctx, prealloc_clone, msg32, seckey);

    FUZZ_CHECK(secp256k1_context_randomize(ctx, NULL) == 1);
    secp256k1_fuzz_check_tagged_sha256(ctx, clone, input + tag_offset, taglen, input + msg_offset, msglen);
    secp256k1_fuzz_check_context_null_reset_signing(ctx, msg32, seckey, compact);
#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_SCHNORRSIG)
    secp256k1_fuzz_check_context_null_reset_schnorr(ctx, msg32, seckey, schnorr_sig64);
#endif
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_clone, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &pubkey_clone) == 0);

    secp256k1_context_destroy(clone);
    secp256k1_context_preallocated_destroy(prealloc_clone);
    free(prealloc_clone_mem);
    secp256k1_context_preallocated_destroy(prealloc_ctx);
    free(prealloc_mem);
    secp256k1_context_destroy(ctx);
    return 0;
}
