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

/* get_bounds(m) represents 2*m times the all-ones field value. Therefore a
 * sum whose magnitudes total 32 has the exact value 64 * (2^256 - 1). Since
 * p = 2^256 - 2^32 - 977, its canonical residue is 64 * (2^32 + 976).
 * Keep this reference byte-level so a shared normalization mistake cannot
 * make both the input and a production-derived reference agree. */
static void secp256k1_fuzz_fe_check_magnitude32_reference(const secp256k1_fe *input) {
    unsigned char expected32[32] = { 0 };
    unsigned char actual32[32];
    secp256k1_fe normalized;
    uint64_t expected_value = UINT64_C(64) * ((UINT64_C(1) << 32) + UINT64_C(976));
    size_t i;

    for (i = 0; i < sizeof(expected_value); i++) {
        expected32[sizeof(expected32) - 1 - i] = (unsigned char)expected_value;
        expected_value >>= 8;
    }

    normalized = *input;
    secp256k1_fe_normalize(&normalized);
    secp256k1_fe_get_b32(actual32, &normalized);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);

    normalized = *input;
    secp256k1_fe_normalize_var(&normalized);
    secp256k1_fe_get_b32(actual32, &normalized);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);

    normalized = *input;
    secp256k1_fe_normalize_weak(&normalized);
    secp256k1_fe_normalize_var(&normalized);
    secp256k1_fe_get_b32(actual32, &normalized);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
}

static void secp256k1_fuzz_fe_check_set_b32_mod(const unsigned char *input32);

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
    secp256k1_fuzz_fe_check_magnitude32_reference(&input);

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
    secp256k1_fuzz_fe_check_set_b32_mod(bytes32);
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

static void secp256k1_fuzz_fe_check_max_magnitude_inverse(const unsigned char *input, size_t size) {
    unsigned char value32[32];
    secp256k1_fe canonical;
    secp256k1_fe raised;
    secp256k1_fe zero31;
    secp256k1_fe expected;
    secp256k1_fe actual;
    secp256k1_fe expected_var;
    secp256k1_fe actual_var;

    /* fe_inv and fe_inv_var accept every valid magnitude. Compare the same
     * residue in canonical and maximum-valid representations so a backend
     * cannot accidentally depend on the representation's carry budget. */
    secp256k1_fuzz_derive(value32, sizeof(value32), input, size, 193);
    secp256k1_fe_set_b32_mod(&canonical, value32);
    secp256k1_fe_normalize_var(&canonical);

    secp256k1_fe_set_int(&zero31, 0);
    secp256k1_fe_negate(&zero31, &zero31, 0);
    secp256k1_fe_mul_int_unchecked(&zero31, 31);
    raised = canonical;
    secp256k1_fe_add(&raised, &zero31);
#ifdef VERIFY
    FUZZ_CHECK(raised.magnitude == 32);
    FUZZ_CHECK(raised.normalized == 0);
#endif

    secp256k1_fe_inv(&expected, &canonical);
    secp256k1_fe_inv(&actual, &raised);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&actual, &expected));

    secp256k1_fe_inv_var(&expected_var, &canonical);
    secp256k1_fe_inv_var(&actual_var, &raised);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&actual_var, &expected_var));
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&expected, &expected_var));
}

static void secp256k1_fuzz_fe_check_add_int_boundary(void) {
    secp256k1_fe actual;
    secp256k1_fe expected;

    /* add_int raises magnitude by one, so 31 is its largest valid input. */
    secp256k1_fe_get_bounds(&actual, 31);
    expected = actual;
    secp256k1_fe_normalize_var(&expected);
    secp256k1_fe_add_int(&actual, 0x7FFF);
    secp256k1_fe_add_int(&expected, 0x7FFF);
#ifdef VERIFY
    FUZZ_CHECK(actual.magnitude == 32);
    FUZZ_CHECK(expected.magnitude == 2);
#endif
    secp256k1_fe_normalize_var(&actual);
    secp256k1_fe_normalize_var(&expected);
    FUZZ_CHECK(secp256k1_fuzz_fe_identical(&actual, &expected));
}

static const unsigned char secp256k1_fuzz_field_prime[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
};

