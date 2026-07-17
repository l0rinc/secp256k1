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

/* Keep an affine model independent from the projective formulas and scalar
 * recoding used by the production multiplication paths. */
static void secp256k1_fuzz_ecmult_const_reference_double(secp256k1_ge *result, const secp256k1_ge *point) {
    secp256k1_fe x = point->x;
    secp256k1_fe y = point->y;
    secp256k1_fe lambda;
    secp256k1_fe tmp;

    if (point->infinity || secp256k1_fe_normalizes_to_zero_var(&y)) {
        secp256k1_ge_set_infinity(result);
        return;
    }
    secp256k1_fe_normalize_var(&x);
    secp256k1_fe_normalize_var(&y);

    secp256k1_fe_sqr(&lambda, &x);
    secp256k1_fe_mul_int(&lambda, 3);
    tmp = y;
    secp256k1_fe_add(&tmp, &y);
    secp256k1_fe_inv_var(&tmp, &tmp);
    secp256k1_fe_mul(&lambda, &lambda, &tmp);

    secp256k1_fe_sqr(&result->x, &lambda);
    secp256k1_fe_negate(&tmp, &x, 1);
    secp256k1_fe_add(&result->x, &tmp);
    secp256k1_fe_add(&result->x, &tmp);
    secp256k1_fe_normalize_var(&result->x);

    secp256k1_fe_negate(&tmp, &result->x, 1);
    secp256k1_fe_add(&tmp, &x);
    secp256k1_fe_mul(&result->y, &lambda, &tmp);
    secp256k1_fe_negate(&tmp, &y, 1);
    secp256k1_fe_add(&result->y, &tmp);
    secp256k1_fe_normalize_var(&result->y);
    result->infinity = 0;
}

static void secp256k1_fuzz_ecmult_const_reference_add(secp256k1_ge *result, const secp256k1_ge *a, const secp256k1_ge *b) {
    secp256k1_fe ax = a->x;
    secp256k1_fe ay = a->y;
    secp256k1_fe bx = b->x;
    secp256k1_fe by = b->y;
    secp256k1_fe lambda;
    secp256k1_fe tmp;

    if (a->infinity) {
        *result = *b;
        return;
    }
    if (b->infinity) {
        *result = *a;
        return;
    }
    secp256k1_fe_normalize_var(&ax);
    secp256k1_fe_normalize_var(&ay);
    secp256k1_fe_normalize_var(&bx);
    secp256k1_fe_normalize_var(&by);

    if (secp256k1_fe_equal(&ax, &bx)) {
        if (secp256k1_fe_equal(&ay, &by)) {
            secp256k1_ge point = *a;
            point.x = ax;
            point.y = ay;
            secp256k1_fuzz_ecmult_const_reference_double(result, &point);
        } else {
            secp256k1_ge_set_infinity(result);
        }
        return;
    }

    secp256k1_fe_negate(&lambda, &ax, 1);
    secp256k1_fe_add(&lambda, &bx);
    secp256k1_fe_inv_var(&lambda, &lambda);
    secp256k1_fe_negate(&tmp, &ay, 1);
    secp256k1_fe_add(&tmp, &by);
    secp256k1_fe_mul(&lambda, &lambda, &tmp);

    secp256k1_fe_sqr(&result->x, &lambda);
    secp256k1_fe_negate(&tmp, &ax, 1);
    secp256k1_fe_add(&result->x, &tmp);
    secp256k1_fe_negate(&tmp, &bx, 1);
    secp256k1_fe_add(&result->x, &tmp);
    secp256k1_fe_normalize_var(&result->x);

    secp256k1_fe_negate(&tmp, &result->x, 1);
    secp256k1_fe_add(&tmp, &ax);
    secp256k1_fe_mul(&result->y, &lambda, &tmp);
    secp256k1_fe_negate(&tmp, &ay, 1);
    secp256k1_fe_add(&result->y, &tmp);
    secp256k1_fe_normalize_var(&result->y);
    result->infinity = 0;
}

