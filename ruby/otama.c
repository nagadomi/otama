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

#include "otama.h"
#include <ruby.h>
#include <errno.h>

static VALUE cOtama,
	cOtamaError,
	cOtamaNoData,
	cOtamaArgumentError,
	cOtamaAssertionFailure,
	cOtamaSystemError,
	cOtamaNotImplemented,
	cOtamaRecord,
	cOtamaFeatureRaw;

static void
otama_rb_raise(otama_status_t ret)
{
	switch (ret) {
	case OTAMA_STATUS_OK:
		break;
	case OTAMA_STATUS_NODATA:
		rb_raise(cOtamaNoData, "%s", otama_status_message(ret));
		break;
	case OTAMA_STATUS_INVALID_ARGUMENTS:
		rb_raise(cOtamaArgumentError, "%s", otama_status_message(ret));
		break;
	case OTAMA_STATUS_ASSERTION_FAILURE:
		rb_raise(cOtamaAssertionFailure, "%s", otama_status_message(ret));
		break;
	case OTAMA_STATUS_SYSERROR:
		rb_raise(cOtamaSystemError, "%s", otama_status_message(ret));
		break;
	case OTAMA_STATUS_NOT_IMPLEMENTED:
		rb_raise(cOtamaNotImplemented, "%s", otama_status_message(ret));
		break;
	default:
		rb_raise(cOtamaError, "Unknown Error");
		break;
	}
}

#define OTAMA_CHECK_NULL(otama) \
if (otama == NULL) { rb_raise(cOtamaError, "Not connected."); }

static void
otama_rb_free(otama_t *otama)
{
    if (otama) {
		otama_close(&otama);
    }
}

static VALUE
variant2rubyobj(otama_variant_t *var)
{
	switch (otama_variant_type(var)) {
	case OTAMA_VARIANT_TYPE_NULL:
		return Qnil;
	case OTAMA_VARIANT_TYPE_INT:
		return rb_int2inum(otama_variant_to_int(var));
	case OTAMA_VARIANT_TYPE_FLOAT:
		return rb_float_new((double)otama_variant_to_float(var));
	case OTAMA_VARIANT_TYPE_STRING:
		return rb_str_new2(otama_variant_to_string(var));
	case OTAMA_VARIANT_TYPE_ARRAY: {
		long count = otama_variant_array_count(var);
		long i;
		VALUE ary = rb_ary_new2(count);
		
		for (i = 0; i < count; ++i) {
			rb_ary_store(ary, i, variant2rubyobj(otama_variant_array_at(var, i)));
		}
		return ary;
	}
	case OTAMA_VARIANT_TYPE_HASH: {
		otama_variant_t *keys = otama_variant_hash_keys(var);
		long count = otama_variant_array_count(keys);
		long i;
		VALUE hash = rb_hash_new();
		
		for (i = 0; i < count; ++i) {
			rb_hash_aset(hash,
						 ID2SYM(rb_intern(otama_variant_to_string(otama_variant_array_at(keys, i)))),
						 variant2rubyobj(otama_variant_hash_at2(var,
															otama_variant_array_at(keys, i))));
		}
		return hash;
	}
	default:
		break;
	}
	
	return Qnil;
}

static void
rubyobj2variant(VALUE value, otama_variant_t *var);

static VALUE
rubyobj2variant_pair(VALUE pair, otama_variant_t *var)
{
	VALUE key, value;

	key = rb_ary_entry(pair, 0);
	value = rb_ary_entry(pair, 1);

	key = rb_funcall(key, rb_intern("to_s"), 0);
	
	rubyobj2variant(value, otama_variant_hash_at(var, StringValuePtr(key)));
	
	return Qnil;
}

