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
#include "message.h"
#if PHP_MAJOR_VERSION < 7
#include "ext/standard/php_smart_str.h"
#else
#include "zend_smart_str.h"
#endif
#include "unknown_field_set.h"
#include "extension_registry.h"

//#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION == 4) && (PHP_RELEASE_VERSION >= 26)) || ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION == 5) && (PHP_RELEASE_VERSION >= 10))
//#include "ext/json/php_json.h"
//#endif

enum ProtocolBuffers_MagicMethod {
	MAGICMETHOD_GET    = 1,
	MAGICMETHOD_SET    = 2,
	MAGICMETHOD_APPEND = 3,
	MAGICMETHOD_CLEAR  = 4,
	MAGICMETHOD_HAS    = 5,
	MAGICMETHOD_MUTABLE= 6,
};

static zend_object_handlers php_protocolbuffers_message_object_handlers;
static int json_serializable_checked = 0;

#define PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME2(instance, container, proto) \
	{\
		zend_class_entry *__ce;\
		int __err;\
		\
		__ce  = Z_OBJCE_P(instance);\
		__err = php_protocolbuffers_get_scheme_container(ZSTR_VAL(__ce->name), ZSTR_LEN(__ce->name), container TSRMLS_CC);\
		if (__err) {\
			if (EG(exception)) {\
				return;\
			} else {\
				/* TODO: improve displaying error message */\
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_get_scheme_container failed. %s does not have getDescriptor method", __ce->name);\
				return;\
			}\
		}\
	}


#define PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME2(instance, &container, proto)

static void php_protocolbuffers_message_free_storage(php_protocolbuffers_message *object TSRMLS_DC)
{
	if (object->container != NULL) {
		ZVAL_NULL(object->container);
		zval_ptr_dtor(object->container);
		object->container = NULL;
	}
	zend_object_std_dtor(&object->zo TSRMLS_CC);
	efree(object);
}

zend_object *php_protocolbuffers_message_new(zend_class_entry *ce TSRMLS_DC)
{
	PHP_PROTOCOLBUFFERS_STD_CREATE_OBJECT(php_protocolbuffers_message);

	intern->max    = 0;
	intern->offset = 0;
	intern->container = (zval *)emalloc(sizeof(zval));
	return &intern->zo;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message___construct, 0, 0, 0)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_set_from, 0, 0, 1)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_serialize_to_string, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_parse_from_string, 0, 0, 1)
	ZEND_ARG_INFO(0, string)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message___call, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_merge_from, 0, 0, 1)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_discard_unknown_fields, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_clear, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_clear_alls, 0, 0, 0)
	ZEND_ARG_INFO(0, clear_unknown_fields)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_has, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_get, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_mutable, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_set, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_append, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_get_extension, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_has_extension, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_set_extension, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_message_clear_extension, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

static void php_protocolbuffers_get_hash(php_protocolbuffers_scheme_container *container, php_protocolbuffers_scheme *scheme, zval *object, char **name, int *name_len, HashTable **ht TSRMLS_DC)
{
	char *n = {0};
	int n_len = 0;
	HashTable *htt = NULL;

	if (container->use_single_property > 0) {
		n     = container->single_property_name;
		n_len = container->single_property_name_len;
		zend_string *n_key = zend_string_init(n,n_len,0);
		if ((htt=(HashTable *)zend_hash_find(Z_OBJPROP_P(object), n_key)) == NULL) {
			return;
		}

		n = scheme->name;
		n_len = scheme->name_len;
	} else {
		htt = Z_OBJPROP_P(object);

		n = scheme->mangled_name;
		n_len = scheme->mangled_name_len;
	}

	name = &n;
	name_len = &n_len;
	*ht = htt;

}

static void php_protocolbuffers_message_merge_from(php_protocolbuffers_scheme_container *container, HashTable *htt, HashTable *hts TSRMLS_DC)
{
	php_protocolbuffers_scheme *scheme;

	int i = 0;
	for (i = 0; i < container->size; i++) {
		zval *tmp = NULL;
		char *name = {0};
		int name_len = 0;
		zend_string *name_key = NULL;
		scheme = &(container->scheme[i]);

		if (container->use_single_property > 0) {
			name = scheme->name;
			name_len = scheme->name_len;
		} else {
			name = scheme->mangled_name;
			name_len = scheme->mangled_name_len;
		}
		name_key = zend_string_init(name,name_len,0);
		if ((tmp=zend_hash_find(hts, name_key)) != NULL) {
			zval *val;

			switch (Z_TYPE_P(tmp)) {
			case IS_NULL:
			break;
			case IS_STRING:
//				MAKE_STD_ZVAL(val);
				ZVAL_STRINGL(val, Z_STRVAL_P(tmp), Z_STRLEN_P(tmp));

				Z_ADDREF_P(val);
				zend_hash_update(htt, name_key, val);
				zval_ptr_dtor(val);
				break;
			case IS_LONG:
//				MAKE_STD_ZVAL(val);
				ZVAL_LONG(val, Z_LVAL_P(tmp));

				Z_ADDREF_P(val);
				zend_hash_update(htt, name_key, val);
				zval_ptr_dtor(val);
			break;
			case IS_DOUBLE:
//				MAKE_STD_ZVAL(val);
				ZVAL_DOUBLE(val, Z_DVAL_P(tmp));

				Z_ADDREF_P(val);
				zend_hash_update(htt, name_key, val);
				zval_ptr_dtor(val);
			break;
			case IS_OBJECT:
			{
				zval *tmp2 = NULL;

				if ((tmp2=zend_hash_find(htt, name_key)) != NULL) {
					if (Z_TYPE_P(tmp2) == IS_OBJECT) {
						char *n;
						int n_len;
						php_protocolbuffers_scheme_container *c;
						HashTable *_htt = NULL, *_hts = NULL;

						PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME2(tmp, &c, p)
						php_protocolbuffers_get_hash(c, c->scheme, tmp, &n, &n_len, &_htt TSRMLS_CC);
						php_protocolbuffers_get_hash(c, c->scheme, tmp2, &n, &n_len, &_hts TSRMLS_CC);

						php_protocolbuffers_message_merge_from(c, _htt, _hts TSRMLS_CC);
					} else {
//						MAKE_STD_ZVAL(val);
						ZVAL_ZVAL(val, tmp, 1, 0);

						Z_ADDREF_P(val);
						zend_hash_update(htt, name_key, val);
						zval_ptr_dtor(val);
					}
				} else {
//					MAKE_STD_ZVAL(val);
					ZVAL_ZVAL(val, tmp, 1, 0);

					Z_ADDREF_P(val);
					zend_hash_update(htt, name_key, val);
					zval_ptr_dtor(val);
				}
			}
			break;
			case IS_ARRAY: {
				zval *tmp2 = NULL;

				if ((tmp2=zend_hash_find(htt, name_key)) != NULL) {
					if (Z_TYPE_P(tmp2) == IS_ARRAY) {
						php_array_merge(Z_ARRVAL_P(tmp2), Z_ARRVAL_P(tmp));
					} else {
//						MAKE_STD_ZVAL(val);
						ZVAL_ZVAL(val, tmp, 1, 0);

						Z_ADDREF_P(val);
						zend_hash_update(htt, name_key, val);
						zval_ptr_dtor(val);
					}
				} else {
//					MAKE_STD_ZVAL(val);
					ZVAL_ZVAL(val, tmp, 1, 0);

					Z_ADDREF_P(val);
					zend_hash_update(htt, name_key, val);
					zval_ptr_dtor(val);
				}
			}
			break;
			default:
				zend_error(E_NOTICE, "mergeFrom: zval type %d is not supported.", Z_TYPE_P(tmp));
			}
		}

	}
}

