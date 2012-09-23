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

#include "otama_status.h"

/*
typedef enum {
	OTAMA_STATUS_OK,
	OTAMA_STATUS_NODATA,
	OTAMA_STATUS_INVALID_ARGUMENTS,
	OTAMA_STATUS_ASSERTION_FAILURE,
	OTAMA_STATUS_SYSERROR,
	OTAMA_STATUS_NOT_IMPLEMENTED,	
	OTAMA_STATUS_END
} otama_status_t;
*/

static const char *s_messages[] = {
	"OK",
	"NoData",
	"Invalid Arguments",
	"Assertion Failure",
	"Internal Error",
	"Not Implemented",
	NULL
};

const char *
otama_status_message(otama_status_t code)
{
	switch (code) {
	case OTAMA_STATUS_OK:
	case OTAMA_STATUS_NODATA:
	case OTAMA_STATUS_INVALID_ARGUMENTS:
	case OTAMA_STATUS_ASSERTION_FAILURE:		
	case OTAMA_STATUS_SYSERROR:
	case OTAMA_STATUS_NOT_IMPLEMENTED:
		return s_messages[(int)code];
	default:
		assert("unknown status code " == NULL);
		break;
	}
	return "unknown code";
}