static void
rubyobj2variant(VALUE value, otama_variant_t *var)
{
	VALUE s;
	
	switch (TYPE(value)) {
	case T_NIL:
		otama_variant_set_null(var);
		break;
	case T_FLOAT:
		otama_variant_set_float(var, NUM2DBL(value));
		break;
	case T_FIXNUM:
	case T_BIGNUM:
		otama_variant_set_int(var, NUM2LL(value));
		break;
	case T_TRUE:
		otama_variant_set_int(var, 1);
		break;
	case T_FALSE:
		otama_variant_set_int(var, 0);
		break;
	case T_STRING:
		StringValue(value);
		if ((int)strlen(RSTRING_PTR(value)) == RSTRING_LEN(value)) {
			otama_variant_set_string(var, RSTRING_PTR(value));
		} else {
			otama_variant_set_binary(var, RSTRING_PTR(value), RSTRING_LEN(value));
		}
		break;
	case T_SYMBOL:
		s = rb_funcall(value, rb_intern("to_s"), 0);
		otama_variant_set_string(var, StringValuePtr(s));
		break;
	case T_ARRAY: {
		int len = RARRAY_LEN(value), i;
		otama_variant_set_array(var);
		for (i = 0; i < len; ++i) {
			VALUE elm = rb_ary_entry(value, i);
			rubyobj2variant(elm, otama_variant_array_at(var, i));
		}
	}
		break;
	case T_HASH:
		otama_variant_set_hash(var);
		rb_iterate(rb_each, value, rubyobj2variant_pair, (VALUE)var);
		break;
	default:
		if (rb_obj_is_instance_of(value, cOtamaFeatureRaw) == Qtrue) {
			otama_feature_raw_t *raw = NULL;
			Data_Get_Struct(value, otama_feature_raw_t, raw);
			otama_variant_set_pointer(var, raw);
		} else {
			otama_variant_set_null(var);
		}
		break;
	}
}

static VALUE
otama_rb_initialize(VALUE self, VALUE options)
{
	otama_t *otama = NULL;
	otama_status_t ret = OTAMA_STATUS_OK;
	
	if (TYPE(options) == T_HASH) {
		otama_variant_pool_t *pool;
		otama_variant_t *var;
		
		pool = otama_variant_pool_alloc();
		var = otama_variant_new(pool);
		
		rubyobj2variant(options, var);
		ret = otama_open_opt(&otama, var);

		otama_variant_pool_free(&pool);
	} else if (TYPE(options) == T_STRING) {
		ret = otama_open(&otama, StringValueCStr(options));
	} else {
		rb_raise(cOtamaArgumentError, "wrong option type"); 
	}
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}
	DATA_PTR(self) = otama;
	
	return self;
}

static VALUE
otama_rb_alloc(VALUE klass)
{
    return Data_Wrap_Struct(klass, 0, otama_rb_free, 0);
}

static VALUE
otama_rb_close(VALUE self)
{
	otama_t *otama;

	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);

	otama_close(&otama);
	DATA_PTR(self) = NULL;

	return Qnil;
}

static VALUE
otama_rb_s_open(VALUE self, VALUE options)
{
    VALUE obj = Data_Wrap_Struct(self, 0, otama_rb_free, 0);

    if (NIL_P(otama_rb_initialize(obj, options))) {
		return Qnil;
    }
	
    if (rb_block_given_p()) {
        return rb_ensure(rb_yield, obj, otama_rb_close, obj);
    }
	
	return obj;
}

static VALUE
otama_rb_s_set_log_level(VALUE self, VALUE level)
{
	otama_log_set_level((otama_log_level_e)NUM2INT(level));
	return Qnil;
}

static VALUE
otama_rb_s_set_log_file(VALUE self,
						VALUE file)
{
	int ret;
	
	ret = otama_log_set_file(StringValuePtr(file));
	if (ret != 0) {
		otama_rb_raise(ret);
	}
	return Qnil;
}

static VALUE
otama_rb_active(VALUE self)
{
	otama_t *otama;

	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
		
	return otama_active(otama) ? Qtrue: Qfalse;
}

static VALUE
otama_rb_count(VALUE self)
{
	otama_t *otama;
	otama_status_t ret;
	int64_t count;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	ret = otama_count(otama, &count);
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}
	return INT2FIX(count);
}

static VALUE
otama_rb_exists(VALUE self, VALUE id)
{
	otama_t *otama;
	int result = 0;
	otama_status_t ret;
	otama_id_t otama_id;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	
	ret = otama_id_hexstr2bin(&otama_id, StringValuePtr(id));
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}
	
	ret = otama_exists(otama, &result, &otama_id);
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}
	
	return result == 0 ? Qfalse: Qtrue;
}

static VALUE
otama_rb_insert(VALUE self, VALUE data)
{
	otama_t *otama;
	otama_status_t ret;
	otama_id_t id;
	char hexid[OTAMA_ID_HEXSTR_LEN];
	otama_variant_pool_t *pool;
	otama_variant_t *var;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	Check_Type(data, T_HASH);

	pool = otama_variant_pool_alloc();
	var = otama_variant_new(pool);
	rubyobj2variant(data, var);
	
	ret = otama_insert(otama, &id, var);
	if (ret != OTAMA_STATUS_OK) {
		otama_variant_pool_free(&pool);
		otama_rb_raise(ret);
		return Qnil;
	}
	otama_id_bin2hexstr(hexid, &id);
	otama_variant_pool_free(&pool);
	
	return rb_str_new2(hexid);
}