static inline void php_protocolbuffers_typeconvert(php_protocolbuffers_scheme *scheme, zval *vl TSRMLS_DC)
{
	// TODO: check message
#define TYPE_CONVERT(vl) \
		switch (scheme->type) {\
			case TYPE_STRING:\
				if (Z_TYPE_P(vl) != IS_STRING) {\
					zend_string * tostring_key = zend_string_init(ZEND_STRS("__tostring"),0);\
					if (Z_TYPE_P(vl) == IS_OBJECT && zend_hash_exists(&(Z_OBJCE_P(vl)->function_table), tostring_key)) {\
					} else {\
						convert_to_string(vl);\
					}\
				}\
			break;\
			case TYPE_SINT32:\
			case TYPE_INT32:\
			case TYPE_UINT32:\
				if (Z_TYPE_P(vl) != IS_LONG) {\
					if (Z_TYPE_P(vl) != IS_STRING) {\
						convert_to_long(vl);\
					}\
				}\
			break;\
			case TYPE_SINT64:\
			case TYPE_INT64:\
			case TYPE_UINT64:\
				if (Z_TYPE_P(vl) != IS_LONG) {\
					if (Z_TYPE_P(vl) != IS_STRING) {\
						convert_to_long(vl);\
					}\
				}\
			break;\
			case TYPE_DOUBLE:\
			case TYPE_FLOAT:\
				if (Z_TYPE_P(vl) != IS_DOUBLE) {\
					if (Z_TYPE_P(vl) != IS_STRING) {\
						convert_to_double(vl);\
					}\
				}\
			break;\
			case TYPE_BOOL:\
				if (Z_TYPE_P(vl) != IS_TRUE&&Z_TYPE_P(vl) != IS_FALSE) {\
					convert_to_boolean(vl);\
				}\
			break;\
		}\


	if (scheme->repeated < 1) {
		TYPE_CONVERT(vl)
	} else {
		zval *entry;
		HashPosition pos;

		if (Z_TYPE_P(vl) == IS_ARRAY) {
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(vl), &pos);
			while ((entry=zend_hash_get_current_data_ex(Z_ARRVAL_P(vl), &pos)) != NULL) {
				TYPE_CONVERT(entry)

				zend_hash_move_forward_ex(Z_ARRVAL_P(vl), &pos);
			}
		}
	}

#undef TYPE_CONVERT
}


static php_protocolbuffers_scheme *php_protocolbuffers_message_get_scheme_by_name(php_protocolbuffers_scheme_container *container, char *name, int name_len, char *name2, int name2_len)
{
	int i = 0;
	php_protocolbuffers_scheme *scheme = NULL;

	for (i = 0; i < container->size; i++) {
		scheme = &container->scheme[i];

		if (strcmp(scheme->name, name) == 0) {
			break;
		}
		if (name2 != NULL) {
			if (scheme->magic_type == 1 && strcasecmp(scheme->original_name, name2) == 0) {
				break;
			}
		}
		scheme = NULL;
	}

	return scheme;
}

static void php_protocolbuffers_message_get_hash_table_by_container(php_protocolbuffers_scheme_container *container, php_protocolbuffers_scheme *scheme, zval *instance, HashTable **hash, char **name, int *name_len TSRMLS_DC)
{
	char *n = NULL;
	int n_len = 0;
	HashTable *htt = NULL;
	zend_string *n_key = NULL;
	if (container->use_single_property > 0) {
		n     = container->single_property_name;
		n_len = container->single_property_name_len;
		n_key = zend_string_init(n,n_len,0);
		if ((htt=(HashTable *)zend_hash_find(Z_OBJPROP_P(instance), n_key)) == NULL) {
			return;
		}
	} else {
		htt = Z_OBJPROP_P(instance);

		n = scheme->mangled_name;
		n_len = scheme->mangled_name_len;
	}

	*hash = htt;
	*name = n;
	*name_len = n_len;
}

static void php_protocolbuffers_message_get(INTERNAL_FUNCTION_PARAMETERS, zval *instance, php_protocolbuffers_scheme_container *container, char *name, int name_len, char *name2, int name2_len, zval *params)
{
	char *n = {0};
	int n_len = 0;
	HashTable *htt = NULL;
	php_protocolbuffers_scheme *scheme;
	zval *e = NULL;
	zend_string *n_key = NULL;
	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "get method can't use for extension value", name);
		return;
	}

	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);
	n_key = zend_string_init(n,n_len,0);
	if ((e=zend_hash_find(htt,n_key)) != NULL) {
		if (scheme->repeated && params != NULL && Z_TYPE_P(params) != IS_NULL) {
			zval *value;

			if (Z_TYPE_P(params) != IS_LONG) {
				convert_to_long(params);
			}

			if ((value=zend_hash_index_find(Z_ARRVAL_P(e), Z_LVAL_P(params))) != NULL) {
				RETURN_ZVAL(value, 1, 0);
			}
		} else {
			if (scheme->ce != NULL && Z_TYPE_P(e) == IS_NULL) {
				zval *tmp;
//				MAKE_STD_ZVAL(tmp);
				object_init_ex(tmp, scheme->ce);
				php_protocolbuffers_properties_init(tmp, scheme->ce TSRMLS_CC);

				RETURN_ZVAL(tmp, 0, 1);
			} else {
				RETURN_ZVAL(e, 1, 0);
			}
		}
	}
}

