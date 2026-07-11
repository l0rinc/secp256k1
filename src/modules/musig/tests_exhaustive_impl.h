/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_MUSIG_TESTS_EXHAUSTIVE_H
#define SECP256K1_MODULE_MUSIG_TESTS_EXHAUSTIVE_H

#include "../../../include/secp256k1_musig.h"
#include "main_impl.h"

/* In the small exhaustive group, the hash-derived keyagg coefficient can be
 * zero. Five copies of G therefore make a deterministic infinity aggregate. */
static void test_exhaustive_musig(const secp256k1_context *ctx) {
    unsigned char seckey[32] = { 0 };
    unsigned char zero_agg[sizeof(secp256k1_xonly_pubkey)] = { 0 };
    unsigned char zero_cache[sizeof(secp256k1_musig_keyagg_cache)] = { 0 };
    secp256k1_pubkey pubkey;
    const secp256k1_pubkey *pubkeys[5];
    secp256k1_xonly_pubkey agg_pk;
    secp256k1_musig_keyagg_cache keyagg_cache;
    int i;

    seckey[31] = 1;
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    for (i = 0; i < 5; i++) {
        pubkeys[i] = &pubkey;
    }

    CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_pk, &keyagg_cache, pubkeys, 5) == 0);
    CHECK(secp256k1_memcmp_var(&agg_pk, zero_agg, sizeof(agg_pk)) == 0);
    CHECK(secp256k1_memcmp_var(&keyagg_cache, zero_cache, sizeof(keyagg_cache)) == 0);
}

#endif
