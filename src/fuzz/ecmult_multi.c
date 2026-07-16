/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../secp256k1.c"
#include "fuzz.h"

#define SECP256K1_FUZZ_ECMULT_MULTI_DIRECT_MAX_POINTS 16
#define SECP256K1_FUZZ_ECMULT_MULTI_REPEAT_MAX_POINTS (2 * ECMULT_PIPPENGER_THRESHOLD)
#define SECP256K1_FUZZ_ECMULT_MULTI_DISTINCT_BATCH_MAX_POINTS (3 * ECMULT_PIPPENGER_THRESHOLD)

typedef struct {
    secp256k1_scalar sc[SECP256K1_FUZZ_ECMULT_MULTI_DIRECT_MAX_POINTS];
    secp256k1_ge pt[SECP256K1_FUZZ_ECMULT_MULTI_DIRECT_MAX_POINTS];
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
    secp256k1_scalar sc[SECP256K1_FUZZ_ECMULT_MULTI_DISTINCT_BATCH_MAX_POINTS];
    secp256k1_ge pt[SECP256K1_FUZZ_ECMULT_MULTI_DISTINCT_BATCH_MAX_POINTS];
    uint64_t seen[(SECP256K1_FUZZ_ECMULT_MULTI_DISTINCT_BATCH_MAX_POINTS + 63) / 64];
    size_t calls;
    size_t fail_at;
    int fail;
} secp256k1_fuzz_ecmult_multi_distinct_batch_data;

typedef struct {
    secp256k1_scalar *sc;
    secp256k1_ge *pt;
    unsigned char *seen;
    size_t n_points;
    size_t calls;
    size_t fail_at;
    int fail;
} secp256k1_fuzz_ecmult_multi_window_data;

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

static void secp256k1_fuzz_check_scratch_accounting_boundary(secp256k1_context *ctx) {
    secp256k1_fuzz_ecmult_multi_error_data error_data;
    secp256k1_scratch *scratch;
    unsigned char storage[8];
    unsigned char expected_storage[sizeof(storage)];
    unsigned int calls;

    scratch = malloc(sizeof(*scratch));
    FUZZ_CHECK(scratch != NULL);
    memset(scratch, 0, sizeof(*scratch));
    memcpy(scratch->magic, "scratch", sizeof(scratch->magic));
    scratch->data = storage;
    scratch->alloc_size = sizeof(storage) + 1;
    scratch->max_size = sizeof(storage);
    memset(storage, 0xA5, sizeof(storage));
    memcpy(expected_storage, storage, sizeof(expected_storage));

    error_data.self = &error_data;
    error_data.calls = 0;
    secp256k1_context_set_error_callback(ctx, secp256k1_fuzz_ecmult_multi_error_callback, &error_data);

    calls = error_data.calls;
    FUZZ_CHECK(secp256k1_scratch_checkpoint(&ctx->error_callback, scratch) == 0);
    FUZZ_CHECK(error_data.calls == calls + 1);
    FUZZ_CHECK(scratch->alloc_size == sizeof(storage) + 1);

    calls = error_data.calls;
    FUZZ_CHECK(secp256k1_scratch_max_allocation(&ctx->error_callback, scratch, 0) == 0);
    FUZZ_CHECK(error_data.calls == calls + 1);

    calls = error_data.calls;
    FUZZ_CHECK(secp256k1_scratch_alloc(&ctx->error_callback, scratch, 1) == NULL);
    FUZZ_CHECK(error_data.calls == calls + 1);
    FUZZ_CHECK(memcmp(storage, expected_storage, sizeof(storage)) == 0);

    calls = error_data.calls;
    secp256k1_scratch_apply_checkpoint(&ctx->error_callback, scratch, 0);
    FUZZ_CHECK(error_data.calls == calls + 1);
    FUZZ_CHECK(scratch->alloc_size == sizeof(storage) + 1);

    calls = error_data.calls;
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    FUZZ_CHECK(error_data.calls == calls + 1);
    free(scratch);
    secp256k1_context_set_error_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_scratch_invalid_checkpoint(secp256k1_context *ctx) {
    const size_t scratch_size = 64;
    const size_t allocation_size = 16;
    secp256k1_fuzz_ecmult_multi_error_data error_data;
    secp256k1_scratch *scratch;
    unsigned char *storage;
    unsigned char expected_storage[16];
    size_t checkpoint;
    unsigned int calls;

    scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size);
    FUZZ_CHECK(scratch != NULL);
    storage = (unsigned char *)secp256k1_scratch_alloc(&ctx->error_callback, scratch, allocation_size);
    FUZZ_CHECK(storage != NULL);
    memset(storage, 0xA5, allocation_size);
    memcpy(expected_storage, storage, sizeof(expected_storage));
    checkpoint = scratch->alloc_size;

    error_data.self = &error_data;
    error_data.calls = 0;
    secp256k1_context_set_error_callback(ctx, secp256k1_fuzz_ecmult_multi_error_callback, &error_data);

    calls = error_data.calls;
    secp256k1_scratch_apply_checkpoint(&ctx->error_callback, scratch, checkpoint + 1u);
    FUZZ_CHECK(error_data.calls == calls + 1);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(memcmp(storage, expected_storage, sizeof(expected_storage)) == 0);

    calls = error_data.calls;
    secp256k1_scratch_apply_checkpoint(&ctx->error_callback, scratch, SIZE_MAX);
    FUZZ_CHECK(error_data.calls == calls + 1);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(memcmp(storage, expected_storage, sizeof(expected_storage)) == 0);

    calls = error_data.calls;
    secp256k1_scratch_apply_checkpoint(&ctx->error_callback, scratch, 0);
    FUZZ_CHECK(error_data.calls == calls);
    FUZZ_CHECK(scratch->alloc_size == 0);
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    FUZZ_CHECK(error_data.calls == calls);
    secp256k1_context_set_error_callback(ctx, NULL, NULL);
}