static void php_protocolbuffers_message_set(INTERNAL_FUNCTION_PARAMETERS, zval *instance, php_protocolbuffers_scheme_container *container, char *name, int name_len, char *name2, int name2_len, zval *value)
{
	php_protocolbuffers_scheme *scheme;
	php_protocolbuffers_message *m;
	zval *tmp;
	zval *e = NULL;
	HashTable *htt;
	char *n = {0};
	int n_len = 0;
	int should_free = 0;
	zend_string *n_key = NULL;
	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zval_ptr_dtor(value);
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zval_ptr_dtor(value);
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "set method can't use for extension value", name);
		return;
	}

	if (Z_TYPE_P(value) == IS_NULL) {
		zval_ptr_dtor(value);
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "set method can't pass null value", name);
		return;
	}

	if (scheme->ce != NULL) {
		if (scheme->repeated) {
			HashPosition pos;
			zval *element, *outer;

			if (zend_hash_num_elements(Z_ARRVAL_P(value)) > 0) {
//				MAKE_STD_ZVAL(outer);
				array_init(outer);

				for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(value), &pos);
								(element=zend_hash_get_current_data_ex(Z_ARRVAL_P(value), &pos)) != NULL;
								zend_hash_move_forward_ex(Z_ARRVAL_P(value), &pos)
				) {
					zval *child, *param;

					if (Z_TYPE_P(element) == IS_NULL) {
						continue;
					}

					if (Z_TYPE_P(element) == IS_OBJECT) {
						if (scheme->ce != Z_OBJCE_P(element)) {
							zval_ptr_dtor(outer);
							zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "expected %s class. given %s class", scheme->ce->name, Z_OBJCE_P(element)->name);
							return;
						} else {
							m = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, element);
							ZVAL_ZVAL(m->container, instance, 0, 0);

							Z_ADDREF_P(element);
							add_next_index_zval(outer, element);
						}
					} else {
//						MAKE_STD_ZVAL(child);
//						MAKE_STD_ZVAL(param);
						ZVAL_ZVAL(param, element, 1, 0);

						object_init_ex(child, scheme->ce);
						php_protocolbuffers_properties_init(child, scheme->ce TSRMLS_CC);

						zend_call_method_with_1_params(child, scheme->ce, NULL, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, param);
						zval_ptr_dtor(param);

						m = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, child);
						ZVAL_ZVAL(m->container, instance, 0, 0);
						add_next_index_zval(outer, child);
					}

				}
				value = outer;
				should_free = 1;
			}

		} else {
			if (Z_TYPE_P(value) == IS_ARRAY) {
				/* NOTE(chobie): Array to Object conversion */
				zval *param;

//				MAKE_STD_ZVAL(tmp);
//				MAKE_STD_ZVAL(param);

				ZVAL_ZVAL(param, value, 1, 0);

				object_init_ex(tmp, scheme->ce);
				php_protocolbuffers_properties_init(tmp, scheme->ce TSRMLS_CC);
				zend_call_method_with_1_params(tmp, scheme->ce, NULL, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, param);
				zval_ptr_dtor(param);

				value = tmp;
				should_free = 1;
			}

			if (scheme->ce != Z_OBJCE_P(value)) {
				zval_ptr_dtor(value);
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "expected %s class. given %s class", scheme->ce->name, Z_OBJCE_P(value)->name);
				return;
			}

			m = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, value);
			ZVAL_ZVAL(m->container, instance, 0, 0);
		}
	}

	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);
	n_key = zend_string_init(n,n_len,0);
	if ((e=zend_hash_find(htt, n_key)) != NULL) {
		zval *vl = NULL;

		if (container->use_single_property > 0) {
//			MAKE_STD_ZVAL(vl);
			ZVAL_ZVAL(vl, e, 1, 0);
			php_protocolbuffers_typeconvert(scheme, vl TSRMLS_CC);
			zend_string *scheme_name_key = zend_string_init(scheme->name, scheme->name_len, 0);
			Z_ADDREF_P(vl);
			zend_hash_update(htt, scheme_name_key, vl);
			zval_ptr_dtor(vl);
		} else {
			zval *garvage = e;

//			MAKE_STD_ZVAL(vl);
			ZVAL_ZVAL(vl, value, 1, 0);

			Z_ADDREF_P(vl);
			php_protocolbuffers_typeconvert(scheme, vl TSRMLS_CC);
			e = vl;
			zval_ptr_dtor(garvage);
			zval_ptr_dtor(vl);
		}
		if (should_free) {
			zval_ptr_dtor(value);
		}
	}
}

static void php_protocolbuffers_message_clear(INTERNAL_FUNCTION_PARAMETERS, zval *instance, php_protocolbuffers_scheme_container *container, char *name, int name_len, char *name2, int name2_len)
{
	php_protocolbuffers_scheme *scheme;
	zval *e = NULL;
	HashTable *htt;
	char *n = {0};
	int n_len = 0;
	zend_string *n_key = NULL;
	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "clear method can't use for extension value", name);
		return;
	}

	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);
	n_key = zend_string_init(n,n_len,0);
	if ((e=zend_hash_find(htt, n_key)) != NULL) {
		zval *vl = NULL;

		if (container->use_single_property > 0) {
//			MAKE_STD_ZVAL(vl);
			if (scheme->repeated > 0) {
				array_init(vl);
			} else {
				ZVAL_NULL(vl);
			}
			php_protocolbuffers_typeconvert(scheme, vl TSRMLS_CC);
			zend_string *scheme_name_key = zend_string_init(scheme->name, scheme->name_len,0);
			zend_hash_update(htt, scheme_name_key,vl);
		} else {
			zval *garvage = e;

//			MAKE_STD_ZVAL(vl);
			if (scheme->repeated > 0) {
				array_init(vl);
			} else {
				ZVAL_NULL(vl);
			}

			e = vl;
			zval_ptr_dtor(garvage);
		}
	}
}

static void php_protocolbuffers_message_append(INTERNAL_FUNCTION_PARAMETERS, zval *instance, php_protocolbuffers_scheme_container *container, char *name, int name_len, char *name2, int name2_len, zval *value)
{
	php_protocolbuffers_scheme *scheme;
	php_protocolbuffers_message *m;
	zval *e = NULL;
	HashTable *htt;
	char *n = {0};
	int n_len = 0;
	zend_string *n_key = NULL;

	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zval_ptr_dtor(value);
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zval_ptr_dtor(value);
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "append method can't use for extension value", name);
		return;
	}

	if (scheme->repeated < 1) {
		zval_ptr_dtor(value);
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "append method can't use for non repeated value", name);
		return;
	}

	if (Z_TYPE_P(value) == IS_NULL) {
		zval_ptr_dtor(value);
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "append method can't pass null value", name);
		return;
	}

	if (scheme->ce != NULL) {
		if (scheme->ce != Z_OBJCE_P(value)) {
			zval_ptr_dtor(value);
			zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "expected %s class. given %s class", scheme->ce->name, Z_OBJCE_P(value)->name);
			return;
		}
		m = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, value);
		ZVAL_ZVAL(m->container, instance, 0, 0);
	}

	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);
	n_key = zend_string_init(n,n_len,0);
	if (container->use_single_property > 0 && zend_hash_exists(htt,n_key) == 0) {
		zval_ptr_dtor(value);
		zend_error(E_ERROR, "not initialized");
		return;
	}

	if ((e=zend_hash_find(htt, n_key)) != NULL) {
		zval *nval = NULL, *val = NULL;
		int flag = 0;

		if (Z_TYPE_P(e) != IS_ARRAY) {
//			MAKE_STD_ZVAL(nval);
			array_init(nval);
			flag = 1;
		} else {
			nval = e;
		}

//		MAKE_STD_ZVAL(val);
		ZVAL_ZVAL(val, value, 1, 0);

		Z_ADDREF_P(nval);
		Z_ADDREF_P(val);
		zend_hash_next_index_insert(Z_ARRVAL_P(nval), val);
		zend_hash_update(htt, n_key, nval);
		zval_ptr_dtor(val);

		if (flag == 1) {
			zval_ptr_dtor(nval);
		}
	}
}

