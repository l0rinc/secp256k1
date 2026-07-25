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
    secp256k1_silentpayments_prevouts_summary combined_summary;
    secp256k1_pubkey combined_prevouts_pubkey;
    secp256k1_silentpayments_recipient recipients[2];
    const secp256k1_silentpayments_recipient *recipient_ptrs[2];
    secp256k1_xonly_pubkey generated_outputs[2];
    secp256k1_xonly_pubkey *generated_output_ptrs[2];
    secp256k1_xonly_pubkey *mixed_output_ptrs[1];
    secp256k1_silentpayments_found_output found_outputs[2];
    secp256k1_silentpayments_found_output *found_output_ptrs[2];
    secp256k1_silentpayments_found_output combined_found_outputs[2];
    secp256k1_silentpayments_found_output *combined_found_output_ptrs[2];
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
        combined_found_output_ptrs[i] = &combined_found_outputs[i];
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

    /* The scanner also accepts a compact summary in which input_hash has
     * already been multiplied into the summed prevout point. Construct that
     * representation from the ordinary summary and require both paths to
     * produce the same wallet result. The summary point and pubkey share the
     * library's documented 64-byte opaque representation; this is deliberate
     * coverage of the implementation-defined combined mode. */
    combined_summary = prevouts_summary;
    memcpy(combined_prevouts_pubkey.data, prevouts_summary.data + 5,
        sizeof(combined_prevouts_pubkey.data));
    FUZZ_CHECK(secp256k1_ec_pubkey_tweak_mul(ctx, &combined_prevouts_pubkey,
        prevouts_summary.data + 69) == 1);
    combined_summary.data[4] = 1;
    memcpy(combined_summary.data + 5, combined_prevouts_pubkey.data,
        sizeof(combined_prevouts_pubkey.data));
    memset(combined_summary.data + 69, 0, 32);
    illegal_data->calls = 0;
    n_found = 0;
    FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
        combined_found_output_ptrs, &n_found, tx_output_ptrs, n_recipients,
        scan_seckey, &combined_summary, &spend_pubkey,
        secp256k1_fuzz_silentpayments_label_lookup, &label_cache) == 1);
    FUZZ_CHECK(illegal_data->calls == 0);
    FUZZ_CHECK(n_found == n_recipients);
    for (i = 0; i < n_recipients; i++) {
        unsigned char ordinary_label[33];
        unsigned char combined_label[33];

        FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx,
            &combined_found_outputs[i].output, &found_outputs[i].output) == 0);
        FUZZ_CHECK(memcmp(combined_found_outputs[i].tweak,
            found_outputs[i].tweak, sizeof(found_outputs[i].tweak)) == 0);
        FUZZ_CHECK(combined_found_outputs[i].found_with_label ==
            found_outputs[i].found_with_label);
        if (found_outputs[i].found_with_label) {
            FUZZ_CHECK(secp256k1_silentpayments_recipient_label_serialize(ctx,
                ordinary_label, &found_outputs[i].label) == 1);
            FUZZ_CHECK(secp256k1_silentpayments_recipient_label_serialize(ctx,
                combined_label, &combined_found_outputs[i].label) == 1);
            FUZZ_CHECK(memcmp(combined_label, ordinary_label,
                sizeof(ordinary_label)) == 0);
        } else {
            FUZZ_CHECK(memcmp(&combined_found_outputs[i].label,
                &found_outputs[i].label, sizeof(found_outputs[i].label)) == 0);
        }
    }

    /* Exercise a full label batch followed by a partial batch. Eight decoys
     * force one flush; the labeled k=0 output then sits at index 8 and the
     * unlabeled direct k=0 output at index 9. The partial batch must be
     * checked before the direct match is accepted. A second ordering puts the
     * label in slot 7, the last candidate of the full batch. */
    if (have_label) {
        secp256k1_silentpayments_recipient batch_recipient;
        const secp256k1_silentpayments_recipient *batch_recipient_ptrs[1];
        secp256k1_xonly_pubkey batch_labeled_output;
        secp256k1_xonly_pubkey *batch_output_ptrs[1];
        secp256k1_xonly_pubkey batch_decoys[8];
        const secp256k1_xonly_pubkey *batch_tx_outputs[10];
        secp256k1_silentpayments_found_output batch_found_outputs[10];
        secp256k1_silentpayments_found_output *batch_found_output_ptrs[10];
        unsigned char decoy_seckey[32];
        unsigned char batch_label_ser[33];
        secp256k1_pubkey decoy_pubkey;
        size_t batch_i;

        batch_recipient.scan_pubkey = scan_pubkey;
        batch_recipient.spend_pubkey = labeled_spend_pubkey;
        batch_recipient.index = 0;
        batch_recipient_ptrs[0] = &batch_recipient;
        batch_output_ptrs[0] = &batch_labeled_output;
        FUZZ_CHECK(secp256k1_silentpayments_sender_create_outputs(ctx,
            batch_output_ptrs, batch_recipient_ptrs, 1, outpoint,
            NULL, 0, input_seckey_ptrs, 1) == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &batch_labeled_output,
            &generated_outputs[0]) != 0);
        for (batch_i = 0; batch_i < 8; batch_i++) {
            secp256k1_fuzz_valid_seckey32(ctx, decoy_seckey, input, size,
                251 + (unsigned int)batch_i);
            FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &decoy_pubkey,
                decoy_seckey) == 1);
            FUZZ_CHECK(secp256k1_xonly_pubkey_from_pubkey(ctx,
                &batch_decoys[batch_i], NULL, &decoy_pubkey) == 1);
            FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &batch_decoys[batch_i],
                &batch_labeled_output) != 0);
            FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &batch_decoys[batch_i],
                &generated_outputs[0]) != 0);
            batch_tx_outputs[batch_i] = &batch_decoys[batch_i];
        }
        batch_tx_outputs[8] = &batch_labeled_output;
        batch_tx_outputs[9] = &generated_outputs[0];
        for (batch_i = 0; batch_i < 10; batch_i++) {
            batch_found_output_ptrs[batch_i] = &batch_found_outputs[batch_i];
        }
        illegal_data->calls = 0;
        n_found = 0;
        FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
            batch_found_output_ptrs, &n_found, batch_tx_outputs, 10,
            scan_seckey, &prevouts_summary, &spend_pubkey,
            secp256k1_fuzz_silentpayments_label_lookup, &label_cache) == 1);
        FUZZ_CHECK(illegal_data->calls == 0);
        FUZZ_CHECK(n_found == 1);
        FUZZ_CHECK(batch_found_outputs[0].found_with_label == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx,
            &batch_found_outputs[0].output, &batch_labeled_output) == 0);
        FUZZ_CHECK(secp256k1_silentpayments_recipient_label_serialize(ctx,
            batch_label_ser, &batch_found_outputs[0].label) == 1);
        FUZZ_CHECK(memcmp(batch_label_ser, label_ser,
            sizeof(batch_label_ser)) == 0);
        secp256k1_fuzz_silentpayments_check_spend_tweak(ctx,
            &batch_found_outputs[0], spend_seckey);

        /* Make the label the last entry in a full batch. This must not be
         * confused with the direct output at index 9. */
        for (batch_i = 0; batch_i < 7; batch_i++) {
            batch_tx_outputs[batch_i] = &batch_decoys[batch_i];
        }
        batch_tx_outputs[7] = &batch_labeled_output;
        batch_tx_outputs[8] = &batch_decoys[7];
        batch_tx_outputs[9] = &generated_outputs[0];
        illegal_data->calls = 0;
        n_found = 0;
        FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
            batch_found_output_ptrs, &n_found, batch_tx_outputs, 10,
            scan_seckey, &prevouts_summary, &spend_pubkey,
            secp256k1_fuzz_silentpayments_label_lookup, &label_cache) == 1);
        FUZZ_CHECK(illegal_data->calls == 0);
        FUZZ_CHECK(n_found == 1);
        FUZZ_CHECK(batch_found_outputs[0].found_with_label == 1);
        FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx,
            &batch_found_outputs[0].output, &batch_labeled_output) == 0);
        FUZZ_CHECK(secp256k1_silentpayments_recipient_label_serialize(ctx,
            batch_label_ser, &batch_found_outputs[0].label) == 1);
        FUZZ_CHECK(memcmp(batch_label_ser, label_ser,
            sizeof(batch_label_ser)) == 0);
        secp256k1_fuzz_silentpayments_check_spend_tweak(ctx,
            &batch_found_outputs[0], spend_seckey);
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

    /* Exercise the multi-input aggregation contract independently of the
     * one-plain/one-taproot mixed-input case above. Two ordinary inputs must
     * be equivalent to their scalar and public-key sum on both sides of the
     * protocol. This is the form used when a transaction has multiple
     * Silent Payments-eligible inputs. */
    {
        unsigned char second_input_seckey[32];
        unsigned char summed_input_seckey[32];
        const unsigned char *multi_input_seckey_ptrs[2];
        const unsigned char *summed_input_seckey_ptrs[1];
        const secp256k1_pubkey *multi_input_pubkey_ptrs[2];
        const secp256k1_pubkey *summed_input_pubkey_ptrs[1];
        secp256k1_pubkey second_input_pubkey;
        secp256k1_pubkey summed_input_pubkey;
        secp256k1_pubkey combined_input_pubkey;
        secp256k1_silentpayments_recipient aggregate_recipient;
        const secp256k1_silentpayments_recipient *aggregate_recipient_ptrs[1];
        secp256k1_xonly_pubkey multi_input_output;
        secp256k1_xonly_pubkey summed_input_output;
        secp256k1_xonly_pubkey *multi_input_output_ptrs[1];
        secp256k1_xonly_pubkey *summed_input_output_ptrs[1];
        const secp256k1_xonly_pubkey *multi_input_tx_output_ptrs[1];
        secp256k1_silentpayments_found_output multi_input_found_output;
        secp256k1_silentpayments_found_output summed_input_found_output;
        secp256k1_silentpayments_found_output *multi_input_found_ptrs[1];
        secp256k1_silentpayments_found_output *summed_input_found_ptrs[1];
        secp256k1_silentpayments_prevouts_summary multi_input_summary;
        secp256k1_silentpayments_prevouts_summary summed_input_summary;
        uint32_t multi_input_n_found;

        secp256k1_fuzz_valid_seckey32(ctx, second_input_seckey, input, size, 243);
        FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &second_input_pubkey,
            second_input_seckey) == 1);
        memcpy(summed_input_seckey, input_seckey, sizeof(summed_input_seckey));
        if (secp256k1_ec_seckey_tweak_add(ctx, summed_input_seckey,
            second_input_seckey) == 1) {
            FUZZ_CHECK(secp256k1_ec_pubkey_create(ctx, &summed_input_pubkey,
                summed_input_seckey) == 1);
            multi_input_seckey_ptrs[0] = input_seckey;
            multi_input_seckey_ptrs[1] = second_input_seckey;
            summed_input_seckey_ptrs[0] = summed_input_seckey;
            multi_input_pubkey_ptrs[0] = &input_pubkey;
            multi_input_pubkey_ptrs[1] = &second_input_pubkey;
            summed_input_pubkey_ptrs[0] = &summed_input_pubkey;
            FUZZ_CHECK(secp256k1_ec_pubkey_combine(ctx, &combined_input_pubkey,
                multi_input_pubkey_ptrs, 2) == 1);
            FUZZ_CHECK(secp256k1_ec_pubkey_cmp(ctx, &combined_input_pubkey,
                &summed_input_pubkey) == 0);

            aggregate_recipient.scan_pubkey = scan_pubkey;
            aggregate_recipient.spend_pubkey = spend_pubkey;
            aggregate_recipient.index = 0;
            aggregate_recipient_ptrs[0] = &aggregate_recipient;
            multi_input_output_ptrs[0] = &multi_input_output;
            summed_input_output_ptrs[0] = &summed_input_output;
            FUZZ_CHECK(secp256k1_silentpayments_sender_create_outputs(ctx,
                multi_input_output_ptrs, aggregate_recipient_ptrs, 1,
                outpoint, NULL, 0, multi_input_seckey_ptrs, 2) == 1);
            FUZZ_CHECK(secp256k1_silentpayments_sender_create_outputs(ctx,
                summed_input_output_ptrs, aggregate_recipient_ptrs, 1,
                outpoint, NULL, 0, summed_input_seckey_ptrs, 1) == 1);
            FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx, &multi_input_output,
                &summed_input_output) == 0);

            FUZZ_CHECK(secp256k1_silentpayments_recipient_prevouts_summary_create(ctx,
                &multi_input_summary, outpoint, NULL, 0,
                multi_input_pubkey_ptrs, 2) == 1);
            FUZZ_CHECK(secp256k1_silentpayments_recipient_prevouts_summary_create(ctx,
                &summed_input_summary, outpoint, NULL, 0,
                summed_input_pubkey_ptrs, 1) == 1);
            FUZZ_CHECK(memcmp(multi_input_summary.data,
                summed_input_summary.data, sizeof(multi_input_summary.data)) == 0);

            multi_input_tx_output_ptrs[0] = &multi_input_output;
            multi_input_found_ptrs[0] = &multi_input_found_output;
            summed_input_found_ptrs[0] = &summed_input_found_output;
            illegal_data->calls = 0;
            multi_input_n_found = 0;
            FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
                multi_input_found_ptrs, &multi_input_n_found,
                multi_input_tx_output_ptrs, 1, scan_seckey,
                &multi_input_summary, &spend_pubkey, NULL, NULL) == 1);
            FUZZ_CHECK(illegal_data->calls == 0);
            FUZZ_CHECK(multi_input_n_found == 1);
            FUZZ_CHECK(multi_input_found_output.found_with_label == 0);
            FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx,
                &multi_input_found_output.output, &multi_input_output) == 0);
            secp256k1_fuzz_silentpayments_check_spend_tweak(ctx,
                &multi_input_found_output, spend_seckey);

            illegal_data->calls = 0;
            multi_input_n_found = 0;
            FUZZ_CHECK(secp256k1_silentpayments_recipient_scan_outputs(ctx,
                summed_input_found_ptrs, &multi_input_n_found,
                multi_input_tx_output_ptrs, 1, scan_seckey,
                &summed_input_summary, &spend_pubkey, NULL, NULL) == 1);
            FUZZ_CHECK(illegal_data->calls == 0);
            FUZZ_CHECK(multi_input_n_found == 1);
            FUZZ_CHECK(summed_input_found_output.found_with_label == 0);
            FUZZ_CHECK(secp256k1_xonly_pubkey_cmp(ctx,
                &summed_input_found_output.output, &summed_input_output) == 0);
            FUZZ_CHECK(memcmp(multi_input_found_output.tweak,
                summed_input_found_output.tweak,
                sizeof(multi_input_found_output.tweak)) == 0);
            secp256k1_fuzz_silentpayments_check_spend_tweak(ctx,
                &summed_input_found_output, spend_seckey);
        }
        memset(second_input_seckey, 0, sizeof(second_input_seckey));
        memset(summed_input_seckey, 0, sizeof(summed_input_seckey));
    }

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
