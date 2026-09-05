/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECKEY_H
#define SECP256K1_ECKEY_H

#include <stddef.h>

#include "group.h"
#include "scalar.h"
#include "ecmult_gen.h"

static int secp256k1_eckey_seckey_tweak_add(secp256k1_scalar *key, const secp256k1_scalar *tweak);
/** Set key to key + tweak*G, in constant time with respect to tweak. key must not be infinity.
 *  Returns 0 (and sets key to infinity) if the result is infinity. ecmult_gen_ctx may be unbuilt
 *  (e.g., that of secp256k1_context_static), in which case the multiplication is not blinded. */
static int secp256k1_eckey_pubkey_tweak_add(const secp256k1_ecmult_gen_context *ecmult_gen_ctx, secp256k1_ge *key, const secp256k1_scalar *tweak);
static int secp256k1_eckey_seckey_tweak_mul(secp256k1_scalar *key, const secp256k1_scalar *tweak);
/** Set key to tweak*key, in constant time with respect to tweak. key must not be infinity.
 *  Returns 0 (and sets key to infinity) if tweak is zero. */
static int secp256k1_eckey_pubkey_tweak_mul(secp256k1_ge *key, const secp256k1_scalar *tweak);

#endif /* SECP256K1_ECKEY_H */
