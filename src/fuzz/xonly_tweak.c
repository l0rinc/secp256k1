/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

#ifdef ENABLE_MODULE_EXTRAKEYS
#include "pubkey_reference.h"

static const unsigned char secp256k1_fuzz_xonly_two_g_x[32] = {
    0xC6, 0x04, 0x7F, 0x94, 0x41, 0xED, 0x7D, 0x6D,
    0x30, 0x45, 0x40, 0x6E, 0x95, 0xC0, 0x7C, 0xD8,
    0x5C, 0x77, 0x8E, 0x4B, 0x8C, 0xEF, 0x3C, 0xA7,
    0xAB, 0xAC, 0x09, 0xB9, 0x5C, 0x70, 0x9E, 0xE5
};

static void secp256k1_fuzz_check_tweak_input_output_alias(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "tweak input-output overlap\n";
    unsigned char one32[32] = { 0 };
    unsigned char tweak32[32];
    unsigned char seckey_expected[32];
    unsigned char seckey_actual[32];
    secp256k1_pubkey pubkey_base;
    secp256k1_pubkey pubkey_expected;
    secp256k1_pubkey pubkey_actual;
    secp256k1_pubkey pubkey_mul_expected;
    secp256k1_pubkey pubkey_mul_actual;
    secp256k1_keypair keypair_base;
    secp256k1_keypair keypair_expected;
    secp256k1_keypair keypair_actual;
    secp256k1_pubkey keypair_expected_pubkey;
    secp256k1_pubkey keypair_actual_pubkey;
    size_t offset;
    size_t pubkey_shifted_cases = 0;
    size_t keypair_shifted_cases = 0;
    int expected_ret;
    int actual_ret;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    one32[31] = 1;
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey_base, one32) == 1);
    /* The in/out object and tweak input are separate API roles, but the
     * declarations do not prohibit their storage from overlapping. Exercise
     * windows at multiple offsets so a fix that handles only the object base
     * does not appear complete. */
    for (offset = 0; offset <= sizeof(pubkey_base.data) - sizeof(tweak32); offset += 16) {
        memcpy(tweak32, pubkey_base.data + offset, sizeof(tweak32));
        if (!secp256k1_ec_seckey_verify(ctx, tweak32)) {
            continue;
        }
        if (offset != 0) {
            pubkey_shifted_cases++;
        }

        pubkey_expected = pubkey_base;
        pubkey_actual = pubkey_base;
        expected_ret = secp256k1_ec_pubkey_tweak_add(ctx, &pubkey_expected, tweak32);
        actual_ret = secp256k1_ec_pubkey_tweak_add(ctx, &pubkey_actual, pubkey_actual.data + offset);
        FUZZ_CHECK(expected_ret == actual_ret);
        if (expected_ret) {
            FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey_expected, &pubkey_actual) == 0);
        }

        /* Multiplication must snapshot the factor before clearing output,
         * just as the addition helper does. */
        pubkey_mul_expected = pubkey_base;
        pubkey_mul_actual = pubkey_base;
        expected_ret = secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey_mul_expected, tweak32);
        actual_ret = secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey_mul_actual, pubkey_mul_actual.data + offset);
        FUZZ_CHECK(expected_ret == actual_ret);
        if (expected_ret) {
            FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &pubkey_mul_expected, &pubkey_mul_actual) == 0);
        }
    }
    FUZZ_CHECK(pubkey_shifted_cases != 0);

    memcpy(seckey_expected, one32, sizeof(seckey_expected));
    memcpy(seckey_actual, one32, sizeof(seckey_actual));
    FUZZ_CHECK(secp256k1_ec_seckey_tweak_add(ctx, seckey_expected, one32) == 1);
    FUZZ_CHECK(secp256k1_ec_seckey_tweak_add(ctx, seckey_actual, seckey_actual) == 1);
    FUZZ_CHECK(memcmp(seckey_expected, seckey_actual, sizeof(seckey_expected)) == 0);

    memcpy(seckey_expected, one32, sizeof(seckey_expected));
    memcpy(seckey_actual, one32, sizeof(seckey_actual));
    FUZZ_CHECK(secp256k1_ec_seckey_tweak_mul(ctx, seckey_expected, one32) == 1);
    FUZZ_CHECK(secp256k1_ec_seckey_tweak_mul(ctx, seckey_actual, seckey_actual) == 1);
    FUZZ_CHECK(memcmp(seckey_expected, seckey_actual, sizeof(seckey_expected)) == 0);

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair_base, one32) == 1);
    for (offset = 0; offset <= sizeof(keypair_base.data) - sizeof(tweak32); offset += 16) {
        memcpy(tweak32, keypair_base.data + offset, sizeof(tweak32));
        if (!secp256k1_ec_seckey_verify(ctx, tweak32)) {
            continue;
        }
        if (offset != 0) {
            keypair_shifted_cases++;
        }

        keypair_expected = keypair_base;
        keypair_actual = keypair_base;
        expected_ret = secp256k1_keypair_xonly_tweak_add(ctx, &keypair_expected, tweak32);
        actual_ret = secp256k1_keypair_xonly_tweak_add(ctx, &keypair_actual, keypair_actual.data + offset);
        FUZZ_CHECK(expected_ret == actual_ret);
        if (expected_ret) {
            FUZZ_CHECK(secp256k1_keypair_pub(ctx, &keypair_expected_pubkey, &keypair_expected) == 1);
            FUZZ_CHECK(secp256k1_keypair_pub(ctx, &keypair_actual_pubkey, &keypair_actual) == 1);
            FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &keypair_expected_pubkey, &keypair_actual_pubkey) == 0);
        }
    }
    FUZZ_CHECK(keypair_shifted_cases != 0);
}

