/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"
#include "sha256_reference.h"
#include "../int128_impl.h"
#include "../field_impl.h"

#ifdef ENABLE_MODULE_EXTRAKEYS
#include "pubkey_reference.h"

static const unsigned char secp256k1_fuzz_xonly_g_x[32] = {
    0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC,
    0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B, 0x07,
    0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9,
    0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
};
static const unsigned char secp256k1_fuzz_xonly_two_g_x[32] = {
    0xC6, 0x04, 0x7F, 0x94, 0x41, 0xED, 0x7D, 0x6D,
    0x30, 0x45, 0x40, 0x6E, 0x95, 0xC0, 0x7C, 0xD8,
    0x5C, 0x77, 0x8E, 0x4B, 0x8C, 0xEF, 0x3C, 0xA7,
    0xAB, 0xAC, 0x09, 0xB9, 0x5C, 0x70, 0x9E, 0xE5
};
static const unsigned char secp256k1_fuzz_xonly_three_g_x[32] = {
    0xF9, 0x30, 0x8A, 0x01, 0x92, 0x58, 0xC3, 0x10,
    0x49, 0x34, 0x4F, 0x85, 0xF8, 0x9D, 0x52, 0x29,
    0xB5, 0x31, 0xC8, 0x45, 0x83, 0x6F, 0x99, 0xB0,
    0x86, 0x01, 0xF1, 0x13, 0xBC, 0xE0, 0x36, 0xF9
};

static void secp256k1_fuzz_taproot_tagged_hash(unsigned char out32[32], const unsigned char *tag, size_t taglen, const unsigned char *part1, size_t part1_len, const unsigned char *part2, size_t part2_len) {
    unsigned char taghash[32];
    unsigned char transcript[64 + 9 + 256];
    size_t msglen = part1_len + part2_len;

    FUZZ_CHECK(msglen <= sizeof(transcript) - 64);
    secp256k1_fuzz_sha256_standalone(taghash, tag, taglen);
    memcpy(transcript, taghash, sizeof(taghash));
    memcpy(transcript + sizeof(taghash), taghash, sizeof(taghash));
    memcpy(transcript + 2 * sizeof(taghash), part1, part1_len);
    memcpy(transcript + 2 * sizeof(taghash) + part1_len, part2, part2_len);
    secp256k1_fuzz_sha256_standalone(out32, transcript, 2 * sizeof(taghash) + msglen);
}

static size_t secp256k1_fuzz_taproot_compact_size(unsigned char out9[9], size_t size) {
    if (size < 253) {
        out9[0] = (unsigned char)size;
        return 1;
    }
    if (size <= 0xFFFFu) {
        out9[0] = 0xFD;
        out9[1] = (unsigned char)size;
        out9[2] = (unsigned char)(size >> 8);
        return 3;
    }
    if (size <= 0xFFFFFFFFu) {
        out9[0] = 0xFE;
        out9[1] = (unsigned char)size;
        out9[2] = (unsigned char)(size >> 8);
        out9[3] = (unsigned char)(size >> 16);
        out9[4] = (unsigned char)(size >> 24);
        return 5;
    }
    out9[0] = 0xFF;
    out9[1] = (unsigned char)size;
    out9[2] = (unsigned char)(size >> 8);
    out9[3] = (unsigned char)(size >> 16);
    out9[4] = (unsigned char)(size >> 24);
    out9[5] = (unsigned char)(size >> 32);
    out9[6] = (unsigned char)(size >> 40);
    out9[7] = (unsigned char)(size >> 48);
    out9[8] = (unsigned char)(size >> 56);
    return 9;
}

static void secp256k1_fuzz_taproot_tapleaf_hash(unsigned char out32[32], unsigned char leaf_version, const unsigned char *script, size_t script_len) {
    unsigned char compact_size[9];
    unsigned char script_data[9 + 256];
    size_t compact_size_len;

    FUZZ_CHECK(script_len <= 256);
    compact_size_len = secp256k1_fuzz_taproot_compact_size(compact_size, script_len);
    memcpy(script_data, compact_size, compact_size_len);
    memcpy(script_data + compact_size_len, script, script_len);
    secp256k1_fuzz_taproot_tagged_hash(out32, (const unsigned char *)"TapLeaf", sizeof("TapLeaf") - 1, &leaf_version, 1, script_data, compact_size_len + script_len);
}

static void secp256k1_fuzz_taproot_tapbranch_hash(unsigned char out32[32], const unsigned char a32[32], const unsigned char b32[32]) {
    unsigned char ordered[64];

    if (memcmp(a32, b32, 32) < 0) {
        memcpy(ordered, a32, 32);
        memcpy(ordered + 32, b32, 32);
    } else {
        memcpy(ordered, b32, 32);
        memcpy(ordered + 32, a32, 32);
    }
    secp256k1_fuzz_taproot_tagged_hash(out32, (const unsigned char *)"TapBranch", sizeof("TapBranch") - 1, ordered, 32, ordered + 32, 32);
}

static int secp256k1_fuzz_taproot_merkle_root(unsigned char out32[32], const unsigned char control[], size_t control_len, const unsigned char tapleaf_hash32[32]) {
    size_t offset;

    if (control_len < 33 || control_len > 33 + 32 * 128 || (control_len - 33) % 32 != 0) {
        return 0;
    }
    memcpy(out32, tapleaf_hash32, 32);
    for (offset = 33; offset < control_len; offset += 32) {
        unsigned char next32[32];
        secp256k1_fuzz_taproot_tapbranch_hash(next32, out32, control + offset);
        memcpy(out32, next32, sizeof(next32));
    }
    return 1;
}

