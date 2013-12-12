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

#ifndef OTAMA_ID_H
#define OTAMA_ID_H

#include "otama_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OTAMA_ID_LEN (20)
#define OTAMA_ID_HEXSTR_LEN (40 + 1)

typedef struct  {
	uint8_t octets[OTAMA_ID_LEN];
} otama_id_t;
	
otama_status_t otama_id_file(otama_id_t *id,
							 const char *image_filename);
otama_status_t otama_id_data(otama_id_t *id,
							 const  void *image_data, size_t image_data_len);
void otama_id_bin2hexstr(char *hexstr, const otama_id_t *id);
otama_status_t otama_id_hexstr2bin(otama_id_t *id, const char *hexstr);

#ifdef __cplusplus
}
#endif

	
#endif