static void secp256k1_fuzz_check_xonly_tweak_input_output_alias(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "xonly tweak input-output overlap\n";
    unsigned char one32[32] = { 0 };
    secp256k1_keypair keypair;
    secp256k1_xonly_pubkey internal_xonly;
    secp256k1_pubkey expected_pubkey;
    secp256k1_pubkey actual_pubkey;
    size_t offset;
    int expected_ret;
    int actual_ret;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    one32[31] = 1;
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, one32) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &internal_xonly, NULL, &keypair) == 1);

    /* output_pubkey is an Out object and tweak32 is an In byte array. The
     * public contract does not prohibit their storage from overlapping. */
    for (offset = 0; offset <= sizeof(actual_pubkey.data) - sizeof(one32); offset += 16) {
        memset(&actual_pubkey, 0, sizeof(actual_pubkey));
        memcpy(actual_pubkey.data + offset, one32, sizeof(one32));
        expected_ret = secp256k1_xonly_pubkey_tweak_add(ctx, &expected_pubkey, &internal_xonly, one32);
        actual_ret = secp256k1_xonly_pubkey_tweak_add(ctx, &actual_pubkey, &internal_xonly, actual_pubkey.data + offset);
        FUZZ_CHECK(expected_ret == actual_ret);
        if (expected_ret) {
            FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &expected_pubkey, &actual_pubkey) == 0);
        }
    }
}

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_xonly_illegal_data;

typedef int (*secp256k1_fuzz_keypair_xonly_tweak_add_fn)(
    const secp256k1_context *ctx,
    secp256k1_keypair *keypair,
    const unsigned char *tweak32
);

typedef int (*secp256k1_fuzz_xonly_pubkey_cmp_fn)(
    const secp256k1_context *ctx,
    const secp256k1_xonly_pubkey *pk0,
    const secp256k1_xonly_pubkey *pk1
);

typedef int (*secp256k1_fuzz_xonly_pubkey_from_pubkey_fn)(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey *xonly_pubkey,
    int *pk_parity,
    const secp256k1_pubkey *pubkey
);

static void secp256k1_fuzz_xonly_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_xonly_illegal_data *illegal_data = (secp256k1_fuzz_xonly_illegal_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(illegal_data != NULL);
    FUZZ_CHECK(illegal_data->self == illegal_data);
    illegal_data->calls++;
}

static const unsigned char *secp256k1_fuzz_xonly_null_tweak32(const unsigned char *tweak32) {
    const unsigned char * volatile null_tweak32 = tweak32;

    null_tweak32 = NULL;
    return null_tweak32;
}

static const secp256k1_xonly_pubkey *secp256k1_fuzz_xonly_null_pubkey(const secp256k1_xonly_pubkey *pubkey) {
    const secp256k1_xonly_pubkey * volatile null_pubkey = pubkey;

    null_pubkey = NULL;
    return null_pubkey;
}

