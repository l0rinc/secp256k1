/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_RECOVERY
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 91);
    unsigned char seckey[32];
    unsigned char msg32[32];
    unsigned char compact[64];
    unsigned char normal_compact[64];
    unsigned char zero_compact[64] = { 0 };
    secp256k1_pubkey pubkey;
    secp256k1_pubkey recovered_pubkey;
    secp256k1_ecdsa_signature normal_sig;
    secp256k1_ecdsa_signature normalized_sig;
    secp256k1_ecdsa_recoverable_signature recoverable_sig;
    secp256k1_ecdsa_recoverable_signature reparsed_sig;
    int recid;
    int alt_recid;
    int parsed;

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 97);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 101);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);

    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &recoverable_sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &recoverable_sig) == 1);
    FUZZ_CHECK(recid >= 0 && recid <= 3);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, compact, recid) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &reparsed_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normal_compact, &normal_sig) == 1);
    FUZZ_CHECK(memcmp(compact, normal_compact, sizeof(compact)) == 0);
    secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &normal_sig);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normalized_sig, msg32, &pubkey) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &reparsed_sig, msg32) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &recovered_pubkey) == 0);

    for (alt_recid = 0; alt_recid < 4; alt_recid++) {
        if (alt_recid == recid) {
            continue;
        }
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, compact, alt_recid) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &reparsed_sig) == 1);
        secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &normal_sig);
        FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normalized_sig, msg32, &pubkey) == 1);
        if (secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &reparsed_sig, msg32)) {
            FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &recovered_pubkey) != 0);
        }
    }

    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, zero_compact, 0) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &reparsed_sig, msg32) == 0);

    if (size >= 64) {
        recid = secp256k1_fuzz_byte(input, size, 109) & 3;
        parsed = secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, input, recid);
        if (parsed) {
            unsigned char compact2[64];
            int recid2;
            FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &reparsed_sig) == 1);
            FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &recoverable_sig, compact, recid) == 1);
            FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact2, &recid2, &recoverable_sig) == 1);
            FUZZ_CHECK(recid == recid2);
            FUZZ_CHECK(memcmp(compact, compact2, sizeof(compact)) == 0);
            FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &reparsed_sig) == 1);
            FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normal_compact, &normal_sig) == 1);
            FUZZ_CHECK(memcmp(compact, normal_compact, sizeof(compact)) == 0);
            secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &normal_sig);
            if (secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &reparsed_sig, msg32)) {
                FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normalized_sig, msg32, &recovered_pubkey) == 1);
            }
        }
    }

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
