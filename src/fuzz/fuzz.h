/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_FUZZ_H
#define SECP256K1_FUZZ_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "secp256k1.h"

#ifdef ENABLE_MODULE_ECDH
#include "secp256k1_ecdh.h"
#endif
#ifdef ENABLE_MODULE_EXTRAKEYS
#include "secp256k1_extrakeys.h"
#endif
#ifdef ENABLE_MODULE_RECOVERY
#include "secp256k1_recovery.h"
#endif
#ifdef ENABLE_MODULE_SCHNORRSIG
#include "secp256k1_schnorrsig.h"
#endif
#ifdef ENABLE_MODULE_MUSIG
#include "secp256k1_musig.h"
#endif

#define FUZZ_CHECK(cond) do { \
    if (!(cond)) { \
        abort(); \
    } \
} while (0)

static const unsigned char secp256k1_fuzz_scalar_zero[32] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char secp256k1_fuzz_scalar_one[32] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

static const unsigned char secp256k1_fuzz_scalar_order[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
    0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41
};

static const unsigned char secp256k1_fuzz_scalar_order_minus_one[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
    0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
    0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x40
};

static const unsigned char *secp256k1_fuzz_data_or_empty(const unsigned char *data, size_t size) {
    static const unsigned char empty[1] = { 0 };
    return size == 0 ? empty : data;
}

static unsigned char secp256k1_fuzz_byte(const unsigned char *data, size_t size, size_t offset) {
    if (size == 0) {
        return (unsigned char)(0xA5u + (unsigned char)offset);
    }
    return data[offset % size];
}

static void secp256k1_fuzz_derive(unsigned char *out, size_t outlen, const unsigned char *data, size_t size, unsigned int salt) {
    size_t i;
    for (i = 0; i < outlen; i++) {
        unsigned char b = secp256k1_fuzz_byte(data, size, i + salt * 131u);
        out[i] = (unsigned char)(b ^ (unsigned char)(salt * 17u + i * 29u));
    }
}

static void secp256k1_fuzz_scalar32(unsigned char *out32, const unsigned char *data, size_t size, unsigned int salt) {
    unsigned char selector = secp256k1_fuzz_byte(data, size, salt);
    switch (selector & 7u) {
    case 0:
        memcpy(out32, secp256k1_fuzz_scalar_zero, 32);
        break;
    case 1:
        memcpy(out32, secp256k1_fuzz_scalar_one, 32);
        break;
    case 2:
        memcpy(out32, secp256k1_fuzz_scalar_order, 32);
        break;
    case 3:
        memcpy(out32, secp256k1_fuzz_scalar_order_minus_one, 32);
        break;
    default:
        secp256k1_fuzz_derive(out32, 32, data, size, salt);
        break;
    }
}

static void secp256k1_fuzz_valid_seckey32(const secp256k1_context *ctx, unsigned char *out32, const unsigned char *data, size_t size, unsigned int salt) {
    unsigned int attempt;
    for (attempt = 0; attempt < 16; attempt++) {
        secp256k1_fuzz_scalar32(out32, data, size, salt + attempt);
        if (secp256k1_ec_seckey_verify(ctx, out32)) {
            return;
        }
    }
    memcpy(out32, secp256k1_fuzz_scalar_one, 32);
}

static secp256k1_context *secp256k1_fuzz_context(const unsigned char *data, size_t size, unsigned int salt) {
    secp256k1_context *ctx;
    unsigned char seed32[32];
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    FUZZ_CHECK(ctx != NULL);
    secp256k1_fuzz_derive(seed32, sizeof(seed32), data, size, salt);
    FUZZ_CHECK(secp256k1_context_randomize(ctx, seed32) == 1);
    memset(seed32, 0, sizeof(seed32));
    return ctx;
}

