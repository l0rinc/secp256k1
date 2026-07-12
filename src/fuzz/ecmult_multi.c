/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../secp256k1.c"
#include "fuzz.h"

#define SECP256K1_FUZZ_ECMULT_MULTI_REPEAT_MAX_POINTS 128

typedef struct {
    secp256k1_scalar sc[8];
    secp256k1_ge pt[8];
    int fail;
    size_t fail_at;
    size_t calls;
    unsigned int seen_mask;
} secp256k1_fuzz_ecmult_multi_data;

typedef struct {
    secp256k1_scalar sc;
    secp256k1_ge pt;
    uint64_t seen[(SECP256K1_FUZZ_ECMULT_MULTI_REPEAT_MAX_POINTS + 63) / 64];
    size_t calls;
    size_t fail_at;
    int fail;
} secp256k1_fuzz_ecmult_multi_repeat_data;

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_ecmult_multi_error_data;

static void secp256k1_fuzz_ecmult_multi_error_callback(const char *message, void *data) {
    secp256k1_fuzz_ecmult_multi_error_data *error_data = (secp256k1_fuzz_ecmult_multi_error_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(error_data != NULL);
    FUZZ_CHECK(error_data->self == error_data);
    error_data->calls++;
}

static size_t secp256k1_fuzz_size_t(const unsigned char *input, size_t size, unsigned int salt) {
    unsigned char bytes[sizeof(size_t)];
    size_t ret = 0;
    size_t i;

    switch (secp256k1_fuzz_byte(input, size, salt) & 7u) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return ECMULT_MAX_POINTS_PER_BATCH;
    case 3:
        return ECMULT_MAX_POINTS_PER_BATCH + 1u;
    case 4:
        return SIZE_MAX;
    case 5:
        return SIZE_MAX / 2u + 1u;
    default:
        break;
    }

    secp256k1_fuzz_derive(bytes, sizeof(bytes), input, size, salt + 1u);
    for (i = 0; i < sizeof(bytes); i++) {
        ret = (ret << 8) | bytes[i];
    }
    return ret;
}

static size_t secp256k1_fuzz_ceil_div_size(size_t n, size_t d) {
    FUZZ_CHECK(d != 0);
    return n == 0 ? 0 : 1u + (n - 1u) / d;
}

static void secp256k1_fuzz_check_ecmult_multi_batch_size_helper(const unsigned char *input, size_t size) {
    size_t n_batches = SIZE_MAX;
    size_t n_batch_points = SIZE_MAX;
    size_t max_n_batch_points = secp256k1_fuzz_size_t(input, size, 503);
    size_t n = secp256k1_fuzz_size_t(input, size, 521);
    size_t capped_max = max_n_batch_points;
    int ret;

    ret = secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_n_batch_points, n);
    if (max_n_batch_points == 0) {
        FUZZ_CHECK(ret == 0);
        return;
    }

    if (capped_max > ECMULT_MAX_POINTS_PER_BATCH) {
        capped_max = ECMULT_MAX_POINTS_PER_BATCH;
    }

    FUZZ_CHECK(ret == 1);
    if (n == 0) {
        FUZZ_CHECK(n_batches == 0);
        FUZZ_CHECK(n_batch_points == 0);
    } else {
        size_t expected_batches = secp256k1_fuzz_ceil_div_size(n, capped_max);
        size_t expected_batch_points = secp256k1_fuzz_ceil_div_size(n, expected_batches);

        FUZZ_CHECK(n_batches == expected_batches);
        FUZZ_CHECK(n_batch_points == expected_batch_points);
        FUZZ_CHECK(n_batch_points != 0);
        FUZZ_CHECK(n_batch_points <= capped_max);
        FUZZ_CHECK(secp256k1_fuzz_ceil_div_size(n, n_batch_points) == n_batches);
    }
}