static void secp256k1_fuzz_check_scratch_allocation_overflow(secp256k1_context *ctx) {
    const size_t scratch_size = 4096;
    const size_t allocation_size = 16;
    secp256k1_fuzz_ecmult_multi_error_data error_data;
    secp256k1_scratch *scratch;
    unsigned char *storage;
    unsigned char expected_storage[16];
    size_t checkpoint;
    unsigned int calls;

    scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size);
    FUZZ_CHECK(scratch != NULL);
    storage = (unsigned char *)secp256k1_scratch_alloc(&ctx->error_callback, scratch, allocation_size);
    FUZZ_CHECK(storage != NULL);
    memset(storage, 0xA5, allocation_size);
    memcpy(expected_storage, storage, sizeof(expected_storage));
    checkpoint = scratch->alloc_size;

    error_data.self = &error_data;
    error_data.calls = 0;
    secp256k1_context_set_error_callback(ctx, secp256k1_fuzz_ecmult_multi_error_callback, &error_data);

#if ALIGNMENT > 1
    FUZZ_CHECK(scratch->max_size - scratch->alloc_size > ALIGNMENT);
    calls = error_data.calls;
    FUZZ_CHECK(secp256k1_scratch_max_allocation(&ctx->error_callback, scratch, SIZE_MAX / (ALIGNMENT - 1u) + 1u) == 0);
    FUZZ_CHECK(error_data.calls == calls);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
#endif

    calls = error_data.calls;
    FUZZ_CHECK(secp256k1_scratch_alloc(&ctx->error_callback, scratch, SIZE_MAX) == NULL);
    FUZZ_CHECK(error_data.calls == calls);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(memcmp(storage, expected_storage, sizeof(expected_storage)) == 0);

    secp256k1_scratch_apply_checkpoint(&ctx->error_callback, scratch, 0);
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    secp256k1_context_set_error_callback(ctx, NULL, NULL);
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

static void secp256k1_fuzz_check_checked_size_mul_case(const secp256k1_callback *callback, secp256k1_fuzz_ecmult_multi_error_data *error_data, size_t a, size_t b) {
    const size_t sentinel = SIZE_MAX / 3u;
    const int overflow = b != 0 && a > SIZE_MAX / b;
    size_t result = sentinel;
    unsigned int calls = error_data->calls;
    int ret;

    ret = checked_size_mul(callback, &result, a, b);
    FUZZ_CHECK(ret == !overflow);
    FUZZ_CHECK(error_data->calls == calls + (unsigned int)overflow);
    if (overflow) {
        FUZZ_CHECK(result == sentinel);
    } else {
        FUZZ_CHECK(result == a * b);
    }
}

static void secp256k1_fuzz_check_checked_size_mul(secp256k1_context *ctx, const unsigned char *input, size_t size) {
    secp256k1_fuzz_ecmult_multi_error_data error_data;

    error_data.self = &error_data;
    error_data.calls = 0;
    secp256k1_context_set_error_callback(ctx, secp256k1_fuzz_ecmult_multi_error_callback, &error_data);

    secp256k1_fuzz_check_checked_size_mul_case(&ctx->error_callback, &error_data, 0, SIZE_MAX);
    secp256k1_fuzz_check_checked_size_mul_case(&ctx->error_callback, &error_data, SIZE_MAX, 0);
    secp256k1_fuzz_check_checked_size_mul_case(&ctx->error_callback, &error_data, SIZE_MAX, 1);
    secp256k1_fuzz_check_checked_size_mul_case(&ctx->error_callback, &error_data, SIZE_MAX / 2u, 2);
    secp256k1_fuzz_check_checked_size_mul_case(&ctx->error_callback, &error_data, SIZE_MAX / 2u + 1u, 2);
    secp256k1_fuzz_check_checked_size_mul_case(&ctx->error_callback, &error_data, SIZE_MAX, 2);
    secp256k1_fuzz_check_checked_size_mul_case(&ctx->error_callback, &error_data, secp256k1_fuzz_size_t(input, size, 547), secp256k1_fuzz_size_t(input, size, 563));

    secp256k1_context_set_error_callback(ctx, NULL, NULL);
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
    FUZZ_CHECK(n_points <= SECP256K1_FUZZ_ECMULT_MULTI_DIRECT_MAX_POINTS);
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

static void secp256k1_fuzz_ecmult_multi_check_failure_output(const secp256k1_gej *result) {
    /* A failed batch must not expose a partial accumulated result. */
    FUZZ_CHECK(secp256k1_gej_is_infinity(result));
}

static void secp256k1_fuzz_ecmult_multi_check_canonical_infinity(const secp256k1_gej *result) {
    secp256k1_fe zero;

    secp256k1_fe_set_int(&zero, 0);
    FUZZ_CHECK(result->infinity == 1);
    FUZZ_CHECK(memcmp(&result->x, &zero, sizeof(zero)) == 0);
    FUZZ_CHECK(memcmp(&result->y, &zero, sizeof(zero)) == 0);
    FUZZ_CHECK(memcmp(&result->z, &zero, sizeof(zero)) == 0);
}

static void secp256k1_fuzz_ecmult_multi_check_canonical_failure_output(const secp256k1_gej *result) {
    /* A failed batch must use the same canonical identity representation. */
    secp256k1_fuzz_ecmult_multi_check_canonical_infinity(result);
}

static int secp256k1_fuzz_ecmult_multi_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *cbdata);

