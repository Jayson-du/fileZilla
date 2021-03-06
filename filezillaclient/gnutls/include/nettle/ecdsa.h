/* ecdsa.h */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2013 Niels Möller
 *  
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02111-1301, USA.
 */

/* Development of Nettle's ECC support was funded by the .SE Internet Fund. */

#ifndef NETTLE_ECDSA_H_INCLUDED
#define NETTLE_ECDSA_H_INCLUDED

#include "ecc.h"
#include "dsa.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define ecdsa_sign nettle_ecdsa_sign
#define ecdsa_verify nettle_ecdsa_verify
#define ecdsa_generate_keypair nettle_ecdsa_generate_keypair
#define ecc_ecdsa_sign nettle_ecc_ecdsa_sign
#define ecc_ecdsa_sign_itch nettle_ecc_ecdsa_sign_itch
#define ecc_ecdsa_verify nettle_ecc_ecdsa_verify
#define ecc_ecdsa_verify_itch nettle_ecc_ecdsa_verify_itch

/* High level ECDSA functions.
 *
 * A public key is represented as a struct ecc_point, and a private
 * key as a struct ecc_scalar. FIXME: Introduce some aliases? */
void
ecdsa_sign (const struct ecc_scalar *key,
	    void *random_ctx, nettle_random_func *random,
	    unsigned digest_length,
	    const uint8_t *digest,
	    struct dsa_signature *signature);

int
ecdsa_verify (const struct ecc_point *pub,
	      unsigned length, const uint8_t *digest,
	      const struct dsa_signature *signature);

void
ecdsa_generate_keypair (struct ecc_point *pub,
			struct ecc_scalar *key,
			void *random_ctx, nettle_random_func *random);

/* Low-level ECDSA functions. */
mp_size_t
ecc_ecdsa_sign_itch (const struct ecc_curve *ecc);

void
ecc_ecdsa_sign (const struct ecc_curve *ecc,
		const mp_limb_t *zp,
		/* Random nonce, must be invertible mod ecc group
		   order. */
		const mp_limb_t *kp,
		unsigned length, const uint8_t *digest,
		mp_limb_t *rp, mp_limb_t *sp,
		mp_limb_t *scratch);

mp_size_t
ecc_ecdsa_verify_itch (const struct ecc_curve *ecc);

int
ecc_ecdsa_verify (const struct ecc_curve *ecc,
		  const mp_limb_t *pp, /* Public key */
		  unsigned length, const uint8_t *digest,
		  const mp_limb_t *rp, const mp_limb_t *sp,
		  mp_limb_t *scratch);


#ifdef __cplusplus
}
#endif

#endif /* NETTLE_ECDSA_H_INCLUDED */
