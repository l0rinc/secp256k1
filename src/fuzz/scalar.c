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

static int secp256k1_fuzz_scalar_add_reference(unsigned char *out32, const unsigned char *a32, const unsigned char *b32) {
    uint16_t a[SECP256K1_FUZZ_SCALAR_LIMBS];
    uint16_t b[SECP256K1_FUZZ_SCALAR_LIMBS];
    uint16_t order[SECP256K1_FUZZ_SCALAR_LIMBS];
    uint16_t sum[SECP256K1_FUZZ_SCALAR_LIMBS + 1] = { 0 };
    uint32_t carry = 0;
    int overflow;
    int i;

    secp256k1_fuzz_scalar_bytes_to_limbs(a, a32);
    secp256k1_fuzz_scalar_bytes_to_limbs(b, b32);
    secp256k1_fuzz_scalar_bytes_to_limbs(order, secp256k1_fuzz_scalar_order);
    for (i = 0; i < SECP256K1_FUZZ_SCALAR_LIMBS; i++) {
        uint32_t value = (uint32_t)a[i] + b[i] + carry;
        sum[i] = (uint16_t)value;
        carry = value >> 16;
    }
    sum[SECP256K1_FUZZ_SCALAR_LIMBS] = (uint16_t)carry;
    overflow = secp256k1_fuzz_scalar_limbs_ge_order(sum, order);
    if (overflow) {
        secp256k1_fuzz_scalar_limbs_sub_order(sum, order);
    }
    FUZZ_CHECK(sum[SECP256K1_FUZZ_SCALAR_LIMBS] == 0);
    secp256k1_fuzz_scalar_limbs_to_bytes(out32, sum);
    return overflow;
}

static void secp256k1_fuzz_scalar_negate_reference(unsigned char *out32, const unsigned char *input32) {
    if (memcmp(input32, secp256k1_fuzz_scalar_zero, 32) == 0) {
        memset(out32, 0, 32);
    } else {
        secp256k1_fuzz_scalar_bytes_sub(out32, secp256k1_fuzz_scalar_order, input32);
    }
}

static unsigned int secp256k1_fuzz_scalar_byte_bit(const unsigned char *input32, unsigned int bit) {
    FUZZ_CHECK(bit < 256);
    return (input32[31 - (bit >> 3)] >> (bit & 7u)) & 1u;
}

static uint32_t secp256k1_fuzz_scalar_bytes_bits(const unsigned char *input32, unsigned int offset, unsigned int count) {
    uint32_t result = 0;
    unsigned int bit;

    FUZZ_CHECK(count > 0 && count <= 32);
    FUZZ_CHECK(offset <= 256 - count);
    for (bit = 0; bit < count; bit++) {
        result |= secp256k1_fuzz_scalar_byte_bit(input32, offset + bit) << bit;
    }
    return result;
}

/* Recompute WNAF from canonical bytes, without scalar bit access or scalar arithmetic. */
static int secp256k1_fuzz_scalar_wnaf_reference(int *wnaf, int len, const unsigned char *input32, int w) {
    unsigned char absolute32[32];
    int sign = 1;
    int carry = 0;
    int last_set_bit = -1;
    unsigned int bit = 0;

    FUZZ_CHECK(len >= 0 && len <= 256);
    FUZZ_CHECK(w >= 2 && w <= 31);
    memcpy(absolute32, input32, sizeof(absolute32));
    if (secp256k1_fuzz_scalar_byte_bit(absolute32, 255) != 0) {
        secp256k1_fuzz_scalar_negate_reference(absolute32, absolute32);
        sign = -1;
    }
    memset(wnaf, 0, (size_t)len * sizeof(*wnaf));
    while (bit < (unsigned int)len) {
        unsigned int now;
        uint32_t word;

        if (secp256k1_fuzz_scalar_byte_bit(absolute32, bit) == (unsigned int)carry) {
            bit++;
            continue;
        }
        now = (unsigned int)w;
        if (now > (unsigned int)len - bit) {
            now = (unsigned int)len - bit;
        }
        word = secp256k1_fuzz_scalar_bytes_bits(absolute32, bit, now) + (unsigned int)carry;
        carry = (int)((word >> (w - 1)) & 1u);
        word -= (uint32_t)carry << w;
        wnaf[bit] = sign * (int)word;
        last_set_bit = (int)bit;
        bit += now;
    }
    return last_set_bit + 1;
}

/* Recompute the fixed-window representation from the low 128 input bits. */
static int secp256k1_fuzz_scalar_fixed_wnaf_reference(int *wnaf, const unsigned char *input32, int w) {
    int skew = 0;
    int pos;
    int max_pos;
    int last_w;
    const int size = WNAF_SIZE(w);

    FUZZ_CHECK(w >= 2 && w <= PIPPENGER_MAX_BUCKET_WINDOW + 1);
    FUZZ_CHECK(size > 1);
    memset(wnaf, 0, (size_t)size * sizeof(*wnaf));
    if (memcmp(input32, secp256k1_fuzz_scalar_zero, sizeof(secp256k1_fuzz_scalar_zero)) == 0) {
        return 0;
    }
    if ((input32[31] & 1u) == 0) {
        skew = 1;
    }
    wnaf[0] = (int)secp256k1_fuzz_scalar_bytes_bits(input32, 0, (unsigned int)w) + skew;
    last_w = WNAF_BITS - (size - 1) * w;
    for (pos = size - 1; pos > 0; pos--) {
        int val = (int)secp256k1_fuzz_scalar_bytes_bits(input32, (unsigned int)(pos * w), (unsigned int)(pos == size - 1 ? last_w : w));
        if (val != 0) {
            break;
        }
    }
    max_pos = pos;
    for (pos = 1; pos <= max_pos; pos++) {
        int val = (int)secp256k1_fuzz_scalar_bytes_bits(input32, (unsigned int)(pos * w), (unsigned int)(pos == size - 1 ? last_w : w));
        if ((val & 1) == 0) {
            wnaf[pos - 1] -= 1 << w;
            wnaf[pos] = val + 1;
        } else {
            wnaf[pos] = val;
        }
        if (pos >= 2 && ((wnaf[pos - 1] == 1 && wnaf[pos - 2] < 0) || (wnaf[pos - 1] == -1 && wnaf[pos - 2] > 0))) {
            if (wnaf[pos - 1] == 1) {
                wnaf[pos - 2] += 1 << w;
            } else {
                wnaf[pos - 2] -= 1 << w;
            }
            wnaf[pos - 1] = 0;
        }
    }
    return skew;
}

