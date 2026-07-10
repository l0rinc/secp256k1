/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../secp256k1.c"
#include "fuzz.h"

static int secp256k1_fuzz_fe_identical(const secp256k1_fe *a, const secp256k1_fe *b) {
    return secp256k1_memcmp_var(a->n, b->n, sizeof(a->n)) == 0;
}

static void secp256k1_fuzz_fe_check_normalized(const secp256k1_fe *actual, const secp256k1_fe *expected) {
#ifdef VERIFY
    FUZZ_CHECK(actual->normalized == 1);
    FUZZ_CHECK(actual->magnitude == 1);
    FUZZ_CHECK(expected->normalized == 1);
    FUZZ_CHECK(expected->magnitude == 1);
#endif
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(actual, expected));
}

static void secp256k1_fuzz_fe_check_normalize_paths(const secp256k1_fe *input, const secp256k1_fe *expected) {
    secp256k1_fe constant_time = *input;
    secp256k1_fe variable_time = *input;
    secp256k1_fe weak_then_full = *input;
    int expected_zero = secp256k1_fe_is_zero(expected);

    secp256k1_fe_normalize(&constant_time);
    secp256k1_fe_normalize_var(&variable_time);
    secp256k1_fe_normalize_weak(&weak_then_full);
    secp256k1_fe_normalize_var(&weak_then_full);

    secp256k1_fuzz_fe_check_normalized(&constant_time, expected);
    secp256k1_fuzz_fe_check_normalized(&variable_time, expected);
    secp256k1_fuzz_fe_check_normalized(&weak_then_full, expected);
    FUZZ_CHECK(secp256k1_fe_normalizes_to_zero(input) == expected_zero);
    FUZZ_CHECK(secp256k1_fe_normalizes_to_zero_var(input) == expected_zero);
}

static void secp256k1_fuzz_fe_check_bounds_sum(int left_magnitude, int right_magnitude) {
    secp256k1_fe left;
    secp256k1_fe right;
    secp256k1_fe input;
    secp256k1_fe expected;

    FUZZ_CHECK(left_magnitude >= 1);
    FUZZ_CHECK(right_magnitude >= 1);
    FUZZ_CHECK(left_magnitude + right_magnitude == 32);

    secp256k1_fe_get_bounds(&left, left_magnitude);
    secp256k1_fe_get_bounds(&right, right_magnitude);
    input = left;
    secp256k1_fe_add(&input, &right);
#ifdef VERIFY
    FUZZ_CHECK(input.magnitude == 32);
#endif

    secp256k1_fe_normalize_var(&left);
    secp256k1_fe_normalize_var(&right);
    expected = left;
    secp256k1_fe_add(&expected, &right);
    secp256k1_fe_normalize_var(&expected);

    secp256k1_fuzz_fe_check_normalize_paths(&input, &expected);
}

static void secp256k1_fuzz_fe_check_raised_zero(const unsigned char *input, size_t size) {
    unsigned char bytes32[32];
    secp256k1_fe value;
    secp256k1_fe expected;
    secp256k1_fe zero31;

    secp256k1_fuzz_derive(bytes32, sizeof(bytes32), input, size, 33);
    secp256k1_fe_set_b32_mod(&value, bytes32);
    expected = value;
    secp256k1_fe_normalize_var(&expected);

    secp256k1_fe_set_int(&zero31, 0);
    secp256k1_fe_negate(&zero31, &zero31, 0);
    secp256k1_fe_mul_int_unchecked(&zero31, 31);
    secp256k1_fe_add(&value, &zero31);
#ifdef VERIFY
    FUZZ_CHECK(value.magnitude == 32);
#endif

    secp256k1_fuzz_fe_check_normalize_paths(&value, &expected);
}

static const unsigned char secp256k1_fuzz_field_prime[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
};