static VALUE
otama_rb_similarity(VALUE self, VALUE data1, VALUE data2)
{
	otama_t *otama;
	otama_status_t ret;
	otama_variant_pool_t *pool;
	otama_variant_t *var1, *var2;
	float similarity = 0.0f;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	Check_Type(data1, T_HASH);
	Check_Type(data2, T_HASH);

	pool = otama_variant_pool_alloc();
	var1 = otama_variant_new(pool);
	var2 = otama_variant_new(pool);
	
	rubyobj2variant(data1, var1);
	rubyobj2variant(data2, var2);	
	
	ret = otama_similarity(otama, &similarity, var1, var2);
	if (ret != OTAMA_STATUS_OK) {
		otama_variant_pool_free(&pool);
		otama_rb_raise(ret);
		return Qnil;
	}
	otama_variant_pool_free(&pool);
	
	return rb_float_new(similarity);
}

static VALUE
otama_rb_s_id(VALUE self, VALUE query)
{
	otama_status_t ret = OTAMA_STATUS_OK;
	otama_id_t id;
	char hexid[OTAMA_ID_HEXSTR_LEN];
	VALUE data;

	Check_Type(query, T_HASH);
	
	if ((data = rb_hash_aref(query, ID2SYM(rb_intern("data")))) != Qnil
		|| (data = rb_hash_aref(query, rb_str_new2("data"))) != Qnil)
	{
		Check_Type(data, T_STRING);
		ret = otama_id_data(&id, RSTRING_PTR(data), RSTRING_LEN(data));
	} else if((data = rb_hash_aref(query, ID2SYM(rb_intern("file")))) != Qnil
			  || (data = rb_hash_aref(query, rb_str_new2("file"))) != Qnil)
	{
		Check_Type(data, T_STRING);
		ret = otama_id_file(&id, StringValueCStr(data));
	}
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}
	otama_id_bin2hexstr(hexid, &id);
	
	return rb_str_new2(hexid);
}

static VALUE
otama_rb_set(VALUE self, VALUE key, VALUE value)
{
	otama_t *otama;
	otama_status_t ret;
	otama_variant_pool_t *pool;
	otama_variant_t *var;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	StringValue(key);

	pool = otama_variant_pool_alloc();
	var = otama_variant_new(pool);
	
	rubyobj2variant(value, var);
	
	ret = otama_set(otama, RSTRING_PTR(key), var);
	if (ret != OTAMA_STATUS_OK) {
		otama_variant_pool_free(&pool);
		otama_rb_raise(ret);
		return Qfalse;
	}
	otama_variant_pool_free(&pool);
	
	return Qtrue;
}


static VALUE
otama_rb_unset(VALUE self, VALUE key)
{
	otama_t *otama;
	otama_status_t ret;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	StringValue(key);

	ret = otama_unset(otama, RSTRING_PTR(key));
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
		return Qfalse;
	}
	
	return Qtrue;
}

static VALUE
otama_rb_get(VALUE self, VALUE key)
{
	otama_t *otama;
	otama_status_t ret;
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *var = otama_variant_new(pool);
	VALUE value;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	StringValue(key);
	
	ret = otama_get(otama, RSTRING_PTR(key), var);
	if (ret != OTAMA_STATUS_OK) {
		otama_variant_pool_free(&pool);
		otama_rb_raise(ret);
		return Qfalse;
	}
	value = variant2rubyobj(var);
	otama_variant_pool_free(&pool);	
	
	return value;
}

static VALUE
otama_rb_feature_string(VALUE self, VALUE query)
{
	otama_t *otama;
	otama_status_t ret;
	char *feature_string = NULL;
	VALUE rbstr;
	otama_variant_pool_t *pool;
	otama_variant_t *var;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	Check_Type(query, T_HASH);

	pool = otama_variant_pool_alloc();
	var = otama_variant_new(pool);

	rubyobj2variant(query, var);
	
	ret = otama_feature_string(otama, &feature_string, var);
	if (ret != OTAMA_STATUS_OK) {
		otama_variant_pool_free(&pool);
		otama_rb_raise(ret);
		return Qnil;
	}
	rbstr = rb_str_new2(feature_string);
	otama_feature_string_free(&feature_string);
	otama_variant_pool_free(&pool);
	
	return rbstr;
}

