/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../secp256k1.c"
#include "fuzz.h"

static int secp256k1_fuzz_group_all_zero(void *ptr, size_t len) {
    const unsigned char *bytes = (const unsigned char *)ptr;
    size_t i;

    for (i = 0; i < len; i++) {
        if (bytes[i] != 0) {
            return 0;
        }
    }
    return 1;
}

static void secp256k1_fuzz_group_check_ge_clear(void) {
    secp256k1_ge value = secp256k1_ge_const_g;

    secp256k1_ge_clear(&value);
    SECP256K1_CHECKMEM_DEFINE(&value, sizeof(value));
    FUZZ_CHECK(secp256k1_fuzz_group_all_zero(&value, sizeof(value)));
}

/* Keep result checks independent from gej_eq_var. That helper implements
 * equality by adding points, so using it as the only oracle can mask a
 * regression shared by the addition and equality paths. */
static int secp256k1_fuzz_group_gej_equal_independent(const secp256k1_gej *a, const secp256k1_gej *b) {
    secp256k1_ge affine_a;
    secp256k1_ge affine_b;
    secp256k1_gej copy_a = *a;
    secp256k1_gej copy_b = *b;
    unsigned char bytes_a[64];
    unsigned char bytes_b[64];

    secp256k1_ge_set_gej_var(&affine_a, &copy_a);
    secp256k1_ge_set_gej_var(&affine_b, &copy_b);
    secp256k1_ge_to_bytes_ext(bytes_a, &affine_a);
    secp256k1_ge_to_bytes_ext(bytes_b, &affine_b);
    return memcmp(bytes_a, bytes_b, sizeof(bytes_a)) == 0;
}

static void secp256k1_fuzz_group_check_gej_equal(const secp256k1_gej *a, const secp256k1_gej *b) {
    int independent_equal = secp256k1_fuzz_group_gej_equal_independent(a, b);

    FUZZ_CHECK(independent_equal);
    FUZZ_CHECK(secp256k1_gej_eq_var(a, b));
}

static void secp256k1_fuzz_group_check_gej_not_equal(const secp256k1_gej *a) {
    secp256k1_gej negated;

    if (secp256k1_gej_is_infinity(a)) {
        return;
    }
    secp256k1_gej_neg(&negated, a);
    FUZZ_CHECK(!secp256k1_fuzz_group_gej_equal_independent(a, &negated));
    FUZZ_CHECK(!secp256k1_gej_eq_var(a, &negated));
}

static void secp256k1_fuzz_group_check_gej_coordinates(const secp256k1_gej *a, const secp256k1_gej *b) {
    secp256k1_gej normalized_a = *a;
    secp256k1_gej normalized_b = *b;

    FUZZ_CHECK(secp256k1_gej_is_infinity(&normalized_a) == secp256k1_gej_is_infinity(&normalized_b));
    secp256k1_fe_normalize_var(&normalized_a.x);
    secp256k1_fe_normalize_var(&normalized_a.y);
    secp256k1_fe_normalize_var(&normalized_a.z);
    secp256k1_fe_normalize_var(&normalized_b.x);
    secp256k1_fe_normalize_var(&normalized_b.y);
    secp256k1_fe_normalize_var(&normalized_b.z);
    FUZZ_CHECK(secp256k1_fe_equal(&normalized_a.x, &normalized_b.x));
    FUZZ_CHECK(secp256k1_fe_equal(&normalized_a.y, &normalized_b.y));
    FUZZ_CHECK(secp256k1_fe_equal(&normalized_a.z, &normalized_b.z));
}

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_group_illegal_data;

static void secp256k1_fuzz_group_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_group_illegal_data *illegal_data = (secp256k1_fuzz_group_illegal_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(illegal_data != NULL);
    FUZZ_CHECK(illegal_data->self == illegal_data);
    illegal_data->calls++;
}

static void secp256k1_fuzz_group_check_opaque_pubkey_barrier(secp256k1_context *ctx) {
    secp256k1_fuzz_group_illegal_data illegal_data;
    secp256k1_ge invalid_ge;
    secp256k1_ge loaded_ge;
    secp256k1_pubkey invalid_pubkey;
    secp256k1_pubkey mutated_pubkey;
    secp256k1_pubkey combined_pubkey;
    const secp256k1_pubkey *inputs[1];
    unsigned char serialized[33];
    unsigned char tweak32[32] = { 0 };
    unsigned char nonzero_tweak32[32] = { 0 };
    unsigned char zero_pubkey[sizeof(secp256k1_pubkey)] = { 0 };
    size_t serialized_len;
    unsigned int calls;

    /* Build a structurally valid storage object for a point that is not on the curve. */
    secp256k1_fe_set_int(&invalid_ge.x, 1);
    secp256k1_fe_set_int(&invalid_ge.y, 1);
    invalid_ge.infinity = 0;
    FUZZ_CHECK(!secp256k1_ge_is_valid_var(&invalid_ge));
    secp256k1_ge_to_bytes(invalid_pubkey.data, &invalid_ge);
    nonzero_tweak32[31] = 1;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_group_illegal_callback, &illegal_data);

    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_pubkey_load(ctx, &loaded_ge, &invalid_pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    memset(serialized, 0xA5, sizeof(serialized));
    serialized_len = sizeof(serialized);
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, serialized, &serialized_len, &invalid_pubkey, SECP256K1_EC_COMPRESSED) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(serialized_len == 0);
    FUZZ_CHECK(secp256k1_fuzz_group_all_zero(serialized, sizeof(serialized)));

    inputs[0] = &invalid_pubkey;
    memset(&combined_pubkey, 0xA5, sizeof(combined_pubkey));
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined_pubkey, inputs, 1) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&combined_pubkey, zero_pubkey, sizeof(combined_pubkey)) == 0);

    mutated_pubkey = invalid_pubkey;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_add(ctx, &mutated_pubkey, tweak32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&mutated_pubkey, zero_pubkey, sizeof(mutated_pubkey)) == 0);

    mutated_pubkey = invalid_pubkey;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_negate(ctx, &mutated_pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(&mutated_pubkey, zero_pubkey, sizeof(mutated_pubkey)) == 0);

    mutated_pubkey = invalid_pubkey;
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &mutated_pubkey, nonzero_tweak32) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);
    calls = illegal_data.calls;
    FUZZ_CHECK(secp256k1_pubkey_load(ctx, &loaded_ge, &mutated_pubkey) == 0);
    FUZZ_CHECK(illegal_data.calls == calls + 1);

    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_group_scalar(secp256k1_scalar *scalar, unsigned char *scalar32, const unsigned char *input, size_t size, unsigned int salt) {
    secp256k1_fuzz_scalar32(scalar32, input, size, salt);
    secp256k1_scalar_set_b32(scalar, scalar32, NULL);
    secp256k1_scalar_get_b32(scalar32, scalar);
}

