#include <inttypes.h>
#include <stdio.h>
#include <common/type_to_string.h>
static bool print_superverbose;
#define SUPERVERBOSE(...)					\
	do { if (print_superverbose) printf(__VA_ARGS__); } while(0)
#define PRINT_ACTUAL_FEE
#include "../commit_tx.c"
#include <bitcoin/preimage.h>
#include <bitcoin/privkey.h>
#include <bitcoin/pubkey.h>
#include <ccan/array_size/array_size.h>
#include <ccan/err/err.h>
#include <ccan/str/hex/hex.h>
#include <common/amount.h>
#include <common/channel_id.h>
#include <common/key_derive.h>
#include <common/setup.h>
#include <common/status.h>

/* Turn this on to brute-force fee values */
/*#define DEBUG */

/* AUTOGENERATED MOCKS START */
/* Generated stub for fromwire_bigsize */
bigsize_t fromwire_bigsize(const u8 **cursor UNNEEDED, size_t *max UNNEEDED)
{ fprintf(stderr, "fromwire_bigsize called!\n"); abort(); }
/* Generated stub for fromwire_channel_id */
void fromwire_channel_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED,
			 struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "fromwire_channel_id called!\n"); abort(); }
/* Generated stub for fromwire_node_id */
void fromwire_node_id(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct node_id *id UNNEEDED)
{ fprintf(stderr, "fromwire_node_id called!\n"); abort(); }
/* Generated stub for status_fmt */
void status_fmt(enum log_level level UNNEEDED,
		const struct node_id *peer UNNEEDED,
		const char *fmt UNNEEDED, ...)

