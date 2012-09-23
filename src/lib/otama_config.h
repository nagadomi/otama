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

#ifndef OTAMA_CONFIG_H
#define OTAMA_CONFIG_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef _MSC_VER
#  include "otama_config_msvc.h"
#else
#  include "otama_config_auto.h"
#endif
#include "otama_portable.h"

#endif