static void secp256k1_fuzz_scalar_half_reference(unsigned char *out32, const unsigned char *input32) {
    unsigned char sum[33] = { 0 };
    unsigned int carry = 0;
    int i;

    for (i = 31; i >= 0; i--) {
        unsigned int value = input32[i] + carry;
        if (input32[31] & 1) {
            value += secp256k1_fuzz_scalar_order[i];
        }
        sum[i + 1] = (unsigned char)value;
        carry = value >> 8;
    }
    sum[0] = (unsigned char)carry;
    carry = 0;
    for (i = 0; i < 33; i++) {
        unsigned int next_carry = sum[i] & 1u;
        sum[i] = (unsigned char)((sum[i] >> 1) | (carry << 7));
        carry = next_carry;
    }
    FUZZ_CHECK(carry == 0);
    FUZZ_CHECK(sum[0] == 0);
    memcpy(out32, sum + 1, 32);
}

static uint32_t secp256k1_fuzz_scalar_bits_reference(const unsigned char *input32, unsigned int offset, unsigned int count) {
    uint32_t ret = 0;
    unsigned int bit;

    FUZZ_CHECK(count > 0 && count <= 32);
    FUZZ_CHECK(offset + count <= 256);
    for (bit = 0; bit < count; bit++) {
        unsigned int source_bit = offset + bit;
        ret |= (uint32_t)((input32[31 - (source_bit >> 3)] >> (source_bit & 7)) & 1u) << bit;
    }
    return ret;
}

/* Exercise the boundaries shared by scalar_get_bits_var and scalar_get_bits_limb32.
 * The latter may not cross a 32-bit limb, while the former must handle those
 * crossings, including crossings that stay within a 64-bit implementation limb. */
static void secp256k1_fuzz_scalar_check_bits_boundaries(const secp256k1_scalar *a, const unsigned char *a32) {
    static const unsigned int offsets_in_limb[] = { 0, 1, 7, 8, 15, 16, 23, 24, 25, 31 };
    static const unsigned int counts[] = { 1, 2, 3, 7, 8, 9, 16, 17, 24, 25, 31, 32 };
    size_t i;
    size_t j;
    unsigned int limb;

    for (limb = 0; limb < 8; limb++) {
        for (i = 0; i < sizeof(offsets_in_limb) / sizeof(offsets_in_limb[0]); i++) {
            unsigned int offset = limb * 32u + offsets_in_limb[i];
            for (j = 0; j < sizeof(counts) / sizeof(counts[0]); j++) {
                unsigned int count = counts[j];
                if (offset + count <= 256u) {
                    uint32_t expected = secp256k1_fuzz_scalar_bits_reference(a32, offset, count);
                    FUZZ_CHECK(secp256k1_scalar_get_bits_var(a, offset, count) == expected);
                    if (offsets_in_limb[i] + count <= 32u) {
                        FUZZ_CHECK(secp256k1_scalar_get_bits_limb32(a, offset, count) == expected);
                    }
                }
            }
        }
    }
}