static void secp256k1_fuzz_group_make_point(const secp256k1_context *ctx, secp256k1_gej *point, const secp256k1_scalar *scalar) {
    secp256k1_ecmult_gen_gej(&ctx->ecmult_gen_ctx, point, scalar);
    SECP256K1_GEJ_VERIFY(point);
}

static void secp256k1_fuzz_group_check_independent_generator(const secp256k1_gej *actual, const secp256k1_scalar *scalar) {
    secp256k1_gej expected;

    secp256k1_ecmult_const(&expected, &secp256k1_ge_const_g, scalar);
    secp256k1_fuzz_group_check_gej_equal(actual, &expected);
}

static void secp256k1_fuzz_group_check_affine(const secp256k1_gej *point) {
    secp256k1_ge constant_time;
    secp256k1_ge variable_time;

    {
        secp256k1_gej copy = *point;
        secp256k1_ge_set_gej(&constant_time, &copy);
    }
    {
        secp256k1_gej copy = *point;
        secp256k1_ge_set_gej_var(&variable_time, &copy);
    }
    FUZZ_CHECK(secp256k1_ge_eq_var(&constant_time, &variable_time));
    FUZZ_CHECK(secp256k1_gej_eq_ge_var(point, &constant_time));
    if (!secp256k1_ge_is_infinity(&constant_time)) {
        FUZZ_CHECK(secp256k1_ge_is_valid_var(&constant_time));
        FUZZ_CHECK(secp256k1_ge_x_on_curve_var(&constant_time.x));
    }
}

static void secp256k1_fuzz_group_check_infinity_invalid(void) {
    secp256k1_ge infinity;

    secp256k1_ge_set_infinity(&infinity);
    FUZZ_CHECK(secp256k1_ge_is_infinity(&infinity));
    FUZZ_CHECK(!secp256k1_ge_is_valid_var(&infinity));
}

static void secp256k1_fuzz_group_check_canonical_infinity(void) {
    secp256k1_ge infinity;
    secp256k1_fe zero;

    /* The constructor promises a complete affine infinity representation. */
    memset(&infinity, 0xA5, sizeof(infinity));
    secp256k1_ge_set_infinity(&infinity);
    secp256k1_fe_set_int(&zero, 0);
    FUZZ_CHECK(secp256k1_ge_is_infinity(&infinity));
    FUZZ_CHECK(memcmp(&infinity.x, &zero, sizeof(zero)) == 0);
    FUZZ_CHECK(memcmp(&infinity.y, &zero, sizeof(zero)) == 0);
}

static void secp256k1_fuzz_group_check_ge_eq_infinity(void) {
    secp256k1_ge infinity;
    secp256k1_ge generator = secp256k1_ge_const_g;

    secp256k1_ge_set_infinity(&infinity);
    FUZZ_CHECK(secp256k1_ge_eq_var(&infinity, &infinity));
    FUZZ_CHECK(!secp256k1_ge_eq_var(&infinity, &generator));
    FUZZ_CHECK(!secp256k1_ge_eq_var(&generator, &infinity));
}

/* The existing equality checks exercise only the true branch. Keep a fixed,
 * independently distinguishable pair to guard against false positives. */
static void secp256k1_fuzz_group_check_gej_eq_ge_negative(const secp256k1_gej *point) {
    secp256k1_ge affine;
    secp256k1_ge doubled_affine;
    secp256k1_ge infinity;
    secp256k1_gej point_copy = *point;
    secp256k1_gej doubled = *point;
    secp256k1_gej infinity_j;
    unsigned char affine_bytes[64];
    unsigned char doubled_bytes[64];

    FUZZ_CHECK(!secp256k1_gej_is_infinity(point));
    secp256k1_ge_set_gej_var(&affine, &point_copy);
    secp256k1_gej_double(&doubled, &doubled);
    secp256k1_ge_set_gej_var(&doubled_affine, &doubled);
    secp256k1_ge_to_bytes_ext(affine_bytes, &affine);
    secp256k1_ge_to_bytes_ext(doubled_bytes, &doubled_affine);
    FUZZ_CHECK(memcmp(affine_bytes, doubled_bytes, sizeof(affine_bytes)) != 0);
    FUZZ_CHECK(!secp256k1_gej_eq_ge_var(point, &doubled_affine));

    secp256k1_ge_set_infinity(&infinity);
    secp256k1_gej_set_infinity(&infinity_j);
    FUZZ_CHECK(!secp256k1_gej_eq_ge_var(point, &infinity));
    FUZZ_CHECK(!secp256k1_gej_eq_ge_var(&infinity_j, &affine));
    FUZZ_CHECK(secp256k1_gej_eq_ge_var(&infinity_j, &infinity));
}

static void secp256k1_fuzz_group_check_z_ratio(const secp256k1_gej *a, const secp256k1_gej *result, const secp256k1_fe *rzr) {
    secp256k1_fe expected_z = result->z;
    secp256k1_fe actual_z;

    secp256k1_fe_mul(&actual_z, &a->z, rzr);
    secp256k1_fe_normalize_var(&actual_z);
    secp256k1_fe_normalize_var(&expected_z);
    FUZZ_CHECK(secp256k1_fe_equal(&actual_z, &expected_z));
}