static void secp256k1_fuzz_check_scratch_create_boundaries(const secp256k1_context *ctx) {
    const size_t base_alloc = ROUND_TO_ALIGN(sizeof(secp256k1_scratch));

    FUZZ_CHECK(secp256k1_scratch_create(&ctx->error_callback, SIZE_MAX) == NULL);
    FUZZ_CHECK(base_alloc != 0);
    FUZZ_CHECK(secp256k1_scratch_create(&ctx->error_callback, SIZE_MAX - base_alloc + 1u) == NULL);
}

static void secp256k1_fuzz_check_error_callback_routing(secp256k1_context *ctx) {
    secp256k1_fuzz_ecmult_multi_error_data error_data;
    secp256k1_scratch invalid_scratch;

    error_data.self = &error_data;
    error_data.calls = 0;
    memset(&invalid_scratch, 0, sizeof(invalid_scratch));
    secp256k1_context_set_error_callback(ctx, secp256k1_fuzz_ecmult_multi_error_callback, &error_data);
    secp256k1_scratch_destroy(&ctx->error_callback, &invalid_scratch);
    FUZZ_CHECK(error_data.calls == 1);
    secp256k1_context_set_error_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_error_callback_clone(secp256k1_context *ctx) {
    secp256k1_fuzz_ecmult_multi_error_data cloned_data;
    secp256k1_fuzz_ecmult_multi_error_data original_data;
    secp256k1_context *callback_clone;
    secp256k1_context *callback_prealloc_clone;
    secp256k1_scratch invalid_scratch;
    void *callback_prealloc_clone_mem;
    size_t prealloc_size;

    cloned_data.self = &cloned_data;
    cloned_data.calls = 0;
    original_data.self = &original_data;
    original_data.calls = 0;
    memset(&invalid_scratch, 0, sizeof(invalid_scratch));

    secp256k1_context_set_error_callback(ctx, secp256k1_fuzz_ecmult_multi_error_callback, &cloned_data);
    callback_clone = secp256k1_context_clone(ctx);
    FUZZ_CHECK(callback_clone != NULL);
    prealloc_size = secp256k1_context_preallocated_clone_size(ctx);
    callback_prealloc_clone_mem = malloc(prealloc_size);
    FUZZ_CHECK(callback_prealloc_clone_mem != NULL);
    callback_prealloc_clone = secp256k1_context_preallocated_clone(ctx, callback_prealloc_clone_mem);
    FUZZ_CHECK(callback_prealloc_clone != NULL);

    secp256k1_context_set_error_callback(ctx, secp256k1_fuzz_ecmult_multi_error_callback, &original_data);
    secp256k1_scratch_destroy(&callback_clone->error_callback, &invalid_scratch);
    FUZZ_CHECK(cloned_data.calls == 1);
    FUZZ_CHECK(original_data.calls == 0);
    secp256k1_scratch_destroy(&callback_prealloc_clone->error_callback, &invalid_scratch);
    FUZZ_CHECK(cloned_data.calls == 2);
    FUZZ_CHECK(original_data.calls == 0);
    secp256k1_scratch_destroy(&ctx->error_callback, &invalid_scratch);
    FUZZ_CHECK(cloned_data.calls == 2);
    FUZZ_CHECK(original_data.calls == 1);

    secp256k1_context_set_error_callback(ctx, NULL, NULL);
    secp256k1_context_set_error_callback(callback_clone, NULL, NULL);
    secp256k1_context_set_error_callback(callback_prealloc_clone, NULL, NULL);
    secp256k1_context_destroy(callback_clone);
    secp256k1_context_preallocated_destroy(callback_prealloc_clone);
    free(callback_prealloc_clone_mem);
}

static void secp256k1_fuzz_ecmult_multi_reset_trace(secp256k1_fuzz_ecmult_multi_data *data) {
    data->calls = 0;
    data->seen_mask = 0;
}

static unsigned int secp256k1_fuzz_ecmult_multi_seen_mask(size_t n_points) {
    FUZZ_CHECK(n_points <= 8);
    return n_points == 0 ? 0u : ((1u << n_points) - 1u);
}

static void secp256k1_fuzz_ecmult_multi_check_success_trace(const secp256k1_fuzz_ecmult_multi_data *data, size_t n_points) {
    FUZZ_CHECK(data->calls == n_points);
    FUZZ_CHECK(data->seen_mask == secp256k1_fuzz_ecmult_multi_seen_mask(n_points));
}

static void secp256k1_fuzz_ecmult_multi_check_failure_trace(const secp256k1_fuzz_ecmult_multi_data *data, size_t n_points) {
    /* All ecmult_multi implementations enumerate input indices in order. Once
     * the callback rejects fail_at, no later input may be requested. */
    FUZZ_CHECK(data->fail_at < n_points);
    FUZZ_CHECK(data->calls == data->fail_at + 1);
    FUZZ_CHECK((data->seen_mask & (1u << data->fail_at)) != 0);
}

static int secp256k1_fuzz_ecmult_multi_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *cbdata) {
    secp256k1_fuzz_ecmult_multi_data *data = (secp256k1_fuzz_ecmult_multi_data *)cbdata;
    FUZZ_CHECK(idx < 8);
    FUZZ_CHECK((data->seen_mask & (1u << idx)) == 0);
    data->seen_mask |= 1u << idx;
    data->calls++;
    if (data->fail && idx == data->fail_at) {
        return 0;
    }
    *sc = data->sc[idx];
    *pt = data->pt[idx];
    return 1;
}