static void secp256k1_fuzz_fe_half_reference(unsigned char *out32, const unsigned char *in32) {
    unsigned char sum[33] = { 0 };
    unsigned int carry = 0;
    size_t i;

    memcpy(sum + 1, in32, 32);
    if (in32[31] & 1u) {
        for (i = 33; i-- > 1;) {
            unsigned int value = (unsigned int)sum[i] + secp256k1_fuzz_field_prime[i - 1] + carry;
            sum[i] = (unsigned char)value;
            carry = value >> 8;
        }
        sum[0] = (unsigned char)carry;
    }
    for (i = 32; i > 0; i--) {
        sum[i] = (unsigned char)((sum[i] >> 1) | ((sum[i - 1] & 1u) << 7));
    }
    sum[0] >>= 1;
    memcpy(out32, sum + 1, 32);
}

static void secp256k1_fuzz_fe_check_half(const unsigned char *input, size_t size) {
    secp256k1_fe canonical;
    secp256k1_fe zero;
    secp256k1_fe raised;
    secp256k1_fe actual;
    secp256k1_fe doubled;
    unsigned char canonical32[32];
    unsigned char expected32[32];
    unsigned char actual32[32];
    int magnitude;

    secp256k1_fuzz_derive(canonical32, sizeof(canonical32), input, size, 167);
    secp256k1_fe_set_b32_mod(&canonical, canonical32);
    secp256k1_fe_normalize_var(&canonical);
    secp256k1_fe_get_b32(canonical32, &canonical);
    secp256k1_fuzz_fe_half_reference(expected32, canonical32);

    /* Add multiples of p without changing the field value to exercise every
     * input magnitude accepted by fe_half. */
    secp256k1_fe_set_int(&zero, 0);
    secp256k1_fe_negate(&zero, &zero, 0);
    raised = canonical;
    for (magnitude = 1; magnitude <= 31; magnitude++) {
        if (magnitude > 1) {
            secp256k1_fe_add(&raised, &zero);
        }
        actual = raised;
        secp256k1_fe_half(&actual);
        secp256k1_fe_normalize_var(&actual);
        secp256k1_fe_get_b32(actual32, &actual);
        FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);

        doubled = actual;
        secp256k1_fe_add(&doubled, &actual);
        secp256k1_fe_normalize_var(&doubled);
        secp256k1_fe_get_b32(actual32, &doubled);
        FUZZ_CHECK(memcmp(actual32, canonical32, sizeof(actual32)) == 0);
    }
}

static void secp256k1_fuzz_fe_check_cmov(const secp256k1_fe *a, const secp256k1_fe *b) {
    secp256k1_fe selected;
    secp256k1_fe expected;
    secp256k1_fe alias;
    secp256k1_fe_storage storage_a;
    secp256k1_fe_storage storage_b;
    secp256k1_fe_storage storage_selected;
    secp256k1_fe_storage storage_alias;
    secp256k1_fe storage_result;
    int flag;

    secp256k1_fe_to_storage(&storage_a, a);
    secp256k1_fe_to_storage(&storage_b, b);
    for (flag = 0; flag <= 1; flag++) {
        selected = *a;
        expected = flag ? *b : *a;
        secp256k1_fe_cmov(&selected, b, flag);
        FUZZ_CHECK(secp256k1_fuzz_fe_identical(&selected, &expected));

        alias = *a;
        secp256k1_fe_cmov(&alias, &alias, flag);
        FUZZ_CHECK(secp256k1_fuzz_fe_identical(&alias, a));

        storage_selected = storage_a;
        secp256k1_fe_storage_cmov(&storage_selected, &storage_b, flag);
        secp256k1_fe_from_storage(&storage_result, &storage_selected);
        secp256k1_fe_normalize_var(&storage_result);
        FUZZ_CHECK(secp256k1_fuzz_fe_identical(&storage_result, &expected));

        storage_alias = storage_a;
        secp256k1_fe_storage_cmov(&storage_alias, &storage_alias, flag);
        secp256k1_fe_from_storage(&storage_result, &storage_alias);
        secp256k1_fe_normalize_var(&storage_result);
        FUZZ_CHECK(secp256k1_fuzz_fe_identical(&storage_result, a));
    }
}