static void secp256k1_fuzz_check_ecmult_multi_direct_allocation_failure(const secp256k1_context *ctx, const unsigned char *input, size_t size, secp256k1_ecmult_multi_func batch_fn, const secp256k1_scalar *g_sc, secp256k1_fuzz_ecmult_multi_data *data) {
    static const unsigned char trigger[] = "ecmult direct allocation failure\n";
    const secp256k1_scalar *g_sc_cases[2];
    size_t i;

    g_sc_cases[0] = NULL;
    g_sc_cases[1] = g_sc != NULL ? g_sc : &secp256k1_scalar_one;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    for (i = 0; i < sizeof(g_sc_cases) / sizeof(g_sc_cases[0]); i++) {
        secp256k1_scratch *scratch = secp256k1_scratch_create(&ctx->error_callback, 0);
        secp256k1_gej result;
        size_t checkpoint;

        FUZZ_CHECK(scratch != NULL);
        checkpoint = scratch->alloc_size;
        data->fail = 0;
        secp256k1_fuzz_ecmult_multi_reset_trace(data);
        secp256k1_gej_set_ge(&result, &secp256k1_ge_const_g);
        FUZZ_CHECK(batch_fn(&ctx->error_callback, scratch, &result, g_sc_cases[i], secp256k1_fuzz_ecmult_multi_callback, data, 1) == 0);
        FUZZ_CHECK(data->calls == 0);
        FUZZ_CHECK(data->seen_mask == 0);
        secp256k1_fuzz_ecmult_multi_check_failure_output(&result);
        FUZZ_CHECK(scratch->alloc_size == checkpoint);
        secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    }
}

static void secp256k1_fuzz_check_ecmult_multi_pippenger_second_allocation_failure(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "ecmult pippenger second allocation failure\n";
    const size_t n_points = 1;
    const size_t entries = 2 * n_points + 2;
    const size_t first_alloc = ROUND_TO_ALIGN(entries * sizeof(secp256k1_ge))
        + ROUND_TO_ALIGN(entries * sizeof(secp256k1_scalar))
        + ROUND_TO_ALIGN(sizeof(struct secp256k1_pippenger_state));
    const size_t second_alloc = ROUND_TO_ALIGN(entries * sizeof(struct secp256k1_pippenger_point_state));
    const secp256k1_scalar *g_sc_cases[2] = { NULL, &secp256k1_scalar_one };
    secp256k1_fuzz_ecmult_multi_data data;
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    /* Admit points, scalars, and state_space, but reject state_space->ps. */
    FUZZ_CHECK(first_alloc > 0);
    FUZZ_CHECK(second_alloc > 1);
    for (i = 0; i < sizeof(g_sc_cases) / sizeof(g_sc_cases[0]); i++) {
        secp256k1_scratch *scratch = secp256k1_scratch_create(&ctx->error_callback, first_alloc + second_alloc - 1);
        secp256k1_gej result;
        size_t checkpoint;

        FUZZ_CHECK(scratch != NULL);
        memset(&data, 0, sizeof(data));
        checkpoint = scratch->alloc_size;
        secp256k1_gej_set_ge(&result, &secp256k1_ge_const_g);
        FUZZ_CHECK(secp256k1_ecmult_pippenger_batch_single(&ctx->error_callback, scratch, &result, g_sc_cases[i], secp256k1_fuzz_ecmult_multi_callback, &data, n_points) == 0);
        FUZZ_CHECK(data.calls == 0);
        FUZZ_CHECK(data.seen_mask == 0);
        secp256k1_fuzz_ecmult_multi_check_canonical_failure_output(&result);
        FUZZ_CHECK(scratch->alloc_size == checkpoint);
        secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    }
}

static void secp256k1_fuzz_check_ecmult_multi_direct_empty_batch(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "ecmult direct empty batch\n";
    static const secp256k1_ecmult_multi_func batch_fns[2] = {
        secp256k1_ecmult_strauss_batch_single,
        secp256k1_ecmult_pippenger_batch_single
    };
    secp256k1_fuzz_ecmult_multi_data data;
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    for (i = 0; i < sizeof(batch_fns) / sizeof(batch_fns[0]); i++) {
        secp256k1_scratch *scratch = secp256k1_scratch_create(&ctx->error_callback, 0);
        secp256k1_gej result;
        size_t checkpoint;

        FUZZ_CHECK(scratch != NULL);
        memset(&data, 0, sizeof(data));
        checkpoint = scratch->alloc_size;
        secp256k1_gej_set_ge(&result, &secp256k1_ge_const_g);
        FUZZ_CHECK(batch_fns[i](&ctx->error_callback, scratch, &result, NULL, secp256k1_fuzz_ecmult_multi_callback, &data, 0) == 1);
        FUZZ_CHECK(data.calls == 0);
        FUZZ_CHECK(data.seen_mask == 0);
        secp256k1_fuzz_ecmult_multi_check_canonical_infinity(&result);
        FUZZ_CHECK(scratch->alloc_size == checkpoint);
        secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    }
}

