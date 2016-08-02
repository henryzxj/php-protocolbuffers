/*
 * Protocol Buffers for PHP
 * Copyright 2013 Shuhei Tanuma.  All rights reserved.
 *
 * https://github.com/chobie/php-protocolbuffers
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "protocolbuffers.h"
#include "descriptor_builder.h"
#include "message_options.h"
#include "descriptor.h"

#define PHP_PROTOCOLBUFFERS_MAX_EXTENSION_RANGE 536870912

static zend_object_handlers php_protocolbuffers_descriptor_builder_object_handlers;

int php_protocolbuffers_field_descriptor_get_property(HashTable *hash, const char *name, size_t name_len, zval **result TSRMLS_DC)
{
	zend_string *key;

	size_t key_len;
	zval *resval;

	key = zend_mangle_property_name("*", 1, (char*)name, name_len, 0);
	if ((resval=zend_hash_find(hash, key)) != NULL) {
		*result = resval;
	}

	efree(key);
	return 0;
}


static void php_protocolbuffers_descriptor_builder_free_storage(zend_object *object TSRMLS_DC)
{
	zend_object_std_dtor(object TSRMLS_CC);
	efree(object);
}

zend_object *php_protocolbuffers_descriptor_builder_new(zend_class_entry *ce TSRMLS_DC)
{
//	zend_object_value retval;
	zend_object *object;

//	object = (zend_object *)ecalloc(1, sizeof(*object));
//	zend_object_std_init(object, ce TSRMLS_CC);
//	object_properties_init(object, ce);

//	retval->handle = zend_objects_store_put(object);
//	retval.handle = zend_objects_store_put(object,
//		(zend_objects_store_dtor_t)zend_objects_destroy_object,
//		(zend_objects_free_object_storage_t) php_protocolbuffers_descriptor_builder_free_storage,
//	NULL TSRMLS_CC);
//	retval->handlers = zend_get_std_object_handlers();



		object= (zend_object *)ecalloc(1, sizeof(zend_object*) + zend_object_properties_size(ce));
		zend_object_std_init(object, ce TSRMLS_CC);
		object_properties_init(object, ce);
		php_protocolbuffers_descriptor_builder_object_handlers.offset = 0;
		php_protocolbuffers_descriptor_builder_object_handlers.free_obj = php_protocolbuffers_descriptor_builder_free_storage;
		object->handlers = &php_protocolbuffers_descriptor_builder_object_handlers;


	return object;
}


ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_builder_add_field, 0, 0, 2)
	ZEND_ARG_INFO(0, index)
	ZEND_ARG_INFO(0, field)
	ZEND_ARG_INFO(0, force_add)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_builder_get_name, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_builder_set_name, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_builder_build, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_builder_set_extension, 0, 0, 2)
	ZEND_ARG_INFO(0, index)
	ZEND_ARG_INFO(0, field)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_builder_add_extension_range, 0, 0, 2)
	ZEND_ARG_INFO(0, begin)
	ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_builder_get_options, 0, 0, 0)
ZEND_END_ARG_INFO()

static void php_protocolbuffers_build_extension_ranges(zval *instance, php_protocolbuffers_descriptor *descriptor TSRMLS_DC)
{
	zval *entry = NULL, *tmp = NULL;
	php_protocolbuffers_extension_range *ranges;
	HashPosition pos;
	int i = 0;

	zend_string *name = zend_string_init(ZEND_STRS("extension_ranges"),0);
	if ((tmp=zend_hash_find(Z_OBJPROP_P(instance), name)) == NULL) {
		return;
	}

	descriptor->container->extension_cnt = zend_hash_num_elements(Z_ARRVAL_P(tmp));
	ranges = (php_protocolbuffers_extension_range*)emalloc(sizeof(php_protocolbuffers_extension_range) * descriptor->container->extension_cnt);
	memset(ranges, '\0', sizeof(php_protocolbuffers_extension_range) * descriptor->container->extension_cnt);

	descriptor->container->extensions = ranges;
	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(tmp), &pos);
	while ((entry=zend_hash_get_current_data_ex(Z_ARRVAL_P(tmp), &pos)) != NULL) {
		zval *value = NULL;
		zend_string *begin_name = zend_string_init(ZEND_STRS("begin"),0);
		if ((value=zend_hash_find(Z_ARRVAL_P(entry), begin_name)) != NULL) {
			ranges[i].begin = Z_LVAL_P(value);
		}
		zend_string *end_name = zend_string_init(ZEND_STRS("end"),0);
		if ((value=zend_hash_find(Z_ARRVAL_P(entry), end_name)) == SUCCESS) {
			ranges[i].end = Z_LVAL_P(value);
		}

		zend_hash_move_forward_ex(Z_ARRVAL_P(tmp), &pos);
		i++;
	}

}

static void php_protocolbuffers_build_options(zval *instance, php_protocolbuffers_descriptor *descriptor TSRMLS_DC)
{
	zval *tmp = NULL;
	zval *val;
	zval *ext;
	HashPosition pos;
	zval *element;

	tmp  = pb_zend_read_property(php_protocol_buffers_descriptor_builder_class_entry, instance, ZEND_STRS("options")-1, 0 TSRMLS_CC);
	if (Z_TYPE_P(tmp) != IS_OBJECT) {
		return;
	}

	ext = pb_zend_read_property(php_protocol_buffers_descriptor_builder_class_entry, tmp, ZEND_STRS("extensions")-1, 0 TSRMLS_CC);
	if (Z_TYPE_P(ext) != IS_ARRAY) {
		return;
	}

	for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(ext), &pos);
			(element=zend_hash_get_current_data_ex(Z_ARRVAL_P(ext), &pos)) != NULL;
					zend_hash_move_forward_ex(Z_ARRVAL_P(ext), &pos)
	) {
		if (Z_OBJCE_P(element) != php_protocol_buffers_php_message_options_class_entry) {
			continue;
		}

		val = pb_zend_read_property(php_protocol_buffers_php_message_options_class_entry, element, ZEND_STRS("use_single_property")-1, 0 TSRMLS_CC);
		if (Z_TYPE_P(val) != IS_LONG) {
			convert_to_long(val);
		}
		descriptor->container->use_single_property = Z_LVAL_P(val);

		val = pb_zend_read_property(php_protocol_buffers_php_message_options_class_entry, element, ZEND_STRS("use_wakeup_and_sleep")-1, 0 TSRMLS_CC);
		if (Z_TYPE_P(val) != IS_LONG) {
			convert_to_long(val);
		}
		descriptor->container->use_wakeup_and_sleep = Z_LVAL_P(val);

		if (descriptor->container->use_single_property > 0) {
			val = pb_zend_read_property(php_protocol_buffers_php_message_options_class_entry, element, ZEND_STRS("single_property_name")-1, 0 TSRMLS_CC);
			efree(descriptor->container->single_property_name);

			zend_string * property_name_key = zend_mangle_property_name((char*)"*", 1, (char*)Z_STRVAL_P(val), Z_STRLEN_P(val), 0);
			descriptor->container->single_property_name = ZSTR_VAL(property_name_key);
			descriptor->container->single_property_name_len = ZSTR_LEN(property_name_key);
			descriptor->container->single_property_h = zend_inline_hash_func(descriptor->container->single_property_name, descriptor->container->single_property_name_len+1);

			if (memcmp(descriptor->container->orig_single_property_name, Z_STRVAL_P(val), Z_STRLEN_P(val)) != 0) {
				descriptor->container->orig_single_property_name = emalloc(sizeof(char*) * Z_STRLEN_P(val));
				memcpy(descriptor->container->orig_single_property_name, Z_STRVAL_P(val), Z_STRLEN_P(val));
				descriptor->container->orig_single_property_name[Z_STRLEN_P(val)] = '\0';
				descriptor->container->orig_single_property_name_len = Z_STRLEN_P(val)+1;
			}
		}

		val = pb_zend_read_property(php_protocol_buffers_php_message_options_class_entry, element, ZEND_STRS("process_unknown_fields")-1, 0 TSRMLS_CC);
		if (Z_TYPE_P(val) == IS_TRUE||Z_TYPE_P(val) == IS_FALSE) {
			descriptor->container->process_unknown_fields = Z_LVAL_P(val);
		}
	}
}

int php_protocolbuffers_init_scheme_with_zval(php_protocolbuffers_scheme *scheme, int tag, zval *element TSRMLS_DC)
{
	zval *tmp = NULL;
	int tsize = 0;
	char *mangle;
	int mangle_len;
	zval *def;

	scheme->ce = NULL;
	scheme->is_extension = 0;
	scheme->tag = tag;

	php_protocolbuffers_field_descriptor_get_property(Z_OBJPROP_P(element), ZEND_STRS("type"), &tmp TSRMLS_CC);
	if (Z_TYPE_P(tmp) != IS_LONG) {
		convert_to_long(tmp);
	}
	scheme->type = Z_LVAL_P(tmp);

	php_protocolbuffers_field_descriptor_get_property(Z_OBJPROP_P(element), ZEND_STRS("name"), &tmp TSRMLS_CC);
	if (Z_TYPE_P(tmp) != IS_STRING) {
		convert_to_string(tmp);
	}

	tsize = Z_STRLEN_P(tmp)+1;

	scheme->original_name     = (char*)emalloc(sizeof(char*) * tsize);
	scheme->original_name_len = tsize;

	memcpy(scheme->original_name, Z_STRVAL_P(tmp), tsize);
	scheme->original_name[tsize] = '\0';

	scheme->name     = (char*)emalloc(sizeof(char*) * tsize);
	scheme->name_len = tsize;

	memcpy(scheme->name, Z_STRVAL_P(tmp), tsize);
	scheme->name[tsize] = '\0';
	php_strtolower(scheme->name, tsize);
	scheme->name_h = zend_inline_hash_func(scheme->name, tsize);

	/* use strcmp or fuzzy match */
	scheme->magic_type = (strcmp(scheme->name, scheme->original_name) == 0) ? 0 : 1;

	zend_string *mangle_key = zend_mangle_property_name((char*)"*", 1, (char*)scheme->original_name, scheme->original_name_len, 0);
	scheme->mangled_name     = ZSTR_VAL(mangle_key);
	scheme->mangled_name_len = ZSTR_LEN(mangle_key);
	scheme->mangled_name_h   = zend_inline_hash_func(mangle, mangle_len);
	scheme->skip = 0;

	if (scheme->type == TYPE_MESSAGE) {
		zend_class_entry *c;

		php_protocolbuffers_field_descriptor_get_property(Z_OBJPROP_P(element), ZEND_STRS("message"), &tmp TSRMLS_CC);

		if (Z_TYPE_P(tmp) == IS_STRING) {
			zend_string *tmp_name = zend_string_init(Z_STRVAL_P(tmp), Z_STRLEN_P(tmp),0);
			if ((c=zend_lookup_class(tmp_name)) == NULL) {
				efree(scheme->original_name);
				efree(scheme->name);
				efree(scheme->mangled_name);
				zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "the class %s does not find.", Z_STRVAL_P(tmp));
				return 0;
			}

			scheme->ce = c;
		} else {
			efree(scheme->original_name);
			efree(scheme->name);
			efree(scheme->mangled_name);
			zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "message wiretype set. we need message parameter for referencing class entry.");
			return 0;
		}
	}


	php_protocolbuffers_field_descriptor_get_property(Z_OBJPROP_P(element), ZEND_STRS("default"), &tmp TSRMLS_CC);
	/* TODO(chobie): check types */

