/***********************************************************************
 * Copyright (c) 2020 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_EXTRAKEYS_TESTS_EXHAUSTIVE_H
#define SECP256K1_MODULE_EXTRAKEYS_TESTS_EXHAUSTIVE_H

#include "../../../include/secp256k1_extrakeys.h"
#include "main_impl.h"

static void test_exhaustive_extrakeys(const secp256k1_context *ctx, const secp256k1_ge* group) {
    secp256k1_keypair keypair[EXHAUSTIVE_TEST_ORDER - 1];
    secp256k1_pubkey pubkey[EXHAUSTIVE_TEST_ORDER - 1];
    secp256k1_xonly_pubkey xonly_pubkey[EXHAUSTIVE_TEST_ORDER - 1];
    int parities[EXHAUSTIVE_TEST_ORDER - 1];
    unsigned char xonly_pubkey_bytes[EXHAUSTIVE_TEST_ORDER - 1][32];
    int i;

    for (i = 1; i < EXHAUSTIVE_TEST_ORDER; i++) {
        secp256k1_fe fe;
        secp256k1_scalar scalar_i;
        unsigned char buf[33];
        int parity;

        secp256k1_scalar_set_int(&scalar_i, i);
        secp256k1_scalar_get_b32(buf, &scalar_i);

        /* Construct pubkey and keypair. */
        CHECK(secp256k1_keypair_create(ctx, &keypair[i - 1], buf));
        CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey[i - 1], buf));

        /* Construct serialized xonly_pubkey from keypair. */
        CHECK(secp256k1_keypair_xonly_pub(ctx, &xonly_pubkey[i - 1], &parities[i - 1], &keypair[i - 1]));
        CHECK(secp256k1_xonly_pubkey_serialize(ctx, xonly_pubkey_bytes[i - 1], &xonly_pubkey[i - 1]));

        /* Parse the xonly_pubkey back and verify it matches the previously serialized value. */
        CHECK(secp256k1_xonly_pubkey_parse(ctx, &xonly_pubkey[i - 1], xonly_pubkey_bytes[i - 1]));
        CHECK(secp256k1_xonly_pubkey_serialize(ctx, buf, &xonly_pubkey[i - 1]));
        CHECK(secp256k1_memcmp_var(xonly_pubkey_bytes[i - 1], buf, 32) == 0);

        /* Construct the xonly_pubkey from the pubkey, and verify it matches the same. */
        CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &xonly_pubkey[i - 1], &parity, &pubkey[i - 1]));
        CHECK(parity == parities[i - 1]);
        CHECK(secp256k1_xonly_pubkey_serialize(ctx, buf, &xonly_pubkey[i - 1]));
        CHECK(secp256k1_memcmp_var(xonly_pubkey_bytes[i - 1], buf, 32) == 0);

        /* Compare the xonly_pubkey bytes against the precomputed group. */
        secp256k1_fe_set_b32_mod(&fe, xonly_pubkey_bytes[i - 1]);
        CHECK(secp256k1_fe_equal(&fe, &group[i].x));

        /* Check the parity against the precomputed group. */
        fe = group[i].y;
        secp256k1_fe_normalize_var(&fe);
        CHECK(secp256k1_fe_is_odd(&fe) == parities[i - 1]);

        /* Verify that the higher half is identical to the lower half mirrored. */
        if (i > EXHAUSTIVE_TEST_ORDER / 2) {
            CHECK(secp256k1_memcmp_var(xonly_pubkey_bytes[i - 1], xonly_pubkey_bytes[EXHAUSTIVE_TEST_ORDER - i - 1], 32) == 0);
            CHECK(parities[i - 1] == 1 - parities[EXHAUSTIVE_TEST_ORDER - i - 1]);
        }
    }

    for (i = 1; i < EXHAUSTIVE_TEST_ORDER; i++) {
        int j;
        int internal = parities[i - 1] ? EXHAUSTIVE_TEST_ORDER - i : i;

        for (j = 0; j < EXHAUSTIVE_TEST_ORDER; j++) {
            int expected = (internal + j) % EXHAUSTIVE_TEST_ORDER;
            secp256k1_scalar tweak_scalar;
            unsigned char tweak32[32];
            secp256k1_pubkey output_pk;
            secp256k1_keypair tweaked_keypair;
            secp256k1_pubkey output_pk_from_keypair;
            unsigned char expected32[32];
            unsigned char ser[33];
            size_t ser_len;
            secp256k1_fe expected_x;
            secp256k1_fe expected_y;
            int expected_parity;
            int ret_pk;
            int ret_keypair;

            secp256k1_scalar_set_int(&tweak_scalar, j);
            secp256k1_scalar_get_b32(tweak32, &tweak_scalar);

            tweaked_keypair = keypair[i - 1];
            ret_pk = secp256k1_xonly_pubkey_tweak_add(ctx, &output_pk, &xonly_pubkey[i - 1], tweak32);
            ret_keypair = secp256k1_keypair_xonly_tweak_add(ctx, &tweaked_keypair, tweak32);
            CHECK(ret_pk == (expected != 0));
            CHECK(ret_keypair == (expected != 0));

            if (expected == 0) {
                continue;
            }

            expected_x = group[expected].x;
            expected_y = group[expected].y;
            secp256k1_fe_normalize_var(&expected_x);
            secp256k1_fe_normalize_var(&expected_y);
            secp256k1_fe_get_b32(expected32, &expected_x);
            expected_parity = secp256k1_fe_is_odd(&expected_y);

            CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, expected32, expected_parity, &xonly_pubkey[i - 1], tweak32) == 1);
            CHECK(secp256k1_xonly_pubkey_tweak_add_check(ctx, expected32, !expected_parity, &xonly_pubkey[i - 1], tweak32) == 0);

            ser_len = sizeof(ser);
            CHECK(secp256k1_ec_pubkey_serialize(ctx, ser, &ser_len, &output_pk, SECP256K1_EC_COMPRESSED) == 1);
            CHECK(ser_len == sizeof(ser));
            CHECK(ser[0] == 0x02 + expected_parity);
            CHECK(secp256k1_memcmp_var(&ser[1], expected32, 32) == 0);

            CHECK(secp256k1_keypair_pub(ctx, &output_pk_from_keypair, &tweaked_keypair) == 1);
            ser_len = sizeof(ser);
            CHECK(secp256k1_ec_pubkey_serialize(ctx, ser, &ser_len, &output_pk_from_keypair, SECP256K1_EC_COMPRESSED) == 1);
            CHECK(ser_len == sizeof(ser));
            CHECK(ser[0] == 0x02 + expected_parity);
            CHECK(secp256k1_memcmp_var(&ser[1], expected32, 32) == 0);
        }
    }
}

#endif