#if defined(SECP256K1_WIDEMUL_INT64)
static void secp256k1_fuzz_fe_check_shifted_zero(void) {
    secp256k1_fe shifted_zero;
    secp256k1_fe expected;

    /* This is p with 63 base-2^26 units shifted from limb 1 into limb 0.
     * It is still a valid magnitude-32 representation of zero, but its low
     * limb is close enough to UINT32_MAX to exercise the carry into limb 1. */
    secp256k1_fe_get_bounds(&shifted_zero, 32);
    shifted_zero.n[0] = 0xFFFFFC2FUL;
    shifted_zero.n[1] = 0x03FFFF80UL;
    shifted_zero.n[2] = 0x03FFFFFFUL;
    shifted_zero.n[3] = 0x03FFFFFFUL;
    shifted_zero.n[4] = 0x03FFFFFFUL;
    shifted_zero.n[5] = 0x03FFFFFFUL;
    shifted_zero.n[6] = 0x03FFFFFFUL;
    shifted_zero.n[7] = 0x03FFFFFFUL;
    shifted_zero.n[8] = 0x03FFFFFFUL;
    shifted_zero.n[9] = 0x003FFFFFUL;
    secp256k1_fe_set_int(&expected, 0);
    secp256k1_fe_normalize_var(&expected);
    secp256k1_fuzz_fe_check_normalize_paths(&shifted_zero, &expected);
}
#endif