static int secp256k1_fuzz_scalar_all_zero(void *ptr, size_t len) {
    const unsigned char *bytes = (const unsigned char *)ptr;
    size_t i;

    for (i = 0; i < len; i++) {
        if (bytes[i] != 0) {
            return 0;
        }
    }
    return 1;
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

/* Compute a scalar inverse with Fermat's little theorem using only the
 * standalone product and binary-reduction model above. */
static void secp256k1_fuzz_scalar_inverse_reference(unsigned char *out32, const unsigned char *input32) {
    unsigned char exponent[32];
    unsigned char exponent_minus_one[32];
    unsigned char result[32] = { 0 };
    uint16_t product[SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS];
    unsigned int bit;

    if (memcmp(input32, secp256k1_fuzz_scalar_zero, sizeof(result)) == 0) {
        memset(out32, 0, sizeof(result));
        return;
    }
    memcpy(exponent, secp256k1_fuzz_scalar_order, sizeof(exponent));
    secp256k1_fuzz_scalar_decrement(exponent_minus_one, exponent);
    secp256k1_fuzz_scalar_decrement(exponent, exponent_minus_one);
    result[31] = 1;
    for (bit = 0; bit < 256; bit++) {
        secp256k1_fuzz_scalar_product(product, result, result);
        secp256k1_fuzz_scalar_reduce_product(result, product);
        if ((exponent[bit >> 3] & (unsigned char)(0x80u >> (bit & 7u))) != 0) {
            secp256k1_fuzz_scalar_product(product, result, input32);
            secp256k1_fuzz_scalar_reduce_product(result, product);
        }
    }
    memcpy(out32, result, sizeof(result));
}

/* Recompute the GLV scalar split from its fixed constants and rounded products. */
static void secp256k1_fuzz_scalar_split_lambda_reference(unsigned char *split1_32, unsigned char *split2_32, const unsigned char *input32) {
    static const unsigned char minus_b1[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xE4, 0x43, 0x7E, 0xD6, 0x01, 0x0E, 0x88, 0x28,
        0x6F, 0x54, 0x7F, 0xA9, 0x0A, 0xBF, 0xE4, 0xC3
    };
    static const unsigned char minus_b2[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
        0x8A, 0x28, 0x0A, 0xC5, 0x07, 0x74, 0x34, 0x6D,
        0xD7, 0x65, 0xCD, 0xA8, 0x3D, 0xB1, 0x56, 0x2C
    };
    static const unsigned char g1[32] = {
        0x30, 0x86, 0xD2, 0x21, 0xA7, 0xD4, 0x6B, 0xCD,
        0xE8, 0x6C, 0x90, 0xE4, 0x92, 0x84, 0xEB, 0x15,
        0x3D, 0xAA, 0x8A, 0x14, 0x71, 0xE8, 0xCA, 0x7F,
        0xE8, 0x93, 0x20, 0x9A, 0x45, 0xDB, 0xB0, 0x31
    };
    static const unsigned char g2[32] = {
        0xE4, 0x43, 0x7E, 0xD6, 0x01, 0x0E, 0x88, 0x28,
        0x6F, 0x54, 0x7F, 0xA9, 0x0A, 0xBF, 0xE4, 0xC4,
        0x22, 0x12, 0x08, 0xAC, 0x9D, 0xF5, 0x06, 0xC6,
        0x15, 0x71, 0xB4, 0xAE, 0x8A, 0xC4, 0x7F, 0x71
    };
    static const unsigned char lambda[32] = {
        0x53, 0x63, 0xAD, 0x4C, 0xC0, 0x5C, 0x30, 0xE0,
        0xA5, 0x26, 0x1C, 0x02, 0x88, 0x12, 0x64, 0x5A,
        0x12, 0x2E, 0x22, 0xEA, 0x20, 0x81, 0x66, 0x78,
        0xDF, 0x02, 0x96, 0x7C, 0x1B, 0x23, 0xBD, 0x72
    };
    unsigned char c1[32];
    unsigned char c2[32];
    unsigned char lambda_r2[32];
    unsigned char negated[32];
    uint16_t product[SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS];

    secp256k1_fuzz_scalar_product(product, input32, g1);
    secp256k1_fuzz_scalar_shift_reference(c1, product, 384);
    secp256k1_fuzz_scalar_product(product, input32, g2);
    secp256k1_fuzz_scalar_shift_reference(c2, product, 384);
    secp256k1_fuzz_scalar_product(product, c1, minus_b1);
    secp256k1_fuzz_scalar_reduce_product(c1, product);
    secp256k1_fuzz_scalar_product(product, c2, minus_b2);
    secp256k1_fuzz_scalar_reduce_product(c2, product);
    (void)secp256k1_fuzz_scalar_add_reference(split2_32, c1, c2);
    secp256k1_fuzz_scalar_product(product, split2_32, lambda);
    secp256k1_fuzz_scalar_reduce_product(lambda_r2, product);
    secp256k1_fuzz_scalar_negate_reference(negated, lambda_r2);
    (void)secp256k1_fuzz_scalar_add_reference(split1_32, negated, input32);
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
    unsigned char expected_inverse32[32];
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

    secp256k1_fuzz_scalar_inverse_reference(expected_inverse32, a32);
    secp256k1_scalar_inverse(&inverse, a);
    secp256k1_scalar_inverse_var(&inverse_var, a);
    FUZZ_CHECK(secp256k1_scalar_eq(&inverse, &inverse_var));
    secp256k1_scalar_get_b32(inverse32, &inverse);
    FUZZ_CHECK(memcmp(inverse32, expected_inverse32, sizeof(inverse32)) == 0);
    secp256k1_scalar_get_b32(actual32, &inverse_var);
    FUZZ_CHECK(memcmp(actual32, expected_inverse32, sizeof(actual32)) == 0);
    secp256k1_fuzz_scalar_product(inverse_product, a32, inverse32);
    secp256k1_fuzz_scalar_reduce_product(actual32, inverse_product);
    if (memcmp(a32, secp256k1_fuzz_scalar_zero, sizeof(actual32)) == 0) {
        FUZZ_CHECK(memcmp(actual32, secp256k1_fuzz_scalar_zero, sizeof(actual32)) == 0);
    } else {
        FUZZ_CHECK(memcmp(actual32, secp256k1_fuzz_scalar_one, sizeof(actual32)) == 0);
    }
}

static void secp256k1_fuzz_scalar_check_linear_arithmetic(const secp256k1_scalar *a, const secp256k1_scalar *b, const unsigned char *a32, const unsigned char *b32, const unsigned char *input, size_t size, unsigned int salt) {
    secp256k1_scalar actual;
    secp256k1_scalar actual_alias;
    unsigned char actual32[32];
    unsigned char expected32[32];
    unsigned char negated32[32];
    unsigned char bit32[32] = { 0 };
    unsigned int bit;
    unsigned int count;
    unsigned int offset;
    int expected_overflow;
    int expected_high;

    expected_overflow = secp256k1_fuzz_scalar_add_reference(expected32, a32, b32);
    FUZZ_CHECK(secp256k1_scalar_add(&actual, a, b) == expected_overflow);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(expected32)) == 0);
    actual_alias = *a;
    FUZZ_CHECK(secp256k1_scalar_add(&actual_alias, &actual_alias, b) == expected_overflow);
    secp256k1_scalar_get_b32(actual32, &actual_alias);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(expected32)) == 0);

    secp256k1_fuzz_scalar_negate_reference(negated32, a32);
    secp256k1_scalar_negate(&actual, a);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, negated32, sizeof(negated32)) == 0);
    actual_alias = *a;
    secp256k1_scalar_negate(&actual_alias, &actual_alias);
    secp256k1_scalar_get_b32(actual32, &actual_alias);
    FUZZ_CHECK(memcmp(actual32, negated32, sizeof(negated32)) == 0);

    secp256k1_fuzz_scalar_half_reference(expected32, a32);
    actual_alias = *a;
    secp256k1_scalar_half(&actual_alias, &actual_alias);
    secp256k1_scalar_get_b32(actual32, &actual_alias);
    FUZZ_CHECK(memcmp(actual32, expected32, sizeof(expected32)) == 0);

    expected_high = memcmp(a32, secp256k1_fuzz_scalar_zero, sizeof(actual32)) != 0 && secp256k1_fuzz_scalar_bytes_cmp(a32, negated32) > 0;
    FUZZ_CHECK(secp256k1_scalar_is_high(a) == expected_high);
    FUZZ_CHECK(secp256k1_scalar_is_even(a) == ((a32[31] & 1u) == 0));

    actual = *a;
    secp256k1_scalar_cmov(&actual, b, 0);
    FUZZ_CHECK(secp256k1_scalar_eq(&actual, a));
    secp256k1_scalar_cmov(&actual, b, 1);
    FUZZ_CHECK(secp256k1_scalar_eq(&actual, b));
    actual = *a;
    FUZZ_CHECK(secp256k1_scalar_cond_negate(&actual, 0) == 1);
    FUZZ_CHECK(secp256k1_scalar_eq(&actual, a));
    FUZZ_CHECK(secp256k1_scalar_cond_negate(&actual, 1) == -1);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, negated32, sizeof(negated32)) == 0);

    offset = secp256k1_fuzz_byte(input, size, salt) % 256u;
    count = 1u + secp256k1_fuzz_byte(input, size, salt + 1u) % (256u - offset < 32u ? 256u - offset : 32u);
    FUZZ_CHECK(secp256k1_scalar_get_bits_var(a, offset, count) == secp256k1_fuzz_scalar_bits_reference(a32, offset, count));
    FUZZ_CHECK(secp256k1_scalar_get_bits_var(a, 0, 32) == secp256k1_fuzz_scalar_bits_reference(a32, 0, 32));
    FUZZ_CHECK(secp256k1_scalar_get_bits_var(a, 224, 32) == secp256k1_fuzz_scalar_bits_reference(a32, 224, 32));
    offset = (secp256k1_fuzz_byte(input, size, salt + 2u) % 8u) * 32u + secp256k1_fuzz_byte(input, size, salt + 3u) % 32u;
    count = 1u + secp256k1_fuzz_byte(input, size, salt + 4u) % (32u - (offset & 31u));
    FUZZ_CHECK(secp256k1_scalar_get_bits_limb32(a, offset, count) == secp256k1_fuzz_scalar_bits_reference(a32, offset, count));

    bit = secp256k1_fuzz_byte(input, size, salt + 5u) % 256u;
    bit32[31 - (bit >> 3)] = (unsigned char)(1u << (bit & 7));
    expected_overflow = secp256k1_fuzz_scalar_add_reference(expected32, a32, bit32);
    if (!expected_overflow) {
        actual = *a;
        secp256k1_scalar_cadd_bit(&actual, bit, 1);
        secp256k1_scalar_get_b32(actual32, &actual);
        FUZZ_CHECK(memcmp(actual32, expected32, sizeof(expected32)) == 0);
        secp256k1_scalar_cadd_bit(&actual, bit, 0);
        secp256k1_scalar_get_b32(actual32, &actual);
        FUZZ_CHECK(memcmp(actual32, expected32, sizeof(expected32)) == 0);
    }

    actual = *a;
    secp256k1_scalar_clear(&actual);
    SECP256K1_CHECKMEM_DEFINE(&actual, sizeof(actual));
    FUZZ_CHECK(secp256k1_fuzz_scalar_all_zero(&actual, sizeof(actual)));
}

