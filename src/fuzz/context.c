/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

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

    FUZZ_CHECK(ctx != NULL);
    secp256k1_fuzz_derive(seed32, sizeof(seed32), input, size, 31);
    secp256k1_fuzz_derive(reset_seed32, sizeof(reset_seed32), input, size, 37);
    FUZZ_CHECK(secp256k1_context_randomize(ctx, seed32) == 1);
    clone = secp256k1_context_clone(ctx);
    FUZZ_CHECK(clone != NULL);
    FUZZ_CHECK(secp256k1_context_randomize(ctx, reset_seed32) == 1);

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
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_clone, seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &pubkey_clone) == 0);

    secp256k1_context_destroy(clone);
    secp256k1_context_destroy(ctx);
    return 0;
}
