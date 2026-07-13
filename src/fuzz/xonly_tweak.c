/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

#ifdef ENABLE_MODULE_EXTRAKEYS
static const unsigned char secp256k1_fuzz_field_p_plus_one[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
};

static const unsigned char secp256k1_fuzz_xonly_field_prime[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
};

static const unsigned char secp256k1_fuzz_xonly_two_g_x[32] = {
    0xC6, 0x04, 0x7F, 0x94, 0x41, 0xED, 0x7D, 0x6D,
    0x30, 0x45, 0x40, 0x6E, 0x95, 0xC0, 0x7C, 0xD8,
    0x5C, 0x77, 0x8E, 0x4B, 0x8C, 0xEF, 0x3C, 0xA7,
    0xAB, 0xAC, 0x09, 0xB9, 0x5C, 0x70, 0x9E, 0xE5
};

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

/* Byte-level arithmetic keeps this parser oracle independent from the field
 * representation and square-root addition chain used by production code. */
static void secp256k1_fuzz_xonly_add_mod(unsigned char out32[32], const unsigned char a32[32], const unsigned char b32[32]) {
    unsigned char sum32[32];
    unsigned int carry = 0;
    size_t i;

    for (i = 32; i-- > 0;) {
        unsigned int value = (unsigned int)a32[i] + b32[i] + carry;
        sum32[i] = (unsigned char)value;
        carry = value >> 8;
    }
    if (carry != 0 || memcmp(sum32, secp256k1_fuzz_xonly_field_prime, sizeof(sum32)) >= 0) {
        int borrow = 0;
        for (i = 32; i-- > 0;) {
            int value = (int)sum32[i] - secp256k1_fuzz_xonly_field_prime[i] - borrow;
            if (value < 0) {
                value += 256;
                borrow = 1;
            } else {
                borrow = 0;
            }
            sum32[i] = (unsigned char)value;
        }
    }
    memcpy(out32, sum32, sizeof(sum32));
}

static void secp256k1_fuzz_xonly_mul_mod(unsigned char out32[32], const unsigned char a32[32], const unsigned char b32[32]) {
    unsigned char result32[32] = { 0 };
    unsigned char addend32[32];
    size_t i;
    unsigned int bit;

    memcpy(addend32, a32, sizeof(addend32));
    for (i = 32; i-- > 0;) {
        for (bit = 0; bit < 8; bit++) {
            if ((b32[i] & (1u << bit)) != 0) {
                secp256k1_fuzz_xonly_add_mod(result32, result32, addend32);
            }
            secp256k1_fuzz_xonly_add_mod(addend32, addend32, addend32);
        }
    }
    memcpy(out32, result32, sizeof(result32));
}

static void secp256k1_fuzz_xonly_sqrt_mod(unsigned char out32[32], const unsigned char input32[32]) {
    unsigned char exponent32[32];
    unsigned char result32[32] = { 0 };
    unsigned char one32[32] = { 0 };
    size_t i;
    unsigned int bit;
    unsigned int carry;

    /* p is 3 mod 4, so sqrt(a) = a^((p+1)/4) for quadratic residues. */
    memcpy(exponent32, secp256k1_fuzz_field_p_plus_one, sizeof(exponent32));
    for (i = 0; i < 2; i++) {
        carry = 0;
        for (bit = 0; bit < sizeof(exponent32); bit++) {
            unsigned int next_carry = exponent32[bit] & 1u;
            exponent32[bit] = (unsigned char)((exponent32[bit] >> 1) | (carry << 7));
            carry = next_carry;
        }
    }
    one32[31] = 1;
    memcpy(result32, one32, sizeof(result32));
    for (i = 0; i < sizeof(exponent32); i++) {
        for (bit = 8; bit-- > 0;) {
            secp256k1_fuzz_xonly_mul_mod(result32, result32, result32);
            if ((exponent32[i] & (1u << bit)) != 0) {
                secp256k1_fuzz_xonly_mul_mod(result32, result32, input32);
            }
        }
    }
    memcpy(out32, result32, sizeof(result32));
}

static int secp256k1_fuzz_xonly_parse_reference(const unsigned char input32[32]) {
    unsigned char x_squared32[32];
    unsigned char rhs32[32];
    unsigned char root32[32];
    unsigned char root_squared32[32];
    unsigned char seven32[32] = { 0 };

    if (memcmp(input32, secp256k1_fuzz_xonly_field_prime, 32) >= 0) {
        return 0;
    }
    seven32[31] = 7;
    secp256k1_fuzz_xonly_mul_mod(x_squared32, input32, input32);
    secp256k1_fuzz_xonly_mul_mod(rhs32, x_squared32, input32);
    secp256k1_fuzz_xonly_add_mod(rhs32, rhs32, seven32);
    secp256k1_fuzz_xonly_sqrt_mod(root32, rhs32);
    secp256k1_fuzz_xonly_mul_mod(root_squared32, root32, root32);
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
    secp256k1_fuzz_check_invalid_keypair_xonly_pub(ctx);
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
    secp256k1_fuzz_check_xonly_parse(ctx, secp256k1_fuzz_field_p_plus_one);
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