static unsigned int secp256k1_fuzz_ecmult_const_reference_bit(const unsigned char *scalar32, unsigned int bit) {
    FUZZ_CHECK(bit < 256);
    return (scalar32[31 - (bit >> 3)] >> (bit & 7u)) & 1u;
}

static void secp256k1_fuzz_ecmult_const_reference(secp256k1_ge *result, const secp256k1_ge *base, const unsigned char *scalar32) {
    secp256k1_ge accumulator;
    int bit = 255;

    secp256k1_ge_set_infinity(&accumulator);
    while (bit > 0 && secp256k1_fuzz_ecmult_const_reference_bit(scalar32, (unsigned int)bit) == 0) {
        bit--;
    }
    for (; bit >= 0; bit--) {
        secp256k1_ge doubled;
        secp256k1_ge added;

        secp256k1_fuzz_ecmult_const_reference_double(&doubled, &accumulator);
        accumulator = doubled;
        if (secp256k1_fuzz_ecmult_const_reference_bit(scalar32, (unsigned int)bit) != 0) {
            secp256k1_fuzz_ecmult_const_reference_add(&added, &accumulator, base);
            accumulator = added;
        }
    }
    *result = accumulator;
}

static void secp256k1_fuzz_ecmult_const_check_null_generator(const secp256k1_gej *base, const secp256k1_scalar *scalar) {
    secp256k1_gej null_generator;
    secp256k1_gej zero_generator;

    /* The NULL generator term is documented as exactly equivalent to zero. */
    secp256k1_ecmult(&null_generator, base, scalar, NULL);
    secp256k1_ecmult(&zero_generator, base, scalar, &secp256k1_scalar_zero);
    FUZZ_CHECK(secp256k1_gej_eq_var(&null_generator, &zero_generator));
}

static void secp256k1_fuzz_ecmult_const_check_canonical_infinity_result(const secp256k1_gej *result) {
    secp256k1_fe zero_fe;

    secp256k1_fe_set_int(&zero_fe, 0);

    FUZZ_CHECK(result->infinity == 1);
    FUZZ_CHECK(memcmp(&result->x, &zero_fe, sizeof(zero_fe)) == 0);
    FUZZ_CHECK(memcmp(&result->y, &zero_fe, sizeof(zero_fe)) == 0);
    FUZZ_CHECK(memcmp(&result->z, &zero_fe, sizeof(zero_fe)) == 0);
}

static void secp256k1_fuzz_ecmult_const_check_canonical_infinity(const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "ecmult const canonical infinity\n";
    secp256k1_ge infinity;
    secp256k1_gej result;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    secp256k1_ge_set_infinity(&infinity);

    memset(&result, 0xA5, sizeof(result));
    secp256k1_ecmult_const(&result, &infinity, &secp256k1_scalar_one);
    secp256k1_fuzz_ecmult_const_check_canonical_infinity_result(&result);
}

static void secp256k1_fuzz_ecmult_const_check_generator(const secp256k1_context *ctx, const secp256k1_scalar *scalar) {
    secp256k1_gej generated;
    secp256k1_ge generated_affine;
    secp256k1_ge expected_affine;
    secp256k1_gej generic;
    secp256k1_gej constant_time;
    secp256k1_gej generatorj;
    secp256k1_gej generated_copy;

    secp256k1_gej_set_ge(&generatorj, &secp256k1_ge_const_g);
    secp256k1_fuzz_ecmult_const_check_null_generator(&generatorj, scalar);
    secp256k1_ecmult_gen_gej(&ctx->ecmult_gen_ctx, &generated, scalar);
    secp256k1_ecmult_gen_ge(&ctx->ecmult_gen_ctx, &generated_affine, scalar);
    generated_copy = generated;
    secp256k1_ge_set_gej(&expected_affine, &generated_copy);
    secp256k1_gej_clear(&generated_copy);
    FUZZ_CHECK(secp256k1_ge_eq_var(&generated_affine, &expected_affine));
    secp256k1_ecmult(&generic, &generatorj, scalar, NULL);
    secp256k1_ecmult_const(&constant_time, &secp256k1_ge_const_g, scalar);
    FUZZ_CHECK(secp256k1_gej_eq_var(&generated, &generic));
    FUZZ_CHECK(secp256k1_gej_eq_var(&generated, &constant_time));
    FUZZ_CHECK(secp256k1_gej_eq_var(&generic, &constant_time));
    secp256k1_gej_clear(&generated);
}

