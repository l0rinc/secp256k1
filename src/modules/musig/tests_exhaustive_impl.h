/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_MUSIG_TESTS_EXHAUSTIVE_H
#define SECP256K1_MODULE_MUSIG_TESTS_EXHAUSTIVE_H

#include "../../../include/secp256k1_musig.h"
#include "main_impl.h"

static void test_exhaustive_musig(const secp256k1_context *ctx) {
    unsigned char counter[32] = { 0 };
    unsigned char seckey[32] = { 0 };
    unsigned char pubkey_ser[33];
    size_t pubkey_ser_len = sizeof(pubkey_ser);
    secp256k1_pubkey pubkey;
    secp256k1_scalar k[2];

    secp256k1_write_be64(counter, 2);
    seckey[31] = 1;
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    CHECK(secp256k1_ec_pubkey_serialize(ctx, pubkey_ser, &pubkey_ser_len, &pubkey, SECP256K1_EC_COMPRESSED) == 1);
    secp256k1_nonce_function_musig(secp256k1_get_hash_context(ctx), k, counter, NULL, seckey, pubkey_ser, NULL, NULL);

    /* Counter 2 makes the second nonce scalar zero in the order-13 group. */
    CHECK(!secp256k1_scalar_is_zero(&k[0]));
    CHECK(secp256k1_scalar_is_zero(&k[1]));
    secp256k1_scalar_clear(&k[0]);
    secp256k1_scalar_clear(&k[1]);
}

#endif