/* Model Bitcoin Core's VerifyTaprootCommitment after the witness and control
 * block size checks. The hashes are independently reconstructed from raw
 * script/control-block bytes before the library tweak check is called. */
static int secp256k1_fuzz_core_taproot_commitment(const secp256k1_context *ctx, const unsigned char control[], size_t control_len, const unsigned char program32[32], size_t program_len, const unsigned char *script, size_t script_len) {
    unsigned char tapleaf_hash32[32];
    unsigned char merkle_root32[32];
    unsigned char tweak32[32];
    secp256k1_xonly_pubkey internal_key;

    if (program_len != 32 || control_len < 33 || control_len > 33 + 32 * 128 || (control_len - 33) % 32 != 0) {
        return 0;
    }
    secp256k1_fuzz_taproot_tapleaf_hash(tapleaf_hash32, (unsigned char)(control[0] & 0xFEu), script, script_len);
    if (!secp256k1_fuzz_taproot_merkle_root(merkle_root32, control, control_len, tapleaf_hash32)) {
        return 0;
    }
    secp256k1_fuzz_taproot_tagged_hash(tweak32, (const unsigned char *)"TapTweak", sizeof("TapTweak") - 1, control + 1, 32, merkle_root32, 32);
    if (!secp256k1_xonly_pubkey_parse(ctx, &internal_key, control + 1)) {
        return 0;
    }
    return secp256k1_xonly_pubkey_tweak_add_check(ctx, program32, control[0] & 1u, &internal_key, tweak32);
}

