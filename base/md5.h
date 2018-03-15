/*
 * RFC 1321 compliant MD5 implementation
 *
 * Copyright (C) 2001-2003	 Christophe Devine
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef MD5_H_
#define MD5_H_ 1

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t total[2];
	uint32_t state[4];
	unsigned char buffer[64];
} md5_context;

void md5_start(md5_context *ctx);
void md5_update(md5_context *ctx, const void *input, size_t length);
void md5_finish(md5_context *ctx, unsigned char digest[16]);

void md5_get(md5_context *ctx, unsigned char digest[16]);

void md5_checksum(unsigned char digest[16], const void *input, size_t length);

#ifdef __cplusplus
}
#endif

#endif