static VALUE
otama_rb_feature_raw(VALUE self, VALUE query)
{
	otama_t *otama;
	otama_status_t ret;
	otama_feature_raw_t *raw;
	VALUE rbraw;
	otama_variant_pool_t *pool;
	otama_variant_t *var;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	Check_Type(query, T_HASH);

	pool = otama_variant_pool_alloc();
	var = otama_variant_new(pool);

	rubyobj2variant(query, var);
	
	ret = otama_feature_raw(otama, &raw, var);
	if (ret != OTAMA_STATUS_OK) {
		otama_variant_pool_free(&pool);
		otama_rb_raise(ret);
		return Qnil;
	}
	otama_variant_pool_free(&pool);
	
	rbraw = rb_funcall(cOtamaFeatureRaw, rb_intern("new"), 0);
	DATA_PTR(rbraw) = raw;
	
	return rbraw;
}

static VALUE
make_results(const otama_result_t *results)
{
	VALUE result_array;
	long nresult = otama_result_count(results);
	long i;
	
	result_array = rb_ary_new2(nresult);
	for (i = 0; i < nresult; ++i) {
		otama_variant_t *value = otama_result_value(results, i);
		char hexid[OTAMA_ID_HEXSTR_LEN];

		otama_id_bin2hexstr(hexid, otama_result_id(results, i));
		rb_ary_push(result_array,
					rb_funcall(cOtamaRecord, rb_intern("new"), 2,
							   rb_str_new2(hexid),
							   variant2rubyobj(value)));
	}
	
	return result_array;
}

static VALUE
otama_rb_search(VALUE self, VALUE n, VALUE query)
{
	otama_t *otama;
	otama_status_t ret;
	VALUE result_array;
	otama_result_t *results = NULL;
	otama_variant_pool_t *pool;
	otama_variant_t *var;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	Check_Type(query, T_HASH);

	pool = otama_variant_pool_alloc();
	var = otama_variant_new(pool);
	rubyobj2variant(query, var);
	
	ret = otama_search(otama, &results, NUM2LL(n), var);
	if (ret != OTAMA_STATUS_OK) {
		otama_variant_pool_free(&pool);
		otama_rb_raise(ret);
		return Qnil;
	}
	result_array = make_results(results);
	
	otama_result_free(&results);
	otama_variant_pool_free(&pool);
	
	return result_array;
}

static VALUE
otama_rb_remove(VALUE self, VALUE id)
{
	otama_t *otama;
	otama_status_t ret;
	otama_id_t remove_id;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	
	ret = otama_id_hexstr2bin(&remove_id, StringValuePtr(id));
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}
	
	ret = otama_remove(otama, &remove_id);
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}

	return Qnil;
}

static VALUE
otama_rb_pull(VALUE self)
{
	otama_t *otama;
	otama_status_t ret;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	
	ret = otama_pull(otama);
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}

	return Qnil;
}

static VALUE
otama_rb_create_table(VALUE self)
{
	otama_t *otama;
	otama_status_t ret;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);

	ret = otama_create_table(otama);
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}
	
	return Qnil;
}

static VALUE
otama_rb_drop_table(VALUE self)
{
	otama_t *otama;
	otama_status_t ret;
	
	Data_Get_Struct(self, otama_t, otama);
	OTAMA_CHECK_NULL(otama);
	
	ret = otama_drop_table(otama);
	if (ret != OTAMA_STATUS_OK) {
		otama_rb_raise(ret);
	}
	
	return Qnil;
}

static VALUE
otama_rb_s_version_string(VALUE self)
{
	return rb_str_new2(otama_version_string());
}

static VALUE
otama_record_rb_initialize(VALUE self, VALUE id, VALUE value)
{
	rb_iv_set(self, "@id", id);
	rb_iv_set(self, "@value", value);

	return self;
}

static VALUE
otama_record_rb_id(VALUE self)
{
	return rb_iv_get(self, "@id");
}

static VALUE
otama_record_rb_value(VALUE self)
{
	return rb_iv_get(self, "@value");
}

static void
otama_raw_rb_free(otama_feature_raw_t *raw)
{
	otama_feature_raw_free(&raw);
}

static VALUE
otama_raw_rb_alloc(VALUE klass)
{
	return Data_Wrap_Struct(klass, 0, otama_raw_rb_free, NULL);
}