static void secp256k1_fuzz_group_check_addition(const secp256k1_gej *a, const secp256k1_gej *b, const secp256k1_gej *expected) {
    secp256k1_gej result;
    secp256k1_ge b_affine;
    secp256k1_fe rzr;

    secp256k1_gej_add_var(&result, a, b, a->infinity ? NULL : &rzr);
    secp256k1_fuzz_group_check_gej_equal(&result, expected);
    if (!a->infinity) {
        secp256k1_fuzz_group_check_z_ratio(a, &result, &rzr);
    }
    result = *a;
    secp256k1_gej_add_var(&result, &result, b, NULL);
    secp256k1_fuzz_group_check_gej_equal(&result, expected);

    {
        secp256k1_gej copy = *b;
        secp256k1_ge_set_gej_var(&b_affine, &copy);
    }
    secp256k1_gej_add_ge_var(&result, a, &b_affine, a->infinity ? NULL : &rzr);
    secp256k1_fuzz_group_check_gej_equal(&result, expected);
    if (!a->infinity) {
        secp256k1_fuzz_group_check_z_ratio(a, &result, &rzr);
    }
    result = *a;
    secp256k1_gej_add_ge_var(&result, &result, &b_affine, NULL);
    secp256k1_fuzz_group_check_gej_equal(&result, expected);
    if (!secp256k1_ge_is_infinity(&b_affine)) {
        secp256k1_gej_add_ge(&result, a, &b_affine);
        secp256k1_fuzz_group_check_gej_equal(&result, expected);
        result = *a;
        secp256k1_gej_add_ge(&result, &result, &b_affine);
        secp256k1_fuzz_group_check_gej_equal(&result, expected);
    }
}

static void secp256k1_fuzz_group_check_lambda_degenerate_addition(const secp256k1_gej *point) {
    secp256k1_ge affine;
    secp256k1_ge lambda_negated;
    secp256k1_gej lambda_negated_j;
    secp256k1_gej expected;
    secp256k1_gej copy = *point;

    FUZZ_CHECK(!secp256k1_gej_is_infinity(point));
    secp256k1_ge_set_gej_var(&affine, &copy);
    lambda_negated = affine;
    secp256k1_ge_mul_lambda(&lambda_negated, &lambda_negated);
    secp256k1_ge_neg(&lambda_negated, &lambda_negated);
    FUZZ_CHECK(!secp256k1_ge_eq_var(&affine, &lambda_negated));

    secp256k1_gej_set_ge(&lambda_negated_j, &lambda_negated);
    secp256k1_gej_add_var(&expected, point, &lambda_negated_j, NULL);
    secp256k1_fuzz_group_check_addition(point, &lambda_negated_j, &expected);
}

static void secp256k1_fuzz_group_check_double(const secp256k1_gej *point, const secp256k1_gej *expected) {
    secp256k1_gej constant_time;
    secp256k1_gej variable_time;
    secp256k1_fe rzr;

    secp256k1_gej_double(&constant_time, point);
    secp256k1_gej_double_var(&variable_time, point, &rzr);
    secp256k1_fuzz_group_check_gej_equal(&constant_time, expected);
    secp256k1_fuzz_group_check_gej_equal(&variable_time, expected);
    secp256k1_fuzz_group_check_z_ratio(point, &variable_time, &rzr);
    constant_time = *point;
    secp256k1_gej_double(&constant_time, &constant_time);
    variable_time = *point;
    secp256k1_gej_double_var(&variable_time, &variable_time, NULL);
    secp256k1_fuzz_group_check_gej_equal(&constant_time, expected);
    secp256k1_fuzz_group_check_gej_equal(&variable_time, expected);
}

static void secp256k1_fuzz_group_check_batch(const secp256k1_gej *a, const secp256k1_gej *b, const secp256k1_gej *sum) {
    secp256k1_gej points[3];
    secp256k1_ge affine[3];
    size_t i;

    points[0] = *a;
    points[1] = *b;
    points[2] = *sum;
    secp256k1_ge_set_all_gej_var(affine, points, 3);
    for (i = 0; i < 3; i++) {
        FUZZ_CHECK(secp256k1_gej_eq_ge_var(&points[i], &affine[i]));
    }
}

static void secp256k1_fuzz_group_check_zinv_addition(const secp256k1_gej *a, const secp256k1_gej *b, const secp256k1_gej *expected, const secp256k1_fe *bzinv) {
    secp256k1_ge b_affine;
    secp256k1_ge b_scaled;
    secp256k1_fe bz2;
    secp256k1_fe bz3;
    secp256k1_gej result;
    secp256k1_gej copy = *b;
    int b_is_infinity;

    secp256k1_ge_set_gej_var(&b_affine, &copy);
    b_scaled = b_affine;
    b_is_infinity = secp256k1_ge_is_infinity(&b_affine);
    if (!b_is_infinity) {
        secp256k1_fe_inv_var(&bz3, bzinv);
        secp256k1_fe_sqr(&bz2, &bz3);
        secp256k1_fe_mul(&bz3, &bz3, &bz2);
        secp256k1_fe_mul(&b_scaled.x, &b_scaled.x, &bz2);
        secp256k1_fe_mul(&b_scaled.y, &b_scaled.y, &bz3);
    }
    secp256k1_gej_add_zinv_var(&result, a, &b_scaled, bzinv);
    secp256k1_fuzz_group_check_gej_equal(&result, expected);
}

/* The z-inverse path has a distinct inverse-point branch. Pin G + (-G) so
 * this helper cannot depend on random scalar cancellation to reach it. */
static void secp256k1_fuzz_group_check_zinv_inverse(void) {
    secp256k1_gej a;
    secp256k1_gej negated;
    secp256k1_ge negated_affine;
    secp256k1_gej result;
    secp256k1_fe bzinv;

    secp256k1_gej_set_ge(&a, &secp256k1_ge_const_g);
    secp256k1_gej_neg(&negated, &a);
    secp256k1_ge_set_gej_var(&negated_affine, &negated);
    secp256k1_fe_set_int(&bzinv, 1);
    memset(&result, 0xA5, sizeof(result));
    secp256k1_gej_add_zinv_var(&result, &a, &negated_affine, &bzinv);

    FUZZ_CHECK(secp256k1_gej_is_infinity(&result));
    FUZZ_CHECK(secp256k1_fe_is_zero(&result.x));
    FUZZ_CHECK(secp256k1_fe_is_zero(&result.y));
    FUZZ_CHECK(secp256k1_fe_is_zero(&result.z));
}

/* The ordinary inverse-point branches also promise a zero Z-ratio output when
 * callers request it. Exercise both Jacobian and affine-input variants. */
