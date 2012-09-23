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

#include "otama_log.h"
#include "nv_core.h"

#define OTAMA_LOG_BUFSIZ (1024 * 1024 * 2)

struct otama_log {
	FILE *fp;
	otama_log_level_e print_level;
};

static otama_log_t
s_otama_log = { NULL, OTAMA_LOG_LEVEL_NOTICE };

otama_log_level_e
otama_log_get_level(void)
{
	return s_otama_log.print_level;
}

void
otama_log_set_level(
	otama_log_level_e print_level)
{
	s_otama_log.print_level = print_level;
}

int
otama_log_set_file(const char *filename)
{
	int ret = 0;

	OTAMA_LOG_DEBUG("change: %s", filename);
	
	if (filename != NULL) {
		FILE *fp = fopen(filename, "a");
		
		if (fp == NULL) {
			OTAMA_LOG_ERROR("%s: %s\n", filename, strerror(errno));
			ret = -1;
		} else {
			if (s_otama_log.fp) {
				fclose(s_otama_log.fp);
				s_otama_log.fp = NULL;
			}
			s_otama_log.fp = fp;
		}
	} else {
		if (s_otama_log.fp) {
			fclose(s_otama_log.fp);
			s_otama_log.fp = NULL;
		}
	}
	
	return ret;
}

static void
otama_log_print(FILE *fp, otama_log_level_e level, const char *fmt, va_list args)
{
	time_t t = time(NULL);
	struct tm *tv;
	char *buff = (char *)nv_malloc(OTAMA_LOG_BUFSIZ);
	size_t len;
	static const char *marks[] = {"DEBUG", "NOTICE", "ERROR", ""};
	const char *mark = marks[(int)level];
	
	tv = localtime(&t);
	len = nv_snprintf(buff, OTAMA_LOG_BUFSIZ -1,"[%04d %02d/%02d %02d:%02d:%02d] %s: ",
					  1900 + tv->tm_year,
					  tv->tm_mon + 1, tv->tm_mday,
					  tv->tm_hour, tv->tm_min, tv->tm_sec,
					  mark
		);
	nv_vsnprintf(buff + len, OTAMA_LOG_BUFSIZ - len -1, fmt, args);
	
	len = strlen(buff);
	if (buff[len -1] != '\n' && len + 1 < OTAMA_LOG_BUFSIZ) {
		strcat(buff, "\n");
	}
	
	fputs(buff, fp);
	fflush(fp);
	nv_free(buff);
}

void
otama_log(
	otama_log_level_e level,
	const char *fmt, ...)
{
	if (level >= s_otama_log.print_level) {
		va_list args;
		FILE *fp;
		
		switch (level) {
		case OTAMA_LOG_LEVEL_DEBUG:
		case OTAMA_LOG_LEVEL_NOTICE:
			fp = s_otama_log.fp;
			if (fp == NULL) {
				fp = stdout;
			}
			break;
		case OTAMA_LOG_LEVEL_ERROR:
			fp = s_otama_log.fp;
			if (fp == NULL) {
				fp = stderr;
			}
			break;
		default:
			fprintf(stderr, "unknown level %d\n", level);
			fp = NULL;
			break;
		}
		if (fp != NULL) {
			va_start(args, fmt);
			otama_log_print(fp, level, fmt, args);
			va_end(args);
		}
	}
}