static void secp256k1_fuzz_ecmult_const_check_xonly(const secp256k1_ge *base, const secp256k1_gej *expected, const secp256k1_scalar *scalar, const unsigned char *input, size_t size) {
    secp256k1_ge expected_affine;
    secp256k1_ge reference;
    secp256k1_fe result;
    secp256k1_fe result_known;
    secp256k1_fe numerator;
    secp256k1_fe denominator;
    secp256k1_fe invalid_x;
    secp256k1_fe reference_x;
    unsigned char scalar32[32];
    unsigned char denominator32[32];

    {
        secp256k1_gej copy = *expected;
        secp256k1_ge_set_gej_var(&expected_affine, &copy);
    }
    FUZZ_CHECK(!secp256k1_ge_is_infinity(&expected_affine));
    secp256k1_fuzz_derive(denominator32, sizeof(denominator32), input, size, 131);
    secp256k1_fe_set_b32_mod(&denominator, denominator32);
    secp256k1_fe_normalize_var(&denominator);
    if (secp256k1_fe_is_zero(&denominator)) {
        secp256k1_fe_set_int(&denominator, 1);
    }
    secp256k1_fe_mul(&numerator, &base->x, &denominator);

    /* Exercise both x-only input forms with an independent affine result for
     * the full canonical scalar. */
    secp256k1_scalar_get_b32(scalar32, scalar);
    secp256k1_fuzz_ecmult_const_reference(&reference, base, scalar32);
    FUZZ_CHECK(!reference.infinity);
    reference_x = reference.x;
    secp256k1_fe_normalize_var(&reference_x);
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result, &base->x, NULL, scalar, 0) == 1);
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result_known, &base->x, NULL, scalar, 1) == 1);
    secp256k1_fe_normalize_var(&result);
    secp256k1_fe_normalize_var(&result_known);
    FUZZ_CHECK(secp256k1_fe_equal(&result, &reference_x));
    FUZZ_CHECK(secp256k1_fe_equal(&result_known, &reference_x));
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result, &numerator, &denominator, scalar, 0) == 1);
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result_known, &numerator, &denominator, scalar, 1) == 1);
    secp256k1_fe_normalize_var(&result);
    secp256k1_fe_normalize_var(&result_known);
    FUZZ_CHECK(secp256k1_fe_equal(&result, &reference_x));
    FUZZ_CHECK(secp256k1_fe_equal(&result_known, &reference_x));

    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result, &base->x, NULL, scalar, 0) == 1);
    FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result_known, &base->x, NULL, scalar, 1) == 1);
    secp256k1_fe_normalize_var(&result);
    secp256k1_fe_normalize_var(&result_known);
    FUZZ_CHECK(secp256k1_fe_equal(&result, &expected_affine.x));
    FUZZ_CHECK(secp256k1_fe_equal(&result_known, &expected_affine.x));

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

