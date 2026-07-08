/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "../hash_impl.h"

#ifdef ENABLE_MODULE_RECOVERY
static size_t secp256k1_fuzz_recovery_sha256_compression_calls = 0;

static void secp256k1_fuzz_recovery_sha256_compression(uint32_t *state, const unsigned char *blocks64, size_t n_blocks) {
    secp256k1_fuzz_recovery_sha256_compression_calls += n_blocks;
    secp256k1_sha256_transform(state, blocks64, n_blocks);
}

static int secp256k1_fuzz_scalar32_in_order(const unsigned char *input32) {
    return memcmp(input32, secp256k1_fuzz_scalar_order, 32) < 0;
}

static void secp256k1_fuzz_check_recoverable_parse_compact(const secp256k1_context *ctx, const unsigned char *input64, int recid) {
    secp256k1_ecdsa_recoverable_signature recoverable_sig;
    secp256k1_ecdsa_recoverable_signature reparsed_sig;
    secp256k1_ecdsa_signature normal_sig;
    unsigned char compact[64];
    unsigned char normal_compact[64];
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_recoverable_signature)] = { 0 };
    int expected_ret;
    int parse_ret;
    int serialized_recid;

    FUZZ_CHECK(recid >= 0 && recid <= 3);
    expected_ret = secp256k1_fuzz_scalar32_in_order(input64);
    expected_ret &= secp256k1_fuzz_scalar32_in_order(input64 + 32);
    memset(&recoverable_sig, 0xA5, sizeof(recoverable_sig));
    parse_ret = secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &recoverable_sig, input64, recid);
    FUZZ_CHECK(parse_ret == expected_ret);
    if (parse_ret) {
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &serialized_recid, &recoverable_sig) == 1);
        FUZZ_CHECK(serialized_recid == recid);
        FUZZ_CHECK(memcmp(compact, input64, sizeof(compact)) == 0);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, compact, serialized_recid) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &serialized_recid, &reparsed_sig) == 1);
        FUZZ_CHECK(serialized_recid == recid);
        FUZZ_CHECK(memcmp(compact, input64, sizeof(compact)) == 0);
        FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &recoverable_sig) == 1);
        FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normal_compact, &normal_sig) == 1);
        FUZZ_CHECK(memcmp(normal_compact, input64, sizeof(normal_compact)) == 0);
    } else {
        FUZZ_CHECK(memcmp(&recoverable_sig, zero_sig, sizeof(recoverable_sig)) == 0);
    }
}

static void secp256k1_fuzz_check_recover_failure_cleanup(const secp256k1_context *ctx, const secp256k1_ecdsa_recoverable_signature *sig, const unsigned char *msg32) {
    secp256k1_pubkey recovered_pubkey;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };

    memset(&recovered_pubkey, 0xA5, sizeof(recovered_pubkey));
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, sig, msg32) == 0);
    FUZZ_CHECK(memcmp(&recovered_pubkey, zero_pubkey, sizeof(recovered_pubkey)) == 0);
}

static void secp256k1_fuzz_check_sign_recoverable_failure_cleanup(const secp256k1_context *ctx, const unsigned char *msg32, const unsigned char *seckey32) {
    secp256k1_ecdsa_recoverable_signature sig;
    unsigned char zero_sig[sizeof(secp256k1_ecdsa_recoverable_signature)] = { 0 };

    memset(&sig, 0xA5, sizeof(sig));
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &sig, msg32, seckey32, NULL, NULL) == 0);
    FUZZ_CHECK(memcmp(&sig, zero_sig, sizeof(sig)) == 0);
}

