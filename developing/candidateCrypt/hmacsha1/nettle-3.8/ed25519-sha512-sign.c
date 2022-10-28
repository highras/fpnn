/* ed25519-sha512-sign.c

   Copyright (C) 2014, 2015 Niels Möller

   This file is part of GNU Nettle.

   GNU Nettle is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.

   GNU Nettle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see http://www.gnu.org/licenses/.
*/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "eddsa.h"
#include "eddsa-internal.h"

#include "ecc-internal.h"
#include "sha2.h"

void
ed25519_sha512_sign (const uint8_t *pub,
		     const uint8_t *priv,
		     size_t length, const uint8_t *msg,
		     uint8_t *signature)
{
  const struct ecc_curve *ecc = &_nettle_curve25519;
  mp_size_t itch = ecc->q.size + _eddsa_sign_itch (ecc);
  mp_limb_t *scratch = gmp_alloc_limbs (itch);
#define k2 scratch
#define scratch_out (scratch + ecc->q.size)
  struct sha512_ctx ctx;
  uint8_t digest[SHA512_DIGEST_SIZE];

  sha512_init (&ctx);
  _eddsa_expand_key (ecc, &_nettle_ed25519_sha512, &ctx, priv, digest, k2);

  _eddsa_sign (ecc, &_nettle_ed25519_sha512, &ctx,
	       pub, digest + ED25519_KEY_SIZE, k2,
	       length, msg, signature, scratch_out);

  gmp_free_limbs (scratch, itch);
#undef k1
#undef k2
#undef scratch_out
}