static int secp256k1_fuzz_xonly_parse_reference(const unsigned char input32[32]) {
    unsigned char x_squared32[32];
    unsigned char rhs32[32];
    unsigned char root32[32];
    unsigned char root_squared32[32];
    unsigned char seven32[32] = { 0 };

    if (memcmp(input32, secp256k1_fuzz_pubkey_field_prime, 32) >= 0) {
        return 0;
    }
    seven32[31] = 7;
    secp256k1_fuzz_pubkey_mul_mod(x_squared32, input32, input32);
    secp256k1_fuzz_pubkey_mul_mod(rhs32, x_squared32, input32);
    secp256k1_fuzz_pubkey_add_mod(rhs32, rhs32, seven32);
    secp256k1_fuzz_pubkey_sqrt_mod(root32, rhs32);
    secp256k1_fuzz_pubkey_mul_mod(root_squared32, root32, root32);
    return memcmp(root_squared32, rhs32, sizeof(root_squared32)) == 0;
}

static void secp256k1_fuzz_check_xonly_parse(const secp256k1_context *ctx, const unsigned char *input32) {
    unsigned char compressed[33];
    unsigned char serialized[32];
    unsigned char zero_xonly[sizeof(secp256k1_xonly_pubkey)] = { 0 };
    secp256k1_pubkey parsed_pubkey;
    secp256k1_xonly_pubkey parsed_xonly;
    secp256k1_xonly_pubkey xonly_from_pubkey;
    int parse_full_ret;
    int parse_xonly_ret;

    compressed[0] = SECP256K1_TAG_PUBKEY_EVEN;
    memcpy(compressed + 1, input32, 32);
    parse_full_ret = secp256k1_ec_pubkey_parse(ctx, &parsed_pubkey, compressed, sizeof(compressed));
    memset(&parsed_xonly, 0xA5, sizeof(parsed_xonly));
    parse_xonly_ret = secp256k1_xonly_pubkey_parse(ctx, &parsed_xonly, input32);
    FUZZ_CHECK(parse_xonly_ret == secp256k1_fuzz_xonly_parse_reference(input32));
    FUZZ_CHECK(parse_xonly_ret == parse_full_ret);
    if (parse_xonly_ret) {
        FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &xonly_from_pubkey, NULL, &parsed_pubkey) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &parsed_xonly, &xonly_from_pubkey) == 0);
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, serialized, &parsed_xonly) == 1);
        FUZZ_CHECK(memcmp(serialized, input32, sizeof(serialized)) == 0);
    } else {
        FUZZ_CHECK(memcmp(&parsed_xonly, zero_xonly, sizeof(parsed_xonly)) == 0);
    }
}

static void secp256k1_fuzz_check_xonly_parity_pair(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey) {
    secp256k1_pubkey negated_pubkey = *pubkey;
    secp256k1_xonly_pubkey xonly;
    secp256k1_xonly_pubkey negated_xonly;
    unsigned char compressed[33];
    size_t compressed_len = sizeof(compressed);
    int parity;
    int negated_parity;

    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, compressed, &compressed_len, pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(compressed_len == sizeof(compressed));
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &xonly, &parity, pubkey) == 1);
    FUZZ_CHECK(parity == 0 || parity == 1);
    FUZZ_CHECK(parity == (compressed[0] == SECP256K1_TAG_PUBKEY_ODD));

    FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &negated_pubkey) == 1);
    compressed_len = sizeof(compressed);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, compressed, &compressed_len, &negated_pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(compressed_len == sizeof(compressed));
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &negated_xonly, &negated_parity, &negated_pubkey) == 1);
    FUZZ_CHECK(negated_parity == 0 || negated_parity == 1);
    FUZZ_CHECK(negated_parity == (compressed[0] == SECP256K1_TAG_PUBKEY_ODD));

    FUZZ_CHECK(parity != negated_parity);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &negated_xonly) == 0);
}

static void secp256k1_fuzz_check_xonly_opaque_parity_barrier(secp256k1_context *ctx, const secp256k1_pubkey *pubkey, int pubkey_parity) {
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    secp256k1_pubkey odd_pubkey = *pubkey;
    secp256k1_xonly_pubkey odd_xonly;
    secp256k1_pubkey tweaked_pubkey;
    unsigned char serialized[32];
    unsigned char zero32[32] = { 0 };
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    unsigned int calls;

    FUZZ_CHECK(pubkey_parity == 0 || pubkey_parity == 1);
    if (!pubkey_parity) {
        FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &odd_pubkey) == 1);
    }
    /* Keep a curve-valid point but violate the x-only even-Y representation. */
    memcpy(&odd_xonly, &odd_pubkey, sizeof(odd_xonly));

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);

    memset(serialized, 0xA5, sizeof(serialized));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, serialized, &odd_xonly) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(serialized, zero32, sizeof(serialized)) == 0);

    memset(&tweaked_pubkey, 0xA5, sizeof(tweaked_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add(ctx, &tweaked_pubkey, &odd_xonly, zero32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&tweaked_pubkey, zero_pubkey, sizeof(tweaked_pubkey)) == 0);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, zero32, 0, &odd_xonly, zero32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_xonly_tweaked_seckey(const secp256k1_context *ctx, unsigned char *out32, const unsigned char *seckey32, int parity, const unsigned char *tweak32) {
    memcpy(out32, seckey32, 32);
    if (parity) {
        FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, out32) == 1);
    }
    FUZZ_CHECK(secp256k1_ec_seckey_tweak_add(ctx, out32, tweak32) == 1);
}

