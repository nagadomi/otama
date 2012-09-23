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

#ifndef OTAMA_MMAP_POSIX_C
#define OTAMA_MMAP_POSIX_C

#include "otama_status.h"
#include "otama_log.h"
#include "otama_mmap.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

struct otama_mmap {
	int fd;
	void *mem;
	int64_t len;
	char path[PATH_MAX * 2];
	char dir[PATH_MAX];	
	char name[PATH_MAX];
};

int
otama_mmap_open(otama_mmap_t **new_shm, const char *shm_dir,
				const char *name, int64_t len)
{
	otama_mmap_t *shm = (otama_mmap_t *)malloc(sizeof(otama_mmap_t));
	memset(shm, 0, sizeof(*shm));
	
	strncpy(shm->name, name, sizeof(shm->name) -1);
	snprintf(shm->path, sizeof(shm->path) - 1, "%s/%s", shm_dir, name);
	memset(shm->dir, 0, sizeof(shm->dir));
	strncpy(shm->dir, shm_dir, sizeof(shm->dir)-1);
	
	OTAMA_LOG_DEBUG("otama_mmap_open: dir => %s, name => %s, path => %s",
					shm->dir, shm->name, shm->path);
	
#if OTAMA_POSIX_SHMOPEN	
	shm->fd = shm_open(shm->path, O_RDWR, 0600);
#else
	shm->fd = open(shm->path, O_RDWR, 0600);
#endif
	if (shm->fd == -1) {
		//OTAMA_LOG_NOTICE("%s: %s", shm->path, strerror(errno));
		*new_shm = NULL;
		free(shm);
		return -1;
	}
	
	shm->mem = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, shm->fd, 0);
	if (shm->mem == NULL) {
		OTAMA_LOG_ERROR("%s", strerror(errno));
		*new_shm = NULL;
		close(shm->fd);
		free(shm);
		
		return -1;
	}
	
	shm->len = len;
	*new_shm = shm;
	
	return 0;
}

int
otama_mmap_create(const char *shm_dir,
				 const char *name, int64_t len)
{
	int fd;
	int ret;
	void *mem;
	char path[PATH_MAX];

	snprintf(path, sizeof(path) - 1, "%s/%s", shm_dir, name);
	OTAMA_LOG_DEBUG("otama_mmap_create: dir => %s, name => %s, path => %s",
					shm_dir, name, path);
	
#if OTAMA_POSIX_SHMOPEN		
	fd = shm_open(path, O_RDWR|O_CREAT, 0600);
#else
	fd = open(path, O_RDWR|O_CREAT, 0600);
#endif
	
	if (fd == -1) {
		OTAMA_LOG_ERROR("%s: %s", path, strerror(errno));
		return -1;
	}
	ret = ftruncate(fd, len);
	if (ret == -1) {
		OTAMA_LOG_ERROR("%s", strerror(errno));		
		close(fd);
		return -1;
	}
	
	mem = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem == NULL) {
		OTAMA_LOG_ERROR("%s", strerror(errno));
		close(fd);
		return -1;
	}
	
	/* FIXME: bus error(SIGBUS) */
	memset(mem, 0, (size_t)len);
	munmap(mem, len);
	close(fd);
	
	return 0;
}

int
otama_mmap_extend(otama_mmap_t **shm, int64_t len)
{
	int ret = 0;
	otama_mmap_t backup = **shm;
	int64_t old_len = (*shm)->len;
	
	otama_mmap_close(shm);
	
	ret = truncate(backup.path, len);
	if (ret == 0) {
		ret = otama_mmap_open(shm, backup.dir, backup.name, len);
		if (ret == 0 && len > old_len) {
			memset(((char *)(*shm)->mem) + old_len,  0, len - old_len);
		}
	} else {
		OTAMA_LOG_ERROR("%s", strerror(errno));
	}
	
	return ret;
}

void *
otama_mmap_mem(const otama_mmap_t *shm)
{
	return shm->mem;
}

int64_t
otama_mmap_len(const otama_mmap_t *shm)
{
	return shm->len;
}

int
otama_mmap_sync(const otama_mmap_t *shm)
{
	return fsync(shm->fd);
}

int
otama_mmap_unlink(const char *shm_dir, const char *name)
{
	char path[PATH_MAX];

	snprintf(path, sizeof(path) - 1, "%s/%s", shm_dir, name);
	
#if OTAMA_POSIX_SHMOPEN
	return shm_unlink(path);
#else
	return unlink(path);	
#endif
}

void
otama_mmap_close(otama_mmap_t **shm)
{
	if (shm && *shm) {
		munmap((*shm)->mem, (*shm)->len);
		close((*shm)->fd);
		free(*shm);
		*shm = NULL;
	}
}

#endif
