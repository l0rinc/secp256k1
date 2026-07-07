/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "../secp256k1.c"
#include "fuzz.h"

typedef struct {
    secp256k1_scalar sc[8];
    secp256k1_ge pt[8];
    int fail;
    size_t fail_at;
} secp256k1_fuzz_ecmult_multi_data;

static int secp256k1_fuzz_ecmult_multi_callback(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *cbdata) {
    secp256k1_fuzz_ecmult_multi_data *data = (secp256k1_fuzz_ecmult_multi_data *)cbdata;
    FUZZ_CHECK(idx < 8);
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

static void secp256k1_fuzz_ecmult_multi_compare(const secp256k1_context *ctx, size_t scratch_size, const secp256k1_scalar *g_sc, size_t n_points, secp256k1_fuzz_ecmult_multi_data *data) {
    secp256k1_scratch *scratch;
    secp256k1_gej no_scratch;
    secp256k1_gej with_scratch;
    size_t checkpoint;
    int no_scratch_ret;
    int with_scratch_ret;

    scratch = secp256k1_scratch_create(&ctx->error_callback, scratch_size);
    FUZZ_CHECK(scratch != NULL);
    checkpoint = scratch->alloc_size;

    no_scratch_ret = secp256k1_ecmult_multi_var(&ctx->error_callback, NULL, &no_scratch, g_sc, secp256k1_fuzz_ecmult_multi_callback, data, n_points);
    with_scratch_ret = secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &with_scratch, g_sc, secp256k1_fuzz_ecmult_multi_callback, data, n_points);
    FUZZ_CHECK(scratch->alloc_size == checkpoint);
    FUZZ_CHECK(no_scratch_ret == with_scratch_ret);
    if (with_scratch_ret) {
        FUZZ_CHECK(secp256k1_gej_eq_var(&no_scratch, &with_scratch));
    }

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
    scratch = secp256k1_scratch_create(&ctx->error_callback, 65536);
    FUZZ_CHECK(scratch != NULL);
    checkpoint = scratch->alloc_size;
    data->fail = 1;
    data->fail_at %= n_points;
    ret = secp256k1_ecmult_multi_var(&ctx->error_callback, scratch, &r, g_sc, secp256k1_fuzz_ecmult_multi_callback, data, n_points);
    data->fail = 0;
    FUZZ_CHECK(ret == 0);
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
    secp256k1_fuzz_ecmult_multi_fail_callback(ctx, g_sc_ptr, n_points, &data);

    secp256k1_context_destroy(ctx);
    return 0;
}