static void secp256k1_fuzz_check_xonly_invalid_tweak(secp256k1_context *ctx, const unsigned char *xonly32, int parity, const unsigned char *tweak32) {
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    secp256k1_xonly_pubkey invalid_xonly;
    secp256k1_pubkey tweaked_pubkey;
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    unsigned int calls;

    memset(&invalid_xonly, 0, sizeof(invalid_xonly));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);

    memset(&tweaked_pubkey, 0xA5, sizeof(tweaked_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add(ctx, &tweaked_pubkey, &invalid_xonly, tweak32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&tweaked_pubkey, zero_pubkey, sizeof(tweaked_pubkey)) == 0);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, xonly32, parity, &invalid_xonly, tweak32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_keypair_null_tweak(secp256k1_context *ctx, const secp256k1_keypair *keypair, const unsigned char *tweak32) {
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    secp256k1_fuzz_keypair_xonly_tweak_add_fn tweak_add = secp256k1_keypair_xonly_tweak_add;
    secp256k1_keypair null_tweaked_keypair = *keypair;
    const unsigned char *null_tweak32 = secp256k1_fuzz_xonly_null_tweak32(tweak32);
    unsigned char zero_keypair[sizeof(secp256k1_keypair)] = { 0 };

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);

    FUZZ_CHECK(tweak_add(ctx, &null_tweaked_keypair, null_tweak32) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&null_tweaked_keypair, zero_keypair, sizeof(null_tweaked_keypair)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_xonly_keypair_consistency(secp256k1_context *ctx, const secp256k1_keypair *keypair, const secp256k1_pubkey *original_pubkey, const unsigned char *tweak32) {
    static const unsigned char scalar_two[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
    };
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    secp256k1_keypair mismatched_keypair;
    secp256k1_pubkey alternate_pubkey;
    unsigned char zero_keypair[sizeof(*keypair)] = { 0 };

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &alternate_pubkey, secp256k1_fuzz_scalar_one) == 1);
    if (secp256k1_ec_pubkey_cmp(ctx, original_pubkey, &alternate_pubkey) == 0) {
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &alternate_pubkey, scalar_two) == 1);
    }
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, original_pubkey, &alternate_pubkey) != 0);
    mismatched_keypair = *keypair;
    memcpy(mismatched_keypair.data + 32, alternate_pubkey.data, sizeof(mismatched_keypair.data) - 32);

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);
    FUZZ_CHECK(secp256k1_keypair_xonly_tweak_add(ctx, &mismatched_keypair, tweak32) == 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(memcmp(&mismatched_keypair, zero_keypair, sizeof(mismatched_keypair)) == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_keypair_projection(secp256k1_context *ctx, const secp256k1_keypair *keypair, const secp256k1_pubkey *valid_pubkey, const secp256k1_xonly_pubkey *valid_xonly, int valid_parity, const unsigned char *valid_seckey) {
    secp256k1_keypair invalid_secret_keypair = *keypair;
    secp256k1_keypair invalid_public_keypair = *keypair;
    secp256k1_pubkey extracted_pubkey;
    secp256k1_xonly_pubkey extracted_xonly;
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    unsigned char extracted_seckey[32];
    unsigned char zero_pubkey[sizeof(extracted_pubkey)] = { 0 };
    unsigned char zero_xonly[sizeof(extracted_xonly)] = { 0 };
    unsigned char zero32[32] = { 0 };
    int extracted_parity;
    unsigned int calls;

    /* Raw extractors intentionally expose their respective keypair halves. */
    memset(invalid_secret_keypair.data, 0, 32);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &extracted_pubkey, &invalid_secret_keypair) == 1);
    FUZZ_CHECK(memcmp(&extracted_pubkey, valid_pubkey, sizeof(extracted_pubkey)) == 0);
    FUZZ_CHECK(secp256k1_keypair_sec(ctx, extracted_seckey, &invalid_secret_keypair) == 1);
    FUZZ_CHECK(memcmp(extracted_seckey, zero32, sizeof(extracted_seckey)) == 0);
    memset(&extracted_xonly, 0xA5, sizeof(extracted_xonly));
    extracted_parity = 7;
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &extracted_xonly, &extracted_parity, &invalid_secret_keypair) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &extracted_xonly, valid_xonly) == 0);
    FUZZ_CHECK(extracted_parity == valid_parity);

    memset(invalid_public_keypair.data + 32, 0, sizeof(invalid_public_keypair.data) - 32);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &extracted_pubkey, &invalid_public_keypair) == 1);
    FUZZ_CHECK(memcmp(&extracted_pubkey, zero_pubkey, sizeof(extracted_pubkey)) == 0);
    FUZZ_CHECK(secp256k1_keypair_sec(ctx, extracted_seckey, &invalid_public_keypair) == 1);
    FUZZ_CHECK(memcmp(extracted_seckey, valid_seckey, sizeof(extracted_seckey)) == 0);

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);
    memset(&extracted_xonly, 0xA5, sizeof(extracted_xonly));
    extracted_parity = 7;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &extracted_xonly, &extracted_parity, &invalid_public_keypair) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&extracted_xonly, zero_xonly, sizeof(extracted_xonly)) == 0);
    FUZZ_CHECK(extracted_parity == 0);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_keypair_tweak_partial_invalid(secp256k1_context *ctx, const secp256k1_keypair *keypair, const unsigned char *tweak32) {
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    secp256k1_keypair invalid_secret_keypair = *keypair;
    secp256k1_keypair invalid_public_keypair = *keypair;
    unsigned char zero_keypair[sizeof(*keypair)] = { 0 };
    unsigned int calls;

    /* Raw keypair projections are intentionally permissive, but a mutating
     * operation must not accept either partial keypair as signing state. */
    memset(invalid_secret_keypair.data, 0, 32);
    memset(invalid_public_keypair.data + 32, 0, sizeof(invalid_public_keypair.data) - 32);

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_keypair_xonly_tweak_add(ctx, &invalid_secret_keypair, tweak32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&invalid_secret_keypair, zero_keypair, sizeof(invalid_secret_keypair)) == 0);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_keypair_xonly_tweak_add(ctx, &invalid_public_keypair, tweak32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&invalid_public_keypair, zero_keypair, sizeof(invalid_public_keypair)) == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_invalid_keypair_xonly_pub(secp256k1_context *ctx) {
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    secp256k1_keypair invalid_keypair;
    secp256k1_xonly_pubkey output;
    unsigned char zero_xonly[sizeof(output)] = { 0 };
    int parity = 7;
    unsigned int calls;

    memset(&invalid_keypair, 0, sizeof(invalid_keypair));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);

    memset(&output, 0xA5, sizeof(output));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &output, &parity, &invalid_keypair) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&output, zero_xonly, sizeof(output)) == 0);
    FUZZ_CHECK(parity == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_keypair_create_failure(const secp256k1_context *ctx) {
    static const unsigned char *invalid_seckeys[] = {
        secp256k1_fuzz_scalar_zero,
        secp256k1_fuzz_scalar_order
    };
    unsigned char zero_keypair[sizeof(secp256k1_keypair)] = { 0 };
    secp256k1_keypair keypair;
    size_t i;

    /* Invalid secrets must not leave the helper's dummy generator state in the output. */
    for (i = 0; i < sizeof(invalid_seckeys) / sizeof(invalid_seckeys[0]); i++) {
        memset(&keypair, 0xA5, sizeof(keypair));
        FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, invalid_seckeys[i]) == 0);
        FUZZ_CHECK(memcmp(&keypair, zero_keypair, sizeof(keypair)) == 0);
    }
}