static void secp256k1_fuzz_group_check_inverse_rzr(void) {
    secp256k1_gej a;
    secp256k1_gej negated;
    secp256k1_ge negated_affine;
    secp256k1_gej result;
    secp256k1_fe rzr;

    secp256k1_gej_set_ge(&a, &secp256k1_ge_const_g);
    secp256k1_gej_neg(&negated, &a);
    secp256k1_ge_set_gej_var(&negated_affine, &negated);

    memset(&result, 0xA5, sizeof(result));
    memset(&rzr, 0xA5, sizeof(rzr));
    secp256k1_gej_add_var(&result, &a, &negated, &rzr);
    FUZZ_CHECK(secp256k1_gej_is_infinity(&result));
    FUZZ_CHECK(secp256k1_fe_is_zero(&result.x));
    FUZZ_CHECK(secp256k1_fe_is_zero(&result.y));
    FUZZ_CHECK(secp256k1_fe_is_zero(&result.z));
    FUZZ_CHECK(secp256k1_fe_is_zero(&rzr));

    memset(&result, 0xA5, sizeof(result));
    memset(&rzr, 0xA5, sizeof(rzr));
    secp256k1_gej_add_ge_var(&result, &a, &negated_affine, &rzr);
    FUZZ_CHECK(secp256k1_gej_is_infinity(&result));
    FUZZ_CHECK(secp256k1_fe_is_zero(&result.x));
    FUZZ_CHECK(secp256k1_fe_is_zero(&result.y));
    FUZZ_CHECK(secp256k1_fe_is_zero(&result.z));
    FUZZ_CHECK(secp256k1_fe_is_zero(&rzr));
}

static void secp256k1_fuzz_group_check_zinv_in_place(const secp256k1_gej *a, const secp256k1_ge *b) {
    secp256k1_ge b_scaled;
    secp256k1_gej expected;
    secp256k1_gej actual;
    secp256k1_fe bzinv;
    secp256k1_fe bz2;
    secp256k1_fe bz3;

    FUZZ_CHECK(!secp256k1_gej_is_infinity(a));
    FUZZ_CHECK(!secp256k1_ge_is_infinity(b));
    bzinv = a->y;
    b_scaled = *b;
    secp256k1_fe_inv_var(&bz3, &bzinv);
    secp256k1_fe_sqr(&bz2, &bz3);
    secp256k1_fe_mul(&b_scaled.x, &b_scaled.x, &bz2);
    secp256k1_fe_mul(&b_scaled.y, &b_scaled.y, &bz3);

    expected = *a;
    secp256k1_gej_add_zinv_var(&expected, a, &b_scaled, &bzinv);
    actual = *a;
    secp256k1_gej_add_zinv_var(&actual, &actual, &b_scaled, &actual.y);
    secp256k1_fuzz_group_check_gej_equal(&actual, &expected);
}

static void secp256k1_fuzz_group_check_rescale_alias(const secp256k1_gej *point) {
    secp256k1_gej expected;
    secp256k1_gej actual;
    secp256k1_fe scale;

    FUZZ_CHECK(!secp256k1_gej_is_infinity(point));

    expected = *point;
    actual = *point;
    scale = actual.z;
    secp256k1_gej_rescale(&expected, &scale);
    secp256k1_gej_rescale(&actual, &actual.z);
    secp256k1_fuzz_group_check_gej_equal(&actual, &expected);

    expected = *point;
    actual = *point;
    scale = actual.y;
    secp256k1_gej_rescale(&expected, &scale);
    secp256k1_gej_rescale(&actual, &actual.y);
    secp256k1_fuzz_group_check_gej_equal(&actual, &expected);

    expected = *point;
    actual = *point;
    scale = actual.x;
    secp256k1_gej_rescale(&expected, &scale);
    secp256k1_gej_rescale(&actual, &actual.x);
    secp256k1_fuzz_group_check_gej_equal(&actual, &expected);
}

static void secp256k1_fuzz_group_check_rescale_nonnormalized_scale(const secp256k1_gej *point) {
    secp256k1_fe zero_multiple;
    secp256k1_fe raised_scale;
    secp256k1_fe normalized_scale;
    secp256k1_gej expected;
    secp256k1_gej actual;

    FUZZ_CHECK(!secp256k1_gej_is_infinity(point));

    /* A nonzero scale may be nonnormalized as long as it remains in the
     * field-element precondition accepted by fe_sqr. */
    secp256k1_fe_set_int(&zero_multiple, 0);
    secp256k1_fe_negate(&zero_multiple, &zero_multiple, 0);
    secp256k1_fe_mul_int_unchecked(&zero_multiple, 7);
    secp256k1_fe_set_int(&raised_scale, 1);
    secp256k1_fe_add(&raised_scale, &zero_multiple);
#ifdef VERIFY
    FUZZ_CHECK(raised_scale.magnitude == 8);
    FUZZ_CHECK(raised_scale.normalized == 0);
#endif
    normalized_scale = raised_scale;
    secp256k1_fe_normalize_var(&normalized_scale);

    expected = *point;
    actual = *point;
    secp256k1_gej_rescale(&expected, &normalized_scale);
    secp256k1_gej_rescale(&actual, &raised_scale);
    secp256k1_fuzz_group_check_gej_equal(&actual, &expected);
    secp256k1_fuzz_group_check_gej_coordinates(&actual, &expected);
}

