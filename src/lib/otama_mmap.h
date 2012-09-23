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

#include "otama_config.h"
#ifndef OTAMA_MMAP_H
#define OTAMA_MMAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct otama_mmap otama_mmap_t;

int otama_mmap_open(otama_mmap_t **shm, const char *shm_dir,
				   const char *name, int64_t len);
int otama_mmap_create(const char *shm_dir,
					 const char *name, int64_t len);
int otama_mmap_extend(otama_mmap_t **shm, int64_t len);
void *otama_mmap_mem(const otama_mmap_t *shm);
int64_t otama_mmap_len(const otama_mmap_t *shm);
int otama_mmap_sync(const otama_mmap_t *shm);
int otama_mmap_unlink(const char *shm_dir, const char *name);
void otama_mmap_close(otama_mmap_t **shm);

#ifdef __cplusplus
}
#endif

#endif

