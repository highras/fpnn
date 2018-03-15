/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
 *  Copyright (C) 2001-2003  Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef SHA1_H_
#define SHA1_H_ 1

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t total[2];
	uint32_t state[5];
	unsigned char buffer[64];
} sha1_context;

void sha1_start(sha1_context *ctx);
void sha1_update(sha1_context *ctx, const void *input, size_t length);
void sha1_finish(sha1_context *ctx, unsigned char digest[20]);

void sha1_get(sha1_context *ctx, unsigned char digest[20]);

void sha1_checksum(unsigned char digest[20], const void *input, size_t length);

#ifdef __cplusplus
}
#endif

#endif