static void secp256k1_fuzz_scalar_check_wnaf_reference(const secp256k1_scalar *number, const unsigned char *number32, int len, int max_w) {
    int actual[256];
    int expected[256];
    int w;

    for (w = 2; w <= max_w; w++) {
        int actual_bits = secp256k1_ecmult_wnaf(actual, len, number, w);
        int expected_bits = secp256k1_fuzz_scalar_wnaf_reference(expected, len, number32, w);
        int i;

        FUZZ_CHECK(actual_bits == expected_bits);
        for (i = 0; i < len; i++) {
            FUZZ_CHECK(actual[i] == expected[i]);
        }
    }
}

static void secp256k1_fuzz_scalar_check_wnaf(const secp256k1_scalar *number) {
    secp256k1_scalar reconstructed;
    secp256k1_scalar two;
    int wnaf[256];
    int w;

    secp256k1_scalar_set_int(&two, 2);
    for (w = 2; w <= 31; w++) {
        int bits;
        int zeroes = -1;
        int i;

        bits = secp256k1_ecmult_wnaf(wnaf, 256, number, w);
        secp256k1_scalar_set_int(&reconstructed, 0);
        for (i = bits - 1; i >= 0; i--) {
            int digit = wnaf[i];
            secp256k1_scalar term;

            secp256k1_scalar_mul(&reconstructed, &reconstructed, &two);
            if (digit != 0) {
                FUZZ_CHECK(zeroes == -1 || zeroes >= w - 1);
                FUZZ_CHECK((digit & 1) != 0);
                FUZZ_CHECK(digit <= (int)((INT64_C(1) << (w - 1)) - 1));
                FUZZ_CHECK(digit >= -(int)((INT64_C(1) << (w - 1)) - 1));
                zeroes = 0;
                secp256k1_scalar_set_int(&term, (unsigned int)(digit < 0 ? -digit : digit));
                if (digit < 0) {
                    secp256k1_scalar_negate(&term, &term);
                }
                secp256k1_scalar_add(&reconstructed, &reconstructed, &term);
            } else if (zeroes != -1) {
                zeroes++;
            }
        }
        FUZZ_CHECK(secp256k1_scalar_eq(&reconstructed, number));
    }
}