static void secp256k1_fuzz_fe_negate_reference(unsigned char *out32, const unsigned char *in32) {
    size_t i;
    int borrow = 0;
    int is_zero = 1;

    for (i = 0; i < 32; i++) {
        is_zero &= in32[i] == 0;
    }
    if (is_zero) {
        memset(out32, 0, 32);
        return;
    }
    for (i = 32; i-- > 0;) {
        int value = (int)secp256k1_fuzz_field_prime[i] - in32[i] - borrow;
        if (value < 0) {
            value += 256;
            borrow = 1;
        } else {
            borrow = 0;
        }
        out32[i] = (unsigned char)value;
    }
    FUZZ_CHECK(borrow == 0);
}

static void secp256k1_fuzz_fe_mul_int_reference(unsigned char *out32, const unsigned char *in32, unsigned int multiplier) {
    unsigned char value32[32] = { 0 };
    unsigned int round;

    FUZZ_CHECK(multiplier <= 32);
    for (round = 0; round < multiplier; round++) {
        unsigned int carry = 0;
        size_t i;

        for (i = 32; i-- > 0;) {
            unsigned int value = (unsigned int)value32[i] + in32[i] + carry;
            value32[i] = (unsigned char)value;
            carry = value >> 8;
        }
        if (carry != 0 || memcmp(value32, secp256k1_fuzz_field_prime, 32) >= 0) {
            int borrow = 0;

            for (i = 32; i-- > 0;) {
                int value = (int)value32[i] - secp256k1_fuzz_field_prime[i] - borrow;
                if (value < 0) {
                    value += 256;
                    borrow = 1;
                } else {
                    borrow = 0;
                }
                value32[i] = (unsigned char)value;
            }
            if (carry == 0) {
                FUZZ_CHECK(borrow == 0);
            }
        }
    }
    memcpy(out32, value32, sizeof(value32));
}

static void secp256k1_fuzz_fe_add_int_reference(unsigned char *out32, const unsigned char *in32, unsigned int addend) {
    unsigned int carry = addend;
    size_t i;

    memcpy(out32, in32, 32);
    for (i = 32; i-- > 0;) {
        unsigned int value = (unsigned int)out32[i] + carry;
        out32[i] = (unsigned char)value;
        carry = value >> 8;
    }
    if (carry != 0 || memcmp(out32, secp256k1_fuzz_field_prime, 32) >= 0) {
        int borrow = 0;

        for (i = 32; i-- > 0;) {
            int value = (int)out32[i] - secp256k1_fuzz_field_prime[i] - borrow;
            if (value < 0) {
                value += 256;
                borrow = 1;
            } else {
                borrow = 0;
            }
            out32[i] = (unsigned char)value;
        }
        if (carry == 0) {
            FUZZ_CHECK(borrow == 0);
        }
    }
}

static void secp256k1_fuzz_fe_check_negation(const unsigned char *input, size_t size) {
    unsigned char value32[32];
    unsigned char expected32[32];
    unsigned char actual32[32];
    secp256k1_fe value;
    secp256k1_fe zero7;
    secp256k1_fe raised;
    secp256k1_fe negated;

    secp256k1_fuzz_derive(value32, sizeof(value32), input, size, 227);
    secp256k1_fe_set_b32_mod(&value, value32);
    secp256k1_fe_normalize_var(&value);
    secp256k1_fe_get_b32(value32, &value);
    secp256k1_fuzz_fe_negate_reference(expected32, value32);

    secp256k1_fe_negate(&negated, &value, 1);
#ifdef VERIFY
    FUZZ_CHECK(negated.magnitude == 2);
    FUZZ_CHECK(negated.normalized == 0);
#endif
    secp256k1_fe_normalize_var(&negated);
    secp256k1_fe_get_b32(actual32, &negated);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);

    /* Preserve the same residue in a nonnormalized representation accepted by
     * fe_negate, so the output-magnitude contract is exercised independently
     * of the canonical path. */
    secp256k1_fe_set_int(&zero7, 0);
    secp256k1_fe_negate(&zero7, &zero7, 0);
    secp256k1_fe_mul_int_unchecked(&zero7, 7);
    raised = value;
    secp256k1_fe_add(&raised, &zero7);
#ifdef VERIFY
    FUZZ_CHECK(raised.magnitude == 8);
    FUZZ_CHECK(raised.normalized == 0);
#endif
    secp256k1_fe_negate(&negated, &raised, 8);
#ifdef VERIFY
    FUZZ_CHECK(negated.magnitude == 9);
    FUZZ_CHECK(negated.normalized == 0);
#endif
    secp256k1_fe_normalize_var(&negated);
    secp256k1_fe_get_b32(actual32, &negated);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
}

