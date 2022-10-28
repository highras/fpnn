/* cmac-des3-meta.c

   Copyright (C) 2020 Dmitry Baryshkov

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

#include <assert.h>

#include "nettle-meta.h"

#include "cmac.h"

const struct nettle_mac nettle_cmac_des3 =
{
  "cmac_des3",
  sizeof(struct cmac_des3_ctx),
  CMAC64_DIGEST_SIZE,
  DES3_KEY_SIZE,

  (nettle_set_key_func*) cmac_des3_set_key,
  (nettle_hash_update_func*) cmac_des3_update,
  (nettle_hash_digest_func*) cmac_des3_digest
};