static void secp256k1_fuzz_ecmult_multi_repeat_reset_trace(secp256k1_fuzz_ecmult_multi_repeat_data *data) {
    memset(data->seen, 0, sizeof(data->seen));
    data->calls = 0;
}

static void secp256k1_fuzz_ecmult_multi_repeat_check_trace(const secp256k1_fuzz_ecmult_multi_repeat_data *data, size_t n_points) {
    size_t i;

    FUZZ_CHECK(data->calls == n_points);
    FUZZ_CHECK(n_points <= SECP256K1_FUZZ_ECMULT_MULTI_REPEAT_MAX_POINTS);
    for (i = 0; i < n_points; i++) {
        FUZZ_CHECK((data->seen[i / 64] & ((uint64_t)1 << (i % 64))) != 0);
    }
}

static void secp256k1_fuzz_ecmult_multi_repeat_check_failure_trace(const secp256k1_fuzz_ecmult_multi_repeat_data *data, size_t n_points) {
    size_t i;

    FUZZ_CHECK(data->fail_at < n_points);
    FUZZ_CHECK(data->calls == data->fail_at + 1);
    for (i = 0; i <= data->fail_at; i++) {
        FUZZ_CHECK((data->seen[i / 64] & ((uint64_t)1 << (i % 64))) != 0);
    }
    for (i = data->fail_at + 1; i < n_points; i++) {
        FUZZ_CHECK((data->seen[i / 64] & ((uint64_t)1 << (i % 64))) == 0);
    }
}

static int secp256k1_fuzz_ecmult_multi_repeat_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *cbdata) {
    secp256k1_fuzz_ecmult_multi_repeat_data *data = (secp256k1_fuzz_ecmult_multi_repeat_data *)cbdata;
    uint64_t bit;

    FUZZ_CHECK(idx < SECP256K1_FUZZ_ECMULT_MULTI_REPEAT_MAX_POINTS);
    bit = (uint64_t)1 << (idx % 64);
    FUZZ_CHECK((data->seen[idx / 64] & bit) == 0);
    data->seen[idx / 64] |= bit;
    data->calls++;
    if (data->fail && idx == data->fail_at) {
        return 0;
    }
    *sc = data->sc;
    *pt = data->pt;
    return 1;
}

static void secp256k1_fuzz_ecmult_multi_make_point(const secp256k1_context *ctx, secp256k1_ge *pt, const unsigned char *input, size_t size, unsigned int salt) {
    secp256k1_scalar sc;
    unsigned char scalar32[32];
    int overflow;

    secp256k1_fuzz_scalar32(scalar32, input, size, salt);
    secp256k1_scalar_set_b32(&sc, scalar32, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&sc)) {
        secp256k1_scalar_set_int(&sc, 1);
    }
    secp256k1_ecmult_gen_ge(&ctx->ecmult_gen_ctx, pt, &sc);
    secp256k1_scalar_clear(&sc);
}