static void php_protocolbuffers_message_has(INTERNAL_FUNCTION_PARAMETERS, zval *instance, php_protocolbuffers_scheme_container *container, char *name, int name_len, char *name2, int name2_len)
{
	char *n = {0};
	int n_len = 0;
	HashTable *htt = NULL;
	php_protocolbuffers_scheme *scheme;
	zval *e = NULL;
	zend_string *n_key = NULL;
	scheme = php_protocolbuffers_message_get_scheme_by_name(container, name, name_len, name2, name2_len);
	if (scheme == NULL) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not find", name);
		return;
	}

	if (scheme->is_extension) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "has method can't use for extension value", name);
		return;
	}

	php_protocolbuffers_message_get_hash_table_by_container(container, scheme, instance, &htt, &n, &n_len TSRMLS_CC);
	n_key = zend_string_init(n,n_len,0);
	if ((e=zend_hash_find(htt, n_key)) != NULL) {
		if (Z_TYPE_P(e) == IS_NULL) {
			RETURN_FALSE;
		} else if (Z_TYPE_P(e) == IS_ARRAY) {
			if (zend_hash_num_elements(Z_ARRVAL_P(e)) < 1) {
				RETURN_FALSE;
			} else {
				RETURN_TRUE;
			}
		} else {
			RETURN_TRUE;
		}
	}
}

static int php_protocolbuffers_get_unknown_zval(zval **retval, php_protocolbuffers_scheme_container *container, zval *instance TSRMLS_DC)
{
	zval *unknown_fieldset = NULL;
	HashTable *target = NULL;
	char *unknown_name = {0};
	zend_string * unknown_name_key = NULL;
	int free = 0, unknown_name_len = 0, result = 0;

	if (container->use_single_property > 0) {
		zval *unknown_property;
		zend_string *unknown_property_key = zend_string_init(container->single_property_name,container->single_property_name_len,0);
		if ((unknown_property=zend_hash_find(Z_OBJPROP_P(instance), unknown_property_key)) == NULL) {
			return result;
		}

		unknown_name     = php_protocolbuffers_get_default_unknown_property_name();
		unknown_name_len = php_protocolbuffers_get_default_unknown_property_name_len();
		target = Z_ARRVAL_P(unknown_property);
		unknown_name_key = zend_string_init(unknown_name,unknown_name_len,0);
	} else {
		unknown_name_key  = zend_mangle_property_name((char*)"*", 1, (char*)php_protocolbuffers_get_default_unknown_property_name(), php_protocolbuffers_get_default_unknown_property_name_len(), 0);
		target = Z_OBJPROP_P(instance);
		free = 1;
	}

	if ((unknown_fieldset=zend_hash_find(target, unknown_name_key)) != NULL) {
		*retval = unknown_fieldset;
		result = 1;
	}

	if (free) {
		efree(unknown_name);
	}

	return result;
}

static enum ProtocolBuffers_MagicMethod php_protocolbuffers_parse_magic_method(const char *name, size_t name_len, smart_str *buf, smart_str *buf2)
{
	int i = 0;
	int last_is_capital = 0;
	enum ProtocolBuffers_MagicMethod flag = 0;

	for (i = 0; i < name_len; i++) {
		if (flag == 0) {
			if (i+2 < name_len && name[i] == 'g' && name[i+1] == 'e' && name[i+2] == 't') {
				i += 2;
				flag = MAGICMETHOD_GET;
				continue;
			} else if (i+2 < name_len && name[i] == 's' && name[i+1] == 'e' && name[i+2] == 't') {
				i += 2;
				flag = MAGICMETHOD_SET;
				continue;
			} else if (i+7 < name_len &&
				name[i] == 'm' &&
				name[i+1] == 'u' &&
				name[i+2] == 't' &&
				name[i+3] == 'a' &&
				name[i+4] == 'b' &&
				name[i+5] == 'l' &&
				name[i+6] == 'e'
			){
				i += 7;
				flag = MAGICMETHOD_MUTABLE;
			} else if (i+6 < name_len &&
				name[i] == 'a' &&
				name[i+1] == 'p' &&
				name[i+2] == 'p' &&
				name[i+3] == 'e' &&
				name[i+4] == 'n' &&
				name[i+5] == 'd'
			) {
				i += 6;
				flag = MAGICMETHOD_APPEND;
			} else if (i+5 < name_len &&
				name[i] == 'c' &&
				name[i+1] == 'l' &&
				name[i+2] == 'e' &&
				name[i+3] == 'a' &&
				name[i+4] == 'r') {
				i += 5;
				flag = MAGICMETHOD_CLEAR;
			} else if (i+3 < name_len &&
				name[i] == 'h' &&
				name[i+1] == 'a' &&
				name[i+2] == 's'
			) {
				i += 3;
				flag = MAGICMETHOD_HAS;
			} else {
				break;
			}
		}

		if (name[i] >= 'A' && name[i] <= 'Z') {
			if (ZSTR_LEN(buf->s) > 0) {
				if ((last_is_capital == 0
					&& i+1 >= name_len)
					|| (i+1 < name_len && name[i+1] >= 'a' && name[i+1] <= 'z')) {
					smart_str_appendc(buf, '_');
				}
			}
			smart_str_appendc(buf, name[i] + ('a' - 'A'));
			smart_str_appendc(buf2, name[i]);
			last_is_capital = 1;
		} else {
			smart_str_appendc(buf, name[i]);
			smart_str_appendc(buf2, name[i]);
			last_is_capital = 0;
		}
	}

	smart_str_0(buf);
	smart_str_0(buf2);

	return flag;
}

