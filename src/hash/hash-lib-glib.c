/*
 *   Copyright (C) 2007-2011 Tristan Heaven <tristanheaven@gmail.com>
 *
 *   This file is part of GtkHash.
 *
 *   GtkHash is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   GtkHash is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GtkHash. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <glib.h>

#include "hash-lib-glib.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_s *)func->priv.lib_data)

struct hash_lib_s {
	GChecksumType type;
	GChecksum *checksum;
};

static bool gtkhash_hash_lib_glib_set_type(const enum hash_func_e id,
	GChecksumType *type)
{
	switch (id) {
		case HASH_FUNC_MD5:
			*type = G_CHECKSUM_MD5;
			break;
		case HASH_FUNC_SHA1:
			*type = G_CHECKSUM_SHA1;
			break;
		case HASH_FUNC_SHA256:
			*type = G_CHECKSUM_SHA256;
			break;
		default:
			return false;
	}

	return true;
}

bool gtkhash_hash_lib_glib_is_supported(const enum hash_func_e id)
{
	GChecksumType type;

	if (!gtkhash_hash_lib_glib_set_type(id, &type))
		return false;

	return true;
}

void gtkhash_hash_lib_glib_start(struct hash_func_s *func)
{
	func->priv.lib_data = g_new(struct hash_lib_s, 1);

	if (!gtkhash_hash_lib_glib_set_type(func->id, &LIB_DATA->type))
		g_assert_not_reached();

	LIB_DATA->checksum = g_checksum_new(LIB_DATA->type);
	g_assert(LIB_DATA->checksum);
}

void gtkhash_hash_lib_glib_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
	g_checksum_update(LIB_DATA->checksum, buffer, size);
}

void gtkhash_hash_lib_glib_stop(struct hash_func_s *func)
{
	g_checksum_free(LIB_DATA->checksum);
	g_free(LIB_DATA);
}

char *gtkhash_hash_lib_glib_finish(struct hash_func_s *func)
{
	char *digest = g_strdup(g_checksum_get_string(LIB_DATA->checksum));

	g_checksum_free(LIB_DATA->checksum);
	g_free(LIB_DATA);

	return digest;
}
