/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#if defined(ENABLE_MODULE_EXTRAKEYS) && defined(ENABLE_MODULE_SCHNORRSIG)
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 121);
    unsigned char seckey[32];
    unsigned char aux32[32];
    unsigned char msg32[32];
    unsigned char sig64[64];
    unsigned char sig64_custom[64];
    unsigned char sig64_null_aux[64];
    unsigned char sig64_bad[64];
    unsigned char other_seckey[32];
    unsigned char msg32_bad[32];
    secp256k1_keypair keypair;
    secp256k1_keypair other_keypair;
    secp256k1_xonly_pubkey xonly;
    secp256k1_xonly_pubkey other_xonly;
    secp256k1_schnorrsig_extraparams extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    secp256k1_schnorrsig_extraparams null_extraparams = SECP256K1_SCHNORRSIG_EXTRAPARAMS_INIT;
    size_t msglen;
    size_t wrong_msglen;
    size_t flip_index;
    unsigned char flip_mask;

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 127);
    secp256k1_fuzz_derive(aux32, sizeof(aux32), input, size, 131);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 137);
    msglen = size == 0 ? 0 : (size_t)(secp256k1_fuzz_byte(input, size, 139) % (size + 1));

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly, NULL, &keypair) == 1);
    secp256k1_fuzz_valid_seckey32(ctx, other_seckey, input, size, 149);
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &other_keypair, other_seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &other_xonly, NULL, &other_keypair) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_sign32(ctx, sig64, msg32, &keypair, aux32) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64, msg32, sizeof(msg32), &xonly) == 1);
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
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_custom, msg32, sizeof(msg32), &keypair, &null_extraparams) == 1);
    FUZZ_CHECK(memcmp(sig64_null_aux, sig64_custom, sizeof(sig64_null_aux)) == 0);

    extraparams.ndata = aux32;
    FUZZ_CHECK(secp256k1_schnorrsig_sign_custom(ctx, sig64_custom, input, msglen, &keypair, &extraparams) == 1);
    FUZZ_CHECK(secp256k1_schnorrsig_verify(ctx, sig64_custom, input, msglen, &xonly) == 1);
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