static void secp256k1_fuzz_check_invalid_pubkey_xonly_pub(secp256k1_context *ctx) {
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    secp256k1_pubkey invalid_pubkey;
    secp256k1_xonly_pubkey output;
    unsigned char zero_xonly[sizeof(output)] = { 0 };
    int parity = 7;
    unsigned int calls;

    memset(&invalid_pubkey, 0, sizeof(invalid_pubkey));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);

    memset(&output, 0xA5, sizeof(output));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &output, &parity, &invalid_pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&output, zero_xonly, sizeof(output)) == 0);
    FUZZ_CHECK(parity == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_null_pubkey_xonly_pub(secp256k1_context *ctx, const secp256k1_pubkey *valid_pubkey) {
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    secp256k1_xonly_pubkey output;
    secp256k1_fuzz_xonly_pubkey_from_pubkey_fn from_pubkey = secp256k1_xonly_pubkey_from_pubkey;
    const secp256k1_pubkey * volatile null_pubkey = valid_pubkey;
    unsigned char zero_xonly[sizeof(output)] = { 0 };
    unsigned int calls;
    int parity = 7;

    /* Keep the NULL argument opaque to the compiler so this remains a real API call. */
    null_pubkey = NULL;
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);

    memset(&output, 0xA5, sizeof(output));
    calls = illegal_data.calls;
    FUZZ_CHECK(from_pubkey(ctx, &output, &parity, null_pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&output, zero_xonly, sizeof(output)) == 0);
    FUZZ_CHECK(parity == 0);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_invalid_xonly_cmp(secp256k1_context *ctx, const secp256k1_xonly_pubkey *valid_xonly) {
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    secp256k1_xonly_pubkey invalid_xonly;
    unsigned int calls;

    memset(&invalid_xonly, 0, sizeof(invalid_xonly));
    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &invalid_xonly, valid_xonly) < 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, valid_xonly, &invalid_xonly) > 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &invalid_xonly, &invalid_xonly) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 2);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_null_xonly_cmp(secp256k1_context *ctx, const secp256k1_xonly_pubkey *valid_xonly) {
    secp256k1_fuzz_xonly_illegal_data illegal_data;
    const secp256k1_xonly_pubkey *null_pubkey = secp256k1_fuzz_xonly_null_pubkey(valid_xonly);
    secp256k1_fuzz_xonly_pubkey_cmp_fn cmp = secp256k1_xonly_pubkey_cmp;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_xonly_illegal_callback, &illegal_data);

    /* The comparator's all-zero fallback must make NULL sort below valid keys. */
    FUZZ_CHECK(cmp(ctx, null_pubkey, valid_xonly) < 0);
    FUZZ_CHECK(illegal_data.calls == 1);
    FUZZ_CHECK(cmp(ctx, valid_xonly, null_pubkey) > 0);
    FUZZ_CHECK(illegal_data.calls == 2);
    FUZZ_CHECK(cmp(ctx, null_pubkey, null_pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == 4);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}