static void secp256k1_fuzz_scalar_check_fixed_wnaf_reference(const secp256k1_scalar *number, const unsigned char *number32) {
    secp256k1_scalar low;
    secp256k1_scalar unused;
    unsigned char low32[32] = { 0 };
    int actual[WNAF_SIZE(2)];
    int expected[WNAF_SIZE(2)];
    int w;

    secp256k1_scalar_split_128(&low, &unused, number);
    memcpy(low32 + 16, number32 + 16, 16);
    for (w = 2; w <= PIPPENGER_MAX_BUCKET_WINDOW + 1; w++) {
        int actual_skew = secp256k1_wnaf_fixed(actual, &low, w);
        int expected_skew = secp256k1_fuzz_scalar_fixed_wnaf_reference(expected, low32, w);
        int i;

        FUZZ_CHECK(actual_skew == expected_skew);
        for (i = 0; i < WNAF_SIZE(w); i++) {
            FUZZ_CHECK(actual[i] == expected[i]);
        }
    }
}

static void secp256k1_fuzz_scalar_check_fixed_wnaf(const secp256k1_scalar *number, const unsigned char *number32) {
    secp256k1_scalar num;
    secp256k1_scalar unused;
    secp256k1_scalar adjusted;
    secp256k1_scalar reconstructed;
    secp256k1_scalar shift;
    int wnaf[WNAF_SIZE(2)];
    int w;

    /* The fixed helper is defined for the low 128 bits of its input. */
    secp256k1_scalar_split_128(&num, &unused, number);
    secp256k1_fuzz_scalar_check_fixed_wnaf_reference(number, number32);
    for (w = 2; w <= PIPPENGER_MAX_BUCKET_WINDOW + 1; w++) {
        int skew;
        int i;

        secp256k1_scalar_set_int(&reconstructed, 0);
        secp256k1_scalar_set_int(&shift, 1u << w);
        skew = secp256k1_wnaf_fixed(wnaf, &num, w);
        for (i = WNAF_SIZE(w) - 1; i >= 0; i--) {
            secp256k1_scalar term;
            int digit = wnaf[i];

            FUZZ_CHECK(digit == 0 || (digit & 1) != 0);
            FUZZ_CHECK(digit > -(1 << w));
            FUZZ_CHECK(digit < (1 << w));
            secp256k1_scalar_mul(&reconstructed, &reconstructed, &shift);
            secp256k1_scalar_set_int(&term, (unsigned int)(digit < 0 ? -digit : digit));
            if (digit < 0) {
                secp256k1_scalar_negate(&term, &term);
            }
            secp256k1_scalar_add(&reconstructed, &reconstructed, &term);
        }
        adjusted = num;
        secp256k1_scalar_cadd_bit(&adjusted, 0, skew == 1);
        FUZZ_CHECK(secp256k1_scalar_eq(&reconstructed, &adjusted));
    }
}

static void secp256k1_fuzz_scalar_check_wnaf_small(const secp256k1_scalar *number, const unsigned char *number32) {
    secp256k1_scalar low;
    secp256k1_scalar unused;
    unsigned char low32[32] = { 0 };
    int wnaf[129];
    int8_t wnaf_small[129];
    int w;

    /* Strauss decomposes the lambda-split scalars into 129 entries and stores
     * the digits in int8_t. Check that narrowing preserves the generic output. */
    secp256k1_scalar_split_128(&low, &unused, number);
    memcpy(low32 + 16, number32 + 16, 16);
    secp256k1_fuzz_scalar_check_wnaf_reference(&low, low32, 129, 8);
    for (w = 2; w <= 8; w++) {
        int bits = secp256k1_ecmult_wnaf(wnaf, 129, &low, w);
        int small_bits = secp256k1_ecmult_wnaf_small(wnaf_small, 129, &low, w);
        int i;

        FUZZ_CHECK(small_bits == bits);
        for (i = 0; i < 129; i++) {
            FUZZ_CHECK(wnaf_small[i] == wnaf[i]);
        }
    }
}

