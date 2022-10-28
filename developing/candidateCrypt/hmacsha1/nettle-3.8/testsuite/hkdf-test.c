#include "testutils.h"
#include "hkdf.h"
#include "hmac.h"

static void
test_hkdf_sha256(const struct tstring *ikm,
		 const struct tstring *salt,
		 const struct tstring *info,
		 const struct tstring *extract_output,
		 const struct tstring *expand_output)
{
  struct hmac_sha256_ctx ctx;
  uint8_t prk[SHA256_DIGEST_SIZE];
  uint8_t *buffer = xalloc(expand_output->length);

  hmac_sha256_set_key(&ctx, salt->length, salt->data);
  hkdf_extract(&ctx,
	       (nettle_hash_update_func*) hmac_sha256_update,
	       (nettle_hash_digest_func*) hmac_sha256_digest,
	       SHA256_DIGEST_SIZE,
	       ikm->length, ikm->data, prk);

  if (MEMEQ(SHA256_DIGEST_SIZE, prk, extract_output->data) == 0)
    {
      fprintf(stdout, "\nGot:\n");
      print_hex(SHA256_DIGEST_SIZE, prk);
      fprintf(stdout, "\nExpected:\n");
      print_hex(extract_output->length, extract_output->data);
      abort();
    }

  hmac_sha256_set_key(&ctx, SHA256_DIGEST_SIZE, prk);
  hkdf_expand(&ctx,
	      (nettle_hash_update_func*) hmac_sha256_update,
	      (nettle_hash_digest_func*) hmac_sha256_digest,
	      SHA256_DIGEST_SIZE,
	      info->length, info->data,
	      expand_output->length, buffer);

  if (MEMEQ(expand_output->length, expand_output->data, buffer) == 0)
    {
      fprintf(stdout, "\nGot:\n");
      print_hex(expand_output->length, buffer);
      fprintf(stdout, "\nExpected:\n");
      print_hex(expand_output->length, expand_output->data);
      abort();
    }
  free(buffer);
}

static void
test_hkdf_sha1(const struct tstring *ikm,
	       const struct tstring *salt,
	       const struct tstring *info,
	       const struct tstring *extract_output,
	       const struct tstring *expand_output)
{
  struct hmac_sha1_ctx ctx;
  uint8_t prk[SHA1_DIGEST_SIZE];
  uint8_t *buffer = xalloc(expand_output->length);

  hmac_sha1_set_key(&ctx, salt->length, salt->data);
  hkdf_extract(&ctx,
	       (nettle_hash_update_func*) hmac_sha1_update,
	       (nettle_hash_digest_func*) hmac_sha1_digest,
	       SHA1_DIGEST_SIZE,
	       ikm->length, ikm->data,
	       prk);

  if (MEMEQ(SHA1_DIGEST_SIZE, prk, extract_output->data) == 0)
    {
      fprintf(stdout, "\nGot:\n");
      print_hex(SHA1_DIGEST_SIZE, prk);
      fprintf(stdout, "\nExpected:\n");
      print_hex(extract_output->length, extract_output->data);
      abort();
    }

  hmac_sha1_set_key(&ctx, SHA1_DIGEST_SIZE, prk);
  hkdf_expand(&ctx,
	      (nettle_hash_update_func*) hmac_sha1_update,
	      (nettle_hash_digest_func*) hmac_sha1_digest,
	      SHA1_DIGEST_SIZE,
	      info->length, info->data,
	      expand_output->length, buffer);

  if (MEMEQ(expand_output->length, expand_output->data, buffer) == 0)
    {
      fprintf(stdout, "\nGot:\n");
      print_hex(expand_output->length, buffer);
      fprintf(stdout, "\nExpected:\n");
      print_hex(expand_output->length, expand_output->data);
      abort();
    }
  free(buffer);
}

void
test_main(void)
{
  /* HKDF test vectors from RFC5869 */
  test_hkdf_sha256(SHEX("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"),
	SHEX("000102030405060708090a0b0c"),
	SHEX("f0f1f2f3f4f5f6f7f8f9"),
	SHEX("077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5"),
	SHEX("3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865"));

  test_hkdf_sha256(SHEX("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f"),
	SHEX("606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf"),
	SHEX("b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"),
	SHEX("06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244"),
	SHEX("b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c59045a99cac7827271cb41c65e590e09da3275600c2f09b8367793a9aca3db71cc30c58179ec3e87c14c01d5c1f3434f1d87"));

  test_hkdf_sha256(SHEX("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"),
	SDATA(""),
	SDATA(""),
	SHEX("19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04"),
	SHEX("8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8"));

  test_hkdf_sha1(SHEX("0b0b0b0b0b0b0b0b0b0b0b"),
	SHEX("000102030405060708090a0b0c"),
	SHEX("f0f1f2f3f4f5f6f7f8f9"),
	SHEX("9b6c18c432a7bf8f0e71c8eb88f4b30baa2ba243"),
	SHEX("085a01ea1b10f36933068b56efa5ad81a4f14b822f5b091568a9cdd4f155fda2c22e422478d305f3f896"));

  test_hkdf_sha1(SHEX("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f"),
	SHEX("606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf"),
	SHEX("b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"),
	SHEX("8adae09a2a307059478d309b26c4115a224cfaf6"),
	SHEX("0bd770a74d1160f7c9f12cd5912a06ebff6adcae899d92191fe4305673ba2ffe8fa3f1a4e5ad79f3f334b3b202b2173c486ea37ce3d397ed034c7f9dfeb15c5e927336d0441f4c4300e2cff0d0900b52d3b4"));

  test_hkdf_sha1(SHEX("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b"),
	SDATA(""),
	SDATA(""),
	SHEX("da8c8a73c7fa77288ec6f5e7c297786aa0d32d01"),
	SHEX("0ac1af7002b3d761d1e55298da9d0506b9ae52057220a306e07b6b87e8df21d0ea00033de03984d34918"));

  test_hkdf_sha1(SHEX("0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c"),
	SHEX(""),
	SHEX(""),
	SHEX("2adccada18779e7c2077ad2eb19d3f3e731385dd"),
	SHEX("2c91117204d745f3500d636a62f64f0ab3bae548aa53d423b0d1f27ebba6f5e5673a081d70cce7acfc48"));
}