static void secp256k1_fuzz_group_check_affine_representations(const secp256k1_gej *point) {
    secp256k1_ge affine;
    secp256k1_ge lambda;
    secp256k1_ge lambda_twice;
    secp256k1_ge lambda_thrice;
    secp256k1_ge from_storage;
    secp256k1_ge from_bytes;
    secp256k1_ge from_bytes_ext;
    secp256k1_gej copy = *point;
    secp256k1_ge_storage storage;
    unsigned char bytes[64];
    unsigned char bytes_ext[64];

    secp256k1_ge_set_gej_var(&affine, &copy);
    secp256k1_ge_to_bytes_ext(bytes_ext, &affine);
    secp256k1_ge_from_bytes_ext(&from_bytes_ext, bytes_ext);
    FUZZ_CHECK(secp256k1_ge_eq_var(&from_bytes_ext, &affine));
    if (secp256k1_ge_is_infinity(&affine)) {
        FUZZ_CHECK(secp256k1_fuzz_group_all_zero(bytes_ext, sizeof(bytes_ext)));
        return;
    }

    secp256k1_ge_to_storage(&storage, &affine);
    secp256k1_ge_from_storage(&from_storage, &storage);
    FUZZ_CHECK(secp256k1_ge_eq_var(&from_storage, &affine));
    secp256k1_ge_to_bytes(bytes, &affine);
    secp256k1_ge_from_bytes(&from_bytes, bytes);
    FUZZ_CHECK(secp256k1_ge_eq_var(&from_bytes, &affine));
    FUZZ_CHECK(memcmp(bytes, bytes_ext, sizeof(bytes)) == 0);

    lambda = affine;
    secp256k1_ge_mul_lambda(&lambda, &lambda);
    FUZZ_CHECK(secp256k1_fe_equal(&lambda.y, &affine.y));
    {
        secp256k1_fe expected_x;
        secp256k1_fe_mul(&expected_x, &affine.x, &secp256k1_const_beta);
        FUZZ_CHECK(secp256k1_fe_equal(&lambda.x, &expected_x));
    }
    lambda_twice = lambda;
    secp256k1_ge_mul_lambda(&lambda_twice, &lambda_twice);
    lambda_thrice = lambda_twice;
    secp256k1_ge_mul_lambda(&lambda_thrice, &lambda_thrice);
    FUZZ_CHECK(secp256k1_ge_eq_var(&lambda_thrice, &affine));
}

static void secp256k1_fuzz_group_check_nonnormalized_storage(void) {
    secp256k1_ge canonical = secp256k1_ge_const_g;
    secp256k1_ge nonnormalized = canonical;
    secp256k1_ge recovered;
    secp256k1_ge_storage expected_storage;
    secp256k1_ge_storage actual_storage;
    secp256k1_fe zero3;
    secp256k1_fe zero2;
    unsigned char expected_bytes[64];
    unsigned char actual_bytes[64];

    /* A valid affine point may carry field elements that are equivalent to
     * the canonical coordinates but have not been normalized yet. Storage
     * conversion is the boundary that must make this representation stable. */
    secp256k1_fe_set_int(&zero3, 0);
    secp256k1_fe_negate(&zero3, &zero3, 0);
    secp256k1_fe_mul_int_unchecked(&zero3, 3);
    secp256k1_fe_set_int(&zero2, 0);
    secp256k1_fe_negate(&zero2, &zero2, 0);
    secp256k1_fe_mul_int_unchecked(&zero2, 2);
    secp256k1_fe_add(&nonnormalized.x, &zero3);
    secp256k1_fe_add(&nonnormalized.y, &zero2);
#ifdef VERIFY
    FUZZ_CHECK(nonnormalized.x.magnitude == 4);
    FUZZ_CHECK(nonnormalized.x.normalized == 0);
    FUZZ_CHECK(nonnormalized.y.magnitude == 3);
    FUZZ_CHECK(nonnormalized.y.normalized == 0);
#endif

    secp256k1_ge_to_storage(&expected_storage, &canonical);
    secp256k1_ge_to_storage(&actual_storage, &nonnormalized);
    FUZZ_CHECK(memcmp(&actual_storage, &expected_storage, sizeof(actual_storage)) == 0);
    secp256k1_ge_from_storage(&recovered, &actual_storage);
    FUZZ_CHECK(secp256k1_ge_eq_var(&recovered, &canonical));

    secp256k1_ge_to_bytes(expected_bytes, &canonical);
    secp256k1_ge_to_bytes(actual_bytes, &nonnormalized);
    FUZZ_CHECK(memcmp(actual_bytes, expected_bytes, sizeof(actual_bytes)) == 0);
}

static void secp256k1_fuzz_group_check_xo(const secp256k1_ge *point) {
    secp256k1_ge even;
    secp256k1_ge odd;
    secp256k1_ge negated;
    int on_curve;
    int even_ret;
    int odd_ret;

    on_curve = secp256k1_ge_x_on_curve_var(&point->x);
    even_ret = secp256k1_ge_set_xo_var(&even, &point->x, 0);
    odd_ret = secp256k1_ge_set_xo_var(&odd, &point->x, 1);
    FUZZ_CHECK(even_ret == on_curve);
    FUZZ_CHECK(odd_ret == on_curve);

    if (!on_curve) {
        return;
    }

    FUZZ_CHECK(!secp256k1_ge_is_infinity(&even));
    FUZZ_CHECK(!secp256k1_ge_is_infinity(&odd));
    FUZZ_CHECK(!secp256k1_ge_is_infinity(point));
    secp256k1_fe_normalize_var(&even.y);
    secp256k1_fe_normalize_var(&odd.y);
    FUZZ_CHECK(secp256k1_fe_equal(&even.x, &point->x));
    FUZZ_CHECK(secp256k1_fe_equal(&odd.x, &point->x));
    FUZZ_CHECK(!secp256k1_fe_is_odd(&even.y));
    FUZZ_CHECK(secp256k1_fe_is_odd(&odd.y));
    FUZZ_CHECK(secp256k1_ge_is_valid_var(&even));
    FUZZ_CHECK(secp256k1_ge_is_valid_var(&odd));

    negated = *point;
    secp256k1_ge_neg(&negated, &negated);
    FUZZ_CHECK(secp256k1_ge_eq_var(&even, point) || secp256k1_ge_eq_var(&even, &negated));
    FUZZ_CHECK(secp256k1_ge_eq_var(&odd, point) || secp256k1_ge_eq_var(&odd, &negated));
}

static void secp256k1_fuzz_group_check_eq_x(const secp256k1_gej *point) {
    secp256k1_ge affine;
    secp256k1_fe wrong_x;
    secp256k1_gej copy = *point;

    if (secp256k1_gej_is_infinity(point)) {
        return;
    }
    secp256k1_ge_set_gej_var(&affine, &copy);
    FUZZ_CHECK(secp256k1_gej_eq_x_var(&affine.x, point));
    wrong_x = affine.x;
    secp256k1_fe_add_int(&wrong_x, 1);
    secp256k1_fe_normalize_var(&wrong_x);
    FUZZ_CHECK(!secp256k1_gej_eq_x_var(&wrong_x, point));
}