static int secp256k1_fuzz_ecmult_multi_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *cbdata) {
    secp256k1_fuzz_ecmult_multi_data *data = (secp256k1_fuzz_ecmult_multi_data *)cbdata;
    FUZZ_CHECK(idx < SECP256K1_FUZZ_ECMULT_MULTI_DIRECT_MAX_POINTS);
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

static void secp256k1_fuzz_ecmult_multi_distinct_batch_reset_trace(secp256k1_fuzz_ecmult_multi_distinct_batch_data *data) {
    memset(data->seen, 0, sizeof(data->seen));
    data->calls = 0;
}

static void secp256k1_fuzz_ecmult_multi_distinct_batch_check_trace(const secp256k1_fuzz_ecmult_multi_distinct_batch_data *data, size_t n_points) {
    size_t i;

    FUZZ_CHECK(data->calls == n_points);
    FUZZ_CHECK(n_points <= SECP256K1_FUZZ_ECMULT_MULTI_DISTINCT_BATCH_MAX_POINTS);
    for (i = 0; i < n_points; i++) {
        FUZZ_CHECK((data->seen[i / 64] & ((uint64_t)1 << (i % 64))) != 0);
    }
}

static void secp256k1_fuzz_ecmult_multi_distinct_batch_check_failure_trace(const secp256k1_fuzz_ecmult_multi_distinct_batch_data *data, size_t n_points) {
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

static int secp256k1_fuzz_ecmult_multi_distinct_batch_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *cbdata) {
    secp256k1_fuzz_ecmult_multi_distinct_batch_data *data = (secp256k1_fuzz_ecmult_multi_distinct_batch_data *)cbdata;
    uint64_t bit;

    FUZZ_CHECK(idx < SECP256K1_FUZZ_ECMULT_MULTI_DISTINCT_BATCH_MAX_POINTS);
    bit = (uint64_t)1 << (idx % 64);
    FUZZ_CHECK((data->seen[idx / 64] & bit) == 0);
    data->seen[idx / 64] |= bit;
    data->calls++;
    if (data->fail && idx == data->fail_at) {
        return 0;
    }
    *sc = data->sc[idx];
    *pt = data->pt[idx];
    return 1;
}

static int secp256k1_fuzz_ecmult_multi_window_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *cbdata) {
    secp256k1_fuzz_ecmult_multi_window_data *data = (secp256k1_fuzz_ecmult_multi_window_data *)cbdata;

    FUZZ_CHECK(idx < data->n_points);
    FUZZ_CHECK(data->seen[idx] == 0);
    data->seen[idx] = 1;
    data->calls++;
    if (data->fail && idx == data->fail_at) {
        return 0;
    }
    *sc = data->sc[idx];
    *pt = data->pt[idx];
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

/* Keep multi-scalar result checks independent from gej_eq_var. That helper
 * implements equality by adding points, so using it as the only oracle can
 * mask a regression shared by the arithmetic and equality paths. */
static int secp256k1_fuzz_ecmult_multi_gej_equal_independent(const secp256k1_gej *a, const secp256k1_gej *b) {
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

static void secp256k1_fuzz_ecmult_multi_check_result(const secp256k1_gej *actual, const secp256k1_gej *expected) {
    FUZZ_CHECK(secp256k1_fuzz_ecmult_multi_gej_equal_independent(actual, expected));
    FUZZ_CHECK(secp256k1_gej_eq_var(actual, expected));
}

static void secp256k1_fuzz_ecmult_multi_distinct_batch_reference(secp256k1_gej *expected, const secp256k1_scalar *g_sc, const secp256k1_fuzz_ecmult_multi_distinct_batch_data *data) {
    secp256k1_gej term;
    size_t i;

    if (g_sc == NULL) {
        secp256k1_gej_set_infinity(expected);
    } else {
        secp256k1_ecmult_const(expected, &secp256k1_ge_const_g, g_sc);
    }
    for (i = 0; i < SECP256K1_FUZZ_ECMULT_MULTI_DISTINCT_BATCH_MAX_POINTS; i++) {
        secp256k1_ecmult_const(&term, &data->pt[i], &data->sc[i]);
        secp256k1_gej_add_var(expected, expected, &term, NULL);
    }
}

/* Exercise three Pippenger batches with distinct callback state. The existing
 * large-count fixtures repeat one point, so a batch-boundary bug that drops or
 * misroutes a distinct tail point could agree with their scalar-only model. */
static void secp256k1_fuzz_ecmult_multi_distinct_pippenger_batches(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "distinct ecmult pippenger batches\n";
    const size_t n_points = SECP256K1_FUZZ_ECMULT_MULTI_DISTINCT_BATCH_MAX_POINTS;
    const size_t scratch_points = ECMULT_PIPPENGER_THRESHOLD;
    secp256k1_fuzz_ecmult_multi_distinct_batch_data data;
    secp256k1_scalar generator_sc;
    secp256k1_scalar point_sc;
    secp256k1_scratch *scratch;
    secp256k1_gej actual;
    secp256k1_gej expected;
    size_t n_batches;
    size_t n_batch_points;
    size_t max_points;
    size_t checkpoint;
    size_t failure_positions[2];
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    memset(&data, 0, sizeof(data));
    secp256k1_scalar_set_int(&generator_sc, 17);
    for (i = 0; i < n_points; i++) {
        secp256k1_scalar_set_int(&data.sc[i], (unsigned int)(2 * i + 1));
        secp256k1_scalar_set_int(&point_sc, (unsigned int)(i + 1));
        secp256k1_ecmult_gen_ge(&ctx->ecmult_gen_ctx, &data.pt[i], &point_sc);
    }

    scratch = secp256k1_scratch_create(&ctx->error_callback,
        secp256k1_pippenger_scratch_size(scratch_points, secp256k1_pippenger_bucket_window(scratch_points))
        + PIPPENGER_SCRATCH_OBJECTS * ALIGNMENT);
    FUZZ_CHECK(scratch != NULL);
    max_points = secp256k1_pippenger_max_points(&ctx->error_callback, scratch);
    FUZZ_CHECK(max_points >= scratch_points);
    FUZZ_CHECK(max_points < n_points);
    FUZZ_CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_points, n_points) == 1);
    FUZZ_CHECK(n_batches == 3);
    FUZZ_CHECK(n_batch_points >= ECMULT_PIPPENGER_THRESHOLD);
    FUZZ_CHECK(n_batch_points < n_points);

    secp256k1_fuzz_ecmult_multi_distinct_batch_reference(&expected, &generator_sc, &data);
    checkpoint = scratch->alloc_size;
    secp256k1_fuzz_ecmult_multi_distinct_batch_reset_trace(&data);
    FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &generator_sc, secp256k1_fuzz_ecmult_multi_distinct_batch_callback, &data, n_points) == 1);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    secp256k1_fuzz_ecmult_multi_distinct_batch_check_trace(&data, n_points);
    secp256k1_fuzz_ecmult_multi_check_result(&actual, &expected);

    failure_positions[0] = n_batch_points - 1;
    failure_positions[1] = n_batch_points;
    data.fail = 1;
    for (i = 0; i < sizeof(failure_positions) / sizeof(failure_positions[0]); i++) {
        data.fail_at = failure_positions[i];
        checkpoint = scratch->alloc_size;
        secp256k1_fuzz_ecmult_multi_distinct_batch_reset_trace(&data);
        FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &generator_sc, secp256k1_fuzz_ecmult_multi_distinct_batch_callback, &data, n_points) == 0);
        secp256k1_fuzz_ecmult_multi_distinct_batch_check_failure_trace(&data, n_points);
        secp256k1_fuzz_ecmult_multi_check_failure_output(&actual);
        FUZZ_CHECK(scratch->alloc_size == checkpoint);
    }
    data.fail = 0;

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