static void secp256k1_fuzz_fe_check_mul_int(const unsigned char *input, size_t size) {
    unsigned char value32[32];
    unsigned char expected32[32];
    unsigned char actual32[32];
    secp256k1_fe value;

    secp256k1_fuzz_derive(value32, sizeof(value32), input, size, 229);
    secp256k1_fe_set_b32_mod(&value, value32);
    secp256k1_fe_normalize_var(&value);
    secp256k1_fe_get_b32(value32, &value);
    secp256k1_fuzz_fe_mul_int_reference(expected32, value32, 5);

    secp256k1_fe_mul_int(&value, 5);
#ifdef VERIFY
    FUZZ_CHECK(value.magnitude == 5);
    FUZZ_CHECK(value.normalized == 0);
#endif
    secp256k1_fe_normalize_var(&value);
    secp256k1_fe_get_b32(actual32, &value);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
}

static void secp256k1_fuzz_fe_check_add_int(const unsigned char *input, size_t size) {
    unsigned char value32[32];
    unsigned char expected32[32];
    unsigned char actual32[32];
    secp256k1_fe value;

    secp256k1_fuzz_derive(value32, sizeof(value32), input, size, 231);
    secp256k1_fe_set_b32_mod(&value, value32);
    secp256k1_fe_normalize_var(&value);
    secp256k1_fe_get_b32(value32, &value);
    secp256k1_fuzz_fe_add_int_reference(expected32, value32, 7);

    secp256k1_fe_add_int(&value, 7);
#ifdef VERIFY
    FUZZ_CHECK(value.magnitude == 2);
    FUZZ_CHECK(value.normalized == 0);
#endif
    secp256k1_fe_normalize_var(&value);
    secp256k1_fe_get_b32(actual32, &value);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
}

static void secp256k1_fuzz_fe_check_set_b32_limit(const unsigned char *input32) {
    secp256k1_fe actual;
    unsigned char actual32[32];
    int expected = memcmp(input32, secp256k1_fuzz_field_prime, 32) < 0;
    int actual_ret = secp256k1_fe_set_b32_limit(&actual, input32);

    FUZZ_CHECK(actual_ret == expected);
    if (actual_ret) {
#ifdef VERIFY
        FUZZ_CHECK(actual.normalized == 1);
        FUZZ_CHECK(actual.magnitude == 1);
#endif
        secp256k1_fe_get_b32(actual32, &actual);
        FUZZ_CHECK(memcmp(actual32, input32, sizeof(actual32)) == 0);
    }
}

static void secp256k1_fuzz_fe_reduce_reference(unsigned char *out32, const unsigned char *input32) {
    size_t i;
    int borrow = 0;

    if (memcmp(input32, secp256k1_fuzz_field_prime, 32) < 0) {
        memcpy(out32, input32, 32);
        return;
    }
    for (i = 32; i-- > 0;) {
        int value = (int)input32[i] - secp256k1_fuzz_field_prime[i] - borrow;
        if (value < 0) {
            value += 256;
            borrow = 1;
        } else {
            borrow = 0;
        }
        out32[i] = (unsigned char)value;
    }
    FUZZ_CHECK(borrow == 0);
}

static void secp256k1_fuzz_fe_check_set_b32_mod(const unsigned char *input32) {
    secp256k1_fe actual;
    unsigned char expected32[32];
    unsigned char actual32[32];

    secp256k1_fuzz_fe_reduce_reference(expected32, input32);
    secp256k1_fe_set_b32_mod(&actual, input32);
    secp256k1_fe_normalize_var(&actual);
    secp256k1_fe_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
}

/* A small standalone 8x32-bit modular arithmetic model. It deliberately does
 * not call field or modular-inverse helpers from the library under test. */
static void secp256k1_fuzz_ref_u32_from_be(uint32_t out[8], const unsigned char in[32]) {
    size_t i;

    for (i = 0; i < 8; i++) {
        size_t offset = 32 - 4 * (i + 1);
        out[i] = ((uint32_t)in[offset] << 24)
               | ((uint32_t)in[offset + 1] << 16)
               | ((uint32_t)in[offset + 2] << 8)
               | (uint32_t)in[offset + 3];
    }
}

static void secp256k1_fuzz_ref_u32_to_be(unsigned char out[32], const uint32_t in[8]) {
    size_t i;

    for (i = 0; i < 8; i++) {
        size_t offset = 32 - 4 * (i + 1);
        out[offset] = (unsigned char)(in[i] >> 24);
        out[offset + 1] = (unsigned char)(in[i] >> 16);
        out[offset + 2] = (unsigned char)(in[i] >> 8);
        out[offset + 3] = (unsigned char)in[i];
    }
}

