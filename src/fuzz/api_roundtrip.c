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
    secp256k1_pubkey parsed_pubkey;
    secp256k1_ecdsa_signature sig;
    secp256k1_ecdsa_signature parsed_sig;
    unsigned char seckey[32];
    unsigned char msg32[32];
    int parsed_compact;
    int parsed_der;

    if (secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, input, size)) {
        secp256k1_fuzz_check_pubkey_roundtrip(ctx, &parsed_pubkey);
    }

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 11);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 17);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    secp256k1_fuzz_check_pubkey_roundtrip(ctx, &pubkey);

    FUZZ_CHECK(secp256k1_ecdsa_sign(ctx, &sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &sig, msg32, &pubkey) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, NULL, &sig) == 0);
    secp256k1_fuzz_check_signature_roundtrip(ctx, &sig);

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
