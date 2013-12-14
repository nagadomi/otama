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

#ifndef OTAMA_CONFIG_MSVC_H
#define OTAMA_CONFIG_MSVC_H

#define OTAMA_WITH_LEVELDB               1
#define OTAMA_WITH_PGSQL                 0
#define OTAMA_LIBPQ_H_INCLUDE            0
#define OTAMA_LIBPQ_H_INCLUDE_POSTGRESQL 0
#define OTAMA_WITH_SQLITE3               1
#define OTAMA_WITH_MYSQL                 0
#define OTAMA_MYSQL_H_INCLUDE            0
#define OTAMA_MYSQL_H_INCLUDE_MYSQL      0
#define OTAMA_WITH_UNORDERED_MAP_TR1     0
#define OTAMA_WITH_UNORDERED_MAP         0
#define OTAMA_HAS_KVS                    OTAMA_WITH_LEVELDB

#define OTAMA_PACKAGE "otama"
#define OTAMA_VERSION "0.7.0"

#endif
