/*
 * This file is part of otama.
 *
 * Copyright (C) 2012 nagadomi@nurs.or.jp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "otama_id.h"
#include "nv_core.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

otama_status_t
otama_id_file(otama_id_t *id,
			  const char *image_filename)
{
	int ret;
	
	assert(OTAMA_ID_LEN == NV_SHA1_BINARY_LEN);
	
	ret = nv_sha1_file(id, image_filename);
	if (ret != 0) {
		return OTAMA_STATUS_SYSERROR;
	}
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_id_data(otama_id_t *id,
			  const  void *image_data, size_t image_data_len)
{
	assert(OTAMA_ID_LEN == NV_SHA1_BINARY_LEN);
	
	nv_sha1(id, image_data, image_data_len);
	
	return OTAMA_STATUS_OK;
}

void
otama_id_bin2hexstr(char *hexstr, const otama_id_t *id)
{
	static const char *digits = "0123456789abcdef";
	int i;
	assert(OTAMA_ID_HEXSTR_LEN == NV_SHA1_HEXSTR_LEN);
	
	for (i = 0; i < OTAMA_ID_LEN; ++i) {
		hexstr[i * 2 + 1] = digits[id->octets[i] & 0xf];
		hexstr[i * 2] = digits[(id->octets[i]  >> 4) & 0xf];
	}
	hexstr[OTAMA_ID_HEXSTR_LEN - 1] = '\0';
}

otama_status_t
otama_id_hexstr2bin(otama_id_t *id, const char *hexstr)
{
	int i;
	char hex[3];
	char *errp = NULL;
	
	assert(OTAMA_ID_LEN == NV_SHA1_BINARY_LEN);
	assert(OTAMA_ID_HEXSTR_LEN == NV_SHA1_HEXSTR_LEN);

 	if (strlen(hexstr) != OTAMA_ID_HEXSTR_LEN - 1) {
		memset(id, 0, sizeof(*id));
		return OTAMA_STATUS_INVALID_ARGUMENTS;
	}

	hex[2] = '\0';
	for (i = 0; i < OTAMA_ID_LEN; ++i) {
		hex[0] = hexstr[i * 2 + 0];
		hex[1] = hexstr[i * 2 + 1];
		
		id->octets[i] = (uint8_t)strtol(hex, &errp, 16);
		if (errp != &hex[2]) {
			memset(id, 0, sizeof(otama_id_t));
			return OTAMA_STATUS_INVALID_ARGUMENTS;
		}
	}
	
	return OTAMA_STATUS_OK;
}