static VALUE
otama_raw_rb_initialize(VALUE self)
{
	return Qnil;
}

static VALUE
otama_raw_rb_dispose(VALUE self)
{
	otama_feature_raw_t *raw = NULL;

	Data_Get_Struct(self, otama_feature_raw_t, raw);
	otama_feature_raw_free(&raw);
	DATA_PTR(self) = NULL;
	
	return Qnil;
}

void
Init_otama(void)
{
	cOtama = rb_define_class("Otama", rb_cObject);
	cOtamaError = rb_define_class_under(cOtama, "Error", rb_eRuntimeError);
	cOtamaSystemError = rb_define_class_under(cOtama, "SystemError", cOtamaError);
	cOtamaNotImplemented = rb_define_class_under(cOtama, "NotImplemented", cOtamaError);
	cOtamaNoData = rb_define_class_under(cOtama, "NoData", cOtamaError);
	cOtamaArgumentError = rb_define_class_under(cOtama, "ArgumentError", cOtamaError);
	cOtamaAssertionFailure = rb_define_class_under(cOtama, "AssertionFailure", cOtamaError);
	
	rb_define_alloc_func(cOtama, otama_rb_alloc);
	rb_define_private_method(cOtama, "initialize", otama_rb_initialize, 1);
	rb_define_singleton_method(cOtama, "version_string", otama_rb_s_version_string, 0);
	rb_define_singleton_method(cOtama, "open", otama_rb_s_open, 1);
	rb_define_singleton_method(cOtama, "log_level=", otama_rb_s_set_log_level, 1);
	rb_define_singleton_method(cOtama, "set_log_file", otama_rb_s_set_log_file, 1);
	rb_define_singleton_method(cOtama, "id", otama_rb_s_id, 1);
	
	rb_define_const(cOtama, "LOG_LEVEL_DEBUG", INT2NUM(OTAMA_LOG_LEVEL_DEBUG));
	rb_define_const(cOtama, "LOG_LEVEL_NOTICE", INT2NUM(OTAMA_LOG_LEVEL_NOTICE));
	rb_define_const(cOtama, "LOG_LEVEL_ERROR", INT2NUM(OTAMA_LOG_LEVEL_ERROR));
	rb_define_const(cOtama, "LOG_LEVEL_QUIET", INT2NUM(OTAMA_LOG_LEVEL_QUIET));
	
	rb_define_method(cOtama, "close", otama_rb_close, 0);
	rb_define_method(cOtama, "search", otama_rb_search, 2);
	rb_define_method(cOtama, "similarity", otama_rb_similarity, 2);
	rb_define_method(cOtama, "insert", otama_rb_insert, 1);
	rb_define_method(cOtama, "set", otama_rb_set, 2);
	rb_define_method(cOtama, "unset", otama_rb_unset, 1);
	rb_define_method(cOtama, "get", otama_rb_get, 1);	
	rb_define_method(cOtama, "feature_string", otama_rb_feature_string, 1);
	rb_define_method(cOtama, "feature_raw", otama_rb_feature_raw, 1);
	rb_define_method(cOtama, "remove", otama_rb_remove, 1);
	rb_define_method(cOtama, "pull", otama_rb_pull, 0);
	rb_define_method(cOtama, "create_table", otama_rb_create_table, 0);
	rb_define_method(cOtama, "drop_table", otama_rb_drop_table, 0);

	rb_define_method(cOtama, "active?", otama_rb_active, 0);
	rb_define_method(cOtama, "count", otama_rb_count, 0);
	rb_define_method(cOtama, "exists?", otama_rb_exists, 1);

	cOtamaRecord = rb_define_class("Record", cOtama);
	rb_define_private_method(cOtamaRecord, "initialize", otama_record_rb_initialize, 2);
	rb_define_method(cOtamaRecord, "id", otama_record_rb_id, 0);
	rb_define_method(cOtamaRecord, "value", otama_record_rb_value, 0);
	
	cOtamaFeatureRaw = rb_define_class("FeatureRaw", cOtama);
	rb_define_alloc_func(cOtamaFeatureRaw, otama_raw_rb_alloc);
	rb_define_private_method(cOtamaFeatureRaw, "initialize", otama_raw_rb_initialize, 0);
	rb_define_method(cOtamaFeatureRaw, "dispose", otama_raw_rb_dispose, 0);
	
	otama_log_set_level(OTAMA_LOG_LEVEL_ERROR);
}