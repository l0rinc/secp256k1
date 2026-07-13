/***********************************************************************
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_MUSIG_TESTS_EXHAUSTIVE_H
#define SECP256K1_MODULE_MUSIG_TESTS_EXHAUSTIVE_H

#include <string.h>

#include "../../../include/secp256k1_musig.h"
#include "main_impl.h"

/* All copies have the same key aggregation coefficient. Repeating G once per
 * group element makes the weighted aggregate infinity whether that coefficient
 * is zero or nonzero. */
static void test_exhaustive_musig(const secp256k1_context *ctx) {
#if EXHAUSTIVE_TEST_ORDER == 7
    static const uint64_t zero_nonce_counter = 5;
#elif EXHAUSTIVE_TEST_ORDER == 13
    static const uint64_t zero_nonce_counter = 2;
#elif EXHAUSTIVE_TEST_ORDER == 199
    static const uint64_t zero_nonce_counter = 70;
#else
#  error "Unsupported exhaustive test order"
#endif
    unsigned char seckey[32] = { 0 };
    unsigned char zero_agg[sizeof(secp256k1_xonly_pubkey)] = { 0 };
    unsigned char zero_cache[sizeof(secp256k1_musig_keyagg_cache)] = { 0 };
    unsigned char zero_nonce[sizeof(secp256k1_musig_secnonce)] = { 0 };
    secp256k1_pubkey pubkey;
    secp256k1_keypair keypair;
    const secp256k1_pubkey *pubkeys[EXHAUSTIVE_TEST_ORDER];
    secp256k1_xonly_pubkey agg_pk;
    secp256k1_musig_keyagg_cache keyagg_cache;
    secp256k1_musig_secnonce secnonce;
    secp256k1_musig_pubnonce pubnonce;
    int i;

    seckey[31] = 1;
    CHECK(secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) == 1);
    CHECK(secp256k1_keypair_create(ctx, &keypair, seckey) == 1);
    for (i = 0; i < EXHAUSTIVE_TEST_ORDER; i++) {
        pubkeys[i] = &pubkey;
    }

    CHECK(secp256k1_musig_pubkey_agg(ctx, &agg_pk, &keyagg_cache, pubkeys, EXHAUSTIVE_TEST_ORDER) == 0);
    CHECK(secp256k1_memcmp_var(&agg_pk, zero_agg, sizeof(agg_pk)) == 0);
    CHECK(secp256k1_memcmp_var(&keyagg_cache, zero_cache, sizeof(keyagg_cache)) == 0);

    /* The fixed reduced-order nonce transcript has a zero scalar here. */
    CHECK(secp256k1_musig_nonce_gen_counter(ctx, &secnonce, &pubnonce, zero_nonce_counter, &keypair, NULL, NULL, NULL) == 0);
    CHECK(secp256k1_memcmp_var(&secnonce, zero_nonce, sizeof(secnonce)) == 0);
    CHECK(secp256k1_memcmp_var(&pubnonce, zero_nonce, sizeof(pubnonce)) == 0);
}

#endif
