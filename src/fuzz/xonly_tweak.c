/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_EXTRAKEYS
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 71);
    unsigned char seckey[32];
    unsigned char tweak[32];
    unsigned char xonly32[32];
    unsigned char tweaked32[32];
    secp256k1_keypair keypair;
    secp256k1_keypair tweaked_keypair;
    secp256k1_pubkey pubkey;
    secp256k1_pubkey tweaked_pubkey;
    secp256k1_pubkey tweaked_keypair_pubkey;
    secp256k1_xonly_pubkey xonly;
    secp256k1_xonly_pubkey xonly_from_pubkey;
    secp256k1_xonly_pubkey reparsed;
    secp256k1_xonly_pubkey tweaked_xonly;
    int keypair_parity;
    int pubkey_parity;
    int tweaked_parity;
    int pub_tweak_ret;
    int keypair_tweak_ret;

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 73);
    secp256k1_fuzz_scalar32(tweak, input, size, 79);

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &pubkey, &keypair) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly, &keypair_parity, &keypair) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &xonly_from_pubkey, &pubkey_parity, &pubkey) == 1);
    FUZZ_CHECK(keypair_parity == pubkey_parity);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &xonly_from_pubkey) == 0);

    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly32, &xonly) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_parse(ctx, &reparsed, xonly32) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &reparsed) == 0);

    tweaked_keypair = keypair;
    pub_tweak_ret = secp256k1_xonly_pubkey_tweak_add(ctx, &tweaked_pubkey, &xonly, tweak);
    keypair_tweak_ret = secp256k1_keypair_xonly_tweak_add(ctx, &tweaked_keypair, tweak);
    FUZZ_CHECK(pub_tweak_ret == keypair_tweak_ret);
    if (pub_tweak_ret) {
        FUZZ_CHECK(secp256k1_keypair_pub(ctx, &tweaked_keypair_pubkey, &tweaked_keypair) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &tweaked_pubkey, &tweaked_keypair_pubkey) == 0);
        FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &tweaked_xonly, &tweaked_parity, &tweaked_pubkey) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, tweaked32, &tweaked_xonly) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, tweaked32, tweaked_parity, &xonly, tweak) == 1);
    }

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