static int secp256k1_fuzz_scalar_fits_128(const unsigned char *input32) {
    size_t i;

    for (i = 0; i < 16; i++) {
        if (input32[i] != 0) {
            return 0;
        }
    }
    return 1;
}

static void secp256k1_fuzz_scalar_check_splits(const secp256k1_scalar *a, const unsigned char *a32) {
    secp256k1_scalar low;
    secp256k1_scalar high;
    secp256k1_scalar split1;
    secp256k1_scalar split2;
    unsigned char low32[32];
    unsigned char high32[32];
    unsigned char split1_32[32];
    unsigned char split2_32[32];
    unsigned char lambda32[32];
    unsigned char two128_32[32] = { 0 };
    unsigned char expected32[32];
    unsigned char expected_split1_32[32];
    unsigned char expected_split2_32[32];
    unsigned char negated32[32];
    uint16_t product[SECP256K1_FUZZ_SCALAR_PRODUCT_LIMBS];

    secp256k1_scalar_split_128(&low, &high, a);
    secp256k1_scalar_get_b32(low32, &low);
    secp256k1_scalar_get_b32(high32, &high);
    memset(expected32, 0, sizeof(expected32));
    memcpy(expected32 + 16, a32 + 16, 16);
    FUZZ_CHECK(memcmp(low32, expected32, sizeof(expected32)) == 0);
    memset(expected32, 0, sizeof(expected32));
    memcpy(expected32 + 16, a32, 16);
    FUZZ_CHECK(memcmp(high32, expected32, sizeof(expected32)) == 0);
    two128_32[15] = 1;
    secp256k1_fuzz_scalar_product(product, high32, two128_32);
    secp256k1_fuzz_scalar_reduce_product(expected32, product);
    (void)secp256k1_fuzz_scalar_add_reference(expected32, expected32, low32);
    FUZZ_CHECK(memcmp(expected32, a32, sizeof(expected32)) == 0);

    secp256k1_fuzz_scalar_split_lambda_reference(expected_split1_32, expected_split2_32, a32);
    secp256k1_scalar_split_lambda(&split1, &split2, a);
    secp256k1_scalar_get_b32(split1_32, &split1);
    secp256k1_scalar_get_b32(split2_32, &split2);
    FUZZ_CHECK(memcmp(split1_32, expected_split1_32, sizeof(split1_32)) == 0);
    FUZZ_CHECK(memcmp(split2_32, expected_split2_32, sizeof(split2_32)) == 0);
    secp256k1_scalar_get_b32(lambda32, &secp256k1_const_lambda);
    secp256k1_fuzz_scalar_product(product, lambda32, split2_32);
    secp256k1_fuzz_scalar_reduce_product(expected32, product);
    (void)secp256k1_fuzz_scalar_add_reference(expected32, expected32, split1_32);
    FUZZ_CHECK(memcmp(expected32, a32, sizeof(expected32)) == 0);
    secp256k1_fuzz_scalar_negate_reference(negated32, split1_32);
    FUZZ_CHECK(secp256k1_fuzz_scalar_fits_128(split1_32) || secp256k1_fuzz_scalar_fits_128(negated32));
    secp256k1_fuzz_scalar_negate_reference(negated32, split2_32);
    FUZZ_CHECK(secp256k1_fuzz_scalar_fits_128(split2_32) || secp256k1_fuzz_scalar_fits_128(negated32));
}

static void secp256k1_fuzz_scalar_check_endo_split(const secp256k1_scalar *number) {
    secp256k1_scalar raw1;
    secp256k1_scalar raw2;
    secp256k1_scalar expected1;
    secp256k1_scalar expected2;
    secp256k1_scalar split1;
    secp256k1_scalar split2;
    secp256k1_ge base;
    secp256k1_ge expected_p1;
    secp256k1_ge expected_p2;
    secp256k1_ge p1;
    secp256k1_ge p2;
    secp256k1_gej basej;
    secp256k1_gej expected;
    secp256k1_gej term1;
    secp256k1_gej term2;
    secp256k1_gej actual;

    /* Pippenger splits a scalar and its point together. Check the sign
     * corrections independently before checking the resulting multiplication. */
    secp256k1_scalar_split_lambda(&raw1, &raw2, number);
    base = secp256k1_ge_const_g;
    expected_p1 = base;
    expected_p2 = base;
    secp256k1_ge_mul_lambda(&expected_p2, &expected_p2);
    expected1 = raw1;
    expected2 = raw2;
    if (secp256k1_scalar_is_high(&raw1)) {
        secp256k1_scalar_negate(&expected1, &expected1);
        secp256k1_ge_neg(&expected_p1, &expected_p1);
    }
    if (secp256k1_scalar_is_high(&raw2)) {
        secp256k1_scalar_negate(&expected2, &expected2);
        secp256k1_ge_neg(&expected_p2, &expected_p2);
    }

    p1 = base;
    split1 = *number;
    secp256k1_ecmult_endo_split(&split1, &split2, &p1, &p2);
    FUZZ_CHECK(secp256k1_scalar_eq(&split1, &expected1));
    FUZZ_CHECK(secp256k1_scalar_eq(&split2, &expected2));
    FUZZ_CHECK(secp256k1_ge_eq_var(&p1, &expected_p1));
    FUZZ_CHECK(secp256k1_ge_eq_var(&p2, &expected_p2));

    secp256k1_gej_set_ge(&basej, &base);
    secp256k1_ecmult(&expected, &basej, number, NULL);
    secp256k1_gej_set_ge(&basej, &p1);
    secp256k1_ecmult(&term1, &basej, &split1, NULL);
    secp256k1_gej_set_ge(&basej, &p2);
    secp256k1_ecmult(&term2, &basej, &split2, NULL);
    secp256k1_gej_add_var(&actual, &term1, &term2, NULL);
    FUZZ_CHECK(secp256k1_gej_eq_var(&actual, &expected));
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
    secp256k1_fuzz_scalar_check_linear_arithmetic(&a, &b, a32, b32, input, size, salt + 17u);
    secp256k1_fuzz_scalar_check_splits(&a, a32);
    secp256k1_fuzz_scalar_check_endo_split(&a);
    secp256k1_fuzz_scalar_check_wnaf_reference(&a, a32, 256, 31);
    secp256k1_fuzz_scalar_check_wnaf_reference(&b, b32, 256, 31);
    secp256k1_fuzz_scalar_check_wnaf(&a);
    secp256k1_fuzz_scalar_check_wnaf(&b);
    secp256k1_fuzz_scalar_check_wnaf_small(&a, a32);
    secp256k1_fuzz_scalar_check_wnaf_small(&b, b32);
    secp256k1_fuzz_scalar_check_fixed_wnaf(&a, a32);

    for (i = 0; i < sizeof(boundary_shifts) / sizeof(boundary_shifts[0]); i++) {
        secp256k1_fuzz_scalar_check_shift(&a, &b, product, boundary_shifts[i]);
    }

    random_shift = 256u + secp256k1_fuzz_byte(input, size, salt);
    if ((secp256k1_fuzz_byte(input, size, salt + 1u) & 1u) != 0) {
        random_shift += 257u;
    }
    secp256k1_fuzz_scalar_check_shift(&a, &b, product, random_shift);
}