static int secp256k1_fuzz_ref_u32_ge_modulus(const uint32_t a[8], const uint32_t modulus[8]) {
    size_t i;

    for (i = 8; i-- > 0;) {
        if (a[i] != modulus[i]) {
            return a[i] > modulus[i];
        }
    }
    return 1;
}

static void secp256k1_fuzz_ref_u32_mul_mod(uint32_t out[8], const uint32_t a[8], const uint32_t b[8], const uint32_t modulus[8]) {
    uint32_t product[16] = { 0 };
    uint32_t remainder[9] = { 0 };
    size_t i;
    size_t j;

    /* Schoolbook multiplication uses only 32x32 -> 64-bit products. */
    for (i = 0; i < 8; i++) {
        uint64_t carry = 0;
        for (j = 0; j < 8; j++) {
            uint64_t value = (uint64_t)product[i + j]
                           + (uint64_t)a[i] * b[j]
                           + carry;
            product[i + j] = (uint32_t)value;
            carry = value >> 32;
        }
        for (j = i + 8; carry != 0; j++) {
            uint64_t value = (uint64_t)product[j] + carry;
            product[j] = (uint32_t)value;
            carry = value >> 32;
        }
    }

    /* Reduce the 512-bit product with restoring binary long division. */
    for (i = 512; i-- > 0;) {
        uint32_t carry = (product[i / 32] >> (i % 32)) & 1u;
        int ge_modulus;

        for (j = 0; j < 9; j++) {
            uint32_t next_carry = remainder[j] >> 31;
            remainder[j] = (remainder[j] << 1) | carry;
            carry = next_carry;
        }

        ge_modulus = remainder[8] != 0 || secp256k1_fuzz_ref_u32_ge_modulus(remainder, modulus);
        if (ge_modulus) {
            uint32_t borrow = 0;
            for (j = 0; j < 8; j++) {
                uint64_t subtrahend = (uint64_t)modulus[j] + borrow;
                uint32_t minuend = remainder[j];
                remainder[j] = (uint32_t)((uint64_t)minuend - subtrahend);
                borrow = (uint32_t)((uint64_t)minuend < subtrahend);
            }
            remainder[8] -= borrow;
        }
    }
    memcpy(out, remainder, sizeof(uint32_t) * 8);
}

static void secp256k1_fuzz_ref_field_inverse(unsigned char out32[32], const unsigned char input32[32]) {
    uint32_t modulus[8];
    uint32_t base[8];
    uint32_t result[8];
    unsigned char exponent[32];
    size_t bit;

    secp256k1_fuzz_ref_u32_from_be(modulus, secp256k1_fuzz_field_prime);
    secp256k1_fuzz_ref_u32_from_be(base, input32);
    memcpy(exponent, secp256k1_fuzz_field_prime, sizeof(exponent));
    exponent[31] -= 2;
    memset(result, 0, sizeof(result));
    result[0] = 1;

    /* Fermat's little theorem: x^(-1) = x^(p-2) in this prime field. */
    for (bit = 0; bit < 256; bit++) {
        uint32_t squared[8];
        secp256k1_fuzz_ref_u32_mul_mod(squared, result, result, modulus);
        memcpy(result, squared, sizeof(result));
        if ((exponent[bit / 8] & (unsigned char)(0x80u >> (bit % 8))) != 0) {
            uint32_t multiplied[8];
            secp256k1_fuzz_ref_u32_mul_mod(multiplied, result, base, modulus);
            memcpy(result, multiplied, sizeof(result));
        }
    }
    secp256k1_fuzz_ref_u32_to_be(out32, result);
}

/* Compute the exponentiation candidate independently. For this field p == 3
 * (mod 4), so a^((p + 1) / 4) is a square root exactly when a is a quadratic
 * residue. The reference accepts either root sign from production. */