static void secp256k1_fuzz_check_pubkey_roundtrip(const secp256k1_context *ctx, const secp256k1_pubkey *pubkey) {
    secp256k1_pubkey parsed;
    unsigned char compressed[33];
    unsigned char uncompressed[65];
    size_t compressed_len = sizeof(compressed);
    size_t uncompressed_len = sizeof(uncompressed);

    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, compressed, &compressed_len, pubkey, SECP256K1_EC_COMPRESSED) == 1);
    FUZZ_CHECK(compressed_len == sizeof(compressed));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &parsed, compressed, compressed_len) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, pubkey, &parsed) == 0);

    FUZZ_CHECK(secp256k1_ec_pubkey_serialize(ctx, uncompressed, &uncompressed_len, pubkey, SECP256K1_EC_UNCOMPRESSED) == 1);
    FUZZ_CHECK(uncompressed_len == sizeof(uncompressed));
    FUZZ_CHECK(secp256k1_ec_pubkey_parse(ctx, &parsed, uncompressed, uncompressed_len) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, pubkey, &parsed) == 0);
}

static void secp256k1_fuzz_check_signature_roundtrip(const secp256k1_context *ctx, const secp256k1_ecdsa_signature *sig) {
    secp256k1_ecdsa_signature parsed_compact;
    secp256k1_ecdsa_signature parsed_der;
    secp256k1_ecdsa_signature normalized;
    secp256k1_ecdsa_signature normalized_again;
    unsigned char compact[64];
    unsigned char compact2[64];
    unsigned char der[72];
    unsigned char short_der[72];
    size_t der_len = sizeof(der);
    size_t short_der_len;

    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, sig) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_compact(ctx, &parsed_compact, compact) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact2, &parsed_compact) == 1);
    FUZZ_CHECK(memcmp(compact, compact2, sizeof(compact)) == 0);

    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, der, &der_len, sig) == 1);
    FUZZ_CHECK(der_len <= sizeof(der));
    short_der_len = der_len - 1;
    memset(short_der, 0xA5, sizeof(short_der));
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_der(ctx, short_der, &short_der_len, sig) == 0);
    FUZZ_CHECK(short_der_len == der_len);
    FUZZ_CHECK(secp256k1_ecdsa_signature_parse_der(ctx, &parsed_der, der, der_len) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact2, &parsed_der) == 1);
    FUZZ_CHECK(memcmp(compact, compact2, sizeof(compact)) == 0);

    secp256k1_ecdsa_signature_normalize(ctx, &normalized, sig);
    FUZZ_CHECK(secp256k1_ecdsa_signature_normalize(ctx, &normalized_again, &normalized) == 0);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact, &normalized) == 1);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact2, &normalized_again) == 1);
    FUZZ_CHECK(memcmp(compact, compact2, sizeof(compact)) == 0);

    normalized_again = *sig;
    secp256k1_ecdsa_signature_normalize(ctx, &normalized_again, &normalized_again);
    FUZZ_CHECK(secp256k1_ecdsa_signature_serialize_compact(ctx, compact2, &normalized_again) == 1);
    FUZZ_CHECK(memcmp(compact, compact2, sizeof(compact)) == 0);
}

#ifndef SECP256K1_FUZZ_USE_LIBFUZZER
int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size);

static int secp256k1_fuzz_run_file(const char *path) {
    FILE *file;
    long len;
    unsigned char *buf = NULL;
    size_t read_len;
    int ret = 0;

    file = fopen(path, "rb");
    if (file == NULL) {
        return 1;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 1;
    }
    len = ftell(file);
    if (len < 0) {
        fclose(file);
        return 1;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 1;
    }
    if (len > 0) {
        buf = (unsigned char *)malloc((size_t)len);
        if (buf == NULL) {
            fclose(file);
            return 1;
        }
        read_len = fread(buf, 1, (size_t)len, file);
        if (read_len != (size_t)len) {
            ret = 1;
        }
    }
    if (fclose(file) != 0) {
        ret = 1;
    }
    if (ret == 0) {
        static const unsigned char empty[1] = { 0 };
        const unsigned char *input = len == 0 ? empty : buf;
        LLVMFuzzerTestOneInput(input, (size_t)len);
    }
    free(buf);
    return ret;
}

int main(int argc, char **argv) {
    int i;
    int ret = 0;
    static const unsigned char empty[1] = { 0 };

    if (argc == 1) {
        LLVMFuzzerTestOneInput(empty, 0);
        return 0;
    }
    for (i = 1; i < argc; i++) {
        ret |= secp256k1_fuzz_run_file(argv[i]);
    }
    return ret;
}
#endif

#endif /* SECP256K1_FUZZ_H */
