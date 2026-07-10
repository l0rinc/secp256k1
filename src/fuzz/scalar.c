/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../secp256k1.c"
#include "fuzz.h"

#define SECP256K1_FUZZ_SCALAR_LIMBS 16
#define SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS 32

static void secp256k1_fuzz_scalar_bytes_to_limbs(uint16_t *out, const unsigned char *in32) {
    int i;

    for (i = 0; i < SECP256K1_FUZZ_SCALAR_LIMBS; i++) {
        out[i] = (uint16_t)in32[31 - 2 * i] | (uint16_t)((uint16_t)in32[30 - 2 * i] << 8);
    }
}

static void secp256k1_fuzz_scalar_limbs_to_bytes(unsigned char *out32, const uint16_t *in) {
    int i;

    for (i = 0; i < SECP256K1_FUZZ_SCALAR_LIMBS; i++) {
        out32[31 - 2 * i] = (unsigned char)in[i];
        out32[30 - 2 * i] = (unsigned char)(in[i] >> 8);
    }
}

/* Compute the full 512-bit product independently in base 2^16. */
static void secp256k1_fuzz_scalar_product(uint16_t *product, const unsigned char *a32, const unsigned char *b32) {
    uint16_t a[SECP256K1_FUZZ_SCALAR_LIMBS];
    uint16_t b[SECP256K1_FUZZ_SCALAR_LIMBS];
    uint64_t accum[SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS] = { 0 };
    int i;
    int j;

    secp256k1_fuzz_scalar_bytes_to_limbs(a, a32);
    secp256k1_fuzz_scalar_bytes_to_limbs(b, b32);
    for (i = 0; i < SECP256K1_FUZZ_SCALAR_LIMBS; i++) {
        for (j = 0; j < SECP256K1_FUZZ_SCALAR_LIMBS; j++) {
            accum[i + j] += (uint64_t)a[i] * b[j];
        }
    }
    for (i = 0; i < SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS - 1; i++) {
        accum[i + 1] += accum[i] >> 16;
        product[i] = (uint16_t)accum[i];
    }
    FUZZ_CHECK((accum[SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS - 1] >> 16) == 0);
    product[SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS - 1] = (uint16_t)accum[SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS - 1];
}

static unsigned int secp256k1_fuzz_scalar_product_bit(const uint16_t *product, unsigned int bit) {
    FUZZ_CHECK(bit < 512);
    return (product[bit >> 4] >> (bit & 15)) & 1u;
}

static void secp256k1_fuzz_scalar_shift_reference(unsigned char *out32, const uint16_t *product, unsigned int shift) {
    uint16_t rounded[SECP256K1_FUZZ_SCALAR_LIMBS] = { 0 };
    uint32_t carry;
    unsigned int bit;
    int i;

    FUZZ_CHECK(shift >= 256);
    if (shift > 512) {
        memset(out32, 0, 32);
        return;
    }

    for (bit = 0; bit < 256 && bit + shift < 512; bit++) {
        if (secp256k1_fuzz_scalar_product_bit(product, bit + shift)) {
            rounded[bit >> 4] |= (uint16_t)(1u << (bit & 15));
        }
    }
    carry = secp256k1_fuzz_scalar_product_bit(product, shift - 1);
    for (i = 0; i < SECP256K1_FUZZ_SCALAR_LIMBS; i++) {
        uint32_t value = (uint32_t)rounded[i] + carry;
        rounded[i] = (uint16_t)value;
        carry = value >> 16;
    }
    FUZZ_CHECK(carry == 0);
    secp256k1_fuzz_scalar_limbs_to_bytes(out32, rounded);
}

static void secp256k1_fuzz_scalar_decrement(unsigned char *out32, const unsigned char *in32) {
    int i;

    memcpy(out32, in32, 32);
    for (i = 31; i >= 0; i--) {
        if (out32[i] != 0) {
            out32[i]--;
            return;
        }
        out32[i] = 0xff;
    }
    FUZZ_CHECK(0);
}

static int secp256k1_fuzz_scalar_bytes_cmp(const unsigned char *a32, const unsigned char *b32) {
    size_t i;

    for (i = 0; i < 32; i++) {
        if (a32[i] != b32[i]) {
            return a32[i] < b32[i] ? -1 : 1;
        }
    }
    return 0;
}