static void secp256k1_fuzz_check_core_taproot_control_composition(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "core taproot control composition\n";
    static const unsigned char script[] = { 'f', 'o', 'o', 'b', 'a', 'r' };
    static const unsigned char sibling_leaf[32] = {
        0xA5, 0x75, 0x48, 0x2C, 0xAE, 0x62, 0x20, 0x67,
        0x6D, 0x4F, 0xF4, 0x53, 0x4C, 0xB6, 0x6F, 0x9E,
        0xAA, 0x50, 0x84, 0xE4, 0x07, 0xBB, 0x87, 0x03,
        0x72, 0x70, 0x07, 0x33, 0x10, 0x39, 0x50, 0xB7
    };
    static const unsigned char expected_leaf[32] = {
        0x06, 0x9C, 0x5B, 0x7B, 0x76, 0x36, 0xF1, 0x03,
        0x12, 0x52, 0x45, 0xCD, 0x97, 0xED, 0xD6, 0x5A,
        0x7B, 0x02, 0xB9, 0x05, 0xD6, 0x11, 0xCC, 0xCD,
        0x5D, 0x21, 0xA1, 0x72, 0xC2, 0x10, 0xBC, 0xED
    };
    static const unsigned char expected_branch[32] = {
        0xEA, 0x62, 0x74, 0x50, 0xEB, 0xEF, 0x6D, 0xEE,
        0xFF, 0xBA, 0x20, 0x73, 0xEA, 0x20, 0xA1, 0x8F,
        0xBD, 0xC9, 0x32, 0xD5, 0x26, 0x43, 0xD8, 0xF6,
        0x57, 0x41, 0x15, 0x76, 0xA1, 0xD4, 0x01, 0x77
    };
    static const unsigned char expected_tweak_one[32] = {
        0xB3, 0x49, 0x9F, 0x91, 0x10, 0x38, 0xA2, 0xD7,
        0x62, 0x4A, 0x7B, 0xD4, 0x62, 0xE8, 0x08, 0x9E,
        0x98, 0x95, 0x1B, 0x53, 0xFD, 0x7D, 0x2C, 0xB1,
        0xB2, 0x2D, 0x4A, 0x9D, 0xB7, 0x9B, 0x68, 0x00
    };
    static const unsigned char expected_tweak_two[32] = {
        0xCA, 0x13, 0x00, 0x24, 0x05, 0xBF, 0x0A, 0x9F,
        0x6E, 0x19, 0xD3, 0x18, 0xCE, 0x7F, 0x9F, 0x0C,
        0x78, 0x56, 0xB7, 0x16, 0x8D, 0x3D, 0x57, 0x92,
        0x91, 0xCD, 0xE0, 0x4B, 0xAE, 0xF6, 0x64, 0x52
    };
    static const unsigned char expected_program_one[32] = {
        0x0E, 0x16, 0xD2, 0x1C, 0x42, 0xD6, 0x72, 0x65,
        0x03, 0xF7, 0x69, 0x10, 0x43, 0xB0, 0x74, 0x25,
        0x1E, 0x30, 0x3D, 0x75, 0x8E, 0xC7, 0x4B, 0x10,
        0x11, 0xD7, 0x6A, 0xEC, 0xB6, 0xA3, 0xD8, 0x8D
    };
    static const unsigned char expected_program_two[32] = {
        0x27, 0x52, 0x08, 0x42, 0x42, 0xFE, 0x80, 0xEC,
        0x98, 0xD5, 0xE1, 0x44, 0xCB, 0x7C, 0xCB, 0xBA,
        0xF5, 0x25, 0x22, 0x24, 0x36, 0xEC, 0xC9, 0xE4,
        0x95, 0x76, 0x6C, 0x4D, 0x1C, 0x8C, 0x14, 0x6F
    };
    struct taproot_case {
        const unsigned char *node;
        const unsigned char *expected_merkle;
        const unsigned char *expected_tweak;
        const unsigned char *expected_program;
        unsigned char parity;
    } cases[2] = {
        { NULL, expected_leaf, expected_tweak_one, expected_program_one, 1 },
        { sibling_leaf, expected_branch, expected_tweak_two, expected_program_two, 0 }
    };
    const secp256k1_context *contexts[2];
    unsigned char control[65];
    unsigned char tapleaf_hash[32];
    unsigned char merkle_root[32];
    unsigned char tweak32[32];
    unsigned char bad_control[65];
    unsigned char bad_program[32];
    secp256k1_xonly_pubkey internal_key;
    secp256k1_pubkey actual_pubkey;
    secp256k1_xonly_pubkey actual_xonly;
    unsigned char actual_program[32];
    size_t i;
    size_t j;
    int parity;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    contexts[0] = ctx;
    contexts[1] = secp256k1_context_static;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        memset(control, 0, sizeof(control));
        control[0] = (unsigned char)(0xC0u | cases[i].parity);
        memcpy(control + 1, secp256k1_fuzz_xonly_g_x, 32);
        if (cases[i].node != NULL) {
            memcpy(control + 33, cases[i].node, 32);
        }
        j = cases[i].node == NULL ? 33 : sizeof(control);

        secp256k1_fuzz_taproot_tapleaf_hash(tapleaf_hash, control[0] & 0xFEu, script, sizeof(script));
        FUZZ_CHECK(memcmp(tapleaf_hash, expected_leaf, sizeof(tapleaf_hash)) == 0);
        FUZZ_CHECK(secp256k1_fuzz_taproot_merkle_root(merkle_root, control, j, tapleaf_hash) == 1);
        FUZZ_CHECK(memcmp(merkle_root, cases[i].expected_merkle, sizeof(merkle_root)) == 0);
        secp256k1_fuzz_taproot_tagged_hash(tweak32, (const unsigned char *)"TapTweak", sizeof("TapTweak") - 1, control + 1, 32, merkle_root, 32);
        FUZZ_CHECK(memcmp(tweak32, cases[i].expected_tweak, sizeof(tweak32)) == 0);

        for (j = 0; j < sizeof(contexts) / sizeof(contexts[0]); j++) {
            FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(contexts[j], control, cases[i].node == NULL ? 33 : sizeof(control), cases[i].expected_program, 32, script, sizeof(script)) == 1);
            FUZZ_CHECK(secp256k1_xonly_pubkey_parse(contexts[j], &internal_key, control + 1) == 1);
            FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add(contexts[j], &actual_pubkey, &internal_key, tweak32) == 1);
            parity = -1;
            FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(contexts[j], &actual_xonly, &parity, &actual_pubkey) == 1);
            FUZZ_CHECK(parity == cases[i].parity);
            FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(contexts[j], actual_program, &actual_xonly) == 1);
            FUZZ_CHECK(memcmp(actual_program, cases[i].expected_program, sizeof(actual_program)) == 0);
        }

        memcpy(bad_program, cases[i].expected_program, sizeof(bad_program));
        bad_program[0] ^= 1u;
        FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(ctx, control, cases[i].node == NULL ? 33 : sizeof(control), bad_program, sizeof(bad_program), script, sizeof(script)) == 0);
        memcpy(bad_control, control, sizeof(bad_control));
        bad_control[0] ^= 1u;
        FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(ctx, bad_control, cases[i].node == NULL ? 33 : sizeof(control), cases[i].expected_program, 32, script, sizeof(script)) == 0);
        memcpy(bad_control, control, sizeof(bad_control));
        memcpy(bad_control + 1, secp256k1_fuzz_pubkey_field_prime, 32);
        FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(ctx, bad_control, cases[i].node == NULL ? 33 : sizeof(control), cases[i].expected_program, 32, script, sizeof(script)) == 0);
        if (cases[i].node != NULL) {
            memcpy(bad_control, control, sizeof(bad_control));
            bad_control[33] ^= 1u;
            FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(ctx, bad_control, sizeof(control), cases[i].expected_program, 32, script, sizeof(script)) == 0);
        }
        FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(ctx, control, 32, cases[i].expected_program, 32, script, sizeof(script)) == 0);
        FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(ctx, control, cases[i].node == NULL ? 33 : sizeof(control), cases[i].expected_program, 31, script, sizeof(script)) == 0);
    }
}

