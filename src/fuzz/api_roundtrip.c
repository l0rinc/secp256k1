/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 1);
    secp256k1_pubkey pubkey;
    secp256k1_pubkey pubkey_neg;
    secp256k1_pubkey pubkey_neg_from_seckey;
    secp256k1_pubkey parsed_pubkey;
    secp256k1_pubkey sort_pubkeys[4];
    secp256k1_ecdsa_signature sig;
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char seckey[32];
    unsigned char seckey_neg[32];
    unsigned char sort_seckey[32];
    unsigned char zero_compact[64] = { 0 };
    unsigned char msg32[32];
    const secp256k1_pubkey *sorted_pubkeys[4];
    const secp256k1_pubkey *permuted_pubkeys[4];
    int parsed_compact;
    int parsed_der;
    size_t i;

    if (secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, input, size)) {
        secp256k1_fuzz_check_pubkey_roundtrip(ctx, &parsed_pubkey);
    }

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 11);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 17);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &pubkey);

    memcpy(seckey_neg, seckey, sizeof(seckey_neg));
    pubkey_neg = pubkey;
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, seckey_neg) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &pubkey_neg) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_neg_from_seckey, seckey_neg) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey_neg, &pubkey_neg_from_seckey) == 0);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &pubkey_neg);
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, seckey_neg) == 1);
    FUZZ_CHECK(memcmp(seckey, seckey_neg, sizeof(seckey)) == 0);
    FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &pubkey_neg) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &pubkey_neg) == 0);

    sort_pubkeys[0] = pubkey;
    sort_pubkeys[1] = pubkey_neg_from_seckey;
    secp256k1_fuzz_valid_seckey32(ctx, sort_seckey, input, size, 23);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &sort_pubkeys[2], sort_seckey) == 1);
    secp256k1_fuzz_valid_seckey32(ctx, sort_seckey, input, size, 29);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &sort_pubkeys[3], sort_seckey) == 1);
    sorted_pubkeys[0] = &sort_pubkeys[0];
    sorted_pubkeys[1] = &sort_pubkeys[1];
    sorted_pubkeys[2] = &sort_pubkeys[2];
    sorted_pubkeys[3] = &sort_pubkeys[3];
    permuted_pubkeys[0] = &sort_pubkeys[2];
    permuted_pubkeys[1] = &sort_pubkeys[0];
    permuted_pubkeys[2] = &sort_pubkeys[3];
    permuted_pubkeys[3] = &sort_pubkeys[1];
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, sorted_pubkeys, 4) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_sort(ctx, permuted_pubkeys, 4) == 1);
    for (i = 1; i < 4; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, sorted_pubkeys[i - 1], sorted_pubkeys[i]) <= 0);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, permuted_pubkeys[i - 1], permuted_pubkeys[i]) <= 0);
    }
    for (i = 0; i < 4; i++) {
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, sorted_pubkeys[i], permuted_pubkeys[i]) == 0);
    }

    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &sig, msg32, &pubkey) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, &sig) == 0);
    secp256k1_fuzz_check_signature_roundtrip(ctx, &sig);

    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &parsed_sig, zero_compact) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, zero_compact, &parsed_sig) == 1);
    FUZZ_CHECK(memcmp(zero_compact, secp256k1_fuzz_scalar_zero, sizeof(secp256k1_fuzz_scalar_zero)) == 0);
    FUZZ_CHECK(memcmp(zero_compact + 32, secp256k1_fuzz_scalar_zero, sizeof(secp256k1_fuzz_scalar_zero)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, &pubkey) == 0);

    parsed_compact = 0;
    if (size >= 64) {
        parsed_compact = secp256k1_ecdsa_signature_parse_compact(ctx, &parsed_sig, input);
        secp256k1_fuzz_check_signature_roundtrip(ctx, &parsed_sig);
        if (!parsed_compact) {
            FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, &pubkey) == 0);
        }
    }

    parsed_der = secp256k1_ecdsa_signature_parse_der(ctx, &parsed_sig, input, size);
    secp256k1_fuzz_check_signature_roundtrip(ctx, &parsed_sig);
    if (!parsed_der) {
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &parsed_sig, msg32, &pubkey) == 0);
    }

    (void)parsed_compact;
    secp256k1_context_destroy(ctx);
    return 0;
}