/* The endomorphism-aware schedule intentionally jumps from window 7 to 9.
 * Keep the first point count after that jump explicit so the large bucket
 * allocation and its callback rollback path cannot disappear from coverage. */
static void secp256k1_fuzz_ecmult_multi_pippenger_window_boundary(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "pippenger window 1261\n";
    const size_t n_points = 1261;
    secp256k1_fuzz_ecmult_multi_window_data data;
    secp256k1_scalar generator_sc;
    secp256k1_scalar point_sc;
    secp256k1_scratch *scratch;
    secp256k1_gej actual;
    secp256k1_gej expected;
    size_t scratch_size;
    size_t checkpoint;
    size_t i;
    int bucket_window;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    memset(&data, 0, sizeof(data));
    data.n_points = n_points;
    data.sc = (secp256k1_scalar *)malloc(n_points * sizeof(*data.sc));
    data.pt = (secp256k1_ge *)malloc(n_points * sizeof(*data.pt));
    data.seen = (unsigned char *)calloc(n_points, sizeof(*data.seen));
    FUZZ_CHECK(data.sc != NULL);
    FUZZ_CHECK(data.pt != NULL);
    FUZZ_CHECK(data.seen != NULL);

    secp256k1_scalar_set_int(&generator_sc, 17);
    for (i = 0; i < n_points; i++) {
        secp256k1_scalar_set_int(&data.sc[i], (unsigned int)(2 * i + 1));
        secp256k1_scalar_set_int(&point_sc, (unsigned int)(i + 1));
        secp256k1_ecmult_gen_ge(&ctx->ecmult_gen_ctx, &data.pt[i], &point_sc);
    }

    bucket_window = secp256k1_pippenger_bucket_window(n_points);
    FUZZ_CHECK(bucket_window == 9);
    FUZZ_CHECK(secp256k1_pippenger_bucket_window(n_points - 1) == 7);
    FUZZ_CHECK(secp256k1_pippenger_bucket_window_inv(bucket_window) == 4420);
    scratch_size = secp256k1_pippenger_scratch_size(n_points, bucket_window);
    scratch = secp256k1_scratch_create(&ctx->error_callback,
        scratch_size + PIPPENGER_SCRATCH_OBJECTS * ALIGNMENT);
    FUZZ_CHECK(scratch != NULL);
    FUZZ_CHECK(secp256k1_pippenger_max_points(&ctx->error_callback, scratch) >= n_points);
    checkpoint = scratch->alloc_size;

    secp256k1_gej_set_infinity(&expected);
    secp256k1_ecmult_const(&expected, &secp256k1_ge_const_g, &generator_sc);
    for (i = 0; i < n_points; i++) {
        secp256k1_gej term;
        secp256k1_ecmult_const(&term, &data.pt[i], &data.sc[i]);
        secp256k1_gej_add_var(&expected, &expected, &term, NULL);
    }

    FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &generator_sc,
        secp256k1_fuzz_ecmult_multi_window_callback, &data, n_points) == 1);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(data.calls == n_points);
    for (i = 0; i < n_points; i++) {
        FUZZ_CHECK(data.seen[i] != 0);
    }
    secp256k1_fuzz_ecmult_multi_check_result(&actual, &expected);

    data.fail = 1;
    data.fail_at = n_points - 1;
    memset(data.seen, 0, n_points * sizeof(*data.seen));
    data.calls = 0;
    checkpoint = scratch->alloc_size;
    FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &generator_sc,
        secp256k1_fuzz_ecmult_multi_window_callback, &data, n_points) == 0);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(data.calls == n_points);
    secp256k1_fuzz_ecmult_multi_check_failure_output(&actual);

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    free(data.seen);
    free(data.pt);
    free(data.sc);
}

/* The first window-10 batch is a separate arithmetic boundary from the
 * window-9 case above. Repeated generator terms keep the independent model
 * cheap enough to run while still exercising the larger bucket table. */
static void secp256k1_fuzz_ecmult_multi_pippenger_window_10(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "pippenger window 4421\n";
    const size_t n_points = 4421;
    secp256k1_fuzz_ecmult_multi_window_data data;
    secp256k1_scalar generator_sc;
    secp256k1_scalar point_sc;
    secp256k1_scalar total_sc;
    secp256k1_scratch *scratch;
    secp256k1_gej actual;
    secp256k1_gej expected;
    size_t scratch_size;
    size_t checkpoint;
    size_t i;
    int overflow;
    int bucket_window;
    unsigned char point_scalar32[32] = { 0 };

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    memset(&data, 0, sizeof(data));
    data.n_points = n_points;
    data.sc = (secp256k1_scalar *)malloc(n_points * sizeof(*data.sc));
    data.pt = (secp256k1_ge *)malloc(n_points * sizeof(*data.pt));
    data.seen = (unsigned char *)calloc(n_points, sizeof(*data.seen));
    FUZZ_CHECK(data.sc != NULL);
    FUZZ_CHECK(data.pt != NULL);
    FUZZ_CHECK(data.seen != NULL);

    secp256k1_scalar_set_int(&generator_sc, 17);
    point_scalar32[24] = 0x10;
    point_scalar32[31] = 1;
    secp256k1_scalar_set_b32(&point_sc, point_scalar32, &overflow);
    FUZZ_CHECK(overflow == 0);
    for (i = 0; i < n_points; i++) {
        data.sc[i] = point_sc;
        data.pt[i] = secp256k1_ge_const_g;
    }
    secp256k1_scalar_set_int(&total_sc, (unsigned int)n_points);
    secp256k1_scalar_mul(&total_sc, &total_sc, &point_sc);
    secp256k1_scalar_add(&total_sc, &total_sc, &generator_sc);
    secp256k1_ecmult_const(&expected, &secp256k1_ge_const_g, &total_sc);

    bucket_window = secp256k1_pippenger_bucket_window(n_points);
    FUZZ_CHECK(bucket_window == 10);
    FUZZ_CHECK(secp256k1_pippenger_bucket_window(n_points - 1) == 9);
    FUZZ_CHECK(secp256k1_pippenger_bucket_window_inv(bucket_window) == 7880);
    scratch_size = secp256k1_pippenger_scratch_size(n_points, bucket_window);
    scratch = secp256k1_scratch_create(&ctx->error_callback,
        scratch_size + PIPPENGER_SCRATCH_OBJECTS * ALIGNMENT);
    FUZZ_CHECK(scratch != NULL);
    FUZZ_CHECK(secp256k1_pippenger_max_points(&ctx->error_callback, scratch) >= n_points);
    checkpoint = scratch->alloc_size;

    FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &generator_sc,
        secp256k1_fuzz_ecmult_multi_window_callback, &data, n_points) == 1);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(data.calls == n_points);
    for (i = 0; i < n_points; i++) {
        FUZZ_CHECK(data.seen[i] != 0);
    }
    secp256k1_fuzz_ecmult_multi_check_result(&actual, &expected);

    data.fail = 1;
    data.fail_at = n_points - 1;
    memset(data.seen, 0, n_points * sizeof(*data.seen));
    data.calls = 0;
    checkpoint = scratch->alloc_size;
    FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &generator_sc,
        secp256k1_fuzz_ecmult_multi_window_callback, &data, n_points) == 0);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(data.calls == n_points);
    secp256k1_fuzz_ecmult_multi_check_failure_output(&actual);

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
    free(data.seen);
    free(data.pt);
    free(data.sc);
}