static void secp256k1_fuzz_group_check_gej_cmov(const secp256k1_gej *a, const secp256k1_gej *b) {
    secp256k1_gej selected;
    secp256k1_gej expected;
    int flag;

    for (flag = 0; flag <= 1; flag++) {
        selected = *a;
        expected = flag ? *b : *a;
        secp256k1_gej_cmov(&selected, b, flag);
        secp256k1_fuzz_group_check_gej_equal(&selected, &expected);
    }
}

static void secp256k1_fuzz_group_check_storage_cmov(const secp256k1_ge *a, const secp256k1_ge *b) {
    secp256k1_ge_storage storage_a;
    secp256k1_ge_storage storage_b;
    secp256k1_ge_storage selected;
    secp256k1_ge expected;
    secp256k1_ge result;
    int flag;

    FUZZ_CHECK(!secp256k1_ge_is_infinity(a));
    FUZZ_CHECK(!secp256k1_ge_is_infinity(b));
    secp256k1_ge_to_storage(&storage_a, a);
    secp256k1_ge_to_storage(&storage_b, b);
    for (flag = 0; flag <= 1; flag++) {
        expected = flag ? *b : *a;
        selected = storage_a;
        secp256k1_ge_storage_cmov(&selected, &storage_b, flag);
        secp256k1_ge_from_storage(&result, &selected);
        FUZZ_CHECK(secp256k1_ge_eq_var(&result, &expected));
    }
}

static void secp256k1_fuzz_group_check_batch_conversion(const secp256k1_gej *points, size_t len) {
    secp256k1_ge constant_time[4];
    secp256k1_ge variable_time[4];
    size_t i;

    FUZZ_CHECK(len <= 4);
    secp256k1_ge_set_all_gej(constant_time, points, len);
    secp256k1_ge_set_all_gej_var(variable_time, points, len);
    for (i = 0; i < len; i++) {
        FUZZ_CHECK(secp256k1_ge_eq_var(&constant_time[i], &variable_time[i]));
        FUZZ_CHECK(secp256k1_gej_eq_ge_var(&points[i], &constant_time[i]));
    }
}

static void secp256k1_fuzz_group_check_batch_conversion_finite(const secp256k1_gej *points, size_t len) {
    secp256k1_ge constant_time[4];
    secp256k1_ge variable_time[4];
    size_t i;

    FUZZ_CHECK(len <= 4);
    for (i = 0; i < len; i++) {
        FUZZ_CHECK(!secp256k1_gej_is_infinity(&points[i]));
    }
    secp256k1_ge_set_all_gej(constant_time, points, len);
    secp256k1_ge_set_all_gej_var(variable_time, points, len);
    for (i = 0; i < len; i++) {
        FUZZ_CHECK(secp256k1_ge_eq_var(&constant_time[i], &variable_time[i]));
        FUZZ_CHECK(secp256k1_gej_eq_ge_var(&points[i], &constant_time[i]));
    }
}

static void secp256k1_fuzz_group_check_batch_conversion_boundaries(const secp256k1_gej *a, const secp256k1_gej *b) {
    secp256k1_gej points[4];
    secp256k1_ge affine[4];
    size_t i;

    FUZZ_CHECK(!secp256k1_gej_is_infinity(a));
    FUZZ_CHECK(!secp256k1_gej_is_infinity(b));
    points[0] = *a;
    secp256k1_gej_set_infinity(&points[1]);
    points[2] = *b;
    secp256k1_gej_set_infinity(&points[3]);
    memset(affine, 0xA5, sizeof(affine));
    secp256k1_ge_set_all_gej_var(affine, points, 4);
    for (i = 0; i < 4; i++) {
        if (secp256k1_gej_is_infinity(&points[i])) {
            FUZZ_CHECK(secp256k1_ge_is_infinity(&affine[i]));
        } else {
            secp256k1_ge expected;
            secp256k1_gej point = points[i];
            secp256k1_ge_set_gej_var(&expected, &point);
            FUZZ_CHECK(secp256k1_ge_eq_var(&affine[i], &expected));
        }
    }

    /* Both batch helpers promise a no-op for an empty range, including NULL
     * array pointers. This is an internal boundary used by test/bench code. */
    secp256k1_ge_set_all_gej(NULL, NULL, 0);
    secp256k1_ge_set_all_gej_var(NULL, NULL, 0);
}

