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

#include "nv_core.h"
#include "otama_config.h"
#include "otama_util.h"
#if OTAMA_POSIX
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>
#elif OTAMA_WINDOWS
#include <windows.h>
#include <process.h>
#include <direct.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#define MINGW32_CRT "msvcrt.dll"

int
otama_writeable_directory(const char *dir)
{
#if OTAMA_POSIX
	{
		char file[PATH_MAX];
		FILE *fp;
		int ret = -1;
		
		nv_snprintf(file, sizeof(file) - 1,
					"%s/%u.tmp", dir, (unsigned int)getpid());

		fp = fopen(file, "w");
		if (fp != NULL) {
			ret = 0;
			fclose(fp);
			otama_unlink(file);			
		}
		
		return ret;
		
	}
#elif OTAMA_WINDOWS
	{
		char file[_MAX_PATH];
		FILE *fp;
		int ret = -1;
		
		nv_snprintf(file, sizeof(file) - 1,
					"%s\\%u.tmp", dir, (unsigned int)_getpid());
		
		fp = fopen(file, "w");
		if (fp != NULL) {
			ret = 0;
			fclose(fp);
			otama_unlink(file);
		}
		
		return ret;
	}
#else
# error "not implemented"
#endif
}

int otama_unlink(const char *file)
{
#if OTAMA_POSIX
	return unlink(file);
#elif OTAMA_WINDOWS
	return _unlink(file);
#else
# error "not implemented"
#endif
	
}

void
otama_libc_free(void *mem)
{
	if (mem) {
#ifdef _MSC_VER
		static void (*free_func)(void *) = NULL; 
		if (free_func) {
			(*free_func)(mem);
		} else {
			HMODULE hModule = GetModuleHandle(MINGW32_CRT);
			if (hModule) {
				free_func = (void (*)(void *))GetProcAddress(hModule, "free");
				if (free_func) {
					(*free_func)(mem);
				} else {
					perror(MINGW32_CRT ".free not found");
					abort();
				}
			} else {
				free_func = free;
				(*free_func)(mem);
			}
		}
#else
//#undef free		
		free(mem);
#endif
	}
}
void
otama_libc_string_free(char **mem)
{
	if (*mem) {
		otama_libc_free(*mem);
		*mem = NULL;
	}
}

void
otama_rcpath(char *path, size_t path_len)
{
	char *home = getenv("HOME");
	
	memset(path, 0, path_len);
	
	if (home) {
		nv_snprintf(path, path_len -1, "%s/.otama.yaml", home);
	} else {
		strncpy(path, "./config.yaml", path_len -1);
	}
}

int
otama_split_path(char *dir, 
				 size_t dir_len,
				 char *base,
				 size_t base_len,
				 const char *path)
{
#if OTAMA_POSIX
	char fullpath[PATH_MAX];
	char temp[PATH_MAX];

	memset(dir, 0, dir_len);
	memset(base, 0, base_len);
	
	if (realpath(path, fullpath) == NULL
		&& errno != ENOENT)
	{
		return -1;
	}
	strcpy(temp, fullpath);
	strncpy(dir, dirname(temp), dir_len - 1);
	strcpy(temp, fullpath);
	strncpy(base, basename(temp), base_len);
	
#elif OTAMA_WINDOWS
	char fullpath[MAX_PATH];
	char *p;

	memset(dir, 0, dir_len);
	memset(base, 0, base_len);

	_fullpath(fullpath, path, sizeof(fullpath));
	strncpy(dir, fullpath, dir_len - 1);
	p = dir + strlen(dir);
	while (*p != '\\' && p != dir) {
		--p ;
	}
	*p = '\0';
	strncpy(base, p + 1, base_len - 1);

#else
# error "not implemented"
#endif
	
	return 0;
}

int otama_mkdir(const char *dir)
{
#if OTAMA_POSIX
	return mkdir(dir, 0755);
#elif OTAMA_WINDOWS
	return _mkdir(dir);
#endif
}