static void secp256k1_fuzz_ecmult_multi_reference(secp256k1_gej *expected, const secp256k1_scalar *g_sc, size_t n_points, const secp256k1_fuzz_ecmult_multi_data *data) {
    secp256k1_gej term;
    size_t i;

    if (g_sc == NULL) {
        secp256k1_gej_set_infinity(expected);
    } else {
        secp256k1_ecmult_const(expected, &secp256k1_ge_const_g, g_sc);
    }
    for (i = 0; i < n_points; i++) {
        if (secp256k1_ge_is_infinity(&data->pt[i])) {
            secp256k1_gej_set_infinity(&term);
        } else {
            secp256k1_ecmult_const(&term, &data->pt[i], &data->sc[i]);
        }
        secp256k1_gej_add_var(expected, expected, &term, NULL);
    }
}

static void secp256k1_fuzz_ecmult_multi_compare(const secp256k1_context *ctx, size_t scratch_size, const secp256k1_scalar *g_sc, size_t n_points, secp256k1_fuzz_ecmult_multi_data *data) {
    secp256k1_scratch *scratch;
    secp256k1_gej expected;
    secp256k1_gej no_scratch;
    secp256k1_gej with_scratch;
    size_t checkpoint;
    int no_scratch_ret;
    int with_scratch_ret;

    scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size);
    FUZZ_CHECK(scratch != NULL);
    checkpoint = scratch->alloc_size;
    secp256k1_fuzz_ecmult_multi_reference(&expected, g_sc, n_points, data);

    secp256k1_fuzz_ecmult_multi_reset_trace(data);
    no_scratch_ret = secp256k1_ecmult_multi_var(&ctx->error_callback, NULL, &no_scratch, g_sc, secp256k1_fuzz_ecmult_multi_callback, data, n_points);
    secp256k1_fuzz_ecmult_multi_check_success_trace(data, n_points);
    secp256k1_fuzz_ecmult_multi_reset_trace(data);
    with_scratch_ret = secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &with_scratch, g_sc, secp256k1_fuzz_ecmult_multi_callback, data, n_points);
    secp256k1_fuzz_ecmult_multi_check_success_trace(data, n_points);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(no_scratch_ret == with_scratch_ret);
    if (with_scratch_ret) {
        FUZZ_CHECK(secp256k1_gej_eq_var(&no_scratch, &expected));
        FUZZ_CHECK(secp256k1_gej_eq_var(&no_scratch, &with_scratch));
        FUZZ_CHECK(secp256k1_gej_eq_var(&with_scratch, &expected));
    }

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