static void secp256k1_fuzz_scalar_check_cadd_bit_noop_boundary(void) {
    secp256k1_scalar expected;
    secp256k1_scalar actual;
    unsigned char order_minus_high_bit32[32];

    memcpy(order_minus_high_bit32, secp256k1_fuzz_scalar_order_minus_one, sizeof(order_minus_high_bit32));
    order_minus_high_bit32[0] -= 0x80;
    secp256k1_scalar_set_b32(&expected, order_minus_high_bit32, NULL);
    actual = expected;
    /* flag == 0 is a no-op even at the high-bit boundary. */
    secp256k1_scalar_cadd_bit(&actual, 255, 0);
    FUZZ_CHECK(secp256k1_scalar_eq(&actual, &expected));
}

static void secp256k1_fuzz_scalar_check_cadd_bit_carry_boundaries(void) {
    static const unsigned int bits[] = {
        0, 1, 31, 32, 33, 63, 64, 65, 95, 96, 127,
        128, 129, 159, 160, 191, 192, 193, 223, 224, 255
    };
    unsigned char bit32[32];
    unsigned char order_minus_bit32[32];
    unsigned char expected32[32];
    unsigned char actual32[32];
    secp256k1_scalar boundary;
    secp256k1_scalar actual;
    size_t i;

    /* Exercise every limb boundary while keeping the result below the order. */
    for (i = 0; i < sizeof(bits) / sizeof(bits[0]); i++) {
        memset(bit32, 0, sizeof(bit32));
        bit32[31 - (bits[i] >> 3)] = (unsigned char)(1u << (bits[i] & 7u));
        secp256k1_fuzz_scalar_bytes_sub(order_minus_bit32, secp256k1_fuzz_scalar_order_minus_one, bit32);
        FUZZ_CHECK(secp256k1_fuzz_scalar_add_reference(expected32, order_minus_bit32, bit32) == 0);
        secp256k1_scalar_set_b32(&boundary, order_minus_bit32, NULL);
        actual = boundary;
        secp256k1_scalar_cadd_bit(&actual, bits[i], 1);
        secp256k1_scalar_get_b32(actual32, &actual);
        FUZZ_CHECK(memcmp(actual32, expected32, sizeof(actual32)) == 0);
    }
}

static void secp256k1_fuzz_scalar_check_zero_one_boundaries(const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "scalar zero one predicates\n";
    secp256k1_scalar zero;
    secp256k1_scalar one;
    secp256k1_scalar order;
    secp256k1_scalar order_minus_one;
    unsigned char actual32[32];
    int overflow;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    secp256k1_scalar_set_b32(&zero, secp256k1_fuzz_scalar_zero, &overflow);
    FUZZ_CHECK(overflow == 0);
    FUZZ_CHECK(secp256k1_scalar_is_zero(&zero));
    FUZZ_CHECK(!secp256k1_scalar_is_one(&zero));

    secp256k1_scalar_set_b32(&one, secp256k1_fuzz_scalar_one, &overflow);
    FUZZ_CHECK(overflow == 0);
    FUZZ_CHECK(!secp256k1_scalar_is_zero(&one));
    FUZZ_CHECK(secp256k1_scalar_is_one(&one));

    secp256k1_scalar_set_b32(&order_minus_one, secp256k1_fuzz_scalar_order_minus_one, &overflow);
    FUZZ_CHECK(overflow == 0);
    FUZZ_CHECK(!secp256k1_scalar_is_zero(&order_minus_one));
    FUZZ_CHECK(!secp256k1_scalar_is_one(&order_minus_one));

    /* The non-canonical input n reduces to zero, not one or an uninitialized value. */
    secp256k1_scalar_set_b32(&order, secp256k1_fuzz_scalar_order, &overflow);
    FUZZ_CHECK(overflow == 1);
    secp256k1_scalar_get_b32(actual32, &order);
    FUZZ_CHECK(memcmp(actual32, secp256k1_fuzz_scalar_zero, sizeof(actual32)) == 0);
    FUZZ_CHECK(secp256k1_scalar_is_zero(&order));
    FUZZ_CHECK(!secp256k1_scalar_is_one(&order));
}