//	MAKE_STD_ZVAL(def);
	ZVAL_ZVAL(def, tmp, 1, 0);
	scheme->default_value = def;

	php_protocolbuffers_field_descriptor_get_property(Z_OBJPROP_P(element), ZEND_STRS("required"), &tmp TSRMLS_CC);
	if (Z_TYPE_P(tmp) != IS_LONG) {
		convert_to_long(tmp);
	}
	scheme->required = Z_LVAL_P(tmp);

	php_protocolbuffers_field_descriptor_get_property(Z_OBJPROP_P(element), ZEND_STRS("optional"), &tmp TSRMLS_CC);
	if (Z_TYPE_P(tmp) != IS_LONG) {
		convert_to_long(tmp);
	}
	scheme->optional = Z_LVAL_P(tmp);

	php_protocolbuffers_field_descriptor_get_property(Z_OBJPROP_P(element), ZEND_STRS("repeated"), &tmp TSRMLS_CC);
	if (Z_TYPE_P(tmp) != IS_LONG) {
		convert_to_long(tmp);
	}
	scheme->repeated = Z_LVAL_P(tmp);

	php_protocolbuffers_field_descriptor_get_property(Z_OBJPROP_P(element), ZEND_STRS("packable"), &tmp TSRMLS_CC);
	if (Z_TYPE_P(tmp) != IS_LONG) {
		convert_to_long(tmp);
	}
	scheme->packed = Z_LVAL_P(tmp);

	return 1;
}