static void secp256k1_fuzz_ecmult_multi_repeated_pippenger(const secp256k1_context *ctx, const secp256k1_scalar *g_sc, const unsigned char *input, size_t size) {
    secp256k1_fuzz_ecmult_multi_repeat_data data;
    secp256k1_scratch *scratch;
    secp256k1_scalar n_sc;
    secp256k1_scalar total_sc;
    secp256k1_gej pointj;
    secp256k1_gej actual;
    secp256k1_gej expected;
    unsigned char scalar32[32];
    size_t n_points;
    size_t scratch_size;
    size_t n_batches;
    size_t n_batch_points;
    size_t checkpoint;
    int overflow;

    n_points = ECMULT_PIPPENGER_THRESHOLD + (secp256k1_fuzz_byte(input, size, 443) % (SECP256K1_FUZZ_ECMULT_MULTI_REPEAT_MAX_POINTS - ECMULT_PIPPENGER_THRESHOLD + 1u));
    FUZZ_CHECK(n_points >= ECMULT_PIPPENGER_THRESHOLD);
    FUZZ_CHECK(n_points <= SECP256K1_FUZZ_ECMULT_MULTI_REPEAT_MAX_POINTS);

    memset(&data, 0, sizeof(data));
    secp256k1_fuzz_scalar32(scalar32, input, size, 461);
    secp256k1_scalar_set_b32(&data.sc, scalar32, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&data.sc)) {
        secp256k1_scalar_set_int(&data.sc, 1);
    }
    secp256k1_fuzz_ecmult_multi_make_point(ctx, &data.pt, input, size, 479);

    secp256k1_scalar_set_int(&n_sc, (unsigned int)n_points);
    secp256k1_scalar_mul(&total_sc, &data.sc, &n_sc);
    secp256k1_gej_set_ge(&pointj, &data.pt);
    secp256k1_ecmult(&expected, &pointj, &total_sc, g_sc);

    scratch_size = secp256k1_pippenger_scratch_size(n_points, secp256k1_pippenger_bucket_window(n_points));
    scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size + PIPPENGER_SCRATCH_OBJECTS * ALIGNMENT);
    FUZZ_CHECK(scratch != NULL);
    FUZZ_CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, secp256k1_pippenger_max_points(&ctx->error_callback, scratch), n_points) == 1);
    FUZZ_CHECK(n_batches == 1);
    FUZZ_CHECK(n_batch_points >= ECMULT_PIPPENGER_THRESHOLD);

    checkpoint = scratch->alloc_size;
    secp256k1_fuzz_ecmult_multi_repeat_reset_trace(&data);
    FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, g_sc, secp256k1_fuzz_ecmult_multi_repeat_callback, &data, n_points) == 1);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    secp256k1_fuzz_ecmult_multi_repeat_check_trace(&data, n_points);
    FUZZ_CHECK(secp256k1_gej_eq_var(&actual, &expected));

    /* Reject at every callback position in the Pippenger batch. */
    data.fail = 1;
    for (data.fail_at = 0; data.fail_at < n_points; data.fail_at++) {
        checkpoint = scratch->alloc_size;
        secp256k1_fuzz_ecmult_multi_repeat_reset_trace(&data);
        FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &secp256k1_scalar_one, secp256k1_fuzz_ecmult_multi_repeat_callback, &data, n_points) == 0);
        secp256k1_fuzz_ecmult_multi_repeat_check_failure_trace(&data, n_points);
        FUZZ_CHECK(scratch->alloc_size == checkpoint);
    }
    data.fail = 0;

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

