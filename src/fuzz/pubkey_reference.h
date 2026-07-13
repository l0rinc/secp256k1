/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_FUZZ_PUBKEY_REFERENCE_H
#define SECP256K1_FUZZ_PUBKEY_REFERENCE_H

#include <stddef.h>
#include <string.h>

static const unsigned char secp256k1_fuzz_pubkey_field_prime[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
};

static const unsigned char secp256k1_fuzz_pubkey_field_p_plus_one[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x30
};

/* Keep this model independent from the production field representation and
 * square-root addition chain. */
static void secp256k1_fuzz_pubkey_add_mod(unsigned char out32[32], const unsigned char a32[32], const unsigned char b32[32]) {
    unsigned char sum32[32];
    unsigned int carry = 0;
    size_t i;

    for (i = 32; i-- > 0;) {
        unsigned int value = (unsigned int)a32[i] + b32[i] + carry;
        sum32[i] = (unsigned char)value;
        carry = value >> 8;
    }
    if (carry != 0 || memcmp(sum32, secp256k1_fuzz_pubkey_field_prime, sizeof(sum32)) >= 0) {
        int borrow = 0;
        for (i = 32; i-- > 0;) {
            int value = (int)sum32[i] - secp256k1_fuzz_pubkey_field_prime[i] - borrow;
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

static void secp256k1_fuzz_pubkey_mul_mod(unsigned char out32[32], const unsigned char a32[32], const unsigned char b32[32]) {
    unsigned char result32[32] = { 0 };
    unsigned char addend32[32];
    size_t i;
    unsigned int bit;

    memcpy(addend32, a32, sizeof(addend32));
    for (i = 32; i-- > 0;) {
        for (bit = 0; bit < 8; bit++) {
            if ((b32[i] & (1u << bit)) != 0) {
                secp256k1_fuzz_pubkey_add_mod(result32, result32, addend32);
            }
            secp256k1_fuzz_pubkey_add_mod(addend32, addend32, addend32);
        }
    }
    memcpy(out32, result32, sizeof(result32));
}

static void secp256k1_fuzz_pubkey_sqrt_mod(unsigned char out32[32], const unsigned char input32[32]) {
    unsigned char exponent32[32];
    unsigned char result32[32] = { 0 };
    unsigned char one32[32] = { 0 };
    size_t i;
    unsigned int bit;
    unsigned int carry;

    /* The field prime is 3 modulo 4, so sqrt(a) = a^((p+1)/4). */
    memcpy(exponent32, secp256k1_fuzz_pubkey_field_p_plus_one, sizeof(exponent32));
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
            secp256k1_fuzz_pubkey_mul_mod(result32, result32, result32);
            if ((exponent32[i] & (1u << bit)) != 0) {
                secp256k1_fuzz_pubkey_mul_mod(result32, result32, input32);
            }
        }
    }
    memcpy(out32, result32, sizeof(result32));
}

#endif /* SECP256K1_FUZZ_PUBKEY_REFERENCE_H */