static int php_protocolbuffers_build_fields(zval *fields, php_protocolbuffers_descriptor *descriptor, zval *result  TSRMLS_DC)
{
	HashTable *proto;
	HashPosition pos;
	zval *element;
	int n;
	size_t sz;
	php_protocolbuffers_scheme *ischeme;

	if (fields == NULL || Z_TYPE_P(fields) != IS_ARRAY) {
		return 0;
	}

	proto = Z_ARRVAL_P(fields);
	sz = zend_hash_num_elements(proto);

	ischeme = (php_protocolbuffers_scheme*)emalloc(sizeof(php_protocolbuffers_scheme) * sz);
	memset(ischeme, '\0', sizeof(php_protocolbuffers_scheme) * sz);
	descriptor->container->size = sz;
	descriptor->container->scheme = ischeme;

	for(n = 0, zend_hash_internal_pointer_reset_ex(proto, &pos);
					(element=zend_hash_get_current_data_ex(proto, &pos)) != NULL;
					zend_hash_move_forward_ex(proto, &pos), n++
	) {
		if (!php_protocolbuffers_init_scheme_with_zval(&ischeme[n], pos, element TSRMLS_CC)) {
			efree(ischeme);

			descriptor->container->size = 0;
			descriptor->container->scheme = NULL;
			return 0;
		}
	}

	return 1;
}

static void php_protocolbuffers_build_field_descriptor(php_protocolbuffers_descriptor *descriptor, zval *result  TSRMLS_DC)
{
	int n = 0;
	php_protocolbuffers_scheme *ischeme;
	zval *arrval = NULL;
	zval *tmp = NULL, *value = NULL;

	if (descriptor->container->size < 1) {
		return;
	}

	MAKE_STD_ZVAL(arrval);
	array_init(arrval);
	for (n = 0; n < descriptor->container->size; n++) {
		ischeme = &(descriptor->container->scheme[n]);

		MAKE_STD_ZVAL(tmp);
		object_init_ex(tmp, php_protocol_buffers_field_descriptor_class_entry);

		MAKE_STD_ZVAL(value);
		ZVAL_STRING(value, ischeme->name);
		php_protocolbuffers_set_protected_property(tmp, ZEND_STRS("name"), value TSRMLS_CC);

		MAKE_STD_ZVAL(value);
		ZVAL_LONG(value, ischeme->type);
		php_protocolbuffers_set_protected_property(tmp, ZEND_STRS("type"), value TSRMLS_CC);

		MAKE_STD_ZVAL(value);
		ZVAL_BOOL(value, ischeme->is_extension);
		php_protocolbuffers_set_protected_property(tmp, ZEND_STRS("extension"), value TSRMLS_CC);

		zend_hash_index_update(Z_ARRVAL_P(arrval), ischeme->tag, tmp);
	}

	php_protocolbuffers_set_protected_property(result, ZEND_STRS("fields"), arrval TSRMLS_CC);
}

/* {{{ proto ProtocolBuffers_DescriptorBuilder ProtocolBuffers_DescriptorBuilder::__construct()
*/
PHP_METHOD(protocolbuffers_descriptor_builder, __construct)
{
	zval *instance = getThis();
	HashTable *properties = NULL;
	zval *tmp = NULL;

	ALLOC_HASHTABLE(properties);
	zend_hash_init(properties, 0, NULL, ZVAL_PTR_DTOR, 0);

	MAKE_STD_ZVAL(tmp);
	ZVAL_NULL(tmp);
	zend_string *name_key = zend_string_init("name", sizeof("name"),0);
	zend_hash_update(properties, name_key,tmp);

	MAKE_STD_ZVAL(tmp);
	array_init(tmp);
	zend_string *fields_key = zend_string_init("fields", sizeof("fields"),0);
	zend_hash_update(properties, fields_key, tmp);

	MAKE_STD_ZVAL(tmp);
	object_init_ex(tmp, php_protocol_buffers_message_options_class_entry);
	php_protocolbuffers_message_options_init_properties(tmp TSRMLS_CC);

	zend_string *options_key = zend_string_init("options", sizeof("options"),0);
	zend_hash_update(properties, options_key, tmp);

	MAKE_STD_ZVAL(tmp);
	array_init(tmp);
	zend_string *extension_ranges_key = zend_string_init("extension_ranges", sizeof("extension_ranges"),0);
	zend_hash_update(properties, extension_ranges_key, tmp);

	zend_merge_properties(instance, properties);
}
/* }}} */

/* {{{ proto void ProtocolBuffersDescriptorBuilder::addField(int $index, ProtocolBuffers_DescriptorField $field)
*/
PHP_METHOD(protocolbuffers_descriptor_builder, addField)
{
	zval *instance = getThis();
	zval *field = NULL;
	zval *fields = NULL;
	zend_ulong index;
	zend_bool force_add = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"lO|b", &index, &field, php_protocol_buffers_field_descriptor_class_entry, &force_add) == FAILURE) {
		return;
	}

	if (index < 1 || index > ktagmax) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "index should be in %d - %d", 1, ktagmax);
		return;
	}

	zend_string *fields_name = zend_string_init(ZEND_STRS("fields"),0);
	if ((fields=zend_hash_find(Z_OBJPROP_P(instance), fields_name)) != NULL) {
		if (zend_hash_index_exists(Z_ARRVAL_P(fields), index)) {
			if (force_add < 1) {
				zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "tag number `%d` has already registered.", index);
			}
			return;
		}

		if (Z_REFCOUNTED_P(field)) {
			Z_ADDREF_P(field);
		}
		zend_hash_index_update(Z_ARRVAL_P(fields), index, field);
	}
}
/* }}} */

/* {{{ proto void ProtocolBuffersDescriptorBuilder::setName(string $name)
*/
PHP_METHOD(protocolbuffers_descriptor_builder, setName)
{
	zval *instance = getThis();
	char *name;
	long name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	if (name_len > 0) {
		add_property_string_ex(instance, ZEND_STRS("name"), name);
	}
}
/* }}} */


/* {{{ proto string ProtocolBuffers_DescriptorBuilder::getName()
*/
PHP_METHOD(protocolbuffers_descriptor_builder, getName)
{
	zval *name;

	name = pb_zend_read_property(php_protocol_buffers_descriptor_builder_class_entry, getThis(), ZEND_STRS("name")-1, 0 TSRMLS_CC);
	RETURN_STRINGL(Z_STRVAL_P(name), Z_STRLEN_P(name));
}
/* }}} */


/* {{{ proto ProtocolBuffersDescriptor ProtocolBuffersDescriptorBuilder::build()
*/
PHP_METHOD(protocolbuffers_descriptor_builder, build)
{
	zval *result, *name, *fields = NULL;
	php_protocolbuffers_descriptor *descriptor;

	MAKE_STD_ZVAL(result);
	object_init_ex(result, php_protocol_buffers_descriptor_class_entry);
	php_protocolbuffers_descriptor_properties_init(result TSRMLS_CC);
	descriptor = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_descriptor, result);

	name = pb_zend_read_property(php_protocol_buffers_descriptor_builder_class_entry, getThis(), ZEND_STRS("name")-1, 0 TSRMLS_CC);
	if (Z_TYPE_P(name) == IS_STRING) {
		descriptor->name_len = Z_STRLEN_P(name);
		if (descriptor->name_len > 0) {
			descriptor->name = (char*)emalloc(descriptor->name_len+1);
			memset(descriptor->name, '\0', descriptor->name_len+1);
			memcpy(descriptor->name, Z_STRVAL_P(name), descriptor->name_len);
		}
	}

	fields = pb_zend_read_property(php_protocol_buffers_descriptor_builder_class_entry, getThis(), ZEND_STRS("fields")-1, 0 TSRMLS_CC);
	if (!php_protocolbuffers_build_fields(fields, descriptor, result TSRMLS_CC)) {
		zval_ptr_dtor(result);
		RETURN_NULL();
	}
	php_protocolbuffers_build_options(getThis(), descriptor TSRMLS_CC);
	php_protocolbuffers_build_extension_ranges(getThis(), descriptor TSRMLS_CC);
	php_protocolbuffers_build_field_descriptor(descriptor, result TSRMLS_CC);

	RETURN_ZVAL(result, 0, 1);
}
/* }}} */

/* {{{ proto ProtocolBuffersMessageOptions ProtocolBuffersDescriptorBuilder::getOptions()
*/
PHP_METHOD(protocolbuffers_descriptor_builder, getOptions)
{
	zval *options = NULL;
	options = pb_zend_read_property(php_protocol_buffers_descriptor_builder_class_entry, getThis(), ZEND_STRS("options")-1, 0 TSRMLS_CC);
	RETURN_ZVAL(options, 1, 0);
}
/* }}} */