static void secp256k1_fuzz_ecmult_multi_repeated_strauss(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    secp256k1_fuzz_ecmult_multi_repeat_data data;
    secp256k1_scratch *scratch;
    secp256k1_scalar total_sc;
    secp256k1_gej pointj;
    secp256k1_gej actual;
    secp256k1_gej expected;
    size_t checkpoint;
    size_t n_batches;
    size_t n_batch_points;
    const size_t n_points = 4;
    unsigned char scalar32[32];
    int overflow;

    memset(&data, 0, sizeof(data));
    secp256k1_fuzz_scalar32(scalar32, input, size, 487);
    secp256k1_scalar_set_b32(&data.sc, scalar32, &overflow);
    if (overflow) {
        secp256k1_scalar_clear(&data.sc);
    }
    secp256k1_fuzz_ecmult_multi_make_point(ctx, &data.pt, input, size, 491);

    /* This scratch size admits two Strauss points but not four, forcing two
     * batches while keeping the case small enough for every fuzz execution. */
    scratch = secp256k1_scratch_create(&ctx->error_callback, secp256k1_strauss_scratch_size(2) + STRAUSS_SCRATCH_OBJECTS * ALIGNMENT);
    FUZZ_CHECK(scratch != NULL);
    FUZZ_CHECK(secp256k1_pippenger_max_points(&ctx->error_callback, scratch) < ECMULT_PIPPENGER_THRESHOLD);
    FUZZ_CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, secp256k1_strauss_max_points(&ctx->error_callback, scratch), n_points) == 1);
    FUZZ_CHECK(n_batches == 2);
    FUZZ_CHECK(n_batch_points == 2);

    secp256k1_scalar_set_int(&total_sc, (unsigned int)n_points);
    secp256k1_scalar_mul(&total_sc, &total_sc, &data.sc);
    secp256k1_gej_set_ge(&pointj, &data.pt);
    secp256k1_ecmult(&expected, &pointj, &total_sc, &secp256k1_scalar_one);

    checkpoint = scratch->alloc_size;
    secp256k1_fuzz_ecmult_multi_repeat_reset_trace(&data);
    FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &secp256k1_scalar_one, secp256k1_fuzz_ecmult_multi_repeat_callback, &data, n_points) == 1);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    secp256k1_fuzz_ecmult_multi_repeat_check_trace(&data, n_points);
    FUZZ_CHECK(secp256k1_gej_eq_var(&actual, &expected));

    /* Reject from both the first and second Strauss batches. The latter is
     * distinct from the existing single-batch failure oracle. */
    data.fail = 1;
    for (data.fail_at = 0; data.fail_at < n_points; data.fail_at++) {
        checkpoint = scratch->alloc_size;
        secp256k1_fuzz_ecmult_multi_repeat_reset_trace(&data);
        FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &secp256k1_scalar_one, secp256k1_fuzz_ecmult_multi_repeat_callback, &data, n_points) == 0);
        secp256k1_fuzz_ecmult_multi_repeat_check_failure_trace(&data, n_points);
        FUZZ_CHECK(scratch->alloc_size == checkpoint);
    }
    data.fail = 0;

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

static void secp256k1_fuzz_ecmult_multi_empty(const secp256k1_context *ctx, size_t scratch_size, const secp256k1_scalar *g_sc, secp256k1_fuzz_ecmult_multi_data *data) {
    secp256k1_scratch *scratch = NULL;
    secp256k1_gej actual;
    secp256k1_gej expected;
    secp256k1_gej infinity;
    size_t checkpoint = 0;

    if (scratch_size != 0) {
        scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size);
        FUZZ_CHECK(scratch != NULL);
        checkpoint = scratch->alloc_size;
    }

    secp256k1_fuzz_ecmult_multi_reset_trace(data);
    FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, g_sc, secp256k1_fuzz_ecmult_multi_callback, data, 0) == 1);
    secp256k1_fuzz_ecmult_multi_check_success_trace(data, 0);
    if (scratch != NULL) {
        FUZZ_CHECK(scratch->alloc_size == checkpoint);
    }

    secp256k1_gej_set_infinity(&infinity);
    if (g_sc == NULL) {
        secp256k1_gej_set_infinity(&expected);
    } else {
        secp256k1_ecmult(&expected, &infinity, &secp256k1_scalar_zero, g_sc);
    }
    FUZZ_CHECK(secp256k1_gej_eq_var(&actual, &expected));

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

static void secp256k1_fuzz_ecmult_multi_fail_callback(const secp256k1_context *ctx, const secp256k1_scalar *g_sc, size_t n_points, secp256k1_fuzz_ecmult_multi_data *data) {
    secp256k1_scratch *scratch;
    secp256k1_gej r;
    size_t checkpoint;
    int ret;

    if (n_points == 0) {
        return;
    }
    data->fail = 1;
    data->fail_at %= n_points;
    secp256k1_fuzz_ecmult_multi_reset_trace(data);
    ret = secp256k1_ecmult_multi_var(&ctx->error_callback, NULL, &r, g_sc, secp256k1_fuzz_ecmult_multi_callback, data, n_points);
    FUZZ_CHECK(ret == 0);
    secp256k1_fuzz_ecmult_multi_check_failure_trace(data, n_points);

    scratch = secp256k1_scratch_create(&ctx->error_callback, 65536);
    FUZZ_CHECK(scratch != NULL);
    checkpoint = scratch->alloc_size;
    secp256k1_fuzz_ecmult_multi_reset_trace(data);
    ret = secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &r, g_sc, secp256k1_fuzz_ecmult_multi_callback, data, n_points);
    data->fail = 0;
    FUZZ_CHECK(ret == 0);
    secp256k1_fuzz_ecmult_multi_check_failure_trace(data, n_points);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