static void secp256k1_fuzz_ecmult_multi_check_equality_barrier(void) {
    secp256k1_gej generator;
    secp256k1_gej infinity;

    secp256k1_gej_set_ge(&generator, &secp256k1_ge_const_g);
    secp256k1_gej_set_infinity(&infinity);
    FUZZ_CHECK(!secp256k1_fuzz_ecmult_multi_gej_equal_independent(&generator, &infinity));
    FUZZ_CHECK(!secp256k1_gej_eq_var(&generator, &infinity));
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
        secp256k1_fuzz_ecmult_multi_check_result(&no_scratch, &expected);
        secp256k1_fuzz_ecmult_multi_check_result(&no_scratch, &with_scratch);
        secp256k1_fuzz_ecmult_multi_check_result(&with_scratch, &expected);
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
    secp256k1_fuzz_ecmult_multi_check_result(&actual, &expected);

    /* Reject at every callback position in the Pippenger batch. */
    data.fail = 1;
    for (data.fail_at = 0; data.fail_at < n_points; data.fail_at++) {
        checkpoint = scratch->alloc_size;
        secp256k1_fuzz_ecmult_multi_repeat_reset_trace(&data);
        FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &secp256k1_scalar_one, secp256k1_fuzz_ecmult_multi_repeat_callback, &data, n_points) == 0);
        secp256k1_fuzz_ecmult_multi_repeat_check_failure_trace(&data, n_points);
        secp256k1_fuzz_ecmult_multi_check_failure_output(&actual);
        FUZZ_CHECK(scratch->alloc_size == checkpoint);
    }
    data.fail = 0;

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

/* Pippenger drops zero-scalar and infinity-point terms before building its
 * WNAF state. Keep the all-filtered identity return explicit. */
static void secp256k1_fuzz_ecmult_multi_all_filtered_pippenger(const secp256k1_context *ctx, const unsigned char *input, size_t size) {
    static const unsigned char trigger[] = "pippenger all filtered terms\n";
    const size_t n_points = ECMULT_PIPPENGER_THRESHOLD;
    const secp256k1_scalar *generator_scalars[2];
    secp256k1_fuzz_ecmult_multi_repeat_data data;
    secp256k1_scratch *scratch;
    secp256k1_gej actual;
    size_t scratch_size;
    size_t checkpoint;
    size_t n_batches;
    size_t n_batch_points;
    int results[2];
    int canonical_identity[2];
    size_t i;

    if (size != sizeof(trigger) - 1 || memcmp(input, trigger, sizeof(trigger) - 1) != 0) {
        return;
    }

    memset(&data, 0, sizeof(data));
    generator_scalars[0] = NULL;
    generator_scalars[1] = &secp256k1_scalar_zero;
    scratch_size = secp256k1_pippenger_scratch_size(n_points, secp256k1_pippenger_bucket_window(n_points));
    scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size + PIPPENGER_SCRATCH_OBJECTS * ALIGNMENT);
    FUZZ_CHECK(scratch != NULL);
    FUZZ_CHECK(secp256k1_pippenger_max_points(&ctx->error_callback, scratch) >= n_points);
    FUZZ_CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, secp256k1_pippenger_max_points(&ctx->error_callback, scratch), n_points) == 1);
    FUZZ_CHECK(n_batches == 1);
    FUZZ_CHECK(n_batch_points >= ECMULT_PIPPENGER_THRESHOLD);

    for (i = 0; i < sizeof(generator_scalars) / sizeof(generator_scalars[0]); i++) {
        checkpoint = scratch->alloc_size;
        secp256k1_fuzz_ecmult_multi_repeat_reset_trace(&data);
        if (i == 0) {
            secp256k1_scalar_set_int(&data.sc, 0);
            data.pt = secp256k1_ge_const_g;
        } else {
            secp256k1_scalar_set_int(&data.sc, 1);
            secp256k1_ge_set_infinity(&data.pt);
        }
        results[i] = secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, generator_scalars[i], secp256k1_fuzz_ecmult_multi_repeat_callback, &data, n_points);
        FUZZ_CHECK(scratch->alloc_size == checkpoint);
        secp256k1_fuzz_ecmult_multi_repeat_check_trace(&data, n_points);
        canonical_identity[i] = secp256k1_gej_is_infinity(&actual)
            && secp256k1_fe_is_zero(&actual.x)
            && secp256k1_fe_is_zero(&actual.y)
            && secp256k1_fe_is_zero(&actual.z);
    }
    FUZZ_CHECK(results[0] == 1);
    FUZZ_CHECK(results[1] == 1);
    FUZZ_CHECK(canonical_identity[0]);
    FUZZ_CHECK(canonical_identity[1]);

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