static void secp256k1_fuzz_group_check_ge_zinv(const unsigned char *input, size_t size) {
    secp256k1_gej projective;
    secp256k1_gej projective_copy;
    secp256k1_ge projective_ge;
    secp256k1_ge actual;
    secp256k1_ge expected;
    secp256k1_fe scale;
    secp256k1_fe inverse_scale;
    secp256k1_fe zero7;
    secp256k1_fe raised_inverse;
    secp256k1_fe raised_one;
    unsigned char actual_bytes[64];
    unsigned char expected_bytes[64];

    /* Build a valid projective generator representation and recover its
     * affine point through the inverse-Z helper. Keep the inverse-Z input
     * nonnormalized as well: the helper accepts a valid field element, not a
     * particular limb representation. */
    secp256k1_fuzz_derive(actual_bytes, sizeof(actual_bytes), input, size, 211);
    secp256k1_fe_set_b32_mod(&scale, actual_bytes);
    secp256k1_fe_normalize_var(&scale);
    if (secp256k1_fe_is_zero(&scale)) {
        secp256k1_fe_set_int(&scale, 1);
    }
    secp256k1_gej_set_ge(&projective, &secp256k1_ge_const_g);
    secp256k1_gej_rescale(&projective, &scale);
    projective_ge.infinity = projective.infinity;
    projective_ge.x = projective.x;
    projective_ge.y = projective.y;
    inverse_scale = projective.z;
    secp256k1_fe_inv_var(&inverse_scale, &inverse_scale);

    projective_copy = projective;
    secp256k1_ge_set_gej_var(&expected, &projective_copy);
    secp256k1_ge_set_ge_zinv(&actual, &projective_ge, &inverse_scale);
    FUZZ_CHECK(secp256k1_ge_eq_var(&actual, &expected));
    secp256k1_ge_to_bytes(actual_bytes, &actual);
    secp256k1_ge_to_bytes(expected_bytes, &expected);
    FUZZ_CHECK(memcmp(actual_bytes, expected_bytes, sizeof(actual_bytes)) == 0);

    /* A fixed generator case makes the maximum valid inverse-Z magnitude
     * deterministic and independently exercises the gej-zinv conversion. */
    secp256k1_fe_set_int(&zero7, 0);
    secp256k1_fe_negate(&zero7, &zero7, 0);
    secp256k1_fe_mul_int_unchecked(&zero7, 7);
    raised_inverse = inverse_scale;
    secp256k1_fe_add(&raised_inverse, &zero7);
    raised_one = secp256k1_fe_one;
    secp256k1_fe_add(&raised_one, &zero7);
#ifdef VERIFY
    FUZZ_CHECK(raised_inverse.magnitude == 8);
    FUZZ_CHECK(raised_inverse.normalized == 0);
    FUZZ_CHECK(raised_one.magnitude == 8);
    FUZZ_CHECK(raised_one.normalized == 0);
#endif

    projective_copy = projective;
    secp256k1_ge_set_gej_zinv(&expected, &projective_copy, &raised_inverse);
    secp256k1_ge_set_ge_zinv(&actual, &projective_ge, &raised_inverse);
    FUZZ_CHECK(secp256k1_ge_eq_var(&actual, &expected));

    secp256k1_gej_set_ge(&projective_copy, &secp256k1_ge_const_g);
    projective_ge = secp256k1_ge_const_g;
    secp256k1_ge_set_gej_zinv(&expected, &projective_copy, &raised_one);
    secp256k1_ge_set_ge_zinv(&actual, &projective_ge, &raised_one);
    FUZZ_CHECK(secp256k1_ge_eq_var(&actual, &expected));
    FUZZ_CHECK(secp256k1_ge_eq_var(&actual, &secp256k1_ge_const_g));
}

static int secp256k1_fuzz_group_x_on_curve_reference(const secp256k1_fe *x) {
    secp256k1_fe x2;
    secp256k1_fe x3;

    secp256k1_fe_sqr(&x2, x);
    secp256k1_fe_mul(&x3, &x2, x);
    secp256k1_fe_add_int(&x3, SECP256K1_B);
    return secp256k1_fe_is_square_var(&x3);
}