int LLVMFuzzerTestOneInput(const unsigned char *input, size_t size) {
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 211);
    secp256k1_fuzz_ecmult_multi_data data;
    secp256k1_scalar g_sc;
    unsigned char scalar32[32];
    const secp256k1_scalar *g_sc_ptr;
    size_t n_points;
    size_t i;
    int overflow;

    memset(&data, 0, sizeof(data));
    n_points = secp256k1_fuzz_byte(input, size, 3) % 9u;
    data.fail_at = secp256k1_fuzz_byte(input, size, 5);
    secp256k1_fuzz_check_ecmult_multi_batch_size_helper(input, size);
    secp256k1_fuzz_check_scratch_create_boundaries(ctx);
    secp256k1_fuzz_check_error_callback_routing(ctx);
    secp256k1_fuzz_check_error_callback_clone(ctx);

    secp256k1_fuzz_scalar32(scalar32, input, size, 223);
    secp256k1_scalar_set_b32(&g_sc, scalar32, &overflow);
    if (overflow) {
        secp256k1_scalar_clear(&g_sc);
    }
    g_sc_ptr = (secp256k1_fuzz_byte(input, size, 7) & 1u) ? &g_sc : NULL;

    for (i = 0; i < 8; i++) {
        secp256k1_fuzz_scalar32(scalar32, input, size, (unsigned int)(239 + i * 17u));
        secp256k1_scalar_set_b32(&data.sc[i], scalar32, &overflow);
        if (overflow) {
            secp256k1_scalar_clear(&data.sc[i]);
        }
        secp256k1_fuzz_ecmult_multi_make_point(ctx, &data.pt[i], input, size, (unsigned int)(359 + i * 19u));

        switch (secp256k1_fuzz_byte(input, size, 11 + i) & 3u) {
        case 0:
            if (i > 0) {
                data.pt[i] = data.pt[i - 1];
            }
            break;
        case 1:
            secp256k1_ge_set_infinity(&data.pt[i]);
            break;
        case 2:
            secp256k1_scalar_clear(&data.sc[i]);
            break;
        default:
            if (i > 0) {
                data.pt[i] = data.pt[i - 1];
                secp256k1_scalar_negate(&data.sc[i], &data.sc[i - 1]);
            }
            break;
        }
    }

    secp256k1_fuzz_ecmult_multi_compare(ctx, 0, g_sc_ptr, n_points, &data);
    secp256k1_fuzz_ecmult_multi_compare(ctx, 512, g_sc_ptr, n_points, &data);
    secp256k1_fuzz_ecmult_multi_compare(ctx, 4096, g_sc_ptr, n_points, &data);
    secp256k1_fuzz_ecmult_multi_compare(ctx, 65536, g_sc_ptr, n_points, &data);
    secp256k1_fuzz_ecmult_multi_empty(ctx, 0, NULL, &data);
    secp256k1_fuzz_ecmult_multi_empty(ctx, 4096, NULL, &data);
    secp256k1_fuzz_ecmult_multi_empty(ctx, 0, &secp256k1_scalar_one, &data);
    secp256k1_fuzz_ecmult_multi_empty(ctx, 4096, &secp256k1_scalar_one, &data);
    secp256k1_fuzz_ecmult_multi_empty(ctx, 4096, g_sc_ptr, &data);
    secp256k1_fuzz_ecmult_multi_fail_callback(ctx, g_sc_ptr, n_points, &data);
    secp256k1_fuzz_ecmult_multi_repeated_pippenger(ctx, g_sc_ptr, input, size);
    secp256k1_fuzz_ecmult_multi_repeated_strauss(ctx, input, size);

    secp256k1_context_destroy(ctx);
    return 0;
}
