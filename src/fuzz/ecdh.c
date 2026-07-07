/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

#ifdef ENABLE_MODULE_ECDH
static int fuzz_ecdh_hash_passthrough(unsigned char *output, const unsigned char *x32, const unsigned char *y32, void *data) {
    (void)data;
    memcpy(output, x32, 32);
    memcpy(output + 32, y32, 32);
    return 1;
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_ECDH
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 51);
    unsigned char seckey_a[32];
    unsigned char seckey_b[32];
    secp256k1_pubkey pubkey_a;
    secp256k1_pubkey pubkey_b;
    unsigned char shared_ab[64];
    unsigned char shared_ba[64];
    unsigned char default_ab[32];
    unsigned char default_ba[32];

    secp256k1_fuzz_valid_seckey32(ctx, seckey_a, input, size, 53);
    secp256k1_fuzz_valid_seckey32(ctx, seckey_b, input, size, 59);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_a, seckey_a) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_b, seckey_b) == 1);

    FUZZ_CHECK(secp256k1_ecdh(ctx, default_ab, &pubkey_b, seckey_a, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, default_ba, &pubkey_a, seckey_b, NULL, NULL) == 1);
    FUZZ_CHECK(memcmp(default_ab, default_ba, sizeof(default_ab)) == 0);

    FUZZ_CHECK(secp256k1_ecdh(ctx, shared_ab, &pubkey_b, seckey_a, fuzz_ecdh_hash_passthrough, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdh(ctx, shared_ba, &pubkey_a, seckey_b, fuzz_ecdh_hash_passthrough, NULL) == 1);
    FUZZ_CHECK(memcmp(shared_ab, shared_ba, sizeof(shared_ab)) == 0);

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