static void secp256k1_fuzz_check_core_taproot_tapleaf_boundaries(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "core taproot tapleaf compactsize boundaries\n";
    static const unsigned char expected_leaf_252[32] = {
        0x4C, 0xC7, 0xAC, 0x60, 0x20, 0xB0, 0xB8, 0xA5,
        0x28, 0x45, 0xBE, 0x64, 0xE0, 0x38, 0x2B, 0xAB,
        0x51, 0xD7, 0xC5, 0x65, 0x12, 0x28, 0x5B, 0xD7,
        0xF7, 0xB0, 0x7F, 0x26, 0x19, 0x24, 0xF6, 0x7B
    };
    static const unsigned char expected_leaf_253[32] = {
        0xF5, 0xE5, 0x75, 0xFC, 0xCC, 0x11, 0x21, 0x6B,
        0x89, 0xB6, 0x43, 0xCA, 0x5B, 0x05, 0x91, 0x1E,
        0x5D, 0xE7, 0x39, 0xB5, 0xBA, 0xD7, 0x96, 0x52,
        0x91, 0xD7, 0xB6, 0x99, 0x15, 0xA7, 0x55, 0x1C
    };
    static const unsigned char expected_tweak_252[32] = {
        0x7D, 0x74, 0x97, 0xDC, 0x70, 0x14, 0x57, 0x87,
        0x54, 0x1D, 0x06, 0x4F, 0xCE, 0x94, 0x17, 0xDE,
        0x12, 0x14, 0x9D, 0x86, 0xC8, 0xC6, 0xD5, 0xA8,
        0xA6, 0x60, 0x8B, 0x37, 0xC7, 0x30, 0xF9, 0x4C
    };
    static const unsigned char expected_tweak_253[32] = {
        0xDC, 0xD6, 0xCF, 0x3D, 0xDC, 0x1D, 0xF8, 0xEB,
        0x6F, 0xF5, 0x21, 0xE4, 0x7E, 0x49, 0x38, 0xD6,
        0xA2, 0xE9, 0x60, 0xC5, 0xFD, 0x95, 0x19, 0xBB,
        0xCD, 0x24, 0xC0, 0xC7, 0x60, 0xAC, 0xBB, 0xE7
    };
    static const unsigned char expected_program_252[32] = {
        0x37, 0x88, 0xCE, 0x77, 0xDF, 0xB8, 0x44, 0x02,
        0x62, 0xFD, 0x66, 0xBE, 0xB1, 0x8A, 0xCA, 0x74,
        0xF1, 0x14, 0xF9, 0x45, 0x56, 0xA3, 0x96, 0x00,
        0x89, 0x9D, 0xE8, 0x8C, 0xF6, 0x5F, 0x19, 0xF3
    };
    static const unsigned char expected_program_253[32] = {
        0x52, 0x2E, 0xA7, 0x99, 0xDD, 0x70, 0x7F, 0x61,
        0x98, 0xB2, 0x04, 0x69, 0x8D, 0x89, 0xC3, 0xBD,
        0xAE, 0xBF, 0x14, 0x26, 0x51, 0x6B, 0x96, 0x82,
        0xA3, 0x54, 0x47, 0x47, 0xC1, 0x35, 0xA0, 0x85
    };
    struct tapleaf_case {
        size_t script_len;
        const unsigned char *expected_leaf;
        const unsigned char *expected_tweak;
        const unsigned char *expected_program;
    } cases[2] = {
        { 252, expected_leaf_252, expected_tweak_252, expected_program_252 },
        { 253, expected_leaf_253, expected_tweak_253, expected_program_253 }
    };
    const secp256k1_context *contexts[2];
    unsigned char script[253];
    unsigned char compact_size[9];
    unsigned char control[33];
    unsigned char tapleaf_hash[32];
    unsigned char tweak32[32];
    unsigned char bad_program[32];
    unsigned char actual_program[32];
    secp256k1_xonly_pubkey internal_key;
    secp256k1_xonly_pubkey actual_xonly;
    secp256k1_pubkey actual_pubkey;
    size_t i;
    size_t j;
    int parity;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    for (i = 0; i < sizeof(script); i++) {
        script[i] = (unsigned char)(0x5Du + 29u * i);
    }
    contexts[0] = ctx;
    contexts[1] = secp256k1_context_static;
    memset(control, 0, sizeof(control));
    control[0] = 0xC1;
    memcpy(control + 1, secp256k1_fuzz_xonly_g_x, 32);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        size_t compact_size_len = secp256k1_fuzz_taproot_compact_size(compact_size, cases[i].script_len);

        if (cases[i].script_len == 252) {
            FUZZ_CHECK(compact_size_len == 1 && compact_size[0] == 0xFC);
        } else {
            FUZZ_CHECK(compact_size_len == 3 && compact_size[0] == 0xFD && compact_size[1] == 0xFD && compact_size[2] == 0x00);
        }
        secp256k1_fuzz_taproot_tapleaf_hash(tapleaf_hash, control[0] & 0xFEu, script, cases[i].script_len);
        FUZZ_CHECK(memcmp(tapleaf_hash, cases[i].expected_leaf, sizeof(tapleaf_hash)) == 0);
        secp256k1_fuzz_taproot_tagged_hash(tweak32, (const unsigned char *)"TapTweak", sizeof("TapTweak") - 1, control + 1, 32, tapleaf_hash, 32);
        FUZZ_CHECK(memcmp(tweak32, cases[i].expected_tweak, sizeof(tweak32)) == 0);
        for (j = 0; j < sizeof(contexts) / sizeof(contexts[0]); j++) {
            FUZZ_CHECK(secp256k1_xonly_pubkey_parse(contexts[j], &internal_key, control + 1) == 1);
            FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add(contexts[j], &actual_pubkey, &internal_key, tweak32) == 1);
            parity = -1;
            FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(contexts[j], &actual_xonly, &parity, &actual_pubkey) == 1);
            FUZZ_CHECK(parity == 1);
            FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(contexts[j], actual_program, &actual_xonly) == 1);
            FUZZ_CHECK(memcmp(actual_program, cases[i].expected_program, sizeof(actual_program)) == 0);
            FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(contexts[j], control, sizeof(control), cases[i].expected_program, 32, script, cases[i].script_len) == 1);
        }

        memcpy(bad_program, cases[i].expected_program, sizeof(bad_program));
        bad_program[0] ^= 1u;
        FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(ctx, control, sizeof(control), bad_program, 32, script, cases[i].script_len) == 0);
        FUZZ_CHECK(secp256k1_fuzz_core_taproot_commitment(ctx, control, sizeof(control), cases[i].expected_program, 32, script, cases[i == 0 ? 1 : 0].script_len) == 0);
    }
}