static void secp256k1_fuzz_scalar_check_high_boundary(const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "scalar high boundary\n";
    static const unsigned char half_order32[32] = {
        0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x5D, 0x57, 0x6E, 0x73, 0x57, 0xA4, 0x50, 0x1D,
        0xDF, 0xE9, 0x2F, 0x46, 0x68, 0x1B, 0x20, 0xA0
    };
    static const unsigned char half_order_plus_one32[32] = {
        0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x5D, 0x57, 0x6E, 0x73, 0x57, 0xA4, 0x50, 0x1D,
        0xDF, 0xE9, 0x2F, 0x46, 0x68, 0x1B, 0x20, 0xA1
    };
    secp256k1_scalar half_order;
    secp256k1_scalar half_order_plus_one;
    secp256k1_scalar order_minus_one;
    secp256k1_scalar actual;
    unsigned char actual32[32];
    int overflow;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    secp256k1_scalar_set_b32(&half_order, half_order32, &overflow);
    FUZZ_CHECK(overflow == 0);
    secp256k1_scalar_set_b32(&half_order_plus_one, half_order_plus_one32, &overflow);
    FUZZ_CHECK(overflow == 0);
    secp256k1_scalar_set_b32(&order_minus_one, secp256k1_fuzz_scalar_order_minus_one, &overflow);
    FUZZ_CHECK(overflow == 0);

    FUZZ_CHECK(secp256k1_scalar_is_high(&half_order) == 0);
    FUZZ_CHECK(secp256k1_scalar_is_high(&half_order_plus_one) == 1);
    FUZZ_CHECK(secp256k1_scalar_is_high(&order_minus_one) == 1);
    FUZZ_CHECK(secp256k1_scalar_is_even(&half_order) == 1);
    FUZZ_CHECK(secp256k1_scalar_is_even(&half_order_plus_one) == 0);

    secp256k1_scalar_negate(&actual, &half_order);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, half_order_plus_one32, sizeof(actual32)) == 0);
    secp256k1_scalar_negate(&actual, &half_order_plus_one);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, half_order32, sizeof(actual32)) == 0);

    actual = half_order;
    FUZZ_CHECK(secp256k1_scalar_cond_negate(&actual, 0) == 1);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, half_order32, sizeof(actual32)) == 0);
    FUZZ_CHECK(secp256k1_scalar_cond_negate(&actual, 1) == -1);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, half_order_plus_one32, sizeof(actual32)) == 0);

    actual = half_order_plus_one;
    FUZZ_CHECK(secp256k1_scalar_cond_negate(&actual, 1) == -1);
    secp256k1_scalar_get_b32(actual32, &actual);
    FUZZ_CHECK(memcmp(actual32, half_order32, sizeof(actual32)) == 0);
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    unsigned char a32[32];
    unsigned char b32[32];
    unsigned char order_minus_one32[32];
    unsigned char wnaf_boundary32[32] = { 0 };
    secp256k1_scalar wnaf_boundary;

    secp256k1_fuzz_derive(a32, sizeof(a32), input, size, 31);
    secp256k1_fuzz_derive(b32, sizeof(b32), input, size, 37);
    secp256k1_fuzz_scalar_check_pair(a32, b32, input, size, 41);
    secp256k1_scalar_set_b32(&wnaf_boundary, a32, NULL);
    secp256k1_fuzz_scalar_check_bits_boundaries(&wnaf_boundary, a32);
    secp256k1_scalar_set_int(&wnaf_boundary, 0);
    secp256k1_fuzz_scalar_check_fixed_wnaf(&wnaf_boundary, secp256k1_fuzz_scalar_zero);
    secp256k1_scalar_set_int(&wnaf_boundary, 1);
    secp256k1_fuzz_scalar_check_wnaf_reference(&wnaf_boundary, secp256k1_fuzz_scalar_one, 256, 31);
    secp256k1_fuzz_scalar_check_fixed_wnaf(&wnaf_boundary, secp256k1_fuzz_scalar_one);
    wnaf_boundary32[28] = 0x7f;
    wnaf_boundary32[29] = 0xff;
    wnaf_boundary32[30] = 0xff;
    wnaf_boundary32[31] = 0xff;
    secp256k1_scalar_set_int(&wnaf_boundary, 0x7fffffffU);
    secp256k1_fuzz_scalar_check_wnaf(&wnaf_boundary);
    secp256k1_fuzz_scalar_check_fixed_wnaf(&wnaf_boundary, wnaf_boundary32);

    secp256k1_fuzz_scalar_decrement(order_minus_one32, secp256k1_fuzz_scalar_order);
    secp256k1_fuzz_scalar_check_pair(order_minus_one32, order_minus_one32, input, size, 43);
    secp256k1_fuzz_scalar_check_cadd_bit_noop_boundary();
    secp256k1_fuzz_scalar_check_cadd_bit_carry_boundaries();
    secp256k1_fuzz_scalar_check_zero_one_boundaries(input, size);
    secp256k1_fuzz_scalar_check_high_boundary(input, size);

    return 0;
}
