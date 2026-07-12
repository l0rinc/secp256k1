/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include <stddef.h>
#include <stdlib.h>

#ifdef USE_EXTERNAL_DEFAULT_CALLBACKS
unsigned int secp256k1_fuzz_default_illegal_calls = 0;

void secp256k1_default_illegal_callback_fn(const char *message, void *data) {
    (void)data;
    if (message == NULL) {
        abort();
    }
    secp256k1_fuzz_default_illegal_calls++;
}

void secp256k1_default_error_callback_fn(const char *message, void *data) {
    (void)data;
    if (message == NULL) {
        abort();
    }
    abort();
}
#endif
