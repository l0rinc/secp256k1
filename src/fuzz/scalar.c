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

    secp256k1_scalar_set_b32(&a, a_input32, NULL);
    secp256k1_scalar_set_b32(&b, b_input32, NULL);
    secp256k1_scalar_get_b32(a32, &a);
    secp256k1_scalar_get_b32(b32, &b);
    secp256k1_fuzz_scalar_product(product, a32, b32);

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