static void php_protocolbuffers_set_from(INTERNAL_FUNCTION_PARAMETERS, zval *instance, zval *params)
{
	HashPosition pos;
	zval *param;
	php_protocolbuffers_scheme_container *container = NULL;
	char *key;
	uint key_len;
	unsigned long index= 0;
	zend_string *key_name = zend_string_init(key,key_len,0);

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME


	for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(params), &pos);
		(param=zend_hash_get_current_data_ex(Z_ARRVAL_P(params), &pos)) != NULL;
		zend_hash_move_forward_ex(Z_ARRVAL_P(params), &pos)) {

		if (zend_hash_get_current_key_ex(Z_ARRVAL_P(params), &key_name, &index, &pos) == HASH_KEY_IS_STRING) {
			php_protocolbuffers_message_set(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, key, key_len, key, key_len, param);
		}
	}
}

/* {{{ proto ProtocolBuffersMessage ProtocolBuffersMessage::__construct([array $params])
*/
PHP_METHOD(protocolbuffers_message, __construct)
{
	zval *params = NULL, *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"|a", &params) == FAILURE) {
		return;
	}

	if (php_protocolbuffers_properties_init(instance, Z_OBJCE_P(instance) TSRMLS_CC)) {
		return;
	}

	if (params != NULL) {
		php_protocolbuffers_set_from(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, params);
	}
}
/* }}} */

/* {{{ proto ProtocolBuffersMessage ProtocolBuffersMessage::setFrom(array $params)
*/
PHP_METHOD(protocolbuffers_message, setFrom)
{
	zval *params = NULL, *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"a", &params) == FAILURE) {
		return;
	}

	php_protocolbuffers_set_from(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, params);
}
/* }}} */


/* {{{ proto mixed ProtocolBuffersMessage::serializeToString()
*/
PHP_METHOD(protocolbuffers_message, serializeToString)
{
	zval *instance = getThis();

	php_protocolbuffers_encode(INTERNAL_FUNCTION_PARAM_PASSTHRU, Z_OBJCE_P(instance), instance);
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersMessage::parseFromString()
*/
PHP_METHOD(protocolbuffers_message, parseFromString)
{
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 3)
	/* I tried to lookup current scope from EG(current_execute_data). but it doesn't have current scope. we can't do anymore */
	zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "ProtocolBuffersMessage::parseFromString can't work under PHP 5.3. please use ProtocolBuffers:decode directly");
	return;
#else
	const char *data;
	int data_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &data, &data_len) == FAILURE) {
		return;
	}
	zend_class_entry *called_scope = zend_get_called_scope(execute_data);
	if (called_scope) {
		php_protocolbuffers_decode(INTERNAL_FUNCTION_PARAM_PASSTHRU, data, data_len, ZSTR_VAL(called_scope->name), ZSTR_LEN(called_scope->name));
	} else {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "Missing EG(current_scope). this is bug");
	}

#endif
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersMessage::mergeFrom($message)
*/
PHP_METHOD(protocolbuffers_message, mergeFrom)
{
	zval *object = NULL, *instance = getThis();
	php_protocolbuffers_scheme_container *container = NULL;
	char *n = {0};
	int n_len = 0;
	HashTable *htt = NULL, *hts = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"z", &object) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(object) != IS_OBJECT) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s::mergeFrom expects %s class", Z_OBJCE_P(instance)->name, Z_OBJCE_P(instance)->name);
		return;
	}

	if (Z_OBJCE_P(object) != Z_OBJCE_P(instance)) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s::mergeFrom expects %s class, but %s given", Z_OBJCE_P(instance)->name, Z_OBJCE_P(instance)->name, Z_OBJCE_P(object)->name);
		return;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME

	php_protocolbuffers_get_hash(container, container->scheme, instance, &n, &n_len, &htt TSRMLS_CC);
	php_protocolbuffers_get_hash(container, container->scheme, object, &n, &n_len, &hts TSRMLS_CC);
	php_protocolbuffers_message_merge_from(container, htt, hts TSRMLS_CC);
}
/* }}} */


