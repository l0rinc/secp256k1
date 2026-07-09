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

    return 0;
}