static void secp256k1_fuzz_ecmult_const_check_nonnormalized_fraction(const secp256k1_ge *base, const secp256k1_gej *expected, const secp256k1_scalar *scalar) {
    secp256k1_fe zero7;
    secp256k1_fe base_x;
    secp256k1_fe one;
    secp256k1_fe numerator;
    secp256k1_fe denominator;
    secp256k1_fe result;
    secp256k1_ge expected_affine;
    unsigned int mask;

    {
        secp256k1_gej copy = *expected;
        secp256k1_ge_set_gej_var(&expected_affine, &copy);
    }
    FUZZ_CHECK(!secp256k1_ge_is_infinity(&expected_affine));

    secp256k1_fe_set_int(&zero7, 0);
    secp256k1_fe_negate(&zero7, &zero7, 0);
    secp256k1_fe_mul_int_unchecked(&zero7, 7);
    base_x = base->x;
    secp256k1_fe_normalize_var(&base_x);
    secp256k1_fe_set_int(&one, 1);

    for (mask = 1; mask <= 3; mask++) {
        numerator = base_x;
        denominator = one;
        if ((mask & 1u) != 0) {
            secp256k1_fe_add(&numerator, &zero7);
        }
        if ((mask & 2u) != 0) {
            secp256k1_fe_add(&denominator, &zero7);
        }
#ifdef VERIFY
        if ((mask & 1u) != 0) {
            FUZZ_CHECK(numerator.magnitude == 8);
            FUZZ_CHECK(numerator.normalized == 0);
        }
        if ((mask & 2u) != 0) {
            FUZZ_CHECK(denominator.magnitude == 8);
            FUZZ_CHECK(denominator.normalized == 0);
        }
#endif
        FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result, &numerator, &denominator, scalar, 0) == 1);
        secp256k1_fe_normalize_var(&result);
        FUZZ_CHECK(secp256k1_fe_equal(&result, &expected_affine.x));

        FUZZ_CHECK(secp256k1_ecmult_const_xonly(&result, &numerator, &denominator, scalar, 1) == 1);
        secp256k1_fe_normalize_var(&result);
        FUZZ_CHECK(secp256k1_fe_equal(&result, &expected_affine.x));
    }
}