/* {{{ proto mixed ProtocolBuffersMessage::current()
*/
PHP_METHOD(protocolbuffers_message, current)
{
	zval *tmp = NULL, *instance = getThis();
	php_protocolbuffers_scheme_container *container;
	php_protocolbuffers_message *message;
	const char *name;
	int name_len = 0;
	HashTable *hash;
	zend_string *name_key = NULL;
	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

	if (container->use_single_property < 1) {
		name     = container->scheme[message->offset].mangled_name;
		name_len = container->scheme[message->offset].mangled_name_len;

		hash = Z_OBJPROP_P(instance);
	} else {
		zval *c;

		name     = container->scheme[message->offset].name;
		name_len = container->scheme[message->offset].name_len;
		zend_string *single_property_name_key = zend_string_init(container->single_property_name,container->single_property_name_len+1,0);
		if ((c=zend_hash_find(Z_OBJPROP_P(instance), single_property_name_key)) != NULL) {
			hash = Z_ARRVAL_P(c);
		}

		hash = Z_OBJPROP_P(c);
	}
	name_key = zend_string_init(name,name_len,0);
	if ((tmp=zend_hash_find(hash, name_key)) != NULL) {
		RETVAL_ZVAL(tmp, 1, 0);
	}
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersMessage::key()
*/
PHP_METHOD(protocolbuffers_message, key)
{
	zval *instance = getThis();
	php_protocolbuffers_scheme_container *container;
	php_protocolbuffers_message *message;

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

	RETURN_STRING(container->scheme[message->offset].name);
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersMessage::next()
*/
PHP_METHOD(protocolbuffers_message, next)
{
	zval *instance = getThis();
	php_protocolbuffers_scheme_container *container;
	php_protocolbuffers_message *message;

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);
	message->offset++;
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::rewind()
*/
PHP_METHOD(protocolbuffers_message, rewind)
{
	zval *instance = getThis();
	php_protocolbuffers_scheme_container *container;
	php_protocolbuffers_message *message;

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

	if (message->max == 0) {
		message->max = container->size;
	}
	message->offset = 0;
}
/* }}} */

/* {{{ proto bool ProtocolBuffersMessage::valid()
*/
PHP_METHOD(protocolbuffers_message, valid)
{
	zval *instance = getThis();
	php_protocolbuffers_scheme_container *container;
	php_protocolbuffers_message *message;

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	message = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

	if (-1 < message->offset && message->offset < message->max) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::clear(string $name)
*/
PHP_METHOD(protocolbuffers_message, clear)
{
	zval *instance = getThis();
	char *name = {0};
	int name_len = 0;
	php_protocolbuffers_scheme_container *container;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_clear(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, name, name_len);
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::clearAll(bool $clear_unknown_fields = true)
*/
PHP_METHOD(protocolbuffers_message, clearAll)
{
	zval *instance = getThis();
	php_protocolbuffers_scheme_container *container;
	zend_bool clear_unknown_fields = 1;
	int i = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"|b", &clear_unknown_fields) == FAILURE) {
		return;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME

	for (i = 0; i < container->size; i++) {
		php_protocolbuffers_message_clear(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, container->scheme[i].name, container->scheme[i].name_len, NULL, 0);
	}

	if (clear_unknown_fields > 0 && container->process_unknown_fields > 0) {
		zval *unknown_fieldset = NULL;

		if (php_protocolbuffers_get_unknown_zval(&unknown_fieldset, container, instance TSRMLS_CC)) {
			php_protocolbuffers_unknown_field_clear(INTERNAL_FUNCTION_PARAM_PASSTHRU, unknown_fieldset);
		}
	}
}
/* }}} */

/* {{{ proto bool ProtocolBuffersMessage::__call()
*/
PHP_METHOD(protocolbuffers_message, __call)
{
	zval *params = NULL, *instance = getThis();
	php_protocolbuffers_scheme_container *container;
	char *name = {0};
	int flag = 0, name_len = 0;
	smart_str buf = {0};
	smart_str buf2 = {0};

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"sz", &name, &name_len, &params) == FAILURE) {
		return;
	}

	flag = php_protocolbuffers_parse_magic_method(name, name_len, &buf, &buf2);
	if (flag == 0) {
		zend_error(E_ERROR, "Call to undefined method %s::%s()", Z_OBJCE_P(instance)->name, name);
		return;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	switch (flag) {
		case MAGICMETHOD_GET:
		{
			zval *tmp = NULL;
			if (params != NULL && Z_TYPE_P(params) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(params)) > 0) {
				tmp = zend_hash_get_current_data(Z_ARRVAL_P(params));
				php_protocolbuffers_message_get(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), ZSTR_VAL(buf2.s), ZSTR_LEN(buf2.s), tmp);
			} else {
				php_protocolbuffers_message_get(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), ZSTR_VAL(buf2.s), ZSTR_LEN(buf2.s), NULL);
			}
		}
		break;
		case MAGICMETHOD_MUTABLE:
		{
			php_protocolbuffers_message_get(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), ZSTR_VAL(buf2.s), ZSTR_LEN(buf2.s), NULL);
			php_protocolbuffers_message_set(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), ZSTR_VAL(buf2.s), ZSTR_LEN(buf2.s), return_value);
		}
		break;
		case MAGICMETHOD_SET:
		{
			zval *tmp = NULL;

			tmp=zend_hash_get_current_data(Z_ARRVAL_P(params));
			php_protocolbuffers_message_set(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), ZSTR_VAL(buf2.s), ZSTR_LEN(buf2.s), tmp);
		}
		break;
		case MAGICMETHOD_APPEND:
		{
			zval *tmp = NULL;

			tmp = zend_hash_get_current_data(Z_ARRVAL_P(params));
			php_protocolbuffers_message_append(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), ZSTR_VAL(buf2.s), ZSTR_LEN(buf2.s), tmp);
		}
		break;
		case MAGICMETHOD_CLEAR:
			php_protocolbuffers_message_clear(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), ZSTR_VAL(buf2.s), ZSTR_LEN(buf2.s));
		case MAGICMETHOD_HAS:
			php_protocolbuffers_message_has(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s), ZSTR_VAL(buf2.s), ZSTR_LEN(buf2.s));
		break;
	}

	smart_str_free(&buf);
	smart_str_free(&buf2);
}
/* }}} */

/* {{{ proto ProtocolBuffers_FieldDescriptor ProtocolBuffersMessage::has($key)
*/
PHP_METHOD(protocolbuffers_message, has)
{
	zval *instance = getThis();
	char *name = {0};
	int name_len = 0;
	php_protocolbuffers_scheme_container *container;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_has(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, name, name_len);
}
/* }}} */

/* {{{ proto ProtocolBuffers_FieldDescriptor ProtocolBuffersMessage::get($key)
*/
PHP_METHOD(protocolbuffers_message, get)
{
	zval *instance = getThis();
	char *name = {0};
	int name_len = 0;
	php_protocolbuffers_scheme_container *container;
	zval *params = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s|z", &name, &name_len, &params) == FAILURE) {
		return;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_get(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, name, name_len, params);
}
/* }}} */

/* {{{ proto ProtocolBuffersMessage ProtocolBuffersMessage::mutable($key)
*/
PHP_METHOD(protocolbuffers_message, mutable)
{
	zval *instance = getThis();
	char *name = {0};
	int name_len = 0;
	php_protocolbuffers_scheme_container *container;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_get(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, name, name_len, NULL);
	Z_ADDREF_P(return_value);
	php_protocolbuffers_message_set(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, name, name_len, return_value);
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::set($name, $value)
*/
PHP_METHOD(protocolbuffers_message, set)
{
	zval *value = NULL, *instance = getThis();
	char *name = {0};
	int name_len = 0;
	php_protocolbuffers_scheme_container *container;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"sz", &name, &name_len, &value) == FAILURE) {
		return;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_set(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, name, name_len, value);
}


/* {{{ proto void ProtocolBuffersMessage::append($name, $value)
*/
PHP_METHOD(protocolbuffers_message, append)
{
	zval *value = NULL, *instance = getThis();
	char *name = {0};
	int name_len = 0;
	php_protocolbuffers_scheme_container *container;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"sz", &name, &name_len, &value) == FAILURE) {
		return;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	php_protocolbuffers_message_append(INTERNAL_FUNCTION_PARAM_PASSTHRU, instance, container, name, name_len, name, name_len, value);
}



/* {{{ proto mixed ProtocolBuffersMessage::getExtension(string $name)
*/
PHP_METHOD(protocolbuffers_message, getExtension)
{
	zval *instance = getThis();
	zval *registry = php_protocolbuffers_extension_registry_get_instance(TSRMLS_C);
	zval *e = {0}, *b = {0}, *field_descriptor = NULL, *extension_registry = NULL;
	zend_class_entry *ce;
	HashTable *htt = NULL;
	char *name = {0}, *n = {0};
	int name_len = 0, n_len = 0;
	zend_string *n_key = NULL;
	php_protocolbuffers_scheme_container *container;
	int is_mangled = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	ce = Z_OBJCE_P(instance);
	if (!php_protocolbuffers_extension_registry_get_registry(registry, ZSTR_VAL(ce->name), ZSTR_LEN(ce->name), &extension_registry TSRMLS_CC)) {
		goto err;
	}

	if (!php_protocolbuffers_extension_registry_get_descriptor_by_name(extension_registry, name, name_len, &field_descriptor TSRMLS_CC)) {
		goto err;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	if (container->use_single_property > 0) {
		zend_string * single_property_name_key = zend_string_init(container->single_property_name, container->single_property_name_len,0);
		if ((b=zend_hash_find(Z_OBJPROP_P(instance), single_property_name_key)) == NULL) {
			return;
		}

		n = name;
		n_len = name_len;
		htt = Z_ARRVAL_P(b);
		n_key = zend_string_init(n,n_len,0);
	} else {
		htt = Z_OBJPROP_P(instance);
		n_key = zend_mangle_property_name((char*)"*", 1, (char*)name, name_len+1, 0);
		is_mangled = 1;
	}

	if ((e=zend_hash_find(htt, n_key)) != NULL) {
		if (is_mangled) {
			efree(n);
		}

		if (Z_TYPE_P(e) == IS_NULL) {
			int x = 0;
			for (x = 0; x < container->size; x++) {
				php_protocolbuffers_scheme *scheme;

				scheme = &container->scheme[x];
				if (scheme->ce != NULL && strcmp(scheme->name, name) == 0) {
					zval *tmp;
//					MAKE_STD_ZVAL(tmp);
					object_init_ex(tmp, scheme->ce);
					php_protocolbuffers_properties_init(tmp, scheme->ce TSRMLS_CC);

					RETURN_ZVAL(tmp, 0, 1);
					break;
				}
			}
		}

		RETURN_ZVAL(e, 1, 0);
	}

	return;
err:
	zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "extension %s does not find", name);
}
/* }}} */

/* {{{ proto bool ProtocolBuffersMessage::hasExtension(string $name)
*/
PHP_METHOD(protocolbuffers_message, hasExtension)
{
	zval *instance = getThis();
	zval *registry = php_protocolbuffers_extension_registry_get_instance(TSRMLS_C);
	zval *e = NULL, *b = NULL, *field_descriptor = NULL, *extension_registry = NULL;
	zend_class_entry *ce;
	HashTable *htt = NULL;
	char *name = {0}, *n = {0};
	int name_len = 0, n_len = 0;
	zend_string *n_key = NULL;
	php_protocolbuffers_scheme_container *container;
	int is_mangled = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	ce = Z_OBJCE_P(instance);

	if (!php_protocolbuffers_extension_registry_get_registry(registry, ZSTR_VAL(ce->name), ZSTR_LEN(ce->name), &extension_registry TSRMLS_CC)) {
		goto err;
	}

	if (!php_protocolbuffers_extension_registry_get_descriptor_by_name(extension_registry, name, name_len, &field_descriptor TSRMLS_CC)) {
		goto err;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	if (container->use_single_property > 0) {
		zend_string *single_property_name_key = zend_string_init(container->single_property_name, container->single_property_name_len,0);
		if ((b=zend_hash_find(Z_OBJPROP_P(instance),single_property_name_key)) == NULL) {
			return;
		}

		n = name;
		n_len = name_len;
		htt = Z_ARRVAL_P(b);
		n_key = zend_string_init(n,n_len,0);
	} else {
		htt = Z_OBJPROP_P(instance);
		n_key=zend_mangle_property_name((char*)"*", 1, (char*)name, name_len+1, 0);
		is_mangled = 1;
	}

	if ((e=zend_hash_find(htt, n_key)) != NULL) {
		if (is_mangled) {
			efree(n);
		}
		if (Z_TYPE_P(e) == IS_NULL) {
			RETURN_FALSE;
		} else {
			RETURN_TRUE;
		}
	} else {
		RETURN_FALSE;
	}
err:
	zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "extension %s does not find", name);
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::setExtension(string $name, mixed $value)
*/
PHP_METHOD(protocolbuffers_message, setExtension)
{
	zval *instance = getThis();
	zval *registry = php_protocolbuffers_extension_registry_get_instance(TSRMLS_C);
	zval *field_descriptor = NULL, *extension_registry = NULL, *value = NULL, *b = NULL;
	zend_class_entry *ce;
	HashTable *htt = NULL;
	char *name = {0}, *n = {0};
	int name_len = 0, n_len = 0;
	zend_string *n_key=NULL;
	php_protocolbuffers_scheme_container *container;
	int is_mangled = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"sz", &name, &name_len, &value) == FAILURE) {
		return;
	}

	ce = Z_OBJCE_P(instance);

	if (!php_protocolbuffers_extension_registry_get_registry(registry, ZSTR_VAL(ce->name), ZSTR_LEN(ce->name), &extension_registry TSRMLS_CC)) {
		goto err;
	}

	if (!php_protocolbuffers_extension_registry_get_descriptor_by_name(extension_registry, name, name_len, &field_descriptor TSRMLS_CC)) {
		goto err;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	if (container->use_single_property > 0) {
		zend_string *single_property_name = zend_string_init(container->single_property_name, container->single_property_name_len+1,0);
		if ((b=zend_hash_find(Z_OBJPROP_P(instance), single_property_name)) == NULL) {
			return;
		}

		n = name;
		n_len = name_len+1;
		htt = Z_ARRVAL_P(b);
		n_key = zend_string_init(n,n_len,0);
	} else {
		htt = Z_OBJPROP_P(instance);
		n_key = zend_mangle_property_name((char*)"*", 1, (char*)name, name_len+1, 0);
		is_mangled = 1;
	}

	Z_ADDREF_P(value);
	zend_hash_update(htt, n_key, value);
	if (is_mangled) {
		efree(n);
	}

	return;
err:
	zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "extension %s does not find", name);
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::setExtension(string $name)
*/
PHP_METHOD(protocolbuffers_message, clearExtension)
{
	zval *instance = getThis();
	zval *registry = php_protocolbuffers_extension_registry_get_instance(TSRMLS_C);
	zval *e = {0}, *b = {0}, *field_descriptor = NULL, *extension_registry = NULL;
	zend_class_entry *ce;
	HashTable *htt = NULL;
	char *name = {0}, *n = {0};
	int name_len = 0, n_len = 0;
	zend_string *n_key=NULL;
	php_protocolbuffers_scheme_container *container;
	int is_mangled = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"s", &name, &name_len) == FAILURE) {
		return;
	}

	ce = Z_OBJCE_P(instance);

	if (!php_protocolbuffers_extension_registry_get_registry(registry, ZSTR_VAL(ce->name), ZSTR_LEN(ce->name), &extension_registry TSRMLS_CC)) {
		goto err;
	}

	if (!php_protocolbuffers_extension_registry_get_descriptor_by_name(extension_registry, name, name_len, &field_descriptor TSRMLS_CC)) {
		goto err;
	}

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	if (container->use_single_property > 0) {
		zend_string *single_property_name = zend_string_init(container->single_property_name, container->single_property_name_len,0);
		if ((b=zend_hash_find(Z_OBJPROP_P(instance), single_property_name)) == NULL) {
			return;
		}

		n = name;
		n_len = name_len;
		htt = Z_ARRVAL_P(b);
		n_key = zend_string_init(n,n_len,0);
	} else {
		htt = Z_OBJPROP_P(instance);
		n_key=zend_mangle_property_name((char*)"*", 1, (char*)name, name_len+1, 0);
		is_mangled = 1;
	}

	if ((e=zend_hash_find(htt, n_key)) != NULL) {
		zval *tmp;
		if (is_mangled) {
			efree(n);
		}
		zval_ptr_dtor(e);
//		MAKE_STD_ZVAL(tmp);
		ZVAL_NULL(tmp);
		e = tmp;
		RETURN_NULL();
	}

	return;