static void secp256k1_fuzz_check_recoverable_high_s(const secp256k1_context *ctx, const secp256k1_ecdsa_recoverable_signature *sig, const unsigned char *msg32, const secp256k1_pubkey *pubkey) {
    secp256k1_ecdsa_recoverable_signature high_sig;
    secp256k1_ecdsa_signature high_normal_sig;
    secp256k1_ecdsa_signature normalized_sig;
    secp256k1_pubkey recovered_pubkey;
    unsigned char compact[64];
    unsigned char low_compact[64];
    unsigned char normalized_compact[64];
    int recid;

    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, low_compact, &recid, sig) == 1);
    FUZZ_CHECK(recid >= 0 && recid <= 3);
    memcpy(compact, low_compact, sizeof(compact));
    FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, compact + 32) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &high_sig, compact, recid ^ 1) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &high_sig, msg32) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, pubkey, &recovered_pubkey) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &high_normal_sig, &high_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &high_normal_sig, msg32, pubkey) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &high_normal_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normalized_compact, &normalized_sig) == 1);
    FUZZ_CHECK(memcmp(normalized_compact, low_compact, sizeof(normalized_compact)) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normalized_sig, msg32, pubkey) == 1);
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_RECOVERY
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 91);
    unsigned char seckey[32];
    unsigned char msg32[32];
    unsigned char compact[64];
    unsigned char checked_compact[64];
    unsigned char normal_compact[64];
    unsigned char zero_compact[64] = { 0 };
    unsigned char sig64[64];
    secp256k1_pubkey pubkey;
    secp256k1_pubkey recovered_pubkey;
    secp256k1_ecdsa_signature normal_sig;
    secp256k1_ecdsa_signature normalized_sig;
    secp256k1_ecdsa_recoverable_signature recoverable_sig;
    secp256k1_ecdsa_recoverable_signature checked_recoverable_sig;
    secp256k1_ecdsa_recoverable_signature reparsed_sig;
    int recid;
    int checked_recid;
    int alt_recid;
    int parsed;

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 97);
    secp256k1_fuzz_derive(msg32, sizeof(msg32), input, size, 101);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);

    secp256k1_fuzz_recovery_sha256_compression_calls = 0;
    secp256k1_context_set_sha256_compression(ctx, secp256k1_fuzz_recovery_sha256_compression);
    secp256k1_fuzz_recovery_sha256_compression_calls = 0;
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &recoverable_sig, msg32, seckey, NULL, NULL) == 1);
    FUZZ_CHECK(secp256k1_fuzz_recovery_sha256_compression_calls != 0);
    FUZZ_CHECK(secp256k1_ecdsa_sign_recoverable(ctx, &checked_recoverable_sig, msg32, seckey, secp256k1_nonce_function_rfc6979, NULL) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, compact, &recid, &recoverable_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, checked_compact, &checked_recid, &checked_recoverable_sig) == 1);
    FUZZ_CHECK(recid == checked_recid);
    FUZZ_CHECK(memcmp(compact, checked_compact, sizeof(compact)) == 0);
    secp256k1_context_set_sha256_compression(ctx, NULL);
    FUZZ_CHECK(recid >= 0 && recid <= 3);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &reparsed_sig, compact, recid) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recoverable_signature_convert(ctx, &normal_sig, &reparsed_sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, normal_compact, &normal_sig) == 1);
    FUZZ_CHECK(memcmp(compact, normal_compact, sizeof(compact)) == 0);
    secp256k1_ecdsa_signature_normalize(ctx, &normalized_sig, &normal_sig);
    FUZZ_CHECK(secp256k1_ecdsa_verify(ctx, &normalized_sig, msg32, &pubkey) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_recover(ctx, &recovered_pubkey, &reparsed_sig, msg32) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey, &recovered_pubkey) == 0);
    secp256k1_fuzz_check_recoverable_high_s(ctx, &reparsed_sig, msg32, &pubkey);

    secp256k1_fuzz_check_sign_recoverable_failure_cleanup(ctx, msg32, secp256k1_fuzz_scalar_zero);
    secp256k1_fuzz_check_sign_recoverable_failure_cleanup(ctx, msg32, secp256k1_fuzz_scalar_order);

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
    secp256k1_fuzz_check_recover_failure_cleanup(ctx, &reparsed_sig, msg32);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, zero_compact, 0);
    memcpy(sig64, secp256k1_fuzz_scalar_order_minus_one, 32);
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_order_minus_one, 32);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, sig64, recid);
    memcpy(sig64, secp256k1_fuzz_scalar_order, 32);
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_zero, 32);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, sig64, recid);
    memcpy(sig64, secp256k1_fuzz_scalar_zero, 32);
    memcpy(sig64 + 32, secp256k1_fuzz_scalar_order, 32);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, sig64, recid);
    secp256k1_fuzz_derive(sig64, sizeof(sig64), input, size, 113);
    secp256k1_fuzz_check_recoverable_parse_compact(ctx, sig64, recid);

    if (size >= 64) {
        recid = secp256k1_fuzz_byte(input, size, 109) & 3;
        secp256k1_fuzz_check_recoverable_parse_compact(ctx, input, recid);
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
