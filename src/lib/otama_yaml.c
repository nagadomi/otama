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
#include "otama_yaml.h"
#include "yaml.h"

static int
otama_yaml_parse_sequance(otama_variant_t *array, yaml_parser_t *parser);
static int
otama_yaml_parse_mapping(otama_variant_t *hash, yaml_parser_t *parser);

static int
otama_yaml_parse_mapping(otama_variant_t *hash, yaml_parser_t *parser)
{
	otama_variant_t *p = NULL;
	yaml_event_t event;
	int done = 0;
	int error = 0;
	char *value = NULL;
	
	do {
		if (!yaml_parser_parse(parser, &event)) {
			error = 1;
			break;
		}
		switch (event.type) {
		case YAML_SEQUENCE_START_EVENT:
			if (p) {
				otama_variant_set_array(p);
				error = otama_yaml_parse_sequance(p, parser);
				p = NULL;
			} else {
				error = 1;
			}
			break;
		case YAML_MAPPING_START_EVENT:
			if (p) {
				otama_variant_set_hash(p);
				error = otama_yaml_parse_mapping(p, parser);
				p = NULL;
			} else {
				error = 1;
			}
			break;
		case YAML_SCALAR_EVENT:
			if (p == NULL) {
				value = (char *)malloc(event.data.scalar.length + 1);
				memcpy(value, event.data.scalar.value, event.data.scalar.length);
				value[event.data.scalar.length] = '\0';
				p = otama_variant_hash_at(hash, value);
				free(value);
				value = NULL;
			} else {
				value = (char *)malloc(event.data.scalar.length + 1);
				memcpy(value, event.data.scalar.value, event.data.scalar.length);
				value[event.data.scalar.length] = '\0';
				otama_variant_set_string(p, value);
				free(value);
				value = NULL;
				p = NULL;
			}
			break;
		case YAML_MAPPING_END_EVENT:
			done = 1;
			break;
		default:
			error = 1;
			break;
		}
		yaml_event_delete(&event);
	} while (done == 0 && error == 0);

	return error;
}

static int 
otama_yaml_parse_sequance(otama_variant_t *array, yaml_parser_t *parser)
{
	yaml_event_t event;
	int done = 0;
	int error = 0;
	int i = 0;
	char *value;
	
	do {
		otama_variant_t *p;
		if (!yaml_parser_parse(parser, &event)) {
			error = 1;
			break;
		}
		switch (event.type) {
		case YAML_SEQUENCE_START_EVENT:
			p = otama_variant_array_at(array, i++);
			otama_variant_set_array(p);
			error = otama_yaml_parse_sequance(p, parser);
			break;
		case YAML_MAPPING_START_EVENT:
			p = otama_variant_array_at(array, i++);
			otama_variant_set_hash(p);
			error = otama_yaml_parse_mapping(p, parser);
			break;
		case YAML_SCALAR_EVENT:
			p = otama_variant_array_at(array, i++);
			value = (char *)malloc(event.data.scalar.length + 1);
			memcpy(value, event.data.scalar.value, event.data.scalar.length);
			value[event.data.scalar.length] = '\0';
			
			otama_variant_set_string(p, value);
			
			free(value);
			value = NULL;
			break;
		case YAML_SEQUENCE_END_EVENT:
			done = 1;
			break;
		default:
			error = 1;
			break;
		}
		yaml_event_delete(&event);
	} while (done == 0 && error == 0);

	return error;
}

static int
otama_yaml_parse(otama_variant_t *var,
				 yaml_parser_t *parser)
{
	yaml_event_t event;
	int done = 0;
	int error = 0;	
	do {
		if (!yaml_parser_parse(parser, &event)) {
			error = 1;
			break;
		}
		switch (event.type) {
		case YAML_STREAM_START_EVENT:
			break;
		case YAML_DOCUMENT_START_EVENT:
			break;
		case YAML_SEQUENCE_START_EVENT:
			otama_variant_set_array(var);
			error = otama_yaml_parse_sequance(var, parser);
			done = 1;
			break;
		case YAML_MAPPING_START_EVENT:
			otama_variant_set_hash(var);
			error = otama_yaml_parse_mapping(var, parser);
			done = 1;			
			break;
		case YAML_STREAM_END_EVENT:
		case YAML_DOCUMENT_END_EVENT:
			done = 1;			
			break;
		default:
			error = 1;
			break;
		}
		yaml_event_delete(&event);
	} while (!done);

	return error;
}

otama_variant_t *
otama_yaml_read_file(const char *config_file, otama_variant_pool_t *pool)
{
	yaml_parser_t parser;
	int error = 0;
	FILE *input = fopen(config_file, "r");
	otama_variant_t  *root = otama_variant_new(pool);
	
	if (input == NULL) {
		return NULL;
	}
	
	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, input);

	error = otama_yaml_parse(root, &parser);
	yaml_parser_delete(&parser);

	fclose(input);

	if (error) {
		return NULL;
	}

	return root;
}
