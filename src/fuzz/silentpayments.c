/***********************************************************************
 * Copyright (c) 2026 The Bitcoin Core developers                      *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#include "fuzz.h"

#ifdef ENABLE_MODULE_SILENTPAYMENTS

typedef struct {
    const void *self;
    unsigned int calls;
} secp256k1_fuzz_silentpayments_illegal_data;

typedef struct {
    const void *self;
    unsigned char label33[33];
    unsigned char label_tweak[32];
    size_t entries_used;
} secp256k1_fuzz_silentpayments_label_cache;

static void secp256k1_fuzz_silentpayments_illegal_callback(const char *message, void *data) {
    secp256k1_fuzz_silentpayments_illegal_data *illegal_data = (secp256k1_fuzz_silentpayments_illegal_data *)data;

    FUZZ_CHECK(message != NULL);
    FUZZ_CHECK(illegal_data != NULL);
    FUZZ_CHECK(illegal_data->self == illegal_data);
    illegal_data->calls++;
}

static const unsigned char *secp256k1_fuzz_silentpayments_label_lookup(const unsigned char *label33, const void *label_context) {
    const secp256k1_fuzz_silentpayments_label_cache *cache = (const secp256k1_fuzz_silentpayments_label_cache *)label_context;

    FUZZ_CHECK(label33 != NULL);
    FUZZ_CHECK(cache != NULL);
    FUZZ_CHECK(cache->self == cache);
    if (cache->entries_used == 1 && memcmp(cache->label33, label33, sizeof(cache->label33)) == 0) {
        return cache->label_tweak;
    }
    return NULL;
}

static void secp256k1_fuzz_silentpayments_check_spend_tweak(
    const secp256k1_context *ctx,
    const secp256k1_silentpayments_found_output *found_output,
    const unsigned char *spend_seckey
) {
    unsigned char tweaked_seckey[32];
    secp256k1_pubkey tweaked_pubkey;
    secp256k1_xonly_pubkey tweaked_xonly;
    int parity;

    memcpy(tweaked_seckey, spend_seckey, sizeof(tweaked_seckey));
    FUZZ_CHECK(secp256k1_ec_seckey_tweak_add(ctx, tweaked_seckey, found_output->tweak) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &tweaked_pubkey, tweaked_seckey) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx, &tweaked_xonly, &parity, &tweaked_pubkey) == 1);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &tweaked_xonly, &found_output->output) == 0);
    memset(tweaked_seckey, 0, sizeof(tweaked_seckey));
}

static void secp256k1_fuzz_silentpayments_check_roundtrip(
    const secp256k1_context *ctx,
    const unsigned char *input,
    size_t size,
    secp256k1_fuzz_silentpayments_illegal_data *illegal_data
) {
    unsigned char scan_seckey[32];
    unsigned char spend_seckey[32];
    unsigned char input_seckey[32];
    unsigned char other_scan_seckey[32];
    unsigned char other_spend_seckey[32];
    unsigned char outpoint[36];
    unsigned char label_tweak[32];
    unsigned char label_ser[33];
    unsigned char parsed_label_ser[33];
    const unsigned char *input_seckey_ptrs[1];
    const unsigned char *mixed_seckey_ptrs[1];
    const secp256k1_pubkey *prevout_pubkey_ptrs[1];
    const secp256k1_pubkey *mixed_pubkey_ptrs[1];
    const secp256k1_xonly_pubkey *mixed_xonly_pubkey_ptrs[1];
    const secp256k1_keypair *mixed_keypair_ptrs[1];
    const secp256k1_xonly_pubkey *tx_output_ptrs[2];
    const secp256k1_xonly_pubkey *mixed_tx_output_ptrs[1];
    secp256k1_pubkey scan_pubkey;
    secp256k1_pubkey spend_pubkey;
    secp256k1_pubkey other_scan_pubkey;
    secp256k1_pubkey other_spend_pubkey;
    secp256k1_pubkey labeled_spend_pubkey;
    secp256k1_pubkey input_pubkey;
    secp256k1_keypair taproot_keypair;
    secp256k1_xonly_pubkey taproot_xonly;
    secp256k1_xonly_pubkey mixed_output;
    secp256k1_silentpayments_label label;
    secp256k1_silentpayments_label parsed_label;
    secp256k1_silentpayments_prevouts_summary prevouts_summary;
    secp256k1_silentpayments_prevouts_summary mixed_summary;
    secp256k1_silentpayments_recipient recipients[2];
    const secp256k1_silentpayments_recipient *recipient_ptrs[2];
    secp256k1_xonly_pubkey generated_outputs[2];
    secp256k1_xonly_pubkey *generated_output_ptrs[2];
    secp256k1_xonly_pubkey *mixed_output_ptrs[1];
    secp256k1_silentpayments_found_output found_outputs[2];
    secp256k1_silentpayments_found_output *found_output_ptrs[2];
    secp256k1_silentpayments_found_output mixed_found_output;
    secp256k1_silentpayments_found_output *mixed_found_output_ptrs[1];
    secp256k1_fuzz_silentpayments_label_cache label_cache;
    uint32_t n_found;
    uint32_t label_index;
    size_t n_recipients;
    size_t i;
    size_t j;
    int matched[2] = { 0, 0 };
    int have_label;
    int scan_cmp;
    unsigned int calls;

    secp256k1_fuzz_valid_seckey32(ctx, scan_seckey, input, size, 211);
    secp256k1_fuzz_valid_seckey32(ctx, spend_seckey, input, size, 223);
    secp256k1_fuzz_valid_seckey32(ctx, input_seckey, input, size, 227);
    secp256k1_fuzz_valid_seckey32(ctx, other_scan_seckey, input, size, 239);
    secp256k1_fuzz_valid_seckey32(ctx, other_spend_seckey, input, size, 241);
    secp256k1_fuzz_derive(outpoint, sizeof(outpoint), input, size, 229);
    label_index = ((uint32_t)secp256k1_fuzz_byte(input, size, 233) << 24)
        | ((uint32_t)secp256k1_fuzz_byte(input, size, 234) << 16)
        | ((uint32_t)secp256k1_fuzz_byte(input, size, 235) << 8)
        | (uint32_t)secp256k1_fuzz_byte(input, size, 236);

    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &scan_pubkey, scan_seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &spend_pubkey, spend_seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &input_pubkey, input_seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &other_scan_pubkey, other_scan_seckey) == 1);
    FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &other_spend_pubkey, other_spend_seckey) == 1);
    if (secp256k1_ec_pubkey_cmp(ctx, &scan_pubkey, &other_scan_pubkey) == 0) {
        FUZZ_CHECK(secp256k1_ec_seckey_negate(ctx, other_scan_seckey) == 1);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &other_scan_pubkey, other_scan_seckey) == 1);
    }
    scan_cmp = secp256k1_ec_pubkey_cmp(ctx, &scan_pubkey, &other_scan_pubkey);
    FUZZ_CHECK(scan_cmp != 0);
    FUZZ_CHECK(secp256k1_silentpayments_recipient_label_create(ctx, &label, label_tweak, scan_seckey, label_index) == 1);
    FUZZ_CHECK(secp256k1_silentpayments_recipient_label_serialize(ctx, label_ser, &label) == 1);
    FUZZ_CHECK(secp256k1_silentpayments_recipient_label_parse(ctx, &parsed_label, label_ser) == 1);
    FUZZ_CHECK(secp256k1_silentpayments_recipient_label_serialize(ctx, parsed_label_ser, &parsed_label) == 1);
    FUZZ_CHECK(memcmp(label_ser, parsed_label_ser, sizeof(label_ser)) == 0);

    have_label = secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(ctx, &labeled_spend_pubkey, &spend_pubkey, &label);
    n_recipients = have_label ? 2 : 1;
    for (i = 0; i < n_recipients; i++) {
        recipients[i].scan_pubkey = scan_pubkey;
        recipients[i].spend_pubkey = i == 0 ? spend_pubkey : labeled_spend_pubkey;
        recipients[i].index = i;
        recipient_ptrs[i] = &recipients[i];
        generated_output_ptrs[i] = &generated_outputs[i];
        found_output_ptrs[i] = &found_outputs[i];
        tx_output_ptrs[i] = &generated_outputs[i];
    }
    input_seckey_ptrs[0] = input_seckey;
    prevout_pubkey_ptrs[0] = &input_pubkey;
    FUZZ_CHECK(secp256k1_silentpayments_sender_create_outputs(ctx,
        generated_output_ptrs, recipient_ptrs, n_recipients, outpoint,
        NULL, 0, input_seckey_ptrs, 1) == 1);
    FUZZ_CHECK(secp256k1_silentpayments_recipient_prevouts_summary_create(ctx,
        &prevouts_summary, outpoint, NULL, 0, prevout_pubkey_ptrs, 1) == 1);

    label_cache.self = &label_cache;
    label_cache.entries_used = 0;
    if (have_label) {
        memcpy(label_cache.label33, label_ser, sizeof(label_cache.label33));
        memcpy(label_cache.label_tweak, label_tweak, sizeof(label_cache.label_tweak));
        label_cache.entries_used = 1;
    }
    illegal_data->calls = 0;
    n_found = 0;
    FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
        found_output_ptrs, &n_found, tx_output_ptrs, n_recipients,
        scan_seckey, &prevouts_summary, &spend_pubkey,
        secp256k1_fuzz_silentpayments_label_lookup, &label_cache) == 1);
    FUZZ_CHECK(illegal_data->calls == 0);
    FUZZ_CHECK(n_found == n_recipients);
    for (i = 0; i < n_found; i++) {
        FUZZ_CHECK(found_outputs[i].found_with_label == 0 || found_outputs[i].found_with_label == 1);
        for (j = 0; j < n_recipients; j++) {
            if (!matched[j] && secp256k1_xonly_pubkey_cmp(ctx, &found_outputs[i].output, &generated_outputs[j]) == 0) {
                matched[j] = 1;
                break;
            }
        }
        FUZZ_CHECK(j < n_recipients);
        if (found_outputs[i].found_with_label) {
            FUZZ_CHECK(have_label);
            FUZZ_CHECK(secp256k1_silentpayments_recipient_label_serialize(ctx, label_ser, &found_outputs[i].label) == 1);
            FUZZ_CHECK(memcmp(label_ser, label_cache.label33, sizeof(label_ser)) == 0);
        }
    }
    for (i = 0; i < n_recipients; i++) {
        FUZZ_CHECK(matched[i]);
        secp256k1_fuzz_silentpayments_check_spend_tweak(ctx, &found_outputs[i], spend_seckey);
    }

    /* Exercise the sender's original-index contract across scan-key groups.
     * Deliberately put the lexicographically larger scan key first so the
     * sender must reorder its pointer array and still write outputs to the
     * caller's original slots. */
    {
        const unsigned char *multi_scan_seckey_ptrs[2];
        const unsigned char *multi_spend_seckey_ptrs[2];
        const secp256k1_pubkey *multi_spend_pubkey_ptrs[2];

        if (scan_cmp > 0) {
            recipients[0].scan_pubkey = scan_pubkey;
            recipients[0].spend_pubkey = spend_pubkey;
            recipients[1].scan_pubkey = other_scan_pubkey;
            recipients[1].spend_pubkey = other_spend_pubkey;
            multi_scan_seckey_ptrs[0] = scan_seckey;
            multi_scan_seckey_ptrs[1] = other_scan_seckey;
            multi_spend_seckey_ptrs[0] = spend_seckey;
            multi_spend_seckey_ptrs[1] = other_spend_seckey;
            multi_spend_pubkey_ptrs[0] = &spend_pubkey;
            multi_spend_pubkey_ptrs[1] = &other_spend_pubkey;
        } else {
            recipients[0].scan_pubkey = other_scan_pubkey;
            recipients[0].spend_pubkey = other_spend_pubkey;
            recipients[1].scan_pubkey = scan_pubkey;
            recipients[1].spend_pubkey = spend_pubkey;
            multi_scan_seckey_ptrs[0] = other_scan_seckey;
            multi_scan_seckey_ptrs[1] = scan_seckey;
            multi_spend_seckey_ptrs[0] = other_spend_seckey;
            multi_spend_seckey_ptrs[1] = spend_seckey;
            multi_spend_pubkey_ptrs[0] = &other_spend_pubkey;
            multi_spend_pubkey_ptrs[1] = &spend_pubkey;
        }
        recipients[0].index = 0;
        recipients[1].index = 1;
        recipient_ptrs[0] = &recipients[0];
        recipient_ptrs[1] = &recipients[1];
        generated_output_ptrs[0] = &generated_outputs[0];
        generated_output_ptrs[1] = &generated_outputs[1];
        tx_output_ptrs[0] = &generated_outputs[0];
        tx_output_ptrs[1] = &generated_outputs[1];

        FUZZ_CHECK(secp256k1_silentpayments_sender_create_outputs(ctx,
            generated_output_ptrs, recipient_ptrs, 2, outpoint,
            NULL, 0, input_seckey_ptrs, 1) == 1);
        for (i = 0; i < 2; i++) {
            illegal_data->calls = 0;
            n_found = 0;
            FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
                found_output_ptrs, &n_found, tx_output_ptrs, 2,
                multi_scan_seckey_ptrs[i], &prevouts_summary,
                multi_spend_pubkey_ptrs[i], NULL, NULL) == 1);
            FUZZ_CHECK(illegal_data->calls == 0);
            FUZZ_CHECK(n_found == 1);
            FUZZ_CHECK(found_outputs[0].found_with_label == 0);
            FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx,
                &found_outputs[0].output, &generated_outputs[i]) == 0);
            secp256k1_fuzz_silentpayments_check_spend_tweak(ctx, &found_outputs[0],
                multi_spend_seckey_ptrs[i]);
        }
        /* The sender sorts this pointer array in place. Restore the original
         * layout before the single-recipient mixed-input exercise below. */
        recipient_ptrs[0] = &recipients[0];
        recipient_ptrs[1] = &recipients[1];
        recipients[0].scan_pubkey = scan_pubkey;
        recipients[0].spend_pubkey = spend_pubkey;
    }

    /* Exercise the BIP352 mixed-input contract: taproot inputs are supplied as
     * x-only pubkeys to the recipient and as keypairs to the sender, while
     * ordinary inputs use full pubkeys and secret keys. */
    FUZZ_CHECK(secp256k1_keypair_create(ctx, &taproot_keypair, input_seckey) == 1);
    FUZZ_CHECK(secp256k1_keypair_xonly_pub(ctx, &taproot_xonly, NULL, &taproot_keypair) == 1);
    mixed_keypair_ptrs[0] = &taproot_keypair;
    mixed_seckey_ptrs[0] = scan_seckey;
    mixed_xonly_pubkey_ptrs[0] = &taproot_xonly;
    mixed_pubkey_ptrs[0] = &scan_pubkey;
    mixed_output_ptrs[0] = &mixed_output;
    mixed_tx_output_ptrs[0] = &mixed_output;
    mixed_found_output_ptrs[0] = &mixed_found_output;
    recipients[0].index = 0;
    /* A valid input-key set can still sum to infinity, which is a documented
     * no-output case. Skip that degenerate fuzzer domain rather than treating
     * an expected API failure as an oracle violation. */
    if (!secp256k1_silentpayments_sender_create_outputs(ctx,
        mixed_output_ptrs, recipient_ptrs, 1, outpoint,
        mixed_keypair_ptrs, 1, mixed_seckey_ptrs, 1)) {
        return;
    }
    if (!secp256k1_silentpayments_recipient_prevouts_summary_create(ctx,
        &mixed_summary, outpoint, mixed_xonly_pubkey_ptrs, 1,
        mixed_pubkey_ptrs, 1)) {
        return;
    }
    illegal_data->calls = 0;
    n_found = 0;
    FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
        mixed_found_output_ptrs, &n_found, mixed_tx_output_ptrs, 1,
        scan_seckey, &mixed_summary, &spend_pubkey, NULL, NULL) == 1);
    FUZZ_CHECK(illegal_data->calls == 0);
    FUZZ_CHECK(n_found == 1);
    FUZZ_CHECK(mixed_found_output.found_with_label == 0);
    FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &mixed_found_output.output, &mixed_output) == 0);
    secp256k1_fuzz_silentpayments_check_spend_tweak(ctx, &mixed_found_output, spend_seckey);

    /* Magic-preserving mutations exercise opaque state that ordinary API
     * round trips cannot produce. Keep the input hash intact in the point
     * mutation so the failure is specifically attributable to point loading. */
    {
        secp256k1_silentpayments_label malformed_label = label;
        unsigned char serialized[33];

        memset(malformed_label.data + 4, 0, sizeof(malformed_label.data) - 4);
        memset(serialized, 0xA5, sizeof(serialized));
        calls = illegal_data->calls;
        FUZZ_CHECK(secp256k1_silentpayments_recipient_label_serialize(ctx, serialized, &malformed_label) == 0);
        FUZZ_CHECK(illegal_data->calls == calls + 1);
    }
    {
        secp256k1_silentpayments_prevouts_summary malformed_summary = prevouts_summary;

        memset(malformed_summary.data + 5, 0, 64);
        calls = illegal_data->calls;
        n_found = 0;
        FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
            found_output_ptrs, &n_found, tx_output_ptrs, n_recipients,
            scan_seckey, &malformed_summary, &spend_pubkey,
            secp256k1_fuzz_silentpayments_label_lookup, &label_cache) == 0);
        FUZZ_CHECK(illegal_data->calls == calls + 1);
    }
    {
        secp256k1_silentpayments_prevouts_summary malformed_summary = prevouts_summary;

        malformed_summary.data[4] = 2;
        calls = illegal_data->calls;
        n_found = 0;
        FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
            found_output_ptrs, &n_found, tx_output_ptrs, n_recipients,
            scan_seckey, &malformed_summary, &spend_pubkey,
            secp256k1_fuzz_silentpayments_label_lookup, &label_cache) == 0);
        FUZZ_CHECK(illegal_data->calls == calls + 1);
    }
    {
        secp256k1_silentpayments_prevouts_summary malformed_summary = prevouts_summary;

        memset(malformed_summary.data + 69, 0, 32);
        calls = illegal_data->calls;
        n_found = 0;
        FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
            found_output_ptrs, &n_found, tx_output_ptrs, n_recipients,
            scan_seckey, &malformed_summary, &spend_pubkey,
            secp256k1_fuzz_silentpayments_label_lookup, &label_cache) == 0);
        FUZZ_CHECK(illegal_data->calls == calls + 1);
    }
    {
        secp256k1_silentpayments_prevouts_summary malformed_summary = prevouts_summary;

        memset(malformed_summary.data + 69, 0xFF, 32);
        calls = illegal_data->calls;
        n_found = 0;
        FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
            found_output_ptrs, &n_found, tx_output_ptrs, n_recipients,
            scan_seckey, &malformed_summary, &spend_pubkey,
            secp256k1_fuzz_silentpayments_label_lookup, &label_cache) == 0);
        FUZZ_CHECK(illegal_data->calls == calls + 1);
    }
}

#endif

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
#ifdef ENABLE_MODULE_SILENTPAYMENTS
    const unsigned char *input = secp256k1_fuzz_data_or_empty(data, size);
    secp256k1_context *ctx = secp256k1_fuzz_context(input, size, 207);
    secp256k1_fuzz_silentpayments_illegal_data illegal_data;

    illegal_data.self = &illegal_data;
    illegal_data.calls = 0;
    secp256k1_context_set_illegal_callback(ctx, secp256k1_fuzz_silentpayments_illegal_callback, &illegal_data);
    secp256k1_fuzz_silentpayments_check_roundtrip(ctx, input, size, &illegal_data);
    secp256k1_context_set_illegal_callback(ctx, NULL, NULL);
    secp256k1_context_destroy(ctx);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