static int secp256k1_fuzz_ref_field_sqrt(const unsigned char input32[32]) {
    uint32_t modulus[8];
    uint32_t base[8];
    uint32_t result[8];
    uint32_t squared[8];
    uint32_t input[8];
    unsigned char exponent[32];
    unsigned int carry = 1;
    size_t bit;
    size_t i;

    secp256k1_fuzz_ref_u32_from_be(modulus, secp256k1_fuzz_field_prime);
    secp256k1_fuzz_ref_u32_from_be(base, input32);
    secp256k1_fuzz_ref_u32_from_be(input, input32);
    memcpy(exponent, secp256k1_fuzz_field_prime, sizeof(exponent));
    for (i = sizeof(exponent); i-- > 0 && carry != 0;) {
        unsigned int value = (unsigned int)exponent[i] + carry;
        exponent[i] = (unsigned char)value;
        carry = value >> 8;
    }
    FUZZ_CHECK(carry == 0);
    carry = 0;
    for (i = 0; i < sizeof(exponent); i++) {
        unsigned int value = (carry << 8) | exponent[i];
        exponent[i] = (unsigned char)(value >> 2);
        carry = value & 3u;
    }
    FUZZ_CHECK(carry == 0);

    memset(result, 0, sizeof(result));
    result[0] = 1;
    for (bit = 0; bit < 256; bit++) {
        uint32_t squared_result[8];
        secp256k1_fuzz_ref_u32_mul_mod(squared_result, result, result, modulus);
        memcpy(result, squared_result, sizeof(result));
        if ((exponent[bit / 8] & (unsigned char)(0x80u >> (bit % 8))) != 0) {
            uint32_t multiplied[8];
            secp256k1_fuzz_ref_u32_mul_mod(multiplied, result, base, modulus);
            memcpy(result, multiplied, sizeof(result));
        }
    }
    secp256k1_fuzz_ref_u32_mul_mod(squared, result, result, modulus);
    return secp256k1_memcmp_var(squared, input, sizeof(squared)) == 0;
}

static int secp256k1_fuzz_ref_field_square_matches(const unsigned char root32[32], const unsigned char input32[32]) {
    uint32_t modulus[8];
    uint32_t root[8];
    uint32_t input[8];
    uint32_t squared[8];

    secp256k1_fuzz_ref_u32_from_be(modulus, secp256k1_fuzz_field_prime);
    secp256k1_fuzz_ref_u32_from_be(root, root32);
    secp256k1_fuzz_ref_u32_from_be(input, input32);
    secp256k1_fuzz_ref_u32_mul_mod(squared, root, root, modulus);
    return secp256k1_memcmp_var(squared, input, sizeof(squared)) == 0;
}

static void secp256k1_fuzz_fe_check_sqrt_reference(const unsigned char *input, size_t size) {
    unsigned char value32[32];
    unsigned char actual_root32[32];
    secp256k1_fe value;
    secp256k1_fe raised;
    secp256k1_fe zero7;
    secp256k1_fe root;
    int expected_ret;
    int actual_ret;

    secp256k1_fuzz_derive(value32, sizeof(value32), input, size, 235);
    secp256k1_fe_set_b32_mod(&value, value32);
    secp256k1_fe_normalize_var(&value);
    secp256k1_fe_get_b32(value32, &value);
    expected_ret = secp256k1_fuzz_ref_field_sqrt(value32);
    actual_ret = secp256k1_fe_sqrt(&root, &value);
    FUZZ_CHECK(actual_ret == expected_ret);
    if (actual_ret) {
        secp256k1_fe_normalize_var(&root);
        secp256k1_fe_get_b32(actual_root32, &root);
        FUZZ_CHECK(secp256k1_fuzz_ref_field_square_matches(actual_root32, value32));
    }

    /* The same residue must produce the same success decision when supplied in
     * the largest representation accepted by fe_sqrt. */
    secp256k1_fe_set_int(&zero7, 0);
    secp256k1_fe_negate(&zero7, &zero7, 0);
    secp256k1_fe_mul_int_unchecked(&zero7, 7);
    raised = value;
    secp256k1_fe_add(&raised, &zero7);
    actual_ret = secp256k1_fe_sqrt(&root, &raised);
    FUZZ_CHECK(actual_ret == expected_ret);
    if (actual_ret) {
        secp256k1_fe_normalize_var(&root);
        secp256k1_fe_get_b32(actual_root32, &root);
        FUZZ_CHECK(secp256k1_fuzz_ref_field_square_matches(actual_root32, value32));
    }

    /* Include the zero residue explicitly: zero has a defined square root. */
    secp256k1_fe_set_int(&value, 0);
    memset(value32, 0, sizeof(value32));
    expected_ret = secp256k1_fuzz_ref_field_sqrt(value32);
    actual_ret = secp256k1_fe_sqrt(&root, &value);
    FUZZ_CHECK(actual_ret == expected_ret);
    FUZZ_CHECK(actual_ret == 1);
    secp256k1_fe_normalize_var(&root);
    secp256k1_fe_get_b32(actual_root32, &root);
    FUZZ_CHECK(secp256k1_fuzz_ref_field_square_matches(actual_root32, value32));
}