static void secp256k1_fuzz_ecmult_multi_repeated_pippenger_batches(const secp256k1_context *ctx, const secp256k1_scalar *g_sc, const unsigned char *input, size_t size) {
    secp256k1_fuzz_ecmult_multi_repeat_data data;
    secp256k1_scratch *scratch;
    secp256k1_scalar n_sc;
    secp256k1_scalar total_sc;
    secp256k1_gej pointj;
    secp256k1_gej actual;
    secp256k1_gej expected;
    size_t n_batches;
    size_t n_batch_points;
    size_t checkpoint;
    size_t max_points;
    size_t failure_positions[4];
    size_t i;
    const size_t n_points = SECP256K1_FUZZ_ECMULT_MULTI_REPEAT_MAX_POINTS;
    const size_t scratch_points = ECMULT_PIPPENGER_THRESHOLD;
    unsigned char scalar32[32];
    int overflow;

    memset(&data, 0, sizeof(data));
    secp256k1_fuzz_scalar32(scalar32, input, size, 523);
    secp256k1_scalar_set_b32(&data.sc, scalar32, &overflow);
    if (overflow || secp256k1_scalar_is_zero(&data.sc)) {
        secp256k1_scalar_set_int(&data.sc, 1);
    }
    secp256k1_fuzz_ecmult_multi_make_point(ctx, &data.pt, input, size, 541);

    secp256k1_scalar_set_int(&n_sc, (unsigned int)n_points);
    secp256k1_scalar_mul(&total_sc, &data.sc, &n_sc);
    secp256k1_gej_set_ge(&pointj, &data.pt);
    secp256k1_ecmult(&expected, &pointj, &total_sc, g_sc);

    scratch = secp256k1_scratch_create(&ctx->error_callback,
        secp256k1_pippenger_scratch_size(scratch_points, secp256k1_pippenger_bucket_window(scratch_points))
        + PIPPENGER_SCRATCH_OBJECTS * ALIGNMENT);
    FUZZ_CHECK(scratch != NULL);
    max_points = secp256k1_pippenger_max_points(&ctx->error_callback, scratch);
    FUZZ_CHECK(max_points >= scratch_points);
    FUZZ_CHECK(max_points < n_points);
    FUZZ_CHECK(secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, max_points, n_points) == 1);
    FUZZ_CHECK(n_batches == 2);
    FUZZ_CHECK(n_batch_points >= ECMULT_PIPPENGER_THRESHOLD);
    FUZZ_CHECK(n_batch_points < n_points);

    checkpoint = scratch->alloc_size;
    secp256k1_fuzz_ecmult_multi_repeat_reset_trace(&data);
    FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, g_sc, secp256k1_fuzz_ecmult_multi_repeat_callback, &data, n_points) == 1);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    secp256k1_fuzz_ecmult_multi_repeat_check_trace(&data, n_points);
    secp256k1_fuzz_ecmult_multi_check_result(&actual, &expected);

    /* Pin callback offsets at both sides of the batch boundary without
     * repeating the full failure matrix already covered by the one-batch case. */
    failure_positions[0] = 0;
    failure_positions[1] = n_batch_points - 1;
    failure_positions[2] = n_batch_points;
    failure_positions[3] = n_points - 1;
    data.fail = 1;
    for (i = 0; i < sizeof(failure_positions) / sizeof(failure_positions[0]); i++) {
        data.fail_at = failure_positions[i];
        checkpoint = scratch->alloc_size;
        secp256k1_fuzz_ecmult_multi_repeat_reset_trace(&data);
        FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &secp256k1_scalar_one, secp256k1_fuzz_ecmult_multi_repeat_callback, &data, n_points) == 0);
        secp256k1_fuzz_ecmult_multi_repeat_check_failure_trace(&data, n_points);
        secp256k1_fuzz_ecmult_multi_check_failure_output(&actual);
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
        secp256k1_scalar_set_int(&data.sc, 0);
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
    secp256k1_fuzz_ecmult_multi_check_result(&actual, &expected);

    /* Reject from both the first and second Strauss batches. The latter is
     * distinct from the existing single-batch failure oracle. */
    data.fail = 1;
    for (data.fail_at = 0; data.fail_at < n_points; data.fail_at++) {
        checkpoint = scratch->alloc_size;
        secp256k1_fuzz_ecmult_multi_repeat_reset_trace(&data);
        FUZZ_CHECK(secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &actual, &secp256k1_scalar_one, secp256k1_fuzz_ecmult_multi_repeat_callback, &data, n_points) == 0);
        secp256k1_fuzz_ecmult_multi_repeat_check_failure_trace(&data, n_points);
        secp256k1_fuzz_ecmult_multi_check_failure_output(&actual);
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
    secp256k1_fuzz_ecmult_multi_check_result(&actual, &expected);

    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

static void secp256k1_fuzz_ecmult_multi_fail_callback(const secp256k1_context *ctx, const secp256k1_scalar *g_sc, size_t n_points, secp256k1_fuzz_ecmult_multi_data *data) {
    secp256k1_scratch *scratch;
    secp256k1_gej r;
    unsigned char *prefix;
    unsigned char expected_prefix[32];
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
    secp256k1_fuzz_ecmult_multi_check_failure_output(&r);

    scratch = secp256k1_scratch_create(&ctx->error_callback, 65536);
    FUZZ_CHECK(scratch != NULL);
    prefix = (unsigned char *)secp256k1_scratch_alloc(&ctx->error_callback, scratch, sizeof(expected_prefix));
    FUZZ_CHECK(prefix != NULL);
    memset(prefix, 0xA5, sizeof(expected_prefix));
    memset(expected_prefix, 0xA5, sizeof(expected_prefix));
    checkpoint = scratch->alloc_size;
    secp256k1_fuzz_ecmult_multi_reset_trace(data);
    ret = secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &r, g_sc, secp256k1_fuzz_ecmult_multi_callback, data, n_points);
    data->fail = 0;
    FUZZ_CHECK(ret == 0);
    secp256k1_fuzz_ecmult_multi_check_failure_trace(data, n_points);
    secp256k1_fuzz_ecmult_multi_check_failure_output(&r);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(memcmp(prefix, expected_prefix, sizeof(expected_prefix)) == 0);
    secp256k1_scratch_apply_checkpoint(&ctx->error_callback, scratch, 0);
    secp256k1_scratch_destroy(&ctx->error_callback, scratch);
}