/* Subtract b32 from a32, which must be at least b32. */
static void secp256k1_fuzz_scalar_bytes_sub(unsigned char *out32, const unsigned char *a32, const unsigned char *b32) {
    int borrow = 0;
    int i;

    for (i = 31; i >= 0; i--) {
        int value = (int)a32[i] - b32[i] - borrow;
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

/* Every 256-bit input is below twice the secp256k1 scalar order. */
static int secp256k1_fuzz_scalar_reduce_input(unsigned char *out32, const unsigned char *input32) {
    int overflow = secp256k1_fuzz_scalar_bytes_cmp(input32, secp256k1_fuzz_scalar_order) >= 0;

    if (overflow) {
        secp256k1_fuzz_scalar_bytes_sub(out32, input32, secp256k1_fuzz_scalar_order);
    } else {
        memcpy(out32, input32, 32);
    }
    return overflow;
}

static int secp256k1_fuzz_scalar_limbs_ge_order(const uint16_t *value, const uint16_t *order) {
    int i;

    if (value[SECP256K1_FUZZ_SCALAR_LIMBS] != 0) {
        return 1;
    }
    for (i = SECP256K1_FUZZ_SCALAR_LIMBS - 1; i >= 0; i--) {
        if (value[i] != order[i]) {
            return value[i] > order[i];
        }
    }
    return 1;
}

static void secp256k1_fuzz_scalar_limbs_sub_order(uint16_t *value, const uint16_t *order) {
    int borrow = 0;
    int i;

    for (i = 0; i < SECP256K1_FUZZ_SCALAR_LIMBS; i++) {
        int difference = (int)value[i] - order[i] - borrow;
        if (difference < 0) {
            difference += 0x10000;
            borrow = 1;
        } else {
            borrow = 0;
        }
        value[i] = (uint16_t)difference;
    }
    FUZZ_CHECK(value[SECP256K1_FUZZ_SCALAR_LIMBS] >= (uint16_t)borrow);
    value[SECP256K1_FUZZ_SCALAR_LIMBS] = (uint16_t)(value[SECP256K1_FUZZ_SCALAR_LIMBS] - borrow);
}

/* Reduce a full product with binary long division, independent of scalar code. */
static void secp256k1_fuzz_scalar_reduce_product(unsigned char *out32, const uint16_t *product) {
    uint16_t order[SECP256K1_FUZZ_SCALAR_LIMBS];
    uint16_t remainder[SECP256K1_FUZZ_SCALAR_LIMBS + 1] = { 0 };
    unsigned int bit;
    int i;

    secp256k1_fuzz_scalar_bytes_to_limbs(order, secp256k1_fuzz_scalar_order);
    for (bit = 512; bit > 0; bit--) {
        uint32_t carry = secp256k1_fuzz_scalar_product_bit(product, bit - 1u);
        for (i = 0; i <= SECP256K1_FUZZ_SCALAR_LIMBS; i++) {
            uint32_t value = ((uint32_t)remainder[i] << 1) | carry;
            remainder[i] = (uint16_t)value;
            carry = value >> 16;
        }
        FUZZ_CHECK(carry == 0);
        if (secp256k1_fuzz_scalar_limbs_ge_order(remainder, order)) {
            secp256k1_fuzz_scalar_limbs_sub_order(remainder, order);
        }
    }
    FUZZ_CHECK(remainder[SECP256K1_FUZZ_SCALAR_LIMBS] == 0);
    secp256k1_fuzz_scalar_limbs_to_bytes(out32, remainder);
}

static void secp256k1_fuzz_scalar_check_decode(secp256k1_scalar *scalar, unsigned char *canonical32, const unsigned char *input32) {
    secp256k1_scalar null_overflow_scalar;
    secp256k1_scalar seckey_scalar;
    unsigned char expected32[32];
    unsigned char actual32[32];
    int expected_overflow;
    int overflow;
    int valid_seckey;

    expected_overflow = secp256k1_fuzz_scalar_reduce_input(expected32, input32);
    secp256k1_scalar_set_b32(scalar, input32, &overflow);
    secp256k1_scalar_get_b32(canonical32, scalar);
    FUZZ_CHECK(overflow == expected_overflow);
    FUZZ_CHECK(memcmp(canonical32, expected32, sizeof(expected32)) == 0);

    secp256k1_scalar_set_b32(&null_overflow_scalar, input32, NULL);
    secp256k1_scalar_get_b32(actual32, &null_overflow_scalar);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(expected32)) == 0);

    valid_seckey = secp256k1_scalar_set_b32_seckey(&seckey_scalar, input32);
    FUZZ_CHECK(valid_seckey == (!expected_overflow && memcmp(expected32, secp256k1_fuzz_scalar_zero, sizeof(expected32)) != 0));
    if (valid_seckey) {
        secp256k1_scalar_get_b32(actual32, &seckey_scalar);
        FUZZ_CHECK(memcmp(actual32, expected32, sizeof(expected32)) == 0);
    }
}