/* {{{ proto ProtocolBuffersMessageOptions ProtocolBuffersDescriptorBuilder::addExtensionRange(int $begin, int $end)
*/
PHP_METHOD(protocolbuffers_descriptor_builder, addExtensionRange)
{
	zval *instance = getThis();
	zval *z_begin = NULL, *z_end = NULL, *fields = NULL, *extension_ranges = NULL;
	long begin = 0, end = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"ll", &begin, &end) == FAILURE) {
		return;
	}

	if (begin == end || begin > end) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "range end must be bigger than range begin.");
		return;
	}
	if (end > PHP_PROTOCOLBUFFERS_MAX_EXTENSION_RANGE) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "range end must be smaller than %d", PHP_PROTOCOLBUFFERS_MAX_EXTENSION_RANGE);
		return;
	}
	if (begin < 1 || end < 1) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "range must be greater than zero");
		return;
	}

	zend_string *fields_name = zend_string_init(ZEND_STRS("fields"),0);
	if ((fields=zend_hash_find(Z_OBJPROP_P(instance), fields_name)) != NULL) {
		HashPosition pos = 0;
		char *string_key = {0};
		uint string_key_len = 0;
		ulong num_key = 0;
		zval *entry = NULL;
		zend_string *string_key_name = zend_string_init(string_key,string_key_len,0);
		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(fields), &pos);
		while ((entry=zend_hash_get_current_data_ex(Z_ARRVAL_P(fields), &pos)) != NULL) {
			switch (zend_hash_get_current_key_ex(Z_ARRVAL_P(fields), &string_key_name, &num_key, &pos)) {
				case HASH_KEY_IS_STRING:
					zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "extension range expects long key.");
					return;
				break;
				case HASH_KEY_IS_LONG:
					if (begin <= num_key && num_key <= end) {
						zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "extension range must be greater than existing tag number");
						return;
					}
				break;
			}

			zend_hash_move_forward_ex(Z_ARRVAL_P(fields), &pos);
		}
	}
	zend_string *extension_ranges_key=zend_string_init(ZEND_STRS("extension_ranges"),0);
	if ((extension_ranges=zend_hash_find(Z_OBJPROP_P(instance), extension_ranges_key)) != NULL) {
		zval *array = NULL;
		MAKE_STD_ZVAL(array);
		array_init(array);

		MAKE_STD_ZVAL(z_begin);
		MAKE_STD_ZVAL(z_end);
		ZVAL_LONG(z_begin, begin);
		ZVAL_LONG(z_end, end);

		zend_string *begin_name = zend_string_init(ZEND_STRS("begin"),0);
		zend_hash_update(Z_ARRVAL_P(array), begin_name, z_begin);
		zend_string *end_name = zend_string_init(ZEND_STRS("end"),0);
		zend_hash_update(Z_ARRVAL_P(array), end_name,   z_end);

		zend_hash_next_index_insert(Z_ARRVAL_P(extension_ranges), array);
	}
}
/* }}} */

static zend_function_entry php_protocolbuffers_descriptor_builder_methods[] = {
	PHP_ME(protocolbuffers_descriptor_builder, __construct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(protocolbuffers_descriptor_builder, addField,  arginfo_protocolbuffers_descriptor_builder_add_field, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_descriptor_builder, getName,   arginfo_protocolbuffers_descriptor_builder_get_name, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_descriptor_builder, setName,   arginfo_protocolbuffers_descriptor_builder_set_name, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_descriptor_builder, build,     arginfo_protocolbuffers_descriptor_builder_build, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_descriptor_builder, addExtensionRange, arginfo_protocolbuffers_descriptor_builder_add_extension_range, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_descriptor_builder, getOptions, arginfo_protocolbuffers_descriptor_builder_get_options, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

void php_protocolbuffers_descriptor_builder_class(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "ProtocolBuffersDescriptorBuilder", php_protocolbuffers_descriptor_builder_methods);
	php_protocol_buffers_descriptor_builder_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	php_protocol_buffers_descriptor_builder_class_entry->create_object = php_protocolbuffers_descriptor_builder_new;

	zend_declare_property_null(php_protocol_buffers_descriptor_builder_class_entry, ZEND_STRS("name")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_protocol_buffers_descriptor_builder_class_entry, ZEND_STRS("fields")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_protocol_buffers_descriptor_builder_class_entry, ZEND_STRS("options")-1, ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(php_protocol_buffers_descriptor_builder_class_entry, ZEND_STRS("extension_ranges")-1, ZEND_ACC_PUBLIC TSRMLS_CC);

	PHP_PROTOCOLBUFFERS_REGISTER_NS_CLASS_ALIAS(PHP_PROTOCOLBUFFERS_NAMESPACE, "DescriptorBuilder", php_protocol_buffers_descriptor_builder_class_entry);
}
