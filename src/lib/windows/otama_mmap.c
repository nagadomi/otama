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

#ifndef OTAMA_MMAP_WINDOWS_C
#define OTAMA_MMAP_WINDOWS_C

#if OTAMA_WINDOWS

#include "otama_config.h"
#include "otama_log.h"
#include "otama_mmap.h"
#include "nv_core.h"
#include <windows.h>
#include <stdlib.h>

#ifndef NAME_MAX
#define NAME_MAX MAX_PATH
#endif
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

struct otama_mmap {
	HANDLE fd;
	void *mem;
	int64_t len;
	char path[PATH_MAX * 2];
	char dir[PATH_MAX];	
	char name[PATH_MAX];
};

static const char *
otama_last_error(void)
{
	static char buffer[2048] = {0};
  
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		//MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		(LPTSTR) buffer,
		sizeof(buffer)-1,
		NULL);
	
	return buffer;
}

static const int
otama_file_trunc(HANDLE fd, uint64_t ulen)
{
	LONG lo = (LONG)(ulen & 0xffffffffULL);
	LONG hi = (LONG)((ulen & 0xffffffff00000000ULL) >> 32);

	if (SetFilePointer(fd, lo, &hi, FILE_BEGIN) == -1) {
		OTAMA_LOG_ERROR("SetFilePointer: %s", otama_last_error());
		return -1;		
	}
	
	if (SetEndOfFile(fd) == 0) {
		OTAMA_LOG_ERROR("SetEndOfFile: %s", otama_last_error());
		return -1;
	}
	
	return 0;
}

int
otama_mmap_open(otama_mmap_t **new_shm, const char *shm_dir,
			   const char *name, int64_t len)
{
	otama_mmap_t *shm = (otama_mmap_t *)malloc(sizeof(otama_mmap_t));
	uint64_t ulen = (uint64_t)len; // TODO: filesize
	HANDLE file_fd;
	BOOL exists;
	
	strncpy(shm->name, name, sizeof(shm->name) -1);
	nv_snprintf(shm->path, sizeof(shm->path) - 1, "%s\\%s", shm_dir, name);
	memset(shm->dir, 0, sizeof(shm->dir));
	strncpy(shm->dir, shm_dir, sizeof(shm->dir)-1);
	
	OTAMA_LOG_DEBUG("otama_mmap_open: dir => %s, name => %s, path => %s",
					shm->dir, shm->name, shm->path);
	
	file_fd = CreateFile(shm->path,
						 GENERIC_READ|GENERIC_WRITE, 
						 FILE_SHARE_READ|FILE_SHARE_WRITE,
						 NULL, OPEN_ALWAYS,
						 FILE_ATTRIBUTE_TEMPORARY |
						 FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
						 FILE_FLAG_SEQUENTIAL_SCAN,
						 NULL);
	if (file_fd == INVALID_HANDLE_VALUE) {
		OTAMA_LOG_NOTICE("CreateFile: %s: %s", shm->path, otama_last_error());
		*new_shm = NULL;
		free(shm);
		return -1;
	}
	otama_file_trunc(file_fd, ulen);
	
	shm->fd = CreateFileMapping(file_fd, NULL, PAGE_READWRITE,
								(DWORD)((ulen & 0xffffffff00000000ULL) >> 32),
								(DWORD)(ulen & 0xffffffffULL), shm->name);
	if (shm->fd == NULL) {
		OTAMA_LOG_ERROR("CreateFileMapping: %s: %s", shm->path, otama_last_error());
		CloseHandle(file_fd);
		free(shm);
		*new_shm = NULL;
		return -1;
	}
	exists = (GetLastError() == ERROR_ALREADY_EXISTS) ? TRUE : FALSE;
	
	CloseHandle(file_fd);
	
	shm->mem = MapViewOfFile(shm->fd, FILE_MAP_ALL_ACCESS, 0, 0, (size_t)len);
	if (shm->mem == NULL) {
		OTAMA_LOG_ERROR("MapViewOfFile: %s: %s", shm->path, otama_last_error());
		CloseHandle(shm->fd);
		free(shm);
		*new_shm = NULL;
		return -1;
	}
	shm->len = len;
	*new_shm = shm;

	if (exists == TRUE) {
		ZeroMemory(shm->mem, (size_t)ulen);
	}
	
	return 0;
}

