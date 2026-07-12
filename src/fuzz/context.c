/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"
#include "secp256k1_preallocated.h"

static size_t secp256k1_fuzz_sha256_compression_calls = 0;

#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
static unsigned int secp256k1_fuzz_default_illegal_calls = 0;

void secp256k1_default_illegal_callback_fn(const char *message, void *data) {
    (void)data;
    FUZZ_CHECK(message != NULL);
    secp256k1_fuzz_default_illegal_calls++;
}

void secp256k1_default_error_callback_fn(const char *message, void *data) {
    (void)data;
    FUZZ_CHECK(message != NULL);
    abort();
}
#endif

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_context_callback_data;

static void secp256k1_fuzz_context_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_context_callback_data *callback_data = (secp256k1_fuzz_context_callback_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(callback_data != NULL);
    FUZZ_CHECK(callback_data->self == callback_data);
    callback_data->calls++;
}

static void secp256k1_fuzz_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    secp256k1_fuzz_sha256_compression_calls += n_blocks;
    secp256k1_sha256_transform(state, blocks64, n_blocks);
}

static void secp256k1_fuzz_invalid_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    (void)state;
    (void)blocks64;
    (void)n_blocks;
}

static void secp256k1_fuzz_tagged_sha256_reference(unsigned char *hash32, const unsigned char *tag, size_t taglen, const unsigned char *msg, size_t msglen) {
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
    secp256k1_sha256_finalize(&hash_ctx, &sha, hash32);
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

static void secp256k1_fuzz_check_context_null_prealloc(void) {
#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
    secp256k1_context *(*create_fn)(void *, unsigned int) = secp256k1_context_preallocated_create;
    unsigned int calls = secp256k1_fuzz_default_illegal_calls;

    FUZZ_CHECK(create_fn(NULL, SECP256K1_CONTEXT_NONE) == NULL);
    FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == calls + 1);
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
    size_t taglen;
    size_t msglen;
    size_t tag_offset;
    size_t msg_offset;
    size_t prealloc_size;
    void *prealloc_mem;
    void *prealloc_clone_mem;
    void *prealloc_hash_clone_mem;

    secp256k1_fuzz_check_context_null_prealloc();
    FUZZ_CHECK(ctx != NULL);
    secp256k1_fuzz_derive(seed32, sizeof(seed32), input, size, 31);
    secp256k1_fuzz_derive(reset_seed32, sizeof(reset_seed32), input, size, 37);
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
    secp256k1_context_set_sha256_compression(ctx, NULL);
    secp256k1_fuzz_check_tagged_sha256_compression(ctx, input + tag_offset, taglen, input + msg_offset, msglen, 0);
    secp256k1_fuzz_check_tagged_sha256_compression(hash_clone, input, size, input, size, 1);
    secp256k1_fuzz_check_tagged_sha256_compression(prealloc_hash_clone, input, size, input, size, 1);
    secp256k1_context_preallocated_destroy(prealloc_hash_clone);
    free(prealloc_hash_clone_mem);
    secp256k1_context_destroy(hash_clone);

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 41);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 43);

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(clone, &pubkey_clone, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &pubkey_clone) == 0);

    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_sign(clone, &sig_clone, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(clone, compact_clone, &sig_clone) == 1);
    FUZZ_CHECK(memcmp(compact, compact_clone, sizeof(compact)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(secp256k1_context_static, &sig, msg32, &pubkey) == 1);
    secp256k1_fuzz_check_context_ecdsa_equivalence(ctx, prealloc_ctx, msg32, seckey);
    secp256k1_fuzz_check_context_ecdsa_equivalence(ctx, prealloc_clone, msg32, seckey);

    FUZZ_CHECK(secp256k1_context_randomize(ctx, NULL) == 1);
    secp256k1_fuzz_check_tagged_sha256(ctx, clone, input + tag_offset, taglen, input + msg_offset, msglen);
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
