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

#ifndef OTAMA_LOG_H
#define OTAMA_LOG_H

#include "otama_config.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* logger */
/* NON THREAD SAFE */

typedef enum {
	OTAMA_LOG_LEVEL_DEBUG    = 0,
	OTAMA_LOG_LEVEL_NOTICE   = 1,
	OTAMA_LOG_LEVEL_ERROR    = 2,
	OTAMA_LOG_LEVEL_QUIET    = 3
} otama_log_level_e;

typedef struct otama_log otama_log_t;

otama_log_level_e otama_log_get_level(void);
void otama_log_set_level(otama_log_level_e print_level);
int otama_log_set_file(const char *filename);
void otama_log(otama_log_level_e, const char *fmt, ...);

#if _MSC_VER
# define OTAMA_FUNC_NAME _function
#else
# define OTAMA_FUNC_NAME __FUNCTION__
#endif

#define OTAMA_LOG_DEBUG(fmt, ...)  \
	otama_log(OTAMA_LOG_LEVEL_DEBUG, "%s:%d(%s): " fmt,	\
			  __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define OTAMA_LOG_NOTICE(fmt, ...) \
	otama_log(OTAMA_LOG_LEVEL_NOTICE, "%s:%d(%s): " fmt, \
			  __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)		
#define OTAMA_LOG_ERROR(fmt, ...) \
	otama_log(OTAMA_LOG_LEVEL_ERROR, "%s:%d(%s): " fmt, \
			  __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)		

#ifdef __cplusplus
}
#endif
	
#endif
