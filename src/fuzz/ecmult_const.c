/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../secp256k1.c"
#include "fuzz.h"

static void secp256k1_fuzz_ecmult_const_scalar(secp256k1_scalar *scalar, const unsigned char *input, size_t size, unsigned int salt, int nonzero) {
    unsigned char scalar32[32];

    secp256k1_fuzz_scalar32(scalar32, input, size, salt);
    secp256k1_scalar_set_b32(scalar, scalar32, NULL);
    if (nonzero && secp256k1_scalar_is_zero(scalar)) {
        secp256k1_scalar_set_int(scalar, 1);
    }
}

static void secp256k1_fuzz_ecmult_const_check_xonly(const secp256k1_ge *base, const secp256k1_gej *expected, const secp256k1_scalar *scalar, const unsigned char *input, size_t size) {
    secp256k1_ge expected_affine;
    secp256k1_fe result;
    secp256k1_fe result_known;
    secp256k1_fe numerator;
    secp256k1_fe denominator;
    secp256k1_fe invalid_x;
    unsigned char denominator32[32];

    {
        secp256k1_gej copy = *expected;
        secp256k1_ge_set_gej_var(&expected_affine, &copy);
    }
    FUZZ_CHECK(!secp256k1_ge_is_infinity(&expected_affine));
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result, &base->x, NULL, scalar, 0) == 1);
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result_known, &base->x, NULL, scalar, 1) == 1);
    secp256k1_fe_normalize_var(&result);
    secp256k1_fe_normalize_var(&result_known);
    FUZZ_CHECK(secp256k1_fe_equal(&result, &expected_affine.x));
    FUZZ_CHECK(secp256k1_fe_equal(&result_known, &expected_affine.x));

    secp256k1_fuzz_derive(denominator32, sizeof(denominator32), input, size, 131);
    secp256k1_fe_set_b32_mod(&denominator, denominator32);
    secp256k1_fe_normalize_var(&denominator);
    if (secp256k1_fe_is_zero(&denominator)) {
        secp256k1_fe_set_int(&denominator, 1);
    }
    secp256k1_fe_mul(&numerator, &base->x, &denominator);
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result, &numerator, &denominator, scalar, 0) == 1);
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result_known, &numerator, &denominator, scalar, 1) == 1);
    secp256k1_fe_normalize_var(&result);
    secp256k1_fe_normalize_var(&result_known);
    FUZZ_CHECK(secp256k1_fe_equal(&result, &expected_affine.x));
    FUZZ_CHECK(secp256k1_fe_equal(&result_known, &expected_affine.x));

    /* x = 0 is deterministically off-curve for secp256k1 (7 is nonsquare). */
    secp256k1_fe_set_int(&invalid_x, 0);
    FUZZ_CHECK(!secp256k1_ge_x_on_curve_var(&invalid_x));
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result, &invalid_x, NULL, scalar, 0) == 0);
    secp256k1_fe_mul(&numerator, &invalid_x, &denominator);
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result, &numerator, &denominator, scalar, 0) == 0);
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 97);
    secp256k1_scalar base_scalar;
    secp256k1_scalar scalar;
    secp256k1_scalar product;
    secp256k1_gej basej;
    secp256k1_gej expected;
    secp256k1_gej result;
    secp256k1_ge base;
    secp256k1_ge infinity;

    secp256k1_fuzz_ecmult_const_scalar(&base_scalar, input, size, 101, 1);
    secp256k1_fuzz_ecmult_const_scalar(&scalar, input, size, 107, 1);
    secp256k1_ecmult_gen_gej(&ctx->ecmult_gen_ctx, &basej, &base_scalar);
    {
        secp256k1_gej copy = basej;
        secp256k1_ge_set_gej_var(&base, &copy);
    }
    FUZZ_CHECK(!secp256k1_ge_is_infinity(&base));
    secp256k1_scalar_mul(&product, &base_scalar, &scalar);
    secp256k1_ecmult_gen_gej(&ctx->ecmult_gen_ctx, &expected, &product);
    secp256k1_ecmult_const(&result, &base, &scalar);
    FUZZ_CHECK(secp256k1_gej_eq_var(&result, &expected));
    secp256k1_fuzz_ecmult_const_check_xonly(&base, &expected, &scalar, input, size);

    secp256k1_ecmult_const(&result, &base, &secp256k1_scalar_zero);
    FUZZ_CHECK(secp256k1_gej_is_infinity(&result));
    secp256k1_ge_set_infinity(&infinity);
    secp256k1_ecmult_const(&result, &infinity, &scalar);
    FUZZ_CHECK(secp256k1_gej_is_infinity(&result));

    secp256k1_scalar_clear(&base_scalar);
    secp256k1_scalar_clear(&scalar);
    secp256k1_scalar_clear(&product);
    secp256k1_context_destroy(ctx);
    return 0;
}
