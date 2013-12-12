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
#ifndef OTAMA_UTIL_H
#define OTAMA_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

void otama_libc_free(void *mem);
void
otama_libc_string_free(char **mem);

int otama_split_path(char *dirname, 
					 size_t dirname_len,
					 char *basename,
					 size_t basename_len,
					 const char *path);
void otama_rcpath(char *path, size_t path_len);
int otama_unlink(const char *file);
int otama_writeable_directory(const char *dir);

int otama_mkdir(const char *dir);

typedef int (*otama_file_each_f)(void *user_data, const char *path);
int otama_file_each(const char *dir, otama_file_each_f func, void *userdata);

#ifdef __cplusplus
}
#endif

#endif