static void secp256k1_fuzz_scalar_check_modular_arithmetic(const secp256k1_scalar *a, const secp256k1_scalar *b, const unsigned char *a32, const uint16_t *product) {
    secp256k1_scalar actual;
    secp256k1_scalar actual_alias;
    secp256k1_scalar inverse;
    secp256k1_scalar inverse_var;
    unsigned char actual32[32];
    unsigned char expected32[32];
    unsigned char inverse32[32];
    uint16_t inverse_product[SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS];

    secp256k1_fuzz_scalar_reduce_product(expected32, product);
    secp256k1_scalar_mul(&actual, a, b);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(expected32)) == 0);

    actual_alias = *a;
    secp256k1_scalar_mul(&actual_alias, &actual_alias, b);
    secp256k1_scalar_get_b32(actual32, &actual_alias);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(expected32)) == 0);

    secp256k1_scalar_inverse(&inverse, a);
    secp256k1_scalar_inverse_var(&inverse_var, a);
    FUZZ_CHECK(secp256k1_scalar_eq(&inverse, &inverse_var));
    secp256k1_scalar_get_b32(inverse32, &inverse);
    secp256k1_fuzz_scalar_product(inverse_product, a32, inverse32);
    secp256k1_fuzz_scalar_reduce_product(actual32, inverse_product);
    if (memcmp(a32, secp256k1_fuzz_scalar_zero, sizeof(actual32)) == 0) {
        FUZZ_CHECK(memcmp(actual32, secp256k1_fuzz_scalar_zero, sizeof(actual32)) == 0);
    } else {
        FUZZ_CHECK(memcmp(actual32, secp256k1_fuzz_scalar_one, sizeof(actual32)) == 0);
    }
}

static void secp256k1_fuzz_scalar_check_shift(const secp256k1_scalar *a, const secp256k1_scalar *b, const uint16_t *product, unsigned int shift) {
    secp256k1_scalar actual;
    unsigned char actual32[32];
    unsigned char expected32[32];

    secp256k1_fuzz_scalar_shift_reference(expected32, product, shift);
    secp256k1_scalar_mul_shift_var(&actual, a, b, shift);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
}

static void secp256k1_fuzz_scalar_check_pair(const unsigned char *a_input32, const unsigned char *b_input32, const unsigned char *input, size_t size, unsigned int salt) {
    static const unsigned int boundary_shifts[] = {
        256, 257, 287, 288, 319, 320, 383, 384,
        447, 448, 479, 480, 511, 512, 513, 514,
        ~(unsigned int)0
    };
    secp256k1_scalar a;
    secp256k1_scalar b;
    uint16_t product[SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS];
    unsigned char a32[32];
    unsigned char b32[32];
    unsigned int random_shift;
    size_t i;

    secp256k1_fuzz_scalar_check_decode(&a, a32, a_input32);
    secp256k1_fuzz_scalar_check_decode(&b, b32, b_input32);
    secp256k1_fuzz_scalar_product(product, a32, b32);
    secp256k1_fuzz_scalar_check_modular_arithmetic(&a, &b, a32, product);

    for (i = 0; i < sizeof(boundary_shifts) / sizeof(boundary_shifts[0]); i++) {
        secp256k1_fuzz_scalar_check_shift(&a, &b, product, boundary_shifts[i]);
    }

    random_shift = 256u + secp256k1_fuzz_byte(input, size, salt);
    if ((secp256k1_fuzz_byte(input, size, salt + 1u) & 1u) != 0) {
        random_shift += 257u;
    }
    secp256k1_fuzz_scalar_check_shift(&a, &b, product, random_shift);
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    unsigned char a32[32];
    unsigned char b32[32];
    unsigned char order_minus_one32[32];

    secp256k1_fuzz_derive(a32, sizeof(a32), input, size, 31);
    secp256k1_fuzz_derive(b32, sizeof(b32), input, size, 37);
    secp256k1_fuzz_scalar_check_pair(a32, b32, input, size, 41);

    secp256k1_fuzz_scalar_decrement(order_minus_one32, secp256k1_fuzz_scalar_order);
    secp256k1_fuzz_scalar_check_pair(order_minus_one32, order_minus_one32, input, size, 43);

    return 0;
}