static void secp256k1_fuzz_fe_check_arithmetic(const unsigned char *input, size_t size) {
    unsigned char x32[32];
    unsigned char y32[32];
    unsigned char roundtrip32[32];
    secp256k1_fe x;
    secp256k1_fe y;
    secp256k1_fe x_copy;
    secp256k1_fe product;
    secp256k1_fe product_alias;
    secp256k1_fe square;
    secp256k1_fe square_alias;
    secp256k1_fe inverse;
    secp256k1_fe inverse_var;
    secp256k1_fe inverse_alias;
    secp256k1_fe inverse_var_alias;
    secp256k1_fe inverse_product;
    secp256k1_fe negative_square;
    secp256k1_fe root;
    secp256k1_fe root_squared;
    secp256k1_fe zero7;
    secp256k1_fe raised_square;
    secp256k1_fe raised_negative_square;
    secp256k1_fe_storage storage;
    int x_is_zero;
    int sqrt_ret;

    secp256k1_fuzz_derive(x32, sizeof(x32), input, size, 101);
    secp256k1_fuzz_derive(y32, sizeof(y32), input, size, 137);
    secp256k1_fe_set_b32_mod(&x, x32);
    secp256k1_fe_set_b32_mod(&y, y32);
    secp256k1_fe_normalize_var(&x);
    secp256k1_fe_normalize_var(&y);
    x_is_zero = secp256k1_fe_is_zero(&x);
    secp256k1_fuzz_fe_check_cmov(&x, &y);

    /* Canonical encoding, comparison, parity, and storage must agree. */
    secp256k1_fe_get_b32(roundtrip32, &x);
    FUZZ_CHECK(secp256k1_fe_is_odd(&x) == (int)(roundtrip32[31] & 1u));
    FUZZ_CHECK(secp256k1_fe_cmp_var(&x, &x) == 0);
    secp256k1_fe_to_storage(&storage, &x);
    secp256k1_fe_from_storage(&x_copy, &storage);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&x, &x_copy));

    /* Multiplication must agree with its documented r == a aliasing case. */
    secp256k1_fe_mul(&product, &x, &y);
    secp256k1_fe_normalize_var(&product);
    product_alias = x;
    secp256k1_fe_mul(&product_alias, &product_alias, &y);
    secp256k1_fe_normalize_var(&product_alias);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&product, &product_alias));

    /* Squaring is multiplication with identical values and supports r == a. */
    secp256k1_fe_sqr(&square, &x);
    secp256k1_fe_normalize_var(&square);
    square_alias = x;
    secp256k1_fe_sqr(&square_alias, &square_alias);
    secp256k1_fe_normalize_var(&square_alias);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&square, &square_alias));
    x_copy = x;
    secp256k1_fe_mul(&product_alias, &x, &x_copy);
    secp256k1_fe_normalize_var(&product_alias);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&square, &product_alias));

    /* Constant-time and variable-time inversion must have the same result. */
    secp256k1_fe_inv(&inverse, &x);
    secp256k1_fe_inv_var(&inverse_var, &x);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&inverse, &inverse_var));
    inverse_alias = x;
    secp256k1_fe_inv(&inverse_alias, &inverse_alias);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&inverse_alias, &inverse));
    inverse_var_alias = x;
    secp256k1_fe_inv_var(&inverse_var_alias, &inverse_var_alias);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&inverse_var_alias, &inverse_var));
    if (x_is_zero) {
        FUZZ_CHECK(secp256k1_fe_is_zero(&inverse));
    } else {
        secp256k1_fe_mul(&inverse_product, &x, &inverse);
        secp256k1_fe_normalize_var(&inverse_product);
        FUZZ_CHECK(secp256k1_fuzz_fe_identical(&inverse_product, &secp256k1_fe_one));
    }

    /* sqrt and is_square_var must agree for both a square and its negation. */
    FUZZ_CHECK(secp256k1_fe_is_square_var(&square) == 1);
    sqrt_ret = secp256k1_fe_sqrt(&root, &square);
    FUZZ_CHECK(sqrt_ret == 1);
    secp256k1_fe_sqr(&root_squared, &root);
    secp256k1_fe_normalize_var(&root_squared);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&root_squared, &square));

    secp256k1_fe_negate(&negative_square, &square, 1);
    secp256k1_fe_normalize_var(&negative_square);
    FUZZ_CHECK(secp256k1_fe_is_square_var(&negative_square) == x_is_zero);
    sqrt_ret = secp256k1_fe_sqrt(&root, &negative_square);
    FUZZ_CHECK(sqrt_ret == x_is_zero);
    secp256k1_fe_sqr(&root_squared, &root);
    secp256k1_fe_normalize_var(&root_squared);
    if (x_is_zero) {
        FUZZ_CHECK(secp256k1_fuzz_fe_identical(&root_squared, &negative_square));
    } else {
        FUZZ_CHECK(secp256k1_fuzz_fe_identical(&root_squared, &square));
    }

    /* is_square_var accepts inputs up to magnitude 8, not only normalized
     * values. Raise both a square and its negation without changing value. */
    secp256k1_fe_set_int(&zero7, 0);
    secp256k1_fe_negate(&zero7, &zero7, 0);
    secp256k1_fe_mul_int_unchecked(&zero7, 7);
    raised_square = square;
    secp256k1_fe_add(&raised_square, &zero7);
    raised_negative_square = negative_square;
    secp256k1_fe_add(&raised_negative_square, &zero7);
#ifdef VERIFY
    FUZZ_CHECK(raised_square.magnitude == 8);
    FUZZ_CHECK(raised_negative_square.magnitude == 8);
#endif
    FUZZ_CHECK(secp256k1_fe_is_square_var(&raised_square) == 1);
    FUZZ_CHECK(secp256k1_fe_is_square_var(&raised_negative_square) == x_is_zero);
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    int i;

    secp256k1_fuzz_fe_check_bounds_sum(16, 16);
    for (i = 0; i < 4; i++) {
        unsigned char selector = secp256k1_fuzz_byte(input, size, 17 + (size_t)i);
        int left_magnitude = 1 + (selector % 31);
        int right_magnitude = 32 - left_magnitude;
        secp256k1_fuzz_fe_check_bounds_sum(left_magnitude, right_magnitude);
    }
    secp256k1_fuzz_fe_check_raised_zero(input, size);
    secp256k1_fuzz_fe_check_half(input, size);
#if defined(SECP256K1_WIDEMUL_INT64)
    secp256k1_fuzz_fe_check_shifted_zero();
#endif
    secp256k1_fuzz_fe_check_arithmetic(input, size);

    return 0;
}