typedef struct {
    unsigned char x[32];
    unsigned char y[32];
    int infinity;
} secp256k1_fuzz_xonly_byte_point;

/* These byte operations intentionally avoid the production field and group
 * representations. The affine model uses the production field inverse only
 * for the one gated trigger, keeping its byte arithmetic and group equation
 * independent without making routine fuzz iterations prohibitively costly. */
static void secp256k1_fuzz_xonly_byte_sub_mod(unsigned char out32[32], const unsigned char a32[32], const unsigned char b32[32]) {
    unsigned char negated32[32];
    unsigned int borrow = 0;
    size_t i;

    for (i = 32; i-- > 0;) {
        int value = (int)secp256k1_fuzz_pubkey_field_prime[i] - b32[i] - borrow;
        if (value < 0) {
            value += 256;
            borrow = 1;
        } else {
            borrow = 0;
        }
        negated32[i] = (unsigned char)value;
    }
    FUZZ_CHECK(borrow == 0);
    secp256k1_fuzz_pubkey_add_mod(out32, a32, negated32);
}

static void secp256k1_fuzz_xonly_byte_neg_mod(unsigned char out32[32], const unsigned char input32[32]) {
    unsigned char zero32[32] = { 0 };

    secp256k1_fuzz_xonly_byte_sub_mod(out32, zero32, input32);
}

static void secp256k1_fuzz_xonly_byte_inv_mod(unsigned char out32[32], const unsigned char input32[32]) {
    secp256k1_fe input;
    secp256k1_fe inverse;

    FUZZ_CHECK(secp256k1_fe_set_b32_limit(&input, input32) == 1);
    FUZZ_CHECK(!secp256k1_fe_normalizes_to_zero_var(&input));
    secp256k1_fe_inv_var(&inverse, &input);
    secp256k1_fe_normalize_var(&inverse);
    secp256k1_fe_get_b32(out32, &inverse);
}

static void secp256k1_fuzz_xonly_byte_point_double(secp256k1_fuzz_xonly_byte_point *result, const secp256k1_fuzz_xonly_byte_point *point) {
    unsigned char x_squared32[32];
    unsigned char three_x_squared32[32];
    unsigned char two_y32[32];
    unsigned char inverse32[32];
    unsigned char lambda32[32];
    unsigned char lambda_squared32[32];
    unsigned char x_difference32[32];
    unsigned char y_difference32[32];

    if (point->infinity || memcmp(point->y, secp256k1_fuzz_scalar_zero, 32) == 0) {
        memset(result, 0, sizeof(*result));
        result->infinity = 1;
        return;
    }
    secp256k1_fuzz_pubkey_mul_mod(x_squared32, point->x, point->x);
    secp256k1_fuzz_pubkey_add_mod(three_x_squared32, x_squared32, x_squared32);
    secp256k1_fuzz_pubkey_add_mod(three_x_squared32, three_x_squared32, x_squared32);
    secp256k1_fuzz_pubkey_add_mod(two_y32, point->y, point->y);
    secp256k1_fuzz_xonly_byte_inv_mod(inverse32, two_y32);
    secp256k1_fuzz_pubkey_mul_mod(lambda32, three_x_squared32, inverse32);
    secp256k1_fuzz_pubkey_mul_mod(lambda_squared32, lambda32, lambda32);
    secp256k1_fuzz_xonly_byte_sub_mod(x_difference32, lambda_squared32, point->x);
    secp256k1_fuzz_xonly_byte_sub_mod(result->x, x_difference32, point->x);
    secp256k1_fuzz_xonly_byte_sub_mod(x_difference32, point->x, result->x);
    secp256k1_fuzz_pubkey_mul_mod(y_difference32, lambda32, x_difference32);
    secp256k1_fuzz_xonly_byte_sub_mod(result->y, y_difference32, point->y);
    result->infinity = 0;
}

static void secp256k1_fuzz_xonly_byte_point_add(secp256k1_fuzz_xonly_byte_point *result, const secp256k1_fuzz_xonly_byte_point *a, const secp256k1_fuzz_xonly_byte_point *b) {
    unsigned char x_difference32[32];
    unsigned char y_difference32[32];
    unsigned char inverse32[32];
    unsigned char lambda32[32];
    unsigned char lambda_squared32[32];
    unsigned char x_sum32[32];
    unsigned char x_difference_result32[32];
    unsigned char y_difference_result32[32];

    if (a->infinity) {
        *result = *b;
        return;
    }
    if (b->infinity) {
        *result = *a;
        return;
    }
    if (memcmp(a->x, b->x, 32) == 0) {
        if (memcmp(a->y, b->y, 32) == 0) {
            secp256k1_fuzz_xonly_byte_point_double(result, a);
        } else {
            memset(result, 0, sizeof(*result));
            result->infinity = 1;
        }
        return;
    }
    secp256k1_fuzz_xonly_byte_sub_mod(x_difference32, b->x, a->x);
    secp256k1_fuzz_xonly_byte_sub_mod(y_difference32, b->y, a->y);
    secp256k1_fuzz_xonly_byte_inv_mod(inverse32, x_difference32);
    secp256k1_fuzz_pubkey_mul_mod(lambda32, y_difference32, inverse32);
    secp256k1_fuzz_pubkey_mul_mod(lambda_squared32, lambda32, lambda32);
    secp256k1_fuzz_xonly_byte_sub_mod(x_sum32, lambda_squared32, a->x);
    secp256k1_fuzz_xonly_byte_sub_mod(result->x, x_sum32, b->x);
    secp256k1_fuzz_xonly_byte_sub_mod(x_difference_result32, a->x, result->x);
    secp256k1_fuzz_pubkey_mul_mod(y_difference_result32, lambda32, x_difference_result32);
    secp256k1_fuzz_xonly_byte_sub_mod(result->y, y_difference_result32, a->y);
    result->infinity = 0;
}