static void secp256k1_fuzz_group_check_x_frac_curve(const unsigned char *input, size_t size) {
    unsigned char numerator32[32];
    unsigned char denominator32[32];
    secp256k1_fe numerator;
    secp256k1_fe denominator;
    secp256k1_fe denominator_inverse;
    secp256k1_fe quotient;

    secp256k1_fuzz_derive(numerator32, sizeof(numerator32), input, size, 67);
    secp256k1_fuzz_derive(denominator32, sizeof(denominator32), input, size, 73);
    secp256k1_fe_set_b32_mod(&numerator, numerator32);
    secp256k1_fe_set_b32_mod(&denominator, denominator32);
    secp256k1_fe_normalize_var(&numerator);
    secp256k1_fe_normalize_var(&denominator);
    if (secp256k1_fe_is_zero(&denominator)) {
        secp256k1_fe_set_int(&denominator, 1);
    }

    denominator_inverse = denominator;
    secp256k1_fe_inv_var(&denominator_inverse, &denominator_inverse);
    secp256k1_fe_mul(&quotient, &numerator, &denominator_inverse);
    FUZZ_CHECK(secp256k1_ge_x_frac_on_curve_var(&numerator, &denominator) == secp256k1_fuzz_group_x_on_curve_reference(&quotient));

    /* The generator gives a deterministic true case even when random input is not on-curve. */
    denominator = secp256k1_fe_one;
    numerator = secp256k1_ge_const_g.x;
    FUZZ_CHECK(secp256k1_ge_x_frac_on_curve_var(&numerator, &denominator) == 1);
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
    static const unsigned char infinity_validity_trigger[] = "group infinity validity\n";
    static const unsigned char canonical_infinity_trigger[] = "group canonical infinity storage\n";
    static const unsigned char affine_equality_infinity_trigger[] = "group affine equality infinity\n";
    static const unsigned char zinv_inverse_trigger[] = "group zinv inverse\n";
    static const unsigned char inverse_rzr_trigger[] = "group inverse rzr\n";
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 71);
    secp256k1_scalar a_scalar;
    secp256k1_scalar b_scalar;
    secp256k1_scalar sum_scalar;
    secp256k1_scalar double_scalar;
    secp256k1_gej a;
    secp256k1_gej b;
    secp256k1_gej sum;
    secp256k1_gej doubled;
    secp256k1_gej negated;
    secp256k1_gej cancelled;
    secp256k1_gej rescaled;
    secp256k1_gej infinity;
    secp256k1_gej finite;
    secp256k1_fe scale;
    unsigned char a32[32];
    unsigned char b32[32];
    unsigned char scale32[32];

    secp256k1_fuzz_group_check_x_frac_curve(input, size);
    secp256k1_fuzz_group_check_opaque_pubkey_barrier(ctx);
    secp256k1_fuzz_group_scalar(&a_scalar, a32, input, size, 79);
    secp256k1_fuzz_group_scalar(&b_scalar, b32, input, size, 83);
    (void)secp256k1_scalar_add(&sum_scalar, &a_scalar, &b_scalar);
    (void)secp256k1_scalar_add(&double_scalar, &a_scalar, &a_scalar);
    secp256k1_fuzz_group_make_point(ctx, &a, &a_scalar);
    secp256k1_fuzz_group_make_point(ctx, &b, &b_scalar);
    secp256k1_fuzz_group_make_point(ctx, &sum, &sum_scalar);
    secp256k1_fuzz_group_make_point(ctx, &doubled, &double_scalar);
    secp256k1_fuzz_group_check_independent_generator(&a, &a_scalar);
    secp256k1_fuzz_group_check_independent_generator(&b, &b_scalar);
    secp256k1_fuzz_group_check_independent_generator(&sum, &sum_scalar);
    secp256k1_fuzz_group_check_independent_generator(&doubled, &double_scalar);

    secp256k1_fuzz_group_check_affine(&a);
    secp256k1_fuzz_group_check_affine(&b);
    secp256k1_fuzz_group_check_affine(&sum);
    secp256k1_fuzz_group_check_gej_cmov(&a, &b);
    if (!secp256k1_gej_is_infinity(&a) && !secp256k1_gej_is_infinity(&b)) {
        secp256k1_ge affine_a;
        secp256k1_ge affine_b;
        secp256k1_gej copy_a = a;
        secp256k1_gej copy_b = b;
        secp256k1_ge_set_gej_var(&affine_a, &copy_a);
        secp256k1_ge_set_gej_var(&affine_b, &copy_b);
        secp256k1_fuzz_group_check_storage_cmov(&affine_a, &affine_b);
    }
    secp256k1_fuzz_group_check_affine_representations(&a);
    secp256k1_fuzz_group_check_nonnormalized_storage();
    secp256k1_fuzz_group_check_ge_zinv(input, size);
    if (!secp256k1_gej_is_infinity(&a)) {
        secp256k1_ge affine_a;
        secp256k1_gej copy = a;
        secp256k1_ge_set_gej_var(&affine_a, &copy);
        secp256k1_fuzz_group_check_xo(&affine_a);
    }
    secp256k1_fuzz_group_check_eq_x(&a);
    secp256k1_fuzz_group_check_eq_x(&b);
    secp256k1_fuzz_group_check_eq_x(&sum);
    secp256k1_fuzz_group_check_addition(&a, &b, &sum);
    secp256k1_fuzz_group_check_double(&a, &doubled);
    secp256k1_fuzz_group_check_batch(&a, &b, &sum);

    secp256k1_gej_neg(&negated, &a);
    secp256k1_gej_add_var(&cancelled, &a, &negated, NULL);
    FUZZ_CHECK(secp256k1_gej_is_infinity(&cancelled));

    secp256k1_fuzz_derive(scale32, sizeof(scale32), input, size, 89);
    secp256k1_fe_set_b32_mod(&scale, scale32);
    secp256k1_fe_normalize_var(&scale);
    if (secp256k1_fe_is_zero(&scale)) {
        secp256k1_fe_set_int(&scale, 1);
    }
    secp256k1_fuzz_group_check_zinv_addition(&a, &b, &sum, &scale);
    secp256k1_gej_set_infinity(&infinity);
    secp256k1_fuzz_group_make_point(ctx, &finite, &secp256k1_scalar_one);
    secp256k1_fuzz_group_check_independent_generator(&finite, &secp256k1_scalar_one);
    secp256k1_fuzz_group_check_gej_eq_ge_negative(&finite);
    secp256k1_fuzz_group_check_gej_not_equal(&finite);
    secp256k1_fuzz_group_check_gej_cmov(&infinity, &finite);
    secp256k1_fuzz_group_check_gej_cmov(&finite, &infinity);
    secp256k1_fuzz_group_check_lambda_degenerate_addition(&finite);
    {
        secp256k1_gej finite_batch[4];
        secp256k1_ge finite_affine;
        secp256k1_gej finite_other = finite;
        secp256k1_gej_double(&finite_other, &finite_other);
        secp256k1_ge_set_gej_var(&finite_affine, &finite_other);
        finite_batch[0] = finite;
        finite_batch[1] = finite_other;
        secp256k1_gej_double(&finite_batch[2], &finite_batch[1]);
        secp256k1_gej_double(&finite_batch[3], &finite_batch[2]);
        secp256k1_fuzz_group_check_batch_conversion(finite_batch, 4);
        secp256k1_fuzz_group_check_batch_conversion_finite(finite_batch, 4);
        secp256k1_fuzz_group_check_batch_conversion_boundaries(&finite_batch[0], &finite_batch[3]);
        secp256k1_fuzz_group_check_zinv_in_place(&finite, &finite_affine);
    }
    secp256k1_fuzz_group_check_rescale_alias(&finite);
    secp256k1_fuzz_group_check_rescale_nonnormalized_scale(&finite);
    secp256k1_fuzz_group_check_zinv_addition(&infinity, &finite, &finite, &scale);
    secp256k1_fuzz_group_check_zinv_addition(&finite, &infinity, &finite, &scale);
    secp256k1_fuzz_group_check_zinv_addition(&infinity, &infinity, &infinity, &scale);
    if (size == sizeof(infinity_validity_trigger) - 1 && memcmp(input, infinity_validity_trigger, sizeof(infinity_validity_trigger) - 1) == 0) {
        secp256k1_fuzz_group_check_infinity_invalid();
    }
    if (size == sizeof(canonical_infinity_trigger) - 1 && memcmp(input, canonical_infinity_trigger, sizeof(canonical_infinity_trigger) - 1) == 0) {
        secp256k1_fuzz_group_check_canonical_infinity();
    }
    if (size == sizeof(affine_equality_infinity_trigger) - 1 && memcmp(input, affine_equality_infinity_trigger, sizeof(affine_equality_infinity_trigger) - 1) == 0) {
        secp256k1_fuzz_group_check_ge_eq_infinity();
    }
    if (size == sizeof(zinv_inverse_trigger) - 1 && memcmp(input, zinv_inverse_trigger, sizeof(zinv_inverse_trigger) - 1) == 0) {
        secp256k1_fuzz_group_check_zinv_inverse();
    }
    if (size == sizeof(inverse_rzr_trigger) - 1 && memcmp(input, inverse_rzr_trigger, sizeof(inverse_rzr_trigger) - 1) == 0) {
        secp256k1_fuzz_group_check_inverse_rzr();
    }
    rescaled = a;
    secp256k1_gej_rescale(&rescaled, &scale);
    secp256k1_fuzz_group_check_eq_x(&rescaled);
    secp256k1_fuzz_group_check_gej_equal(&rescaled, &a);
    secp256k1_fuzz_group_check_addition(&rescaled, &b, &sum);

    secp256k1_fuzz_group_check_ge_clear();
    secp256k1_gej_clear(&rescaled);
    SECP256K1_CHECKMEM_DEFINE(&rescaled, sizeof(rescaled));
    FUZZ_CHECK(secp256k1_fuzz_group_all_zero(&rescaled, sizeof(rescaled)));
    secp256k1_context_destroy(ctx);
    return 0;
}
