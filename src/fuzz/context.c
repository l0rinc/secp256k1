/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"

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

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    secp256k1_context *clone;
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

    FUZZ_CHECK(ctx != NULL);
    secp256k1_fuzz_derive(seed32, sizeof(seed32), input, size, 31);
    secp256k1_fuzz_derive(reset_seed32, sizeof(reset_seed32), input, size, 37);
    FUZZ_CHECK(secp256k1_context_randomize(ctx, seed32) == 1);
    clone = secp256k1_context_clone(ctx);
    FUZZ_CHECK(clone != NULL);
    FUZZ_CHECK(secp256k1_context_randomize(ctx, reset_seed32) == 1);

    taglen = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 47) % (size + 1));
    msglen = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 53) % (size + 1));
    tag_offset = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 59) % (size - taglen + 1));
    msg_offset = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 61) % (size - msglen + 1));
    secp256k1_fuzz_check_tagged_sha256(ctx, clone, input, 0, input, 0);
    secp256k1_fuzz_check_tagged_sha256(ctx, clone, input, size, input, size);
    secp256k1_fuzz_check_tagged_sha256(ctx, clone, input + tag_offset, taglen, input + msg_offset, msglen);

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

    FUZZ_CHECK(secp256k1_context_randomize(ctx, NULL) == 1);
    secp256k1_fuzz_check_tagged_sha256(ctx, clone, input + tag_offset, taglen, input + msg_offset, msglen);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_clone, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &pubkey_clone) == 0);

    secp256k1_context_destroy(clone);
    secp256k1_context_destroy(ctx);
    return 0;
}
