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

#ifndef OTAMA_PORTABLE_H
#define OTAMA_PORTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <assert.h>

#ifdef _MSC_VER
#  define OTAMA_MSVC  1
#  define OTAMA_GCC   0
#  define OTAMA_MINGW 0	
#elif __GNUC__
#  define OTAMA_MSVC 0
#  define OTAMA_GCC  1
#  if (defined(__MINGW32__) || defined(__MINGW64__))
#    define OTAMA_MINGW 1
#  else
#    define OTAMA_MINGW 0
#  endif
#else
#  error "unknown compiler"
#endif

#if OTAMA_MSVC
#  include "inttypes.h"
#else
#  include <inttypes.h>
#endif

#if (defined(_WIN32) || defined(_WIN64))
#  define OTAMA_WINDOWS 1
#  define OTAMA_POSIX 0
#else
#  define OTAMA_WINDOWS 0
#  define OTAMA_POSIX 1
#endif

#endif