int LLVMFuzzerTestOneInput(const unsigned char *input, size_t size) {
    static const unsigned char sixteen_trigger[] = "sixteen direct ecmult batch\n";
    static const unsigned char scratch_trigger[] = "scratch invalid checkpoint boundaries\n";
    static const unsigned char scratch_overflow_trigger[] = "scratch allocation overflow boundaries\n";
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 211);
    secp256k1_fuzz_ecmult_multi_data data;
    secp256k1_scalar g_sc;
    unsigned char scalar32[32];
    const secp256k1_scalar *g_sc_ptr;
    size_t n_points;
    size_t initialized_points;
    size_t i;
    int overflow;
    int sixteen_case;
    int scratch_case;
    int scratch_overflow_case;

    memset(&data, 0, sizeof(data));
    secp256k1_fuzz_ecmult_multi_check_equality_barrier();
    sixteen_case = size == sizeof(sixteen_trigger) - 1 && memcmp(input, sixteen_trigger, sizeof(sixteen_trigger) - 1) == 0;
    scratch_case = size == sizeof(scratch_trigger) - 1 && memcmp(input, scratch_trigger, sizeof(scratch_trigger) - 1) == 0;
    scratch_overflow_case = size == sizeof(scratch_overflow_trigger) - 1 && memcmp(input, scratch_overflow_trigger, sizeof(scratch_overflow_trigger) - 1) == 0;
    n_points = sixteen_case ? SECP256K1_FUZZ_ECMULT_MULTI_DIRECT_MAX_POINTS : secp256k1_fuzz_byte(input, size, 3) % 9u;
    initialized_points = n_points < 8 ? 8 : n_points;
    data.fail_at = secp256k1_fuzz_byte(input, size, 5);
    secp256k1_fuzz_check_ecmult_multi_batch_size_helper(input, size);
    secp256k1_fuzz_check_scratch_create_boundaries(ctx);
    secp256k1_fuzz_check_checked_size_mul(ctx, input, size);
    secp256k1_fuzz_check_scratch_accounting_boundary(ctx);
    secp256k1_fuzz_check_error_callback_routing(ctx);
    secp256k1_fuzz_check_error_callback_clone(ctx);
    if (scratch_case) {
        secp256k1_fuzz_check_scratch_invalid_checkpoint(ctx);
    }
    if (scratch_overflow_case) {
        secp256k1_fuzz_check_scratch_allocation_overflow(ctx);
    }

    secp256k1_fuzz_scalar32(scalar32, input, size, 223);
    secp256k1_scalar_set_b32(&g_sc, scalar32, &overflow);
    if (overflow) {
        secp256k1_scalar_set_int(&g_sc, 0);
    }
    if (sixteen_case) {
        secp256k1_scalar_set_int(&g_sc, 17);
        g_sc_ptr = &g_sc;
    } else {
        g_sc_ptr = (secp256k1_fuzz_byte(input, size, 7) & 1u) ? &g_sc : NULL;
    }

    for (i = 0; i < initialized_points; i++) {
        if (sixteen_case) {
            /* Fixed nonzero scalars make every direct callback entry distinct. */
            secp256k1_scalar_set_int(&data.sc[i], (unsigned int)(i + 1));
            secp256k1_ecmult_gen_ge(&ctx->ecmult_gen_ctx, &data.pt[i], &data.sc[i]);
        } else {
            secp256k1_fuzz_scalar32(scalar32, input, size, (unsigned int)(239 + i * 17u));
            secp256k1_scalar_set_b32(&data.sc[i], scalar32, &overflow);
            if (overflow) {
                secp256k1_scalar_set_int(&data.sc[i], 0);
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
                /* This is a logical zero used by later arithmetic, not secret-state cleanup. */
                secp256k1_scalar_set_int(&data.sc[i], 0);
                break;
            default:
                if (i > 0) {
                    data.pt[i] = data.pt[i - 1];
                    secp256k1_scalar_negate(&data.sc[i], &data.sc[i - 1]);
                }
                break;
            }
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
    secp256k1_fuzz_check_ecmult_multi_direct_allocation_failure(ctx, input, size, secp256k1_ecmult_strauss_batch_single, g_sc_ptr, &data);
    secp256k1_fuzz_check_ecmult_multi_direct_allocation_failure(ctx, input, size, secp256k1_ecmult_pippenger_batch_single, g_sc_ptr, &data);
    secp256k1_fuzz_check_ecmult_multi_direct_empty_batch(ctx, input, size);
    secp256k1_fuzz_check_ecmult_multi_pippenger_second_allocation_failure(ctx, input, size);
    secp256k1_fuzz_ecmult_multi_repeated_pippenger(ctx, g_sc_ptr, input, size);
    secp256k1_fuzz_ecmult_multi_all_filtered_pippenger(ctx, input, size);
    secp256k1_fuzz_ecmult_multi_repeated_pippenger_batches(ctx, g_sc_ptr, input, size);
    secp256k1_fuzz_ecmult_multi_distinct_pippenger_batches(ctx, input, size);
    secp256k1_fuzz_ecmult_multi_pippenger_window_boundary(ctx, input, size);
    secp256k1_fuzz_ecmult_multi_pippenger_window_10(ctx, input, size);
    secp256k1_fuzz_ecmult_multi_repeated_strauss(ctx, input, size);

    secp256k1_context_destroy(ctx);
    return 0;
}