#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_EXTRAKEYS
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 71);
    unsigned char seckey[32];
    unsigned char tweak[32];
    unsigned char parse32[32];
    unsigned char ones32[32];
    unsigned char xonly32[32];
    unsigned char tweaked32[32];
    unsigned char tweaked32_bad[32];
    unsigned char zero_tweaked32[32];
    unsigned char cancel_tweak[32];
    unsigned char tweaked_seckey[32];
    unsigned char expected_tweaked_seckey[32];
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    unsigned char zero_keypair[sizeof(secp256k1_keypair)] = { 0 };
    secp256k1_keypair keypair;
    secp256k1_keypair cancel_tweaked_keypair;
    secp256k1_keypair tweaked_keypair;
    secp256k1_keypair zero_tweaked_keypair;
    secp256k1_keypair overflow_tweaked_keypair;
    secp256k1_pubkey pubkey;
    secp256k1_pubkey cancel_tweaked_pubkey;
    secp256k1_pubkey tweaked_pubkey;
    secp256k1_pubkey tweaked_keypair_pubkey;
    secp256k1_pubkey zero_tweaked_pubkey;
    secp256k1_pubkey zero_tweaked_keypair_pubkey;
    secp256k1_pubkey overflow_tweaked_pubkey;
    secp256k1_xonly_pubkey xonly;
    secp256k1_xonly_pubkey xonly_from_pubkey;
    secp256k1_xonly_pubkey xonly_no_parity;
    secp256k1_xonly_pubkey reparsed;
    secp256k1_xonly_pubkey tweaked_xonly;
    secp256k1_xonly_pubkey zero_tweaked_xonly;
    int keypair_parity;
    int pubkey_parity;
    int tweaked_parity;
    int zero_tweaked_parity;
    int pub_tweak_ret;
    int keypair_tweak_ret;
    size_t flip_index;
    unsigned char flip_mask;

    secp256k1_fuzz_valid_seckey32(ctx, seckey, input, size, 73);
    secp256k1_fuzz_scalar32(tweak, input, size, 79);
    secp256k1_fuzz_derive(parse32, sizeof(parse32), input, size, 83);
    flip_index = (size_t)(secp256k1_fuzz_byte(input, size, 89) & 31u);
    flip_mask = (unsigned char)(secp256k1_fuzz_byte(input, size, 97) | 1u);
    memset(ones32, 0xFF, sizeof(ones32));

    FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &pubkey, &keypair) == 1);
    secp256k1_fuzz_check_tweak_input_output_alias(ctx, input, size);
    secp256k1_fuzz_check_xonly_tweak_input_output_alias(ctx, input, size);
    secp256k1_fuzz_check_invalid_keypair_xonly_pub(ctx);
    secp256k1_fuzz_check_invalid_pubkey_xonly_pub(ctx);
    if (size == sizeof("xonly pubkey from pubkey null\n") - 1
        && memcmp(input, "xonly pubkey from pubkey null\n", sizeof("xonly pubkey from pubkey null\n") - 1) == 0) {
        secp256k1_fuzz_check_null_pubkey_xonly_pub(ctx, &pubkey);
    }
    if (size == sizeof("invalid keypair create cleanup\n") - 1
        && memcmp(input, "invalid keypair create cleanup\n", sizeof("invalid keypair create cleanup\n") - 1) == 0) {
        secp256k1_fuzz_check_keypair_create_failure(ctx);
    }
    if (size == sizeof("partial keypair tweak invalid\n") - 1
        && memcmp(input, "partial keypair tweak invalid\n", sizeof("partial keypair tweak invalid\n") - 1) == 0) {
        secp256k1_fuzz_check_keypair_tweak_partial_invalid(ctx, &keypair, tweak);
    }
    secp256k1_fuzz_check_xonly_keypair_consistency(ctx, &keypair, &pubkey, tweak);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly, &keypair_parity, &keypair) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly_no_parity, NULL, &keypair) == 1);
    secp256k1_fuzz_check_keypair_projection(ctx, &keypair, &pubkey, &xonly, keypair_parity, seckey);
    secp256k1_fuzz_check_invalid_xonly_cmp(ctx, &xonly);
    secp256k1_fuzz_check_null_xonly_cmp(ctx, &xonly);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &xonly_no_parity) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &xonly_from_pubkey, &pubkey_parity, &pubkey) == 1);
    FUZZ_CHECK(keypair_parity == pubkey_parity);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &xonly_from_pubkey) == 0);
    secp256k1_fuzz_check_xonly_parity_pair(ctx, &pubkey);
    secp256k1_fuzz_check_xonly_opaque_parity_barrier(ctx, &pubkey, pubkey_parity);

    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly32, &xonly) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_parse(ctx, &reparsed, xonly32) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &reparsed) == 0);
    secp256k1_fuzz_check_xonly_invalid_tweak(ctx, xonly32, keypair_parity, tweak);
    secp256k1_fuzz_check_keypair_null_tweak(ctx, &keypair, tweak);
    secp256k1_fuzz_check_xonly_parse(ctx, xonly32);
    secp256k1_fuzz_check_xonly_parse(ctx, secp256k1_fuzz_scalar_zero);
    secp256k1_fuzz_check_xonly_parse(ctx, secp256k1_fuzz_pubkey_field_p_plus_one);
    secp256k1_fuzz_check_xonly_parse(ctx, ones32);
    secp256k1_fuzz_check_xonly_parse(ctx, parse32);
    secp256k1_fuzz_check_xonly_parse(ctx, secp256k1_fuzz_xonly_two_g_x);

    zero_tweaked_keypair = keypair;
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add(ctx, &zero_tweaked_pubkey, &xonly, secp256k1_fuzz_scalar_zero) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_tweak_add(ctx, &zero_tweaked_keypair, secp256k1_fuzz_scalar_zero) == 1);
    secp256k1_fuzz_xonly_tweaked_seckey(ctx, expected_tweaked_seckey, seckey, keypair_parity, secp256k1_fuzz_scalar_zero);
    FUZZ_CHECK(secp256k1_keypair_sec(ctx, tweaked_seckey, &zero_tweaked_keypair) == 1);
    FUZZ_CHECK(memcmp(tweaked_seckey, expected_tweaked_seckey, sizeof(tweaked_seckey)) == 0);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &zero_tweaked_keypair_pubkey, &zero_tweaked_keypair) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &zero_tweaked_pubkey, &zero_tweaked_keypair_pubkey) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &zero_tweaked_xonly, &zero_tweaked_parity, &zero_tweaked_pubkey) == 1);
    FUZZ_CHECK(zero_tweaked_parity == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &zero_tweaked_xonly) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, zero_tweaked32, &zero_tweaked_xonly) == 1);
    FUZZ_CHECK(memcmp(xonly32, zero_tweaked32, sizeof(xonly32)) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, zero_tweaked32, zero_tweaked_parity, &xonly, secp256k1_fuzz_scalar_zero) == 1);
    memcpy(tweaked32_bad, zero_tweaked32, sizeof(tweaked32_bad));
    tweaked32_bad[flip_index] ^= flip_mask;
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, tweaked32_bad, zero_tweaked_parity, &xonly, secp256k1_fuzz_scalar_zero) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, zero_tweaked32, !zero_tweaked_parity, &xonly, secp256k1_fuzz_scalar_zero) == 0);

    overflow_tweaked_keypair = keypair;
    memset(&overflow_tweaked_pubkey, 0xA5, sizeof(overflow_tweaked_pubkey));
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add(ctx, &overflow_tweaked_pubkey, &xonly, secp256k1_fuzz_scalar_order) == 0);
    FUZZ_CHECK(secp256k1_keypair_xonly_tweak_add(ctx, &overflow_tweaked_keypair, secp256k1_fuzz_scalar_order) == 0);
    FUZZ_CHECK(memcmp(&overflow_tweaked_pubkey, zero_pubkey, sizeof(overflow_tweaked_pubkey)) == 0);
    FUZZ_CHECK(memcmp(&overflow_tweaked_keypair, zero_keypair, sizeof(overflow_tweaked_keypair)) == 0);

    memcpy(cancel_tweak, seckey, sizeof(cancel_tweak));
    if (!keypair_parity) {
        FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, cancel_tweak) == 1);
    }
    cancel_tweaked_keypair = keypair;
    memset(&cancel_tweaked_pubkey, 0xA5, sizeof(cancel_tweaked_pubkey));
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add(ctx, &cancel_tweaked_pubkey, &xonly, cancel_tweak) == 0);
    FUZZ_CHECK(secp256k1_keypair_xonly_tweak_add(ctx, &cancel_tweaked_keypair, cancel_tweak) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, xonly32, keypair_parity, &xonly, cancel_tweak) == 0);
    FUZZ_CHECK(memcmp(&cancel_tweaked_pubkey, zero_pubkey, sizeof(cancel_tweaked_pubkey)) == 0);
    FUZZ_CHECK(memcmp(&cancel_tweaked_keypair, zero_keypair, sizeof(cancel_tweaked_keypair)) == 0);

    tweaked_keypair = keypair;
    pub_tweak_ret = secp256k1_xonly_pubkey_tweak_add(ctx, &tweaked_pubkey, &xonly, tweak);
    keypair_tweak_ret = secp256k1_keypair_xonly_tweak_add(ctx, &tweaked_keypair, tweak);
    FUZZ_CHECK(pub_tweak_ret == keypair_tweak_ret);
    if (pub_tweak_ret) {
        secp256k1_pubkey tweaked_pubkey_from_seckey;
        secp256k1_fuzz_xonly_tweaked_seckey(ctx, expected_tweaked_seckey, seckey, keypair_parity, tweak);
        FUZZ_CHECK(secp256k1_keypair_sec(ctx, tweaked_seckey, &tweaked_keypair) == 1);
        FUZZ_CHECK(memcmp(tweaked_seckey, expected_tweaked_seckey, sizeof(tweaked_seckey)) == 0);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &tweaked_pubkey_from_seckey, tweaked_seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &tweaked_pubkey, &tweaked_pubkey_from_seckey) == 0);
        FUZZ_CHECK(secp256k1_keypair_pub(ctx, &tweaked_keypair_pubkey, &tweaked_keypair) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &tweaked_pubkey, &tweaked_keypair_pubkey) == 0);
        secp256k1_fuzz_check_xonly_parity_pair(ctx, &tweaked_pubkey);
        FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &tweaked_xonly, &tweaked_parity, &tweaked_pubkey) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, tweaked32, &tweaked_xonly) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, tweaked32, tweaked_parity, &xonly, tweak) == 1);
        memcpy(tweaked32_bad, tweaked32, sizeof(tweaked32_bad));
        tweaked32_bad[flip_index] ^= flip_mask;
        FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, tweaked32_bad, tweaked_parity, &xonly, tweak) == 0);
        FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, tweaked32, !tweaked_parity, &xonly, tweak) == 0);
        FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, tweaked32, 2, &xonly, tweak) == 0);
    }

    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