static void secp256k1_fuzz_xonly_byte_point_from_xonly(secp256k1_fuzz_xonly_byte_point *point, const unsigned char x32[32]) {
    unsigned char x_squared32[32];
    unsigned char rhs32[32];
    unsigned char seven32[32] = { 0 };

    memcpy(point->x, x32, sizeof(point->x));
    seven32[31] = 7;
    secp256k1_fuzz_pubkey_mul_mod(x_squared32, x32, x32);
    secp256k1_fuzz_pubkey_mul_mod(rhs32, x_squared32, x32);
    secp256k1_fuzz_pubkey_add_mod(rhs32, rhs32, seven32);
    secp256k1_fuzz_pubkey_sqrt_mod(point->y, rhs32);
    if ((point->y[31] & 1u) != 0) {
        secp256k1_fuzz_xonly_byte_neg_mod(point->y, point->y);
    }
    point->infinity = 0;
}

static void secp256k1_fuzz_check_xonly_tweak_affine(const secp256k1_context *ctx, const secp256k1_xonly_pubkey *xonly, const unsigned char *xonly32, const secp256k1_keypair *keypair) {
    static const unsigned char two32[32] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2
    };
    static const secp256k1_fuzz_xonly_byte_point generator = {
        {
            0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC,
            0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B, 0x07,
            0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9,
            0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
        },
        {
            0x48, 0x3A, 0xDA, 0x77, 0x26, 0xA3, 0xC4, 0x65,
            0x5D, 0xA4, 0xFB, 0xFC, 0x0E, 0x11, 0x08, 0xA8,
            0xFD, 0x17, 0xB4, 0x48, 0xA6, 0x85, 0x54, 0x19,
            0x9C, 0x47, 0xD0, 0x8F, 0xFB, 0x10, 0xD4, 0xB8
        },
        0
    };
    secp256k1_fuzz_xonly_byte_point base;
    secp256k1_fuzz_xonly_byte_point expected;
    secp256k1_fuzz_xonly_byte_point two_g;
    secp256k1_pubkey actual;
    secp256k1_pubkey static_actual;
    secp256k1_pubkey keypair_actual;
    secp256k1_xonly_pubkey static_xonly;
    unsigned char expected33[33];
    unsigned char actual33[33];
    unsigned char static_actual33[33];
    unsigned char keypair_actual33[33];
    size_t actual_len;
    size_t static_actual_len;
    size_t keypair_actual_len;
    secp256k1_keypair tweaked_keypair = *keypair;

    secp256k1_fuzz_xonly_byte_point_from_xonly(&base, xonly32);
    secp256k1_fuzz_xonly_byte_point_double(&two_g, &generator);
    secp256k1_fuzz_xonly_byte_point_add(&expected, &base, &two_g);
    FUZZ_CHECK(!expected.infinity);
    expected33[0] = (unsigned char)(SECP256K1_TAG_PUBKEY_EVEN + (expected.y[31] & 1u));
    memcpy(expected33 + 1, expected.x, 32);

    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add(ctx, &actual, xonly, two32) == 1);
    actual_len = sizeof(actual33);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, actual33, &actual_len, &actual, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(actual_len == sizeof(actual33));
    FUZZ_CHECK(memcmp(actual33, expected33, sizeof(actual33)) == 0);

    FUZZ_CHECK(secp256k1_xonly_pubkey_parse(secp256k1_context_static, &static_xonly, xonly32) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add(secp256k1_context_static, &static_actual, &static_xonly, two32) == 1);
    static_actual_len = sizeof(static_actual33);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(secp256k1_context_static, static_actual33, &static_actual_len, &static_actual, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(static_actual_len == sizeof(static_actual33));
    FUZZ_CHECK(memcmp(static_actual33, expected33, sizeof(static_actual33)) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_tweak_add_check(secp256k1_context_static, expected33 + 1, expected.y[31] & 1u, &static_xonly, two32) == 1);

    FUZZ_CHECK(secp256k1_keypair_xonly_tweak_add(ctx, &tweaked_keypair, two32) == 1);
    FUZZ_CHECK(secp256k1_keypair_pub(ctx, &keypair_actual, &tweaked_keypair) == 1);
    keypair_actual_len = sizeof(keypair_actual33);
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, keypair_actual33, &keypair_actual_len, &keypair_actual, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(keypair_actual_len == sizeof(keypair_actual33));
    FUZZ_CHECK(memcmp(keypair_actual33, expected33, sizeof(keypair_actual33)) == 0);
}

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