static void secp256k1_fuzz_fe_check_inverse_reference(const unsigned char *input, size_t size) {
    unsigned char value32[32];
    unsigned char expected32[32];
    unsigned char actual32[32];
    secp256k1_fe value;
    secp256k1_fe raised;
    secp256k1_fe zero31;
    secp256k1_fe inverse;
    secp256k1_fe inverse_var;

    secp256k1_fuzz_derive(value32, sizeof(value32), input, size, 233);
    secp256k1_fuzz_fe_check_set_b32_mod(value32);
    secp256k1_fe_set_b32_mod(&value, value32);
    secp256k1_fe_normalize_var(&value);
    secp256k1_fe_get_b32(actual32, &value);
    secp256k1_fuzz_ref_field_inverse(expected32, actual32);

    secp256k1_fe_inv(&inverse, &value);
    secp256k1_fe_normalize_var(&inverse);
    secp256k1_fe_get_b32(actual32, &inverse);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);

    secp256k1_fe_inv_var(&inverse_var, &value);
    secp256k1_fe_normalize_var(&inverse_var);
    secp256k1_fe_get_b32(actual32, &inverse_var);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);

    /* Preserve the residue while raising the representation to the maximum
     * accepted magnitude, so the reference covers that contract as well. */
    secp256k1_fe_set_int(&zero31, 0);
    secp256k1_fe_negate(&zero31, &zero31, 0);
    secp256k1_fe_mul_int_unchecked(&zero31, 31);
    raised = value;
    secp256k1_fe_add(&raised, &zero31);
    secp256k1_fe_inv(&inverse, &raised);
    secp256k1_fe_normalize_var(&inverse);
    secp256k1_fe_get_b32(actual32, &inverse);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
    secp256k1_fe_inv_var(&inverse_var, &raised);
    secp256k1_fe_normalize_var(&inverse_var);
    secp256k1_fe_get_b32(actual32, &inverse_var);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);

    /* Zero is the one residue whose inverse is defined as zero by this API. */
    memset(value32, 0, sizeof(value32));
    secp256k1_fe_set_int(&value, 0);
    secp256k1_fuzz_ref_field_inverse(expected32, value32);
    secp256k1_fe_inv(&inverse, &value);
    secp256k1_fe_normalize_var(&inverse);
    secp256k1_fe_get_b32(actual32, &inverse);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
    secp256k1_fe_inv_var(&inverse_var, &value);
    secp256k1_fe_normalize_var(&inverse_var);
    secp256k1_fe_get_b32(actual32, &inverse_var);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);

    raised = value;
    secp256k1_fe_add(&raised, &zero31);
    secp256k1_fe_inv(&inverse, &raised);
    secp256k1_fe_normalize_var(&inverse);
    secp256k1_fe_get_b32(actual32, &inverse);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
    secp256k1_fe_inv_var(&inverse_var, &raised);
    secp256k1_fe_normalize_var(&inverse_var);
    secp256k1_fe_get_b32(actual32, &inverse_var);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
}

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

