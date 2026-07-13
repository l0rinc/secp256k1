/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_FUZZ_SHA256_REFERENCE_H
#define SECP256K1_FUZZ_SHA256_REFERENCE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Keep this model independent from hash_impl.h. It is deliberately a complete
 * one-shot implementation so a production compression or padding defect
 * cannot be shared by the expected-value path. */
static uint32_t secp256k1_fuzz_sha256_standalone_rotr(uint32_t x, unsigned int n) {
    return (x >> n) | (x << (32 - n));
}

static void secp256k1_fuzz_sha256_standalone_compress(uint32_t state[8], const unsigned char block[64]) {
    static const uint32_t k[64] = {
        0x428a2f98ul, 0x71374491ul, 0xb5c0fbcful, 0xe9b5dba5ul,
        0x3956c25bul, 0x59f111f1ul, 0x923f82a4ul, 0xab1c5ed5ul,
        0xd807aa98ul, 0x12835b01ul, 0x243185beul, 0x550c7dc3ul,
        0x72be5d74ul, 0x80deb1feul, 0x9bdc06a7ul, 0xc19bf174ul,
        0xe49b69c1ul, 0xefbe4786ul, 0x0fc19dc6ul, 0x240ca1ccul,
        0x2de92c6ful, 0x4a7484aaul, 0x5cb0a9dcul, 0x76f988daul,
        0x983e5152ul, 0xa831c66dul, 0xb00327c8ul, 0xbf597fc7ul,
        0xc6e00bf3ul, 0xd5a79147ul, 0x06ca6351ul, 0x14292967ul,
        0x27b70a85ul, 0x2e1b2138ul, 0x4d2c6dfcul, 0x53380d13ul,
        0x650a7354ul, 0x766a0abbul, 0x81c2c92eul, 0x92722c85ul,
        0xa2bfe8a1ul, 0xa81a664bul, 0xc24b8b70ul, 0xc76c51a3ul,
        0xd192e819ul, 0xd6990624ul, 0xf40e3585ul, 0x106aa070ul,
        0x19a4c116ul, 0x1e376c08ul, 0x2748774cul, 0x34b0bcb5ul,
        0x391c0cb3ul, 0x4ed8aa4aul, 0x5b9cca4ful, 0x682e6ff3ul,
        0x748f82eeul, 0x78a5636ful, 0x84c87814ul, 0x8cc70208ul,
        0x90befffaul, 0xa4506cebul, 0xbef9a3f7ul, 0xc67178f2ul
    };
    uint32_t w[64];
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];
    size_t i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[4 * i] << 24)
             | ((uint32_t)block[4 * i + 1] << 16)
             | ((uint32_t)block[4 * i + 2] << 8)
             | (uint32_t)block[4 * i + 3];
    }
    for (i = 16; i < 64; i++) {
        uint32_t s0 = secp256k1_fuzz_sha256_standalone_rotr(w[i - 15], 7)
                    ^ secp256k1_fuzz_sha256_standalone_rotr(w[i - 15], 18)
                    ^ (w[i - 15] >> 3);
        uint32_t s1 = secp256k1_fuzz_sha256_standalone_rotr(w[i - 2], 17)
                    ^ secp256k1_fuzz_sha256_standalone_rotr(w[i - 2], 19)
                    ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    for (i = 0; i < 64; i++) {
        uint32_t s1 = secp256k1_fuzz_sha256_standalone_rotr(e, 6)
                    ^ secp256k1_fuzz_sha256_standalone_rotr(e, 11)
                    ^ secp256k1_fuzz_sha256_standalone_rotr(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t s0 = secp256k1_fuzz_sha256_standalone_rotr(a, 2)
                    ^ secp256k1_fuzz_sha256_standalone_rotr(a, 13)
                    ^ secp256k1_fuzz_sha256_standalone_rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t1 = h + s1 + ch + k[i] + w[i];
        uint32_t t2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
    memset(w, 0, sizeof(w));
}

static void secp256k1_fuzz_sha256_standalone(unsigned char out[32], const unsigned char *msg, size_t msglen) {
    static const uint32_t initial_state[8] = {
        0x6a09e667ul, 0xbb67ae85ul, 0x3c6ef372ul, 0xa54ff53aul,
        0x510e527ful, 0x9b05688cul, 0x1f83d9abul, 0x5be0cd19ul
    };
    uint32_t state[8];
    unsigned char block[64];
    uint64_t bit_length;
    size_t offset = 0;
    size_t remaining;
    size_t i;

    memcpy(state, initial_state, sizeof(state));
    while (msglen - offset >= sizeof(block)) {
        secp256k1_fuzz_sha256_standalone_compress(state, msg + offset);
        offset += sizeof(block);
    }
    remaining = msglen - offset;
    memset(block, 0, sizeof(block));
    if (remaining != 0) {
        memcpy(block, msg + offset, remaining);
    }
    block[remaining] = 0x80;
    if (remaining >= 56) {
        secp256k1_fuzz_sha256_standalone_compress(state, block);
        memset(block, 0, sizeof(block));
    }
    bit_length = (uint64_t)msglen << 3;
    for (i = 0; i < 8; i++) {
        block[63 - i] = (unsigned char)(bit_length >> (8 * i));
    }
    secp256k1_fuzz_sha256_standalone_compress(state, block);
    for (i = 0; i < 8; i++) {
        out[4 * i] = (unsigned char)(state[i] >> 24);
        out[4 * i + 1] = (unsigned char)(state[i] >> 16);
        out[4 * i + 2] = (unsigned char)(state[i] >> 8);
        out[4 * i + 3] = (unsigned char)state[i];
    }
    memset(state, 0, sizeof(state));
    memset(block, 0, sizeof(block));
}

#endif /* SECP256K1_FUZZ_SHA256_REFERENCE_H */