static void secp256k1_fuzz_check_static_xonly_parse_case(const secp256k1_context *ctx, const unsigned char input32[32]) {
    unsigned char normal_serialized[32];
    unsigned char static_serialized[32];
    unsigned char cross_serialized[32];
    unsigned char zero_xonly[sizeof(secp256k1_xonly_pubkey)] = { 0 };
    secp256k1_xonly_pubkey normal_xonly;
    secp256k1_xonly_pubkey static_xonly;
    secp256k1_xonly_pubkey normal_before;
    secp256k1_xonly_pubkey static_before;
    int normal_ret;
    int static_ret;

    memset(&normal_xonly, 0xA5, sizeof(normal_xonly));
    memset(&static_xonly, 0xA5, sizeof(static_xonly));
    normal_ret = secp256k1_xonly_pubkey_parse(ctx, &normal_xonly, input32);
    static_ret = secp256k1_xonly_pubkey_parse(secp256k1_context_static, &static_xonly, input32);
    FUZZ_CHECK(normal_ret == secp256k1_fuzz_xonly_parse_reference(input32));
    FUZZ_CHECK(static_ret == normal_ret);
    if (!normal_ret) {
        FUZZ_CHECK(memcmp(&normal_xonly, zero_xonly, sizeof(normal_xonly)) == 0);
        FUZZ_CHECK(memcmp(&static_xonly, zero_xonly, sizeof(static_xonly)) == 0);
        return;
    }

    normal_before = normal_xonly;
    static_before = static_xonly;
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, normal_serialized, &normal_xonly) == 1);
    FUZZ_CHECK(memcmp(&normal_xonly, &normal_before, sizeof(normal_xonly)) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(secp256k1_context_static, static_serialized, &static_xonly) == 1);
    FUZZ_CHECK(memcmp(&static_xonly, &static_before, sizeof(static_xonly)) == 0);
    FUZZ_CHECK(memcmp(normal_serialized, input32, sizeof(normal_serialized)) == 0);
    FUZZ_CHECK(memcmp(static_serialized, normal_serialized, sizeof(static_serialized)) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(secp256k1_context_static, cross_serialized, &normal_xonly) == 1);
    FUZZ_CHECK(memcmp(cross_serialized, normal_serialized, sizeof(cross_serialized)) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, cross_serialized, &static_xonly) == 1);
    FUZZ_CHECK(memcmp(cross_serialized, normal_serialized, sizeof(cross_serialized)) == 0);
}

static void secp256k1_fuzz_check_static_xonly_codecs(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey, const secp256k1_xonly_pubkey *valid_xonly, int valid_parity, const unsigned char valid_xonly32[32]) {
    static const unsigned char all_ones32[32] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    secp256k1_xonly_pubkey static_from_pubkey;
    secp256k1_xonly_pubkey static_from_pubkey_no_parity;
    secp256k1_xonly_pubkey normal_pubkeys[2];
    secp256k1_xonly_pubkey static_pubkeys[2];
    unsigned char serialized[32];
    int static_parity;
    size_t i;
    size_t j;

    FUZZ_CHECK(valid_parity == 0 || valid_parity == 1);
    static_parity = -1;
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(secp256k1_context_static, &static_from_pubkey, &static_parity, pubkey) == 1);
    FUZZ_CHECK(static_parity == valid_parity);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(secp256k1_context_static, &static_from_pubkey, valid_xonly) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(secp256k1_context_static, serialized, &static_from_pubkey) == 1);
    FUZZ_CHECK(memcmp(serialized, valid_xonly32, sizeof(serialized)) == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(secp256k1_context_static, &static_from_pubkey_no_parity, NULL, pubkey) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(secp256k1_context_static, &static_from_pubkey_no_parity, &static_from_pubkey) == 0);

    secp256k1_fuzz_check_static_xonly_parse_case(ctx, valid_xonly32);
    secp256k1_fuzz_check_static_xonly_parse_case(ctx, secp256k1_fuzz_xonly_two_g_x);
    secp256k1_fuzz_check_static_xonly_parse_case(ctx, secp256k1_fuzz_pubkey_field_p_plus_one);
    secp256k1_fuzz_check_static_xonly_parse_case(ctx, all_ones32);

    FUZZ_CHECK(secp256k1_xonly_pubkey_parse(ctx, &normal_pubkeys[0], valid_xonly32) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_parse(ctx, &normal_pubkeys[1], secp256k1_fuzz_xonly_two_g_x) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_parse(secp256k1_context_static, &static_pubkeys[0], valid_xonly32) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_parse(secp256k1_context_static, &static_pubkeys[1], secp256k1_fuzz_xonly_two_g_x) == 1);
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            int normal_cmp = secp256k1_xonly_pubkey_cmp(ctx, &normal_pubkeys[i], &normal_pubkeys[j]);
            int static_cmp = secp256k1_xonly_pubkey_cmp(secp256k1_context_static, &static_pubkeys[i], &static_pubkeys[j]);

            FUZZ_CHECK((static_cmp == 0) == (normal_cmp == 0));
            FUZZ_CHECK((static_cmp < 0) == (normal_cmp < 0));
            FUZZ_CHECK((static_cmp > 0) == (normal_cmp > 0));
        }
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

static void secp256k1_fuzz_check_static_context_keypair(const secp256k1_keypair *keypair, const secp256k1_xonly_pubkey *valid_xonly, int valid_parity) {
    secp256k1_keypair tweaked_keypair = *keypair;
    secp256k1_xonly_pubkey projected_xonly;
    int projected_parity = 7;

    /* Public-only projection does not need generator precomputation, but
     * mutating a keypair does. Keep the two context requirements distinct. */
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(secp256k1_context_static, &projected_xonly, &projected_parity, keypair) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(secp256k1_context_static, &projected_xonly, valid_xonly) == 0);
    FUZZ_CHECK(projected_parity == valid_parity);