static void secp256k1_fuzz_fe_check_comparisons(const unsigned char *input, size_t size) {
    secp256k1_fe canonical;
    secp256k1_fe other;
    secp256k1_fe zero29;
    secp256k1_fe a;
    secp256k1_fe b;
    unsigned char canonical32[32];
    unsigned char other32[32];
    int expected_cmp;

    secp256k1_fuzz_derive(canonical32, sizeof(canonical32), input, size, 179);
    secp256k1_fuzz_derive(other32, sizeof(other32), input, size, 181);
    secp256k1_fe_set_b32_mod(&canonical, canonical32);
    secp256k1_fe_set_b32_mod(&other, other32);
    secp256k1_fe_normalize_var(&canonical);
    secp256k1_fe_normalize_var(&other);
    secp256k1_fe_get_b32(canonical32, &canonical);
    secp256k1_fe_get_b32(other32, &other);
    expected_cmp = memcmp(canonical32, other32, sizeof(canonical32));
    expected_cmp = (expected_cmp > 0) - (expected_cmp < 0);
    FUZZ_CHECK(secp256k1_fe_cmp_var(&canonical, &other) == expected_cmp);
    FUZZ_CHECK(secp256k1_fe_cmp_var(&other, &canonical) == -expected_cmp);

    secp256k1_fe_set_int(&a, 0);
    secp256k1_fe_set_int(&b, 1);
    FUZZ_CHECK(secp256k1_fe_cmp_var(&a, &b) == -1);
    FUZZ_CHECK(secp256k1_fe_cmp_var(&b, &a) == 1);

    /* Pin the documented maximum b magnitude of fe_equal. */
    secp256k1_fe_set_int(&zero29, 0);
    secp256k1_fe_negate(&zero29, &zero29, 0);
    secp256k1_fe_mul_int_unchecked(&zero29, 29);
    b = canonical;
    secp256k1_fe_add(&b, &zero29);
#ifdef VERIFY
    FUZZ_CHECK(b.magnitude == 30);
#endif
    FUZZ_CHECK(secp256k1_fe_equal(&canonical, &b));

    other = canonical;
    secp256k1_fe_add_int(&other, 1);
    secp256k1_fe_normalize_var(&other);
    b = other;
    secp256k1_fe_add(&b, &zero29);
#ifdef VERIFY
    FUZZ_CHECK(b.magnitude == 30);
#endif
    FUZZ_CHECK(!secp256k1_fe_equal(&canonical, &b));
}