int
otama_mmap_create(const char *shm_dir,
				 const char *name, int64_t len)
{
	uint64_t ulen = (uint64_t)len;
	HANDLE fd, file_fd;
	void *mem;
	char path[PATH_MAX];

	nv_snprintf(path, sizeof(path) - 1, "%s\\%s", shm_dir, name);

	file_fd = CreateFile(path, GENERIC_READ|GENERIC_WRITE, 
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);
	if (file_fd == INVALID_HANDLE_VALUE) {
		OTAMA_LOG_ERROR("%s", strerror(errno));
		return -1;
	}
	otama_file_trunc(file_fd, ulen);
	
	fd = CreateFileMapping(file_fd, NULL, PAGE_READWRITE,
		(DWORD)((ulen & 0xffffffff00000000ULL) >> 32),
		(DWORD)(ulen & 0xffffffffULL), name);
	if (fd == NULL) {
		OTAMA_LOG_ERROR("CreateFile: %s", otama_last_error());
		CloseHandle(file_fd);
		return -1;
	}
	
	mem = MapViewOfFile(fd, FILE_MAP_ALL_ACCESS, 0, 0, (size_t)len);
	if (mem == NULL) {
		OTAMA_LOG_ERROR("MapViewOfFile: %s", otama_last_error());
		CloseHandle(fd);
		CloseHandle(file_fd);
		return -1;
	}
	ZeroMemory(mem, (size_t)ulen);
	
	if (CloseHandle(file_fd) == 0) {
		OTAMA_LOG_ERROR("CloseHandle: file_fd: %s: %s",
						path, otama_last_error());
	}
	if (UnmapViewOfFile(mem) == 0) {
		OTAMA_LOG_ERROR("UnmapViewOfFile: %s: %s",
						path, otama_last_error());
	}
	if (CloseHandle(fd) == 0) {
		OTAMA_LOG_ERROR("CloseHandle: fd: %s: %s",
						path, otama_last_error());
	}
	
	return 0;
}

int
otama_mmap_extend(otama_mmap_t **shm, int64_t len)
{
	int ret = 0;
	otama_mmap_t backup = **shm;
	int64_t old_len = (*shm)->len;
	
	otama_mmap_close(shm);
	ret = otama_mmap_open(shm, backup.dir, backup.name, len);
	if (ret == 0) {
		if (len > old_len) {
			memset(((char *)(*shm)->mem) + old_len,  0, (size_t)(len - old_len));
		}
	} else {
	}
	
	return ret;
}

int64_t
otama_mmap_len(const otama_mmap_t *shm)
{
	return shm->len;
}

void *
otama_mmap_mem(const otama_mmap_t *shm)
{
	return shm->mem;
}

int
otama_mmap_sync(const otama_mmap_t *shm)
{
	BOOL ret;

	ret = FlushViewOfFile(shm->mem, (SIZE_T)shm->len);
	if (ret == 0) {
		OTAMA_LOG_ERROR("FlushViewOfFile: %s: %s",
						shm->path, otama_last_error());
		return -1;
	}
	
	return 0;
}

int 
otama_mmap_unlink(const char *shm_dir, const char *name)
{
	char path[PATH_MAX];
	
	nv_snprintf(path, sizeof(path) - 1, "%s\\%s", shm_dir, name);
	
	if (DeleteFile(path) == 0) {
		OTAMA_LOG_ERROR("DeleteFile: %s: %s",
						path, otama_last_error());
		return -1;
	}
	
	return 0;
}

void
otama_mmap_close(otama_mmap_t **shm)
{
	if (shm && *shm) {
		if (UnmapViewOfFile((*shm)->mem) == 0) {
			OTAMA_LOG_ERROR("UnmapViewOfFile: %s: %s",
							(*shm)->path, otama_last_error());
		}
		if (CloseHandle((*shm)->fd) == 0) {
			OTAMA_LOG_ERROR("CloseHandle: %s: %s",
							(*shm)->path, otama_last_error());
		}
		free(*shm);
		*shm = NULL;
	}
}


#endif
#endif