#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
    {
        secp256k1_keypair static_keypair;
        unsigned char zero_keypair[sizeof(tweaked_keypair)] = { 0 };
        unsigned int calls = secp256k1_fuzz_default_illegal_calls;

        memset(&static_keypair, 0xA5, sizeof(static_keypair));
        FUZZ_CHECK(secp256k1_keypair_create(secp256k1_context_static, &static_keypair, secp256k1_fuzz_scalar_one) == 0);
        FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
        FUZZ_CHECK(memcmp(&static_keypair, zero_keypair, sizeof(static_keypair)) == 0);

        FUZZ_CHECK(secp256k1_keypair_xonly_tweak_add(secp256k1_context_static, &tweaked_keypair, secp256k1_fuzz_scalar_zero) == 0);
        FUZZ_CHECK(secp256k1_fuzz_default_illegal_calls == ++calls);
        FUZZ_CHECK(memcmp(&tweaked_keypair, zero_keypair, sizeof(tweaked_keypair)) == 0);
    }
#else
    (void)tweaked_keypair;
#endif
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

static void secp256k1_fuzz_check_keypair_create_vectors(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "keypair create vectors\n";
    unsigned char scalar_two[32] = { 0 };
    unsigned char scalar_three[32] = { 0 };
    struct {
        const unsigned char *seckey32;
        const unsigned char *expected_xonly32;
        int expected_parity;
    } cases[4];
    secp256k1_keypair keypair;
    secp256k1_pubkey pubkey;
    secp256k1_xonly_pubkey xonly;
    secp256k1_xonly_pubkey xonly_no_parity;
    unsigned char expected33[33];
    unsigned char pubkey33[33];
    unsigned char xonly32[32];
    unsigned char extracted_seckey[32];
    size_t output_len;
    size_t i;
    int parity;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    scalar_two[31] = 2;
    scalar_three[31] = 3;
    cases[0].seckey32 = secp256k1_fuzz_scalar_one;
    cases[0].expected_xonly32 = secp256k1_fuzz_xonly_g_x;
    cases[0].expected_parity = 0;
    cases[1].seckey32 = scalar_two;
    cases[1].expected_xonly32 = secp256k1_fuzz_xonly_two_g_x;
    cases[1].expected_parity = 0;
    cases[2].seckey32 = scalar_three;
    cases[2].expected_xonly32 = secp256k1_fuzz_xonly_three_g_x;
    cases[2].expected_parity = 0;
    cases[3].seckey32 = secp256k1_fuzz_scalar_order_minus_one;
    cases[3].expected_xonly32 = secp256k1_fuzz_xonly_g_x;
    cases[3].expected_parity = 1;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        memset(&keypair, 0xA5, sizeof(keypair));
        FUZZ_CHECK(secp256k1_keypair_create(ctx, &keypair, cases[i].seckey32) == 1);

        memset(extracted_seckey, 0x5A, sizeof(extracted_seckey));
        FUZZ_CHECK(secp256k1_keypair_sec(ctx, extracted_seckey, &keypair) == 1);
        FUZZ_CHECK(memcmp(extracted_seckey, cases[i].seckey32, sizeof(extracted_seckey)) == 0);

        expected33[0] = (unsigned char)(cases[i].expected_parity ? SECP256K1_TAG_PUBKEY_ODD : SECP256K1_TAG_PUBKEY_EVEN);
        memcpy(expected33 + 1, cases[i].expected_xonly32, 32);
        FUZZ_CHECK(secp256k1_keypair_pub(ctx, &pubkey, &keypair) == 1);
        output_len = sizeof(pubkey33);
        FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, pubkey33, &output_len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
        FUZZ_CHECK(output_len == sizeof(pubkey33));
        FUZZ_CHECK(memcmp(pubkey33, expected33, sizeof(pubkey33)) == 0);

        parity = -1;
        FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly, &parity, &keypair) == 1);
        FUZZ_CHECK(parity == cases[i].expected_parity);
        FUZZ_CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly32, &xonly) == 1);
        FUZZ_CHECK(memcmp(xonly32, cases[i].expected_xonly32, sizeof(xonly32)) == 0);
        FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly_no_parity, NULL, &keypair) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &xonly, &xonly_no_parity) == 0);
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
    secp256k1_fuzz_check_keypair_create_vectors(ctx, input, size);
    secp256k1_fuzz_check_tweak_input_output_alias(ctx, input, size);
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
    if (size == sizeof("static context keypair barrier\n") - 1
        && memcmp(input, "static context keypair barrier\n", sizeof("static context keypair barrier\n") - 1) == 0) {
        secp256k1_fuzz_check_static_context_keypair(&keypair, &xonly, keypair_parity);
    }
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
    if (size == sizeof("static xonly public codecs\n") - 1
        && memcmp(input, "static xonly public codecs\n", sizeof("static xonly public codecs\n") - 1) == 0) {
        secp256k1_fuzz_check_static_xonly_codecs(ctx, &pubkey, &xonly, keypair_parity, xonly32);
    }
    if (size == sizeof("xonly tweak affine reference\n") - 1
        && memcmp(input, "xonly tweak affine reference\n", sizeof("xonly tweak affine reference\n") - 1) == 0) {
        secp256k1_fuzz_check_xonly_tweak_affine(ctx, &xonly, xonly32, &keypair);
    }
    secp256k1_fuzz_check_core_taproot_control_composition(ctx, input, size);
    secp256k1_fuzz_check_core_taproot_tapleaf_boundaries(ctx, input, size);
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