{ fprintf(stderr, "status_fmt called!\n"); abort(); }
/* Generated stub for towire_bigsize */
void towire_bigsize(u8 **pptr UNNEEDED, const bigsize_t val UNNEEDED)
{ fprintf(stderr, "towire_bigsize called!\n"); abort(); }
/* Generated stub for towire_channel_id */
void towire_channel_id(u8 **pptr UNNEEDED, const struct channel_id *channel_id UNNEEDED)
{ fprintf(stderr, "towire_channel_id called!\n"); abort(); }
/* Generated stub for towire_node_id */
void towire_node_id(u8 **pptr UNNEEDED, const struct node_id *id UNNEEDED)
{ fprintf(stderr, "towire_node_id called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

/* bitcoind loves its backwards txids! */
static struct bitcoin_txid txid_from_hex(const char *hex)
{
	struct bitcoin_txid txid;

	if (!bitcoin_txid_from_hex(hex, strlen(hex), &txid))
		abort();
	return txid;
}

static struct secret secret_from_hex(const char *hex)
{
	struct secret s;
	size_t len;
	if (strstarts(hex, "0x"))
		hex += 2;
	len = strlen(hex);
	/* BOLT #3:
	 *
	 * - Private keys are displayed as 32 bytes plus a trailing 1
	 *   (Bitcoin's convention for "compressed" private keys, i.e. keys
	 *   for which the public key is compressed).
	 */
	if (len == 66 && strends(hex, "01"))
		len -= 2;
	if (!hex_decode(hex, len, &s, sizeof(s)))
		abort();
	return s;
}

static void tx_must_be_eq(const struct bitcoin_tx *a,
			  const struct bitcoin_tx *b)
{
	u8 *lina, *linb;
	size_t i;

	lina = linearize_tx(tmpctx, a);
	linb = linearize_tx(tmpctx, b);

	for (i = 0; i < tal_count(lina); i++) {
		if (i >= tal_count(linb))
			errx(1, "Second tx is truncated:\n"
			     "%s\n"
			     "%s",
			     tal_hex(tmpctx, lina),
			     tal_hex(tmpctx, linb));
		if (lina[i] != linb[i])
			errx(1, "tx differ at offset %zu:\n"
			     "%s\n"
			     "%s",
			     i,
			     tal_hex(tmpctx, lina),
			     tal_hex(tmpctx, linb));
	}
	if (i != tal_count(linb))
		errx(1, "First tx is truncated:\n"
		     "%s\n"
		     "%s",
		     tal_hex(tmpctx, lina),
		     tal_hex(tmpctx, linb));
}

/* BOLT #3:
 *
 *    htlc 0 direction: remote->local
 *    htlc 0 amount_msat: 1000000
 *    htlc 0 expiry: 500
 *    htlc 0 payment_preimage: 0000000000000000000000000000000000000000000000000000000000000000
 *    htlc 1 direction: remote->local
 *    htlc 1 amount_msat: 2000000
 *    htlc 1 expiry: 501
 *    htlc 1 payment_preimage: 0101010101010101010101010101010101010101010101010101010101010101
 *    htlc 2 direction: local->remote
 *    htlc 2 amount_msat: 2000000
 *    htlc 2 expiry: 502
 *    htlc 2 payment_preimage: 0202020202020202020202020202020202020202020202020202020202020202
 *    htlc 3 direction: local->remote
 *    htlc 3 amount_msat: 3000000
 *    htlc 3 expiry: 503
 *    htlc 3 payment_preimage: 0303030303030303030303030303030303030303030303030303030303030303
 *    htlc 4 direction: remote->local
 *    htlc 4 amount_msat: 4000000
 *    htlc 4 expiry: 504
 *    htlc 4 payment_preimage: 0404040404040404040404040404040404040404040404040404040404040404
 */
static const struct htlc **setup_htlcs_0_to_4(const tal_t *ctx)
{
	const struct htlc **htlcs = tal_arr(ctx, const struct htlc *, 5);
	int i;

	for (i = 0; i < 5; i++) {
		struct htlc *htlc = tal(htlcs, struct htlc);

		htlc->id = i;
		switch (i) {
		case 0:
			htlc->state = RCVD_ADD_ACK_REVOCATION;
			htlc->amount = AMOUNT_MSAT(1000000);
			break;
		case 1:
			htlc->state = RCVD_ADD_ACK_REVOCATION;
			htlc->amount = AMOUNT_MSAT(2000000);
			break;
		case 2:
			htlc->state = SENT_ADD_ACK_REVOCATION;
			htlc->amount = AMOUNT_MSAT(2000000);
			break;
		case 3:
			htlc->state = SENT_ADD_ACK_REVOCATION;
			htlc->amount = AMOUNT_MSAT(3000000);
			break;
		case 4:
			htlc->state = RCVD_ADD_ACK_REVOCATION;
			htlc->amount = AMOUNT_MSAT(4000000);
			break;
		}

		htlc->expiry.locktime = 500 + i;
		htlc->r = tal(htlc, struct preimage);
		memset(htlc->r, i, sizeof(*htlc->r));
		sha256(&htlc->rhash, htlc->r, sizeof(*htlc->r));
		htlcs[i] = htlc;
	}
	return htlcs;
}

/* BOLT #3:
 *    htlc 5 direction: local->remote
 *    htlc 5 amount_msat: 5000000
 *    htlc 5 expiry: 505
 *    htlc 5 payment_preimage: 0505050505050505050505050505050505050505050505050505050505050505
 *    htlc 6 direction: local->remote
 *    htlc 6 amount_msat: 5000000
 *    htlc 6 expiry: 506
 *    htlc 6 payment_preimage: 0505050505050505050505050505050505050505050505050505050505050505
*/
static const struct htlc **setup_htlcs_1_5_and_6(const tal_t *ctx)
{
	const struct htlc **htlcs = tal_arr(ctx, const struct htlc *, 3);
	int i;
	const u64 htlc_ids[] = {1, 5, 6};

	for (i = 0; i < 3; i++) {
		struct htlc *htlc = tal(htlcs, struct htlc);

		htlc->r = tal(htlc, struct preimage);
		htlc->id = htlc_ids[i];
		switch (htlc->id) {
		case 1:
			htlc->state = RCVD_ADD_ACK_REVOCATION;
			htlc->amount = AMOUNT_MSAT(2000000);
			htlc->expiry.locktime = 501;
			memset(htlc->r, 1, sizeof(*htlc->r));
			break;
		case 5:
			htlc->state = SENT_ADD_ACK_REVOCATION;
			htlc->amount = AMOUNT_MSAT(5000000);
			htlc->expiry.locktime = 505;
			memset(htlc->r, 5, sizeof(*htlc->r));
			break;
		case 6:
			htlc->state = SENT_ADD_ACK_REVOCATION;
			htlc->amount = AMOUNT_MSAT(5000000);
			htlc->expiry.locktime = 506;
			memset(htlc->r, 5, sizeof(*htlc->r));
			break;
		}
		sha256(&htlc->rhash, htlc->r, sizeof(*htlc->r));
		htlcs[i] = htlc;
	}
	return htlcs;
}

#if 0
static struct pubkey pubkey_from_hex(const char *hex)
{
	struct pubkey pubkey;

	if (strstarts(hex, "0x"))
		hex += 2;
	if (!pubkey_from_hexstr(hex, strlen(hex), &pubkey))
		abort();
	return pubkey;
}
#endif

static void report_htlcs(const struct bitcoin_tx *tx,
			 const struct htlc **htlc_map,
			 u16 to_self_delay,
			 const struct privkey *local_htlcsecretkey,
			 const struct pubkey *localkey,
			 const struct pubkey *local_htlckey,
			 const struct pubkey *local_delayedkey,
			 const struct privkey *x_remote_htlcsecretkey,
			 const struct pubkey *remotekey,
			 const struct pubkey *remote_htlckey,
			 const struct pubkey *remote_revocation_key,
			 u32 feerate_per_kw,
			 bool option_anchor_outputs)
{
	size_t i, n;
	struct bitcoin_txid txid;
	struct bitcoin_tx **htlc_tx;
	struct bitcoin_signature *remotehtlcsig;
	struct keyset keyset;
	u8 **wscript;

	htlc_tx = tal_arrz(tmpctx, struct bitcoin_tx *, tal_count(htlc_map));
	remotehtlcsig = tal_arr(tmpctx, struct bitcoin_signature,
				tal_count(htlc_map));
	wscript = tal_arr(tmpctx, u8 *, tal_count(htlc_map));

	bitcoin_txid(tx, &txid);

	/* First report remote signatures, in order we would receive them. */
	n = 0;
	for (i = 0; i < tal_count(htlc_map); i++)
		n += (htlc_map[i] != NULL);

	printf("num_htlcs: %zu\n", n);

	/* FIXME: naming here is kind of backwards: local revocation key
	 * is derived from remote revocation basepoint, but it's local */
	keyset.self_revocation_key = *remote_revocation_key;
	keyset.self_delayed_payment_key = *local_delayedkey;
	keyset.self_payment_key = *localkey;
	keyset.other_payment_key = *remotekey;
	keyset.self_htlc_key = *local_htlckey;
	keyset.other_htlc_key = *remote_htlckey;

	for (i = 0; i < tal_count(htlc_map); i++) {
		struct bitcoin_signature localhtlcsig;
		const struct htlc *htlc = htlc_map[i];

		if (!htlc)
			continue;

		if (htlc_owner(htlc) == LOCAL) {
			wscript[i] = bitcoin_wscript_htlc_offer(tmpctx,
								local_htlckey,
								remote_htlckey,
								&htlc->rhash,
								remote_revocation_key,
								option_anchor_outputs);
			htlc_tx[i] = htlc_timeout_tx(htlc_tx, tx->chainparams,
						     &txid, i, wscript[i],
						     htlc->amount,
						     htlc->expiry.locktime,
						     to_self_delay,
						     feerate_per_kw,
						     &keyset, option_anchor_outputs);
		} else {
			wscript[i] = bitcoin_wscript_htlc_receive(tmpctx,
								  &htlc->expiry,
								  local_htlckey,
								  remote_htlckey,
								  &htlc->rhash,
								  remote_revocation_key,
								  option_anchor_outputs);
			htlc_tx[i] = htlc_success_tx(htlc_tx, tx->chainparams,
						     &txid, i, wscript[i],
						     htlc->amount,
						     to_self_delay,
						     feerate_per_kw,
						     &keyset,
						     option_anchor_outputs);
		}
		sign_tx_input(htlc_tx[i], 0,
			      NULL,
			      wscript[i],
			      x_remote_htlcsecretkey, remote_htlckey,
			      SIGHASH_ALL,
			      &remotehtlcsig[i]);
		printf("# signature for output #%zi (%s for htlc #%"PRIu64")\n",
		       i, htlc_owner(htlc) == LOCAL ? "htlc-timeout" : "htlc-success", htlc->id);
		printf("remote_htlc_signature = %s\n",
		       type_to_string(tmpctx, secp256k1_ecdsa_signature,
				      &remotehtlcsig[i].s));

		sign_tx_input(htlc_tx[i], 0,
			      NULL,
			      wscript[i],
			      local_htlcsecretkey, local_htlckey,
			      SIGHASH_ALL,
			      &localhtlcsig);
		printf("# local_htlc_signature = %s\n",
		       type_to_string(tmpctx, secp256k1_ecdsa_signature,
				      &localhtlcsig.s));
		if (htlc_owner(htlc) == LOCAL) {
			htlc_timeout_tx_add_witness(htlc_tx[i],
						    local_htlckey,
						    remote_htlckey,
						    &htlc->rhash,
						    remote_revocation_key,
						    &localhtlcsig,
						    &remotehtlcsig[i],
						    option_anchor_outputs);
		} else {
			htlc_success_tx_add_witness(htlc_tx[i],
						    &htlc->expiry,
						    local_htlckey,
						    remote_htlckey,
						    &localhtlcsig,
						    &remotehtlcsig[i],
						    htlc->r,
						    remote_revocation_key,
						    option_anchor_outputs);
		}
		printf("htlc_%s_tx (htlc #%"PRIu64"): %s\n",
		       htlc_owner(htlc) == LOCAL ? "timeout" : "success",
		       htlc->id,
		       tal_hex(tmpctx, linearize_tx(tmpctx, htlc_tx[i])));
	}
}

static void report(struct bitcoin_tx *tx,
		   const u8 *wscript,
		   const struct privkey *x_remote_funding_privkey,
		   const struct pubkey *remote_funding_pubkey,
		   const struct privkey *local_funding_privkey,
		   const struct pubkey *local_funding_pubkey,
		   u16 to_self_delay,
		   const struct privkey *local_htlcsecretkey,
		   const struct pubkey *localkey,
		   const struct pubkey *local_htlckey,
		   const struct pubkey *local_delayedkey,
		   const struct privkey *x_remote_htlcsecretkey,
		   const struct pubkey *remotekey,
		   const struct pubkey *remote_htlckey,
		   const struct pubkey *remote_revocation_key,
		   u32 feerate_per_kw,
		   bool option_anchor_outputs,
		   const struct htlc **htlc_map)
{
	char *txhex;
	struct bitcoin_signature localsig, remotesig;
	u8 **witness;

	sign_tx_input(tx, 0,
		      NULL,
		      wscript,
		      x_remote_funding_privkey, remote_funding_pubkey,
		      SIGHASH_ALL,
		      &remotesig);
	printf("remote_signature = %s\n",
	       type_to_string(tmpctx, secp256k1_ecdsa_signature, &remotesig.s));
	sign_tx_input(tx, 0,
		      NULL,
		      wscript,
		      local_funding_privkey, local_funding_pubkey,
		      SIGHASH_ALL,
		      &localsig);
	printf("# local_signature = %s\n",
	       type_to_string(tmpctx, secp256k1_ecdsa_signature, &localsig.s));

	witness =
	    bitcoin_witness_2of2(tx, &localsig, &remotesig,
				 local_funding_pubkey, remote_funding_pubkey);
	bitcoin_tx_input_set_witness(tx, 0, take(witness));
	txhex = tal_hex(tmpctx, linearize_tx(tx, tx));
	printf("output commit_tx: %s\n", txhex);

	report_htlcs(tx, htlc_map, to_self_delay,
		     local_htlcsecretkey, localkey, local_htlckey,
		     local_delayedkey,
		     x_remote_htlcsecretkey,
		     remotekey, remote_htlckey,
		     remote_revocation_key,
		     feerate_per_kw,
		     option_anchor_outputs);
}

#ifdef DEBUG
static struct amount_sat calc_fee(const struct bitcoin_tx *tx, struct amount_sat input)
{
	size_t i;
	struct amount_sat output = AMOUNT_SAT(0), fee;

	for (i = 0; i < tal_count(tx->output); i++)
		output.satoshis += tx->output[i].amount;

	if (!amount_sub_sat(&fee, input, output))
		abort();
	return fee;
}

/* For debugging, we do brute-force increase to find thresholds */
static u32 increase(u32 feerate_per_kw)
{
	return feerate_per_kw + 1;
}
#else
static u64 increase(u32 feerate_per_kw)
{
	/* BOLT #3:
	 *
	 *     local_feerate_per_kw: 0
	 *     ...
	 *     local_feerate_per_kw: 648
	 *     ...
	 *     local_feerate_per_kw: 2070
	 *     ...
	 *     local_feerate_per_kw: 2195
	 *     ...
	 *     local_feerate_per_kw: 3703
	 *     ...
	 *     local_feerate_per_kw: 4915
	 *     ...
	 *     local_feerate_per_kw: 9651181
	 */
	const u64 rates[] = { 0, 648, 2070, 2195, 3703, 4915, 9651181 };
	size_t i;

	for (i = 0; i < ARRAY_SIZE(rates) - 1; i++)
		if (rates[i] == feerate_per_kw)
			return rates[i+1];

	abort();
}
#endif

/* HTLCs as seen from other side. */
static const struct htlc **invert_htlcs(const struct htlc **htlcs)
{
	size_t i, n = tal_count(htlcs);
	const struct htlc **inv = tal_arr(htlcs, const struct htlc *, n);

	for (i = 0; i < n; i++) {
		struct htlc *htlc;
		inv[i] = htlc = tal_dup(inv, struct htlc, htlcs[i]);
		if (inv[i]->state == RCVD_ADD_ACK_REVOCATION)
			htlc->state = SENT_ADD_ACK_REVOCATION;
		else {
			assert(inv[i]->state == SENT_ADD_ACK_REVOCATION);
			htlc->state = RCVD_ADD_ACK_REVOCATION;
		}
	}
	return inv;
}

int main(int argc, const char *argv[])
{
	common_setup(argv[0]);

	struct bitcoin_txid funding_txid;
	struct amount_sat funding_amount, dust_limit;
	u32 feerate_per_kw;
	u16 to_self_delay;
	/* x_ prefix means internal vars we used to derive spec */
	struct privkey local_funding_privkey, x_remote_funding_privkey;
	struct secret x_local_payment_basepoint_secret, x_remote_payment_basepoint_secret;
	struct secret x_local_htlc_basepoint_secret, x_remote_htlc_basepoint_secret;
	struct secret x_local_per_commitment_secret;
	struct secret x_local_delayed_payment_basepoint_secret;
	struct secret x_remote_revocation_basepoint_secret;
	struct privkey local_htlcsecretkey, x_remote_htlcsecretkey;
	struct privkey x_local_delayed_secretkey;
	struct pubkey local_funding_pubkey, remote_funding_pubkey;
	struct pubkey local_payment_basepoint, remote_payment_basepoint;
	struct pubkey local_htlc_basepoint, remote_htlc_basepoint;
	struct pubkey x_local_delayed_payment_basepoint;
	struct pubkey x_remote_revocation_basepoint;
	struct pubkey x_local_per_commitment_point;
	struct pubkey localkey, remotekey, tmpkey;
	struct pubkey local_htlckey, remote_htlckey;
	struct pubkey local_delayedkey;
	struct pubkey remote_revocation_key;
	struct bitcoin_tx *tx, *tx2;
	struct keyset keyset;
	u8 *wscript;
	unsigned int funding_output_index;
	u64 commitment_number, cn_obscurer;
	struct amount_msat to_local, to_remote;
	const struct htlc **htlcs, **htlc_map, **htlc_map2, **inv_htlcs;
	bool option_anchor_outputs = false;
	bool option_static_remotekey = false;

	/* Allow us to check static-remotekey BOLT 3 vectors, too */
	if (argv[1] && streq(argv[1], "--static-remotekey"))
		option_static_remotekey = true;

	chainparams = chainparams_for_network("bitcoin");

	htlcs = setup_htlcs_0_to_4(tmpctx);
	inv_htlcs = invert_htlcs(htlcs);

#if DEVELOPER
	/* This lets us match test vectors exactly. */
	extern bool dev_no_grind;

	dev_no_grind = true;
#endif /* DEVELOPER */

	/* BOLT #3:
	 *
	 * # Appendix C: Commitment and HTLC Transaction Test Vectors
	 *
	 * In the following:
	 *  - *local* transactions are considered, which implies that all
	 *    payments to *local* are delayed.
	 *  - It's assumed that *local* is the funder.
	 *  - Private keys are displayed as 32 bytes plus a trailing 1
	 *    (Bitcoin's convention for "compressed" private keys, i.e. keys for
	 *    which the public key is compressed).
	 *  - Transaction signatures are all deterministic, using RFC6979 (using
	 *    HMAC-SHA256).
	 *
	 * To start, common basic parameters for each test vector are defined:
	 * the HTLCs are not used for the first "simple commitment tx with no
	 * HTLCs" test, and HTLCs 5 and 6 are only used in the "same amount and
	 * preimage" test.
	 *
	 *     funding_tx_id: 8984484a580b825b9972d7adb15050b3ab624ccd731946b3eeddb92f4e7ef6be
	 *     funding_output_index: 0
	 *     funding_amount_satoshi: 10000000
	 *     commitment_number: 42
	 *     local_delay: 144
	 *     local_dust_limit_satoshi: 546
	 */
	funding_txid = txid_from_hex("8984484a580b825b9972d7adb15050b3ab624ccd731946b3eeddb92f4e7ef6be");
	funding_output_index = 0;
	funding_amount.satoshis = 10000000;
	commitment_number = 42;
	to_self_delay = 144;
	dust_limit.satoshis = 546;

#ifdef DEBUG
	print_superverbose = true;
#endif

	/* BOLT #3:
	 *
	 * <!-- The test vector values are derived, as per Key Derivation, though it's not
	 * required for this test. They're included here for completeness and
	 * in case someone wants to reproduce the test vectors themselves:
	 *
	 * INTERNAL: remote_funding_privkey: 1552dfba4f6cf29a62a0af13c8d6981d36d0ef8d61ba10fb0fe90da7634d7e1301
	 * INTERNAL: local_payment_basepoint_secret: 111111111111111111111111111111111111111111111111111111111111111101
	 * INTERNAL: remote_revocation_basepoint_secret: 222222222222222222222222222222222222222222222222222222222222222201
	 * INTERNAL: local_delayed_payment_basepoint_secret: 333333333333333333333333333333333333333333333333333333333333333301
	 * INTERNAL: remote_payment_basepoint_secret: 444444444444444444444444444444444444444444444444444444444444444401
	 * x_local_per_commitment_secret: 1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a0908070605040302010001
	 * # From remote_revocation_basepoint_secret
	 * INTERNAL: remote_revocation_basepoint: 02466d7fcae563e5cb09a0d1870bb580344804617879a14949cf22285f1bae3f27
	 * # From local_delayed_payment_basepoint_secret
	 * INTERNAL: local_delayed_payment_basepoint: 023c72addb4fdf09af94f0c94d7fe92a386a7e70cf8a1d85916386bb2535c7b1b1
	 * INTERNAL: local_per_commitment_point: 025f7117a78150fe2ef97db7cfc83bd57b2e2c0d0dd25eaf467a4a1c2a45ce1486
	 * INTERNAL: remote_privkey: 8deba327a7cc6d638ab0eb025770400a6184afcba6713c210d8d10e199ff2fda01
	 * # From local_delayed_payment_basepoint_secret, local_per_commitment_point and local_delayed_payment_basepoint
	 * INTERNAL: local_delayed_privkey: adf3464ce9c2f230fd2582fda4c6965e4993ca5524e8c9580e3df0cf226981ad01
	 * -->
	 */
	local_funding_privkey.secret = secret_from_hex("30ff4956bbdd3222d44cc5e8a1261dab1e07957bdac5ae88fe3261ef321f374901");
	x_remote_funding_privkey.secret = secret_from_hex("1552dfba4f6cf29a62a0af13c8d6981d36d0ef8d61ba10fb0fe90da7634d7e1301");
	SUPERVERBOSE("INTERNAL: remote_funding_privkey: %s01\n",
		     type_to_string(tmpctx, struct privkey,
				    &x_remote_funding_privkey));
	x_local_payment_basepoint_secret = secret_from_hex("1111111111111111111111111111111111111111111111111111111111111111");
	SUPERVERBOSE("INTERNAL: local_payment_basepoint_secret: %s\n",
		     type_to_string(tmpctx, struct secret,
				    &x_local_payment_basepoint_secret));
	x_remote_revocation_basepoint_secret = secret_from_hex("2222222222222222222222222222222222222222222222222222222222222222");
	SUPERVERBOSE("INTERNAL: remote_revocation_basepoint_secret: %s\n",
		     type_to_string(tmpctx, struct secret,
				    &x_remote_revocation_basepoint_secret));
	x_local_delayed_payment_basepoint_secret = secret_from_hex("3333333333333333333333333333333333333333333333333333333333333333");
	SUPERVERBOSE("INTERNAL: local_delayed_payment_basepoint_secret: %s\n",
		     type_to_string(tmpctx, struct secret,
				    &x_local_delayed_payment_basepoint_secret));
	x_remote_payment_basepoint_secret = secret_from_hex("4444444444444444444444444444444444444444444444444444444444444444");
	SUPERVERBOSE("INTERNAL: remote_payment_basepoint_secret: %s\n",
		     type_to_string(tmpctx, struct secret,
				    &x_remote_payment_basepoint_secret));
	x_local_per_commitment_secret = secret_from_hex("0x1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a09080706050403020100");
	SUPERVERBOSE("x_local_per_commitment_secret: %s\n",
		     type_to_string(tmpctx, struct secret,
				    &x_local_per_commitment_secret));

	if (!pubkey_from_secret(&x_remote_revocation_basepoint_secret,
				&x_remote_revocation_basepoint))
 		abort();
	SUPERVERBOSE("# From remote_revocation_basepoint_secret\n"
		     "INTERNAL: remote_revocation_basepoint: %s\n",
		     type_to_string(tmpctx, struct pubkey,
				    &x_remote_revocation_basepoint));

	if (!pubkey_from_secret(&x_local_delayed_payment_basepoint_secret,
				&x_local_delayed_payment_basepoint))
		abort();
	SUPERVERBOSE("# From local_delayed_payment_basepoint_secret\n"
		     "INTERNAL: local_delayed_payment_basepoint: %s\n",
		     type_to_string(tmpctx, struct pubkey,
				    &x_local_delayed_payment_basepoint));

	if (!pubkey_from_secret(&x_local_per_commitment_secret,
				&x_local_per_commitment_point))
		abort();
	SUPERVERBOSE("INTERNAL: local_per_commitment_point: %s\n",
		     type_to_string(tmpctx, struct pubkey,
				    &x_local_per_commitment_point));

	if (!pubkey_from_secret(&x_local_payment_basepoint_secret,
				&local_payment_basepoint))
		abort();

	if (!pubkey_from_secret(&x_remote_payment_basepoint_secret,
				&remote_payment_basepoint))
		abort();

	/* FIXME: BOLT should include separate HTLC keys */
	local_htlc_basepoint = local_payment_basepoint;
	remote_htlc_basepoint = remote_payment_basepoint;
	x_local_htlc_basepoint_secret = x_local_payment_basepoint_secret;
	x_remote_htlc_basepoint_secret = x_remote_payment_basepoint_secret;

	if (!derive_simple_privkey(&x_remote_htlc_basepoint_secret,
				   &remote_htlc_basepoint,
				   &x_local_per_commitment_point,
				   &x_remote_htlcsecretkey))
		abort();
	SUPERVERBOSE("INTERNAL: remote_secretkey: %s\n",
		     type_to_string(tmpctx, struct privkey, &x_remote_htlcsecretkey));

	if (!derive_simple_privkey(&x_local_delayed_payment_basepoint_secret,
				   &x_local_delayed_payment_basepoint,
				   &x_local_per_commitment_point,
				   &x_local_delayed_secretkey))
		abort();
	SUPERVERBOSE("# From local_delayed_payment_basepoint_secret, local_per_commitment_point and local_delayed_payment_basepoint\n"
		     "INTERNAL: local_delayed_secretkey: %s\n",
		     type_to_string(tmpctx, struct privkey,
				    &x_local_delayed_secretkey));

	/* These two needed to calculate obscuring factor */
	printf("local_payment_basepoint: %s\n",
	       type_to_string(tmpctx, struct pubkey, &local_payment_basepoint));
	printf("remote_payment_basepoint: %s\n",
	       type_to_string(tmpctx, struct pubkey,&remote_payment_basepoint));
	cn_obscurer = commit_number_obscurer(&local_payment_basepoint,
					     &remote_payment_basepoint);
	printf("# obscured commitment transaction number = 0x%"PRIx64" ^ %"PRIu64"\n",
	       cn_obscurer, commitment_number);


	printf("local_funding_privkey: %s01\n",
	       type_to_string(tmpctx, struct privkey, &local_funding_privkey));
	if (!pubkey_from_privkey(&local_funding_privkey, &local_funding_pubkey))
		abort();
	printf("local_funding_pubkey: %s\n",
	       type_to_string(tmpctx, struct pubkey, &local_funding_pubkey));

	if (!pubkey_from_privkey(&x_remote_funding_privkey, &remote_funding_pubkey))
		abort();
	printf("remote_funding_pubkey: %s\n",
	       type_to_string(tmpctx, struct pubkey, &remote_funding_pubkey));

	if (!derive_simple_privkey(&x_local_htlc_basepoint_secret,
				   &local_payment_basepoint,
				   &x_local_per_commitment_point,
				   &local_htlcsecretkey))
		abort();
	printf("local_secretkey: %s\n",
	       type_to_string(tmpctx, struct privkey, &local_htlcsecretkey));

	if (!derive_simple_key(&local_payment_basepoint,
			       &x_local_per_commitment_point,
			       &localkey))
		abort();
	printf("localkey: %s\n",
	       type_to_string(tmpctx, struct pubkey, &localkey));

	if (option_static_remotekey)
		remotekey = remote_payment_basepoint;
	else {
		if (!derive_simple_key(&remote_payment_basepoint,
				       &x_local_per_commitment_point,
				       &remotekey))
			abort();
	}
	printf("remotekey: %s\n",
	       type_to_string(tmpctx, struct pubkey, &remotekey));

	if (!pubkey_from_privkey(&local_htlcsecretkey, &local_htlckey))
		abort();
	if (!derive_simple_key(&local_htlc_basepoint,
			       &x_local_per_commitment_point,
			       &tmpkey))
		abort();
	assert(pubkey_eq(&tmpkey, &local_htlckey));
	printf("local_htlckey: %s\n",
	       type_to_string(tmpctx, struct pubkey, &local_htlckey));

	if (!derive_simple_key(&remote_htlc_basepoint,
			       &x_local_per_commitment_point,
			       &remote_htlckey))
		abort();
	printf("remote_htlckey: %s\n",
	       type_to_string(tmpctx, struct pubkey, &remote_htlckey));

	if (!pubkey_from_privkey(&x_local_delayed_secretkey, &local_delayedkey))
		abort();
	if (!derive_simple_key(&x_local_delayed_payment_basepoint,
			       &x_local_per_commitment_point,
			       &tmpkey))
		abort();
	assert(pubkey_eq(&tmpkey, &local_delayedkey));
	printf("local_delayedkey: %s\n",
	       type_to_string(tmpctx, struct pubkey, &local_delayedkey));

	if (!derive_revocation_key(&x_remote_revocation_basepoint,
				   &x_local_per_commitment_point,
				   &remote_revocation_key))
		abort();
	printf("remote_revocation_key: %s\n",
	       type_to_string(tmpctx, struct pubkey, &remote_revocation_key));

	wscript = bitcoin_redeem_2of2(tmpctx, &local_funding_pubkey,
				      &remote_funding_pubkey);
	printf("# funding wscript = %s\n", tal_hex(tmpctx, wscript));

	/* BOLT #3:
	 *
	 *    name: simple commitment tx with no HTLCs
	 *    to_local_msat: 7000000000
	 *    to_remote_msat: 3000000000
	 *    local_feerate_per_kw: 15000
	 */
	to_local.millisatoshis = 7000000000;
	to_remote.millisatoshis = 3000000000;
	feerate_per_kw = 15000;
	printf("\n"
	       "name: simple commitment tx with no HTLCs\n"
	       "to_local_msat: %"PRIu64"\n"
	       "to_remote_msat: %"PRIu64"\n"
	       "local_feerate_per_kw: %u\n",
	       to_local.millisatoshis, to_remote.millisatoshis, feerate_per_kw);

	keyset.self_revocation_key = remote_revocation_key;
	keyset.self_delayed_payment_key = local_delayedkey;
	keyset.self_payment_key = localkey;
	keyset.other_payment_key = remotekey;
	keyset.self_htlc_key = local_htlckey;
	keyset.other_htlc_key = remote_htlckey;

	print_superverbose = true;
	tx = commit_tx(tmpctx,
		       &funding_txid, funding_output_index,
		       funding_amount,
		       &local_funding_pubkey,
		       &remote_funding_pubkey,
		       LOCAL, to_self_delay,
		       &keyset,
		       feerate_per_kw,
		       dust_limit,
		       to_local,
		       to_remote,
		       NULL, &htlc_map, NULL, commitment_number ^ cn_obscurer,
		       option_anchor_outputs,
		       LOCAL);
	print_superverbose = false;
	tx2 = commit_tx(tmpctx,
			&funding_txid, funding_output_index,
			funding_amount,
			&local_funding_pubkey,
			&remote_funding_pubkey,
			REMOTE, to_self_delay,
			&keyset,
			feerate_per_kw,
			dust_limit,
			to_local,
			to_remote,
			NULL, &htlc_map2, NULL, commitment_number ^ cn_obscurer,
			option_anchor_outputs,
			REMOTE);
	tx_must_be_eq(tx, tx2);
	report(tx, wscript, &x_remote_funding_privkey, &remote_funding_pubkey,
	       &local_funding_privkey, &local_funding_pubkey,
	       to_self_delay,
	       &local_htlcsecretkey,
	       &localkey,
	       &local_htlckey,
	       &local_delayedkey,
	       &x_remote_htlcsecretkey,
	       &remotekey,
	       &remote_htlckey,
	       &remote_revocation_key,
	       feerate_per_kw,
	       option_anchor_outputs,
	       htlc_map);

	/* BOLT #3:
	 *
	 *    name: commitment tx with all five HTLCs untrimmed (minimum feerate)
	 *    to_local_msat: 6988000000
	 *    to_remote_msat: 3000000000
	 *    local_feerate_per_kw: 0
	 */
	to_local.millisatoshis = 6988000000;
	to_remote.millisatoshis = 3000000000;
	feerate_per_kw = 0;
	printf("\n"
	       "name: commitment tx with all five HTLCs untrimmed (minimum feerate)\n"
	       "to_local_msat: %"PRIu64"\n"
	       "to_remote_msat: %"PRIu64"\n"
	       "local_feerate_per_kw: %u\n",
	       to_local.millisatoshis, to_remote.millisatoshis, feerate_per_kw);

	print_superverbose = true;
	tx = commit_tx(tmpctx,
		       &funding_txid, funding_output_index,
		       funding_amount,
		       &local_funding_pubkey,
		       &remote_funding_pubkey,
		       LOCAL, to_self_delay,
		       &keyset,
		       feerate_per_kw,
		       dust_limit,
		       to_local,
		       to_remote,
		       htlcs, &htlc_map, NULL, commitment_number ^ cn_obscurer,
		       option_anchor_outputs,
		       LOCAL);
	print_superverbose = false;
	tx2 = commit_tx(tmpctx,
			&funding_txid, funding_output_index,
			funding_amount,
			&local_funding_pubkey,
			&remote_funding_pubkey,
			REMOTE, to_self_delay,
			&keyset,
			feerate_per_kw,
			dust_limit,
			to_local,
			to_remote,
			inv_htlcs, &htlc_map2, NULL,
			commitment_number ^ cn_obscurer,
			option_anchor_outputs,
			REMOTE);
	tx_must_be_eq(tx, tx2);
	report(tx, wscript, &x_remote_funding_privkey, &remote_funding_pubkey,
	       &local_funding_privkey, &local_funding_pubkey,
	       to_self_delay,
	       &local_htlcsecretkey,
	       &localkey,
	       &local_htlckey,
	       &local_delayedkey,
	       &x_remote_htlcsecretkey,
	       &remotekey,
	       &remote_htlckey,
	       &remote_revocation_key,
	       feerate_per_kw,
	       option_anchor_outputs,
	       htlc_map);

	do {
		struct bitcoin_tx *newtx;

		feerate_per_kw = increase(feerate_per_kw);
		print_superverbose = false;
		newtx = commit_tx(tmpctx,
				  &funding_txid, funding_output_index,
				  funding_amount,
				  &local_funding_pubkey,
				  &remote_funding_pubkey,
				  LOCAL, to_self_delay,
				  &keyset,
				  feerate_per_kw,
				  dust_limit,
				  to_local,
				  to_remote,
				  htlcs, &htlc_map, NULL,
				  commitment_number ^ cn_obscurer,
				  option_anchor_outputs,
				  LOCAL);
		/* This is what it would look like for peer generating it! */
		tx2 = commit_tx(tmpctx,
				&funding_txid, funding_output_index,
				funding_amount,
				&local_funding_pubkey,
				&remote_funding_pubkey,
				REMOTE, to_self_delay,
				&keyset,
				feerate_per_kw,
				dust_limit,
				to_local,
				to_remote,
				inv_htlcs, &htlc_map2, NULL,
				commitment_number ^ cn_obscurer,
				option_anchor_outputs,
				REMOTE);
		tx_must_be_eq(newtx, tx2);
#ifdef DEBUG
		if (feerate_per_kw % 100000 == 0)
			printf("feerate_per_kw = %u, fees = %"PRIu64"\n",
			       feerate_per_kw, calc_fee(newtx, funding_amount));
		if (tal_count(newtx->output) == tal_count(tx->output)) {
			tal_free(newtx);
			continue;
		}
#endif
		printf("\n"
		       "name: commitment tx with %s untrimmed (maximum feerate)\n"
		       "to_local_msat: %"PRIu64"\n"
		       "to_remote_msat: %"PRIu64"\n"
		       "local_feerate_per_kw: %u\n",
		       /* Spec was "neatened" to change these numbers to words! */
		       tx->wtx->num_outputs == 7 ? "seven outputs"
		       : tx->wtx->num_outputs == 6 ? "six outputs"
		       : tx->wtx->num_outputs == 5 ? "five outputs"
		       : tx->wtx->num_outputs == 4 ? "four outputs"
		       : tx->wtx->num_outputs == 3 ? "three outputs"
		       : tx->wtx->num_outputs == 2 ? "two outputs"
		       : tx->wtx->num_outputs == 1 ? "one output"
		       : "no outputs???",
		       to_local.millisatoshis, to_remote.millisatoshis, feerate_per_kw-1);
		/* Recalc with verbosity on */
		print_superverbose = true;
		tx = commit_tx(tmpctx,
			       &funding_txid, funding_output_index,
			       funding_amount,
			       &local_funding_pubkey,
			       &remote_funding_pubkey,
			       LOCAL, to_self_delay,
			       &keyset,
			       feerate_per_kw-1,
			       dust_limit,
			       to_local,
			       to_remote,
			       htlcs, &htlc_map, NULL,
			       commitment_number ^ cn_obscurer,
			       option_anchor_outputs,
			       LOCAL);
		report(tx, wscript,
		       &x_remote_funding_privkey, &remote_funding_pubkey,
		       &local_funding_privkey, &local_funding_pubkey,
		       to_self_delay,
		       &local_htlcsecretkey,
		       &localkey,
		       &local_htlckey,
		       &local_delayedkey,
		       &x_remote_htlcsecretkey,
		       &remotekey,
		       &remote_htlckey,
		       &remote_revocation_key,
		       feerate_per_kw-1,
		       option_anchor_outputs,
		       htlc_map);

		printf("\n"
		       "name: commitment tx with %s untrimmed (minimum feerate)\n"
		       "to_local_msat: %"PRIu64"\n"
		       "to_remote_msat: %"PRIu64"\n"
		       "local_feerate_per_kw: %u\n",
		       /* Spec was "neatened" to change these numbers to words! */
		       newtx->wtx->num_outputs == 7 ? "seven outputs"
		       : newtx->wtx->num_outputs == 6 ? "six outputs"
		       : newtx->wtx->num_outputs == 5 ? "five outputs"
		       : newtx->wtx->num_outputs == 4 ? "four outputs"
		       : newtx->wtx->num_outputs == 3 ? "three outputs"
		       : newtx->wtx->num_outputs == 2 ? "two outputs"
		       : newtx->wtx->num_outputs == 1 ? "one output"
		       : "no outputs???",
		       to_local.millisatoshis, to_remote.millisatoshis, feerate_per_kw);
		/* Recalc with verbosity on */
		print_superverbose = true;
		newtx = commit_tx(tmpctx,
				  &funding_txid, funding_output_index,
				  funding_amount,
				  &local_funding_pubkey,
				  &remote_funding_pubkey,
				  LOCAL, to_self_delay,
				  &keyset,
				  feerate_per_kw,
				  dust_limit,
				  to_local,
				  to_remote,
				  htlcs, &htlc_map, NULL,
				  commitment_number ^ cn_obscurer,
				  option_anchor_outputs,
				  LOCAL);
		report(newtx, wscript,
		       &x_remote_funding_privkey, &remote_funding_pubkey,
		       &local_funding_privkey, &local_funding_pubkey,
		       to_self_delay,
		       &local_htlcsecretkey,
		       &localkey,
		       &local_htlckey,
		       &local_delayedkey,
		       &x_remote_htlcsecretkey,
		       &remotekey,
		       &remote_htlckey,
		       &remote_revocation_key,
		       feerate_per_kw,
		       option_anchor_outputs,
		       htlc_map);

		assert(newtx->wtx->num_outputs != tx->wtx->num_outputs);

		tal_free(tx);
		tx = newtx;
	} while (tx->wtx->num_outputs > 1);

	/* Now make sure we cover case where opener can't afford the fee;
	 * its output cannot go negative! */
	for (;;) {
		struct amount_sat base_fee
			= commit_tx_base_fee(feerate_per_kw, 0,
					     option_anchor_outputs);

		/* BOLT #3:
		 * If `option_anchor_outputs` applies to the commitment
		 * transaction, also subtract two times the fixed anchor size
		 * of 330 sats from the funder (either `to_local` or
		 * `to_remote`).
		 */
		if (option_anchor_outputs
		    && !amount_sat_add(&base_fee, base_fee, AMOUNT_SAT(660)))
			abort();

		if (amount_msat_greater_eq_sat(to_local, base_fee)) {
			feerate_per_kw++;
			continue;
		}

		/* BOLT #3:
		 *
		 *    name: commitment tx with fee greater than funder amount
		 *    to_local_msat: 6988000000
		 *    to_remote_msat: 3000000000
		 *    local_feerate_per_kw: 9651936
		 */
		assert(feerate_per_kw == 9651936);

		printf("\n"
		       "name: commitment tx with fee greater than funder amount\n"
		       "to_local_msat: %"PRIu64"\n"
		       "to_remote_msat: %"PRIu64"\n"
		       "local_feerate_per_kw: %u\n",
		       to_local.millisatoshis, to_remote.millisatoshis, feerate_per_kw);
		tx = commit_tx(tmpctx,
			       &funding_txid, funding_output_index,
			       funding_amount,
			       &local_funding_pubkey,
			       &remote_funding_pubkey,
			       LOCAL, to_self_delay,
			       &keyset,
			       feerate_per_kw,
			       dust_limit,
			       to_local,
			       to_remote,
			       htlcs, &htlc_map, NULL,
			       commitment_number ^ cn_obscurer,
			       option_anchor_outputs,
			       LOCAL);
		report(tx, wscript,
		       &x_remote_funding_privkey, &remote_funding_pubkey,
		       &local_funding_privkey, &local_funding_pubkey,
		       to_self_delay,
		       &local_htlcsecretkey,
		       &localkey,
		       &local_htlckey,
		       &local_delayedkey,
		       &x_remote_htlcsecretkey,
		       &remotekey,
		       &remote_htlckey,
		       &remote_revocation_key,
		       feerate_per_kw,
		       option_anchor_outputs,
		       htlc_map);
		break;
	}

	htlcs = setup_htlcs_1_5_and_6(tmpctx);
	inv_htlcs = invert_htlcs(htlcs);

	/* BOLT #3:
	 *
	 *     name: commitment tx with 3 htlc outputs, 2 offered having the same amount and preimage
	 *     to_local_msat: 6988000000
	 *     to_remote_msat: 3000000000
	 *     local_feerate_per_kw: 253
	 */
	to_local.millisatoshis = 6988000000;
	to_remote.millisatoshis = 3000000000;
	feerate_per_kw = 253;
	printf("\n"
	       "name: commitment tx with 3 htlc outputs, 2 offered having the same amount and preimage\n"
	       "to_local_msat: %"PRIu64"\n"
	       "to_remote_msat: %"PRIu64"\n"
	       "local_feerate_per_kw: %u\n",
	       to_local.millisatoshis, to_remote.millisatoshis, feerate_per_kw);

	print_superverbose = true;
	tx = commit_tx(tmpctx,
		       &funding_txid, funding_output_index,
		       funding_amount,
		       &local_funding_pubkey,
		       &remote_funding_pubkey,
		       LOCAL, to_self_delay,
		       &keyset,
		       feerate_per_kw,
		       dust_limit,
		       to_local,
		       to_remote,
		       htlcs, &htlc_map, NULL, commitment_number ^ cn_obscurer,
		       option_anchor_outputs,
		       LOCAL);
	print_superverbose = false;
	tx2 = commit_tx(tmpctx,
			&funding_txid, funding_output_index,
			funding_amount,
			&local_funding_pubkey,
			&remote_funding_pubkey,
			REMOTE, to_self_delay,
			&keyset,
			feerate_per_kw,
			dust_limit,
			to_local,
			to_remote,
			inv_htlcs, &htlc_map2, NULL,
			commitment_number ^ cn_obscurer,
			option_anchor_outputs,
			REMOTE);
	tx_must_be_eq(tx, tx2);
	report(tx, wscript, &x_remote_funding_privkey, &remote_funding_pubkey,
	       &local_funding_privkey, &local_funding_pubkey,
	       to_self_delay,
	       &local_htlcsecretkey,
	       &localkey,
	       &local_htlckey,
	       &local_delayedkey,
	       &x_remote_htlcsecretkey,
	       &remotekey,
	       &remote_htlckey,
	       &remote_revocation_key,
	       feerate_per_kw,
	       option_anchor_outputs,
	       htlc_map);
	common_shutdown();

	/* FIXME: Do BOLT comparison! */
	return 0;
}