static void secp256k1_fuzz_fe_check_nonnormalized_arithmetic(void) {
    unsigned char expected_product32[32];
    unsigned char actual_product32[32];
    uint32_t modulus[8];
    uint32_t one_words[8];
    uint32_t two_words[8];
    uint32_t product_words[8];
    secp256k1_fe one;
    secp256k1_fe two;
    secp256k1_fe zero7;
    secp256k1_fe raised_one;
    secp256k1_fe raised_two;
    secp256k1_fe expected_product;
    secp256k1_fe actual_product;
    secp256k1_fe expected_square;
    secp256k1_fe actual_square;
    secp256k1_fe expected_inverse;
    secp256k1_fe actual_inverse;
    secp256k1_fe alias_product;
    secp256k1_fe root;
    secp256k1_fe root_squared;

    secp256k1_fe_set_int(&one, 1);
    secp256k1_fe_set_int(&two, 2);
    secp256k1_fe_set_int(&zero7, 0);
    secp256k1_fe_negate(&zero7, &zero7, 0);
    secp256k1_fe_mul_int_unchecked(&zero7, 7);
    raised_one = one;
    raised_two = two;
    secp256k1_fe_add(&raised_one, &zero7);
    secp256k1_fe_add(&raised_two, &zero7);
#ifdef VERIFY
    FUZZ_CHECK(raised_one.magnitude == 8);
    FUZZ_CHECK(raised_one.normalized == 0);
    FUZZ_CHECK(raised_two.magnitude == 8);
    FUZZ_CHECK(raised_two.normalized == 0);
#endif

    secp256k1_fe_mul(&expected_product, &one, &two);
    secp256k1_fe_normalize_var(&expected_product);
    secp256k1_fe_mul(&actual_product, &raised_one, &raised_two);
    secp256k1_fe_normalize_var(&actual_product);
    secp256k1_fuzz_fe_check_normalized(&actual_product, &expected_product);

    /* fe_mul permits r == a even at the largest accepted input magnitude.
     * Compare the aliased result against a byte-level reference rather than
     * only against another production multiplication. */
    memset(expected_product32, 0, sizeof(expected_product32));
    memset(actual_product32, 0, sizeof(actual_product32));
    expected_product32[31] = 1;
    actual_product32[31] = 2;
    secp256k1_fuzz_ref_u32_from_be(modulus, secp256k1_fuzz_field_prime);
    secp256k1_fuzz_ref_u32_from_be(one_words, expected_product32);
    secp256k1_fuzz_ref_u32_from_be(two_words, actual_product32);
    secp256k1_fuzz_ref_u32_mul_mod(product_words, one_words, two_words, modulus);
    secp256k1_fuzz_ref_u32_to_be(expected_product32, product_words);
    alias_product = raised_one;
    secp256k1_fe_mul(&alias_product, &alias_product, &raised_two);
    secp256k1_fe_normalize_var(&alias_product);
    secp256k1_fe_get_b32(actual_product32, &alias_product);
    FUZZ_CHECK(memcmp(actual_product32, expected_product32, sizeof(actual_product32)) == 0);

    secp256k1_fe_sqr(&expected_square, &one);
    secp256k1_fe_normalize_var(&expected_square);
    secp256k1_fe_sqr(&actual_square, &raised_one);
    secp256k1_fe_normalize_var(&actual_square);
    secp256k1_fuzz_fe_check_normalized(&actual_square, &expected_square);

    secp256k1_fe_inv(&expected_inverse, &one);
    secp256k1_fe_inv(&actual_inverse, &raised_one);
    secp256k1_fuzz_fe_check_normalized(&actual_inverse, &expected_inverse);

    FUZZ_CHECK(secp256k1_fe_is_square_var(&raised_one) == 1);
    FUZZ_CHECK(secp256k1_fe_sqrt(&root, &raised_one) == 1);
    secp256k1_fe_sqr(&root_squared, &root);
    secp256k1_fe_normalize_var(&root_squared);
    secp256k1_fuzz_fe_check_normalized(&root_squared, &one);
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

#if defined(SECP256K1_WIDEMUL_INT64)
static void secp256k1_fuzz_fe_check_zero_predicate_false_positive(void) {
    static const unsigned char expected32[32] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00
    };
    secp256k1_fe input;
    unsigned char actual32[32];

    secp256k1_fe_set_int(&input, 0);
    input.n[0] = 0xFFFF0F91UL;
    input.n[1] = 0xFFFFF040UL;
    input.n[9] = 0x0FC00000UL;
#ifdef VERIFY
    input.magnitude = 32;
    input.normalized = 0;
#endif
    FUZZ_CHECK(secp256k1_fe_normalizes_to_zero(&input) == 0);
    FUZZ_CHECK(secp256k1_fe_normalizes_to_zero_var(&input) == 0);
    secp256k1_fe_normalize_var(&input);
    secp256k1_fe_get_b32(actual32, &input);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
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
    unsigned char boundary[32];
    unsigned char limit_input[32];
    int i;

    secp256k1_fuzz_derive(limit_input, sizeof(limit_input), input, size, 11);
    secp256k1_fuzz_fe_check_set_b32_limit(limit_input);
    memcpy(boundary, secp256k1_fuzz_field_prime, sizeof(boundary));
    secp256k1_fuzz_fe_check_set_b32_limit(boundary);
    secp256k1_fuzz_fe_check_set_b32_mod(boundary);
    boundary[31]--;
    secp256k1_fuzz_fe_check_set_b32_limit(boundary);
    secp256k1_fuzz_fe_check_set_b32_mod(boundary);
    boundary[31] += 2;
    secp256k1_fuzz_fe_check_set_b32_limit(boundary);
    secp256k1_fuzz_fe_check_set_b32_mod(boundary);
    memset(boundary, 0, sizeof(boundary));
    secp256k1_fuzz_fe_check_set_b32_limit(boundary);
    memset(boundary, 0xFF, sizeof(boundary));
    secp256k1_fuzz_fe_check_set_b32_limit(boundary);
    secp256k1_fuzz_fe_check_set_b32_mod(boundary);
    secp256k1_fuzz_fe_check_bounds_sum(16, 16);
    for (i = 0; i < 4; i++) {
        unsigned char selector = secp256k1_fuzz_byte(input, size, 17 + (size_t)i);
        int left_magnitude = 1 + (selector % 31);
        int right_magnitude = 32 - left_magnitude;
        secp256k1_fuzz_fe_check_bounds_sum(left_magnitude, right_magnitude);
    }
    secp256k1_fuzz_fe_check_raised_zero(input, size);
    secp256k1_fuzz_fe_check_max_magnitude_inverse(input, size);
    secp256k1_fuzz_fe_check_inverse_reference(input, size);
    secp256k1_fuzz_fe_check_sqrt_reference(input, size);
    secp256k1_fuzz_fe_check_add_int_boundary();
    secp256k1_fuzz_fe_check_half(input, size);
    secp256k1_fuzz_fe_check_negation(input, size);
    secp256k1_fuzz_fe_check_mul_int(input, size);
    secp256k1_fuzz_fe_check_add_int(input, size);
    secp256k1_fuzz_fe_check_comparisons(input, size);
    secp256k1_fuzz_fe_check_nonnormalized_arithmetic();
#if defined(SECP256K1_WIDEMUL_INT64)
    secp256k1_fuzz_fe_check_shifted_zero();
    secp256k1_fuzz_fe_check_zero_predicate_false_positive();
#endif
    secp256k1_fuzz_fe_check_arithmetic(input, size);

    return 0;
}