static void secp256k1_fuzz_ecmult_const_check_odd_multiples_table(const secp256k1_gej *input) {
    enum { N = 4 };
    secp256k1_ge pre[N];
    secp256k1_fe zr[N];
    secp256k1_fe omitted_z[N];
    secp256k1_fe check_z;
    secp256k1_gej expected;
    secp256k1_gej step;
    size_t i;

    FUZZ_CHECK(!secp256k1_gej_is_infinity(input));
    secp256k1_ecmult_odd_multiples_table(N, pre, zr, &omitted_z[N - 1], input);

    /* Recover the omitted projective coordinates from the documented ratios. */
    for (i = N - 1; i > 0; i--) {
        secp256k1_fe inverse_ratio;

        FUZZ_CHECK(!secp256k1_fe_normalizes_to_zero_var(&zr[i]));
        secp256k1_fe_inv_var(&inverse_ratio, &zr[i]);
        secp256k1_fe_mul(&omitted_z[i - 1], &omitted_z[i], &inverse_ratio);
        secp256k1_fe_mul(&check_z, &omitted_z[i - 1], &zr[i]);
        secp256k1_fe_normalize_var(&check_z);
        secp256k1_fe_normalize_var(&omitted_z[i]);
        FUZZ_CHECK(secp256k1_fe_equal(&check_z, &omitted_z[i]));
    }
    secp256k1_fe_mul(&check_z, &input->z, &zr[0]);
    secp256k1_fe_normalize_var(&check_z);
    secp256k1_fe_normalize_var(&omitted_z[0]);
    FUZZ_CHECK(secp256k1_fe_equal(&check_z, &omitted_z[0]));

    /* Build the odd multiples independently, without the precomputation table. */
    expected = *input;
    secp256k1_gej_double_var(&step, input, NULL);
    for (i = 0; i < N; i++) {
        secp256k1_gej actual;
        secp256k1_gej actual_copy;
        secp256k1_gej expected_copy;
        secp256k1_ge expected_affine;
        secp256k1_ge actual_affine;
        unsigned char expected_bytes[64];
        unsigned char actual_bytes[64];

        actual.infinity = pre[i].infinity;
        actual.x = pre[i].x;
        actual.y = pre[i].y;
        actual.z = omitted_z[i];
        FUZZ_CHECK(!actual.infinity);

        actual_copy = actual;
        secp256k1_ge_set_gej_var(&actual_affine, &actual_copy);
        expected_copy = expected;
        secp256k1_ge_set_gej_var(&expected_affine, &expected_copy);
        secp256k1_ge_to_bytes_ext(actual_bytes, &actual_affine);
        secp256k1_ge_to_bytes_ext(expected_bytes, &expected_affine);
        FUZZ_CHECK(memcmp(actual_bytes, expected_bytes, sizeof(actual_bytes)) == 0);

        if (i + 1 < N) {
            secp256k1_gej_add_var(&expected, &expected, &step, NULL);
        }
    }
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 97);
    secp256k1_scalar base_scalar;
    secp256k1_scalar scalar;
    secp256k1_scalar product;
    secp256k1_gej basej;
    secp256k1_gej expected;
    secp256k1_gej generic;
    secp256k1_gej result;
    secp256k1_ge base;
    secp256k1_ge infinity;

    secp256k1_fuzz_ecmult_const_scalar(&base_scalar, input, size, 101, 1);
    secp256k1_fuzz_ecmult_const_scalar(&scalar, input, size, 107, 1);
    secp256k1_fuzz_ecmult_const_check_generator(ctx, &secp256k1_scalar_one);
    secp256k1_fuzz_ecmult_const_check_generator(ctx, &base_scalar);
    secp256k1_fuzz_ecmult_const_check_generator(ctx, &scalar);
    secp256k1_ecmult_gen_gej(&ctx->ecmult_gen_ctx, &basej, &base_scalar);
    {
        secp256k1_gej copy = basej;
        secp256k1_ge_set_gej_var(&base, &copy);
    }
    FUZZ_CHECK(!secp256k1_ge_is_infinity(&base));
    secp256k1_fuzz_ecmult_const_check_null_generator(&basej, &scalar);
    secp256k1_scalar_mul(&product, &base_scalar, &scalar);
    secp256k1_ecmult_gen_gej(&ctx->ecmult_gen_ctx, &expected, &product);
    secp256k1_ecmult(&generic, &basej, &scalar, NULL);
    secp256k1_ecmult_const(&result, &base, &scalar);
    FUZZ_CHECK(secp256k1_gej_eq_var(&result, &expected));
    FUZZ_CHECK(secp256k1_gej_eq_var(&result, &generic));
    FUZZ_CHECK(secp256k1_gej_eq_var(&generic, &expected));
    secp256k1_fuzz_ecmult_const_check_odd_multiples_table(&basej);
    secp256k1_fuzz_ecmult_const_check_xonly(&base, &expected, &scalar, input, size);
    secp256k1_fuzz_ecmult_const_check_nonnormalized_fraction(&base, &expected, &scalar);

    secp256k1_ecmult_const(&result, &base, &secp256k1_scalar_zero);
    FUZZ_CHECK(secp256k1_gej_is_infinity(&result));
    secp256k1_ge_set_infinity(&infinity);
    secp256k1_fuzz_ecmult_const_check_null_generator(&basej, &secp256k1_scalar_zero);
    secp256k1_gej_set_infinity(&basej);
    secp256k1_fuzz_ecmult_const_check_null_generator(&basej, &scalar);
    secp256k1_fuzz_ecmult_const_check_null_generator(&basej, &secp256k1_scalar_zero);
    secp256k1_ecmult_const(&result, &infinity, &scalar);
    FUZZ_CHECK(secp256k1_gej_is_infinity(&result));
    secp256k1_fuzz_ecmult_const_check_canonical_infinity(input, size);

    secp256k1_scalar_clear(&base_scalar);
    secp256k1_scalar_clear(&scalar);
    secp256k1_scalar_clear(&product);
    secp256k1_context_destroy(ctx);
    return 0;
}
