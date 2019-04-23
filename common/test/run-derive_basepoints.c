#include "../derive_basepoints.c"
#include <ccan/err/err.h>
#include <ccan/mem/mem.h>
#include <ccan/str/hex/hex.h>
#include <ccan/structeq/structeq.h>
#include <common/utils.h>
#include <stdio.h>
#include <wally_core.h>

/* AUTOGENERATED MOCKS START */
/* Generated stub for fromwire_pubkey */
void fromwire_pubkey(const u8 **cursor UNNEEDED, size_t *max UNNEEDED, struct pubkey *pubkey UNNEEDED)
{ fprintf(stderr, "fromwire_pubkey called!\n"); abort(); }
/* Generated stub for towire_pubkey */
void towire_pubkey(u8 **pptr UNNEEDED, const struct pubkey *pubkey UNNEEDED)
{ fprintf(stderr, "towire_pubkey called!\n"); abort(); }
/* AUTOGENERATED MOCKS END */

STRUCTEQ_DEF(basepoints, 0,
	     revocation.pubkey,
	     payment.pubkey,
	     htlc.pubkey,
	     delayed_payment.pubkey);
STRUCTEQ_DEF(secrets, 0,
	     funding_privkey.secret.data,
	     revocation_basepoint_secret.data,
	     payment_basepoint_secret.data,
	     htlc_basepoint_secret.data,
	     delayed_payment_basepoint_secret.data);
STRUCTEQ_DEF(privkey, 0,
	     secret.data);

struct info {
	struct secret seed;
	struct pubkey funding_pubkey;
	struct basepoints basepoints;
	struct secrets secrets;
	struct sha256 shaseed;
};

/* We get a fresh one each time, to catch uninitialized fields */
static struct info *new_info(const tal_t *ctx)
{
	struct info *info = tal(ctx, struct info);
	memset(&info->seed, 7, sizeof(info->seed));

	return info;
}

int main(void)
{
	setup_locale();

	const tal_t *ctx = tal(NULL, char);
	struct info *baseline, *info;

	wally_init(0);
	secp256k1_ctx = wally_get_secp_context();
	baseline = new_info(ctx);
	assert(derive_basepoints(&baseline->seed, &baseline->funding_pubkey,
				 &baseline->basepoints,
				 &baseline->secrets,
				 &baseline->shaseed));

	/* Same seed, same result. */
	info = new_info(ctx);
	assert(derive_basepoints(&info->seed, &info->funding_pubkey,
				 &info->basepoints,
				 &info->secrets,
				 &info->shaseed));
	assert(pubkey_eq(&baseline->funding_pubkey, &info->funding_pubkey));
	assert(basepoints_eq(&baseline->basepoints, &info->basepoints));
	assert(secrets_eq(&baseline->secrets, &info->secrets));
	assert(sha256_eq(&baseline->shaseed, &info->shaseed));

	/* Different seed, different result. */
	for (size_t i = 0; i < sizeof(info->seed); i++) {
		for (size_t b = 0; b < CHAR_BIT; b++) {
			info = new_info(ctx);
			info->seed.data[i] ^= (1 << b);

			assert(derive_basepoints(&info->seed,
						 &info->funding_pubkey,
						 &info->basepoints,
						 &info->secrets,
						 &info->shaseed));
			assert(!pubkey_eq(&baseline->funding_pubkey,
					  &info->funding_pubkey));
			assert(!basepoints_eq(&baseline->basepoints,
					      &info->basepoints));
			assert(!secrets_eq(&baseline->secrets, &info->secrets));
			assert(!sha256_eq(&baseline->shaseed, &info->shaseed));
		}
	}

	/* Any field can be NULL (except seed). */
	info = new_info(ctx);
	assert(derive_basepoints(&info->seed, NULL,
				 &info->basepoints,
				 &info->secrets,
				 &info->shaseed));
	assert(basepoints_eq(&baseline->basepoints, &info->basepoints));
	assert(secrets_eq(&baseline->secrets, &info->secrets));
	assert(sha256_eq(&baseline->shaseed, &info->shaseed));

	info = new_info(ctx);
	assert(derive_basepoints(&info->seed, &info->funding_pubkey,
				 NULL,
				 &info->secrets,
				 &info->shaseed));
	assert(pubkey_eq(&baseline->funding_pubkey, &info->funding_pubkey));
	assert(secrets_eq(&baseline->secrets, &info->secrets));
	assert(sha256_eq(&baseline->shaseed, &info->shaseed));

	info = new_info(ctx);
	assert(derive_basepoints(&info->seed, &info->funding_pubkey,
				 &info->basepoints,
				 NULL,
				 &info->shaseed));
	assert(pubkey_eq(&baseline->funding_pubkey, &info->funding_pubkey));
	assert(basepoints_eq(&baseline->basepoints, &info->basepoints));
	assert(sha256_eq(&baseline->shaseed, &info->shaseed));

	info = new_info(ctx);
	assert(derive_basepoints(&info->seed, &info->funding_pubkey,
				 &info->basepoints,
				 &info->secrets,
				 NULL));
	assert(pubkey_eq(&baseline->funding_pubkey, &info->funding_pubkey));
	assert(basepoints_eq(&baseline->basepoints, &info->basepoints));
	assert(secrets_eq(&baseline->secrets, &info->secrets));

	/* derive_payment_basepoint should give same results. */
	info = new_info(ctx);
	assert(derive_payment_basepoint(&info->seed, &info->basepoints.payment,
					&info->secrets.payment_basepoint_secret));
	assert(pubkey_eq(&baseline->basepoints.payment,
			 &info->basepoints.payment));
	assert(secret_eq_consttime(&baseline->secrets.payment_basepoint_secret,
				   &info->secrets.payment_basepoint_secret));

	/* derive_funding_key should give same results. */
	info = new_info(ctx);
	assert(derive_funding_key(&info->seed, &info->funding_pubkey,
				  &info->secrets.funding_privkey));
	assert(pubkey_eq(&baseline->funding_pubkey, &info->funding_pubkey));
	assert(privkey_eq(&baseline->secrets.funding_privkey,
			  &info->secrets.funding_privkey));

	/* derive_shaseed should give same results. */
	info = new_info(ctx);
	assert(derive_shaseed(&info->seed, &info->shaseed));
	assert(sha256_eq(&baseline->shaseed, &info->shaseed));

	/* derive_revocation_basepoint should give same results. */
	info = new_info(ctx);
	assert(derive_revocation_basepoint(&info->seed, &info->basepoints.revocation,
					&info->secrets.revocation_basepoint_secret));
	assert(pubkey_eq(&baseline->basepoints.revocation,
			 &info->basepoints.revocation));
	assert(secret_eq_consttime(&baseline->secrets.revocation_basepoint_secret,
				   &info->secrets.revocation_basepoint_secret));

	/* derive_htlc_basepoint should give same results. */
	info = new_info(ctx);
	assert(derive_htlc_basepoint(&info->seed, &info->basepoints.htlc,
					&info->secrets.htlc_basepoint_secret));
	assert(pubkey_eq(&baseline->basepoints.htlc,
			 &info->basepoints.htlc));
	assert(secret_eq_consttime(&baseline->secrets.htlc_basepoint_secret,
				   &info->secrets.htlc_basepoint_secret));

	tal_free(ctx);
	wally_cleanup(0);
	return 0;
}