err:
	zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "extension %s does not find", name);
}
/* }}} */

/* {{{ proto void ProtocolBuffersMessage::discardUnknownFields()
*/
PHP_METHOD(protocolbuffers_message, discardUnknownFields)
{
	zval *unknown_fieldset, *instance = getThis();
	php_protocolbuffers_scheme_container *container;

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME
	if (container->process_unknown_fields > 0) {
		if (php_protocolbuffers_get_unknown_zval(&unknown_fieldset, container, instance TSRMLS_CC)) {
			php_protocolbuffers_unknown_field_clear(INTERNAL_FUNCTION_PARAM_PASSTHRU, unknown_fieldset);
		}
	}
}
/* }}} */

/* {{{ proto ProtocolBuffersUnknownFieldSet ProtocolBuffersMessage::getUnknownFieldSet()
*/
PHP_METHOD(protocolbuffers_message, getUnknownFieldSet)
{
	zval *unknown_fieldset = NULL, *instance = getThis();
	php_protocolbuffers_scheme_container *container;

	PHP_PROTOCOLBUFFERS_MESSAGE_CHECK_SCHEME

	if (container->process_unknown_fields < 1) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "process unknown fields flag seems false");
		return;
	}

	if (php_protocolbuffers_get_unknown_zval(&unknown_fieldset, container, instance TSRMLS_CC)) {
		RETVAL_ZVAL(unknown_fieldset, 1, 0);
	} else {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "unknown field property does not find");
	}
}
/* }}} */


/* {{{ proto ProtocolBuffersMessage ProtocolBuffersMessage::containerOf()
*/
PHP_METHOD(protocolbuffers_message, containerOf)
{
	zval *instance = getThis();
	php_protocolbuffers_message *m;

	m = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, instance);

	if (m->container != NULL) {
		RETURN_ZVAL(m->container, 1, 0);
	}
}
/* }}} */

/* {{{ proto array ProtocolBuffersMessage::jsonSerialize()
*/
PHP_METHOD(protocolbuffers_message, jsonSerialize)
{
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4)
	zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "JsonSerializable does not support on this version");
#else
	zval *instance = getThis(), *result = NULL;
	zend_class_entry *json;

	if (json_serializable_checked == 0) {
		/* checks JsonSerializable class (for json dynamic module)*/
		zend_string *JsonSerializable_key = zend_string_init("JsonSerializable", sizeof("JsonSerializable")-1,0);
		if ((json=zend_lookup_class(JsonSerializable_key)) != NULL) {
			if (!instanceof_function(php_protocol_buffers_message_class_entry, json TSRMLS_CC)) {
				zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "JsonSerializable does not support on this version (probably json module doesn't load)");
				return;
			}
		}
		json_serializable_checked = 1;
	}

	if (php_protocolbuffers_jsonserialize(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0, Z_OBJCE_P(instance), instance, &result) == 0) {
		RETURN_ZVAL(result, 0, 1);
	}
	return;
#endif
}
/* }}} */

/* {{{ proto array ProtocolBuffersMessage::toArray()
*/
PHP_METHOD(protocolbuffers_message, toArray)
{
	zval *instance = getThis(), *result = NULL;

	if (php_protocolbuffers_jsonserialize(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1, Z_OBJCE_P(instance), instance, &result) == 0) {
		RETURN_ZVAL(result, 0, 1);
	}
	return;
}
/* }}} */

static zend_function_entry php_protocolbuffers_message_methods[] = {
	PHP_ME(protocolbuffers_message, __construct,          arginfo_protocolbuffers_message___construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(protocolbuffers_message, serializeToString,    arginfo_protocolbuffers_message_serialize_to_string, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, parseFromString,      arginfo_protocolbuffers_message_parse_from_string, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(protocolbuffers_message, mergeFrom,            arginfo_protocolbuffers_message_merge_from, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, clear,                arginfo_protocolbuffers_message_clear, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, clearAll,             NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, has,                  arginfo_protocolbuffers_message_has, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, get,                  arginfo_protocolbuffers_message_get, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, mutable,              arginfo_protocolbuffers_message_mutable, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, set,                  arginfo_protocolbuffers_message_set, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, setFrom,              arginfo_protocolbuffers_message_set_from, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, append,               arginfo_protocolbuffers_message_append, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, hasExtension,         arginfo_protocolbuffers_message_has_extension, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, getExtension,         arginfo_protocolbuffers_message_get_extension, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, setExtension,         arginfo_protocolbuffers_message_set_extension, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, clearExtension,       arginfo_protocolbuffers_message_clear_extension, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, discardUnknownFields, arginfo_protocolbuffers_message_discard_unknown_fields, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, getUnknownFieldSet,   NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, containerOf,          NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, jsonSerialize,        NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, toArray,        NULL, ZEND_ACC_PUBLIC)
		/* iterator */
	PHP_ME(protocolbuffers_message, current,   NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, key,       NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, next,      NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, rewind,    NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_message, valid,     NULL, ZEND_ACC_PUBLIC)
	/* magic method */
	PHP_ME(protocolbuffers_message, __call,   arginfo_protocolbuffers_message___call, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

void php_protocolbuffers_message_class(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "ProtocolBuffersMessage", php_protocolbuffers_message_methods);
	php_protocol_buffers_message_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	zend_class_implements(php_protocol_buffers_message_class_entry TSRMLS_CC, 1, zend_ce_iterator);
	zend_class_implements(php_protocol_buffers_message_class_entry TSRMLS_CC, 1, php_protocol_buffers_serializable_class_entry);

// Note(chobie): Don't implement JsonSerializable here as some environment uses json shared module.
//#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION == 4) && (PHP_RELEASE_VERSION >= 26)) || ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION == 5) && (PHP_RELEASE_VERSION >= 10))
//	zend_class_implements(php_protocol_buffers_message_class_entry TSRMLS_CC, 1, php_json_serializable_ce);
//#endif

	php_protocol_buffers_message_class_entry->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	php_protocol_buffers_message_class_entry->create_object = php_protocolbuffers_message_new;

	memcpy(&php_protocolbuffers_message_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	PHP_PROTOCOLBUFFERS_REGISTER_NS_CLASS_ALIAS(PHP_PROTOCOLBUFFERS_NAMESPACE, "Message", php_protocol_buffers_message_class_entry);
}
