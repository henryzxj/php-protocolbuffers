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
#include "field_descriptor.h"

static int php_protocolbuffers_field_descriptor_process_params(zval **zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key)
{
#if PHP_VERSION_ID < 50300
	TSRMLS_FETCH();
#endif

	if (ZSTR_LEN(hash_key->key) == 0) {
		return 0;
	} else {
		const char *key = ZSTR_VAL(hash_key->key);
		size_t key_length = ZSTR_LEN(hash_key->key);
		zval *value = NULL, *instance = NULL;
		char *name;
		int name_length;
		zend_string * name_key=NULL;

		instance = va_arg(args, zval*);

#define PHP_PROTOCOLBUFFERS_PROCESS_BOOL \
{\
	name_key =zend_mangle_property_name((char*)"*", 1, (char*)key, key_length, 0);\
	\
	if (Z_TYPE_P(*zv) != IS_TRUE&&Z_TYPE_P(*zv) != IS_FALSE) {\
		convert_to_boolean(*zv);\
	}\
	\
	ZVAL_BOOL(value, Z_BVAL_P(*zv));\
	zend_hash_update(Z_OBJPROP_P(instance), name_key, value);\
	efree(name);\
}

		if (strcmp(key, "type") == 0) {
			if (Z_TYPE_P(*zv) != IS_LONG) {
				convert_to_long(*zv);
			}

			if (Z_LVAL_P(*zv) > MAX_FIELD_TYPE) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "ProtocolBuffersFieldDescriptor: type shoud be in %d - %d", 1, MAX_FIELD_TYPE);
				return 1;
			}
			if (Z_LVAL_P(*zv) < 1) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "ProtocolBuffersFieldDescriptor: type shoud be in %d - %d", 1, MAX_FIELD_TYPE);
				return 1;
			}
			if (Z_LVAL_P(*zv) == TYPE_GROUP) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "ProtocolBuffersFieldDescriptor: type shoud be in %d - %d. group type does not supported", 1, MAX_FIELD_TYPE);
				return 1;
			}

			name_key = zend_mangle_property_name((char*)"*", 1, (char*)key, key_length, 0);
//			MAKE_STD_ZVAL(value);
			ZVAL_LONG(value, Z_LVAL_P(*zv));
			zend_hash_update(Z_OBJPROP_P(instance), name_key, value);
			efree(name);

		} else if (strcmp(key, "required") == 0) {
			PHP_PROTOCOLBUFFERS_PROCESS_BOOL;
		} else if (strcmp(key, "optional") == 0) {
			PHP_PROTOCOLBUFFERS_PROCESS_BOOL;
		} else if (strcmp(key, "repeated") == 0) {
			PHP_PROTOCOLBUFFERS_PROCESS_BOOL;
		} else if (strcmp(key, "packable") == 0) {
			PHP_PROTOCOLBUFFERS_PROCESS_BOOL;
		} else if (strcmp(key, "extension") == 0) {
			PHP_PROTOCOLBUFFERS_PROCESS_BOOL;
		} else if (strcmp(key, "message") == 0 && Z_TYPE_P(*zv) == IS_STRING) {
			name_key = zend_mangle_property_name((char*)"*", 1, (char*)key, key_length, 0);

//			MAKE_STD_ZVAL(value);
			ZVAL_STRING(value, Z_STRVAL_P(*zv));
			zend_hash_update(Z_OBJPROP_P(instance), name_key, value);
			efree(name);
		} else if (strcmp(key, "name") == 0) {
			name_key=zend_mangle_property_name((char*)"*", 1, (char*)key, key_length, 0);

//			MAKE_STD_ZVAL(value);
			ZVAL_STRING(value, Z_STRVAL_P(*zv));
			zend_hash_update(Z_OBJPROP_P(instance), name_key, value);
			efree(name);
		} else if (strcmp(key, "default") == 0) {
			name_key=zend_mangle_property_name( (char*)"*", 1, (char*)key, key_length, 0);

//			MAKE_STD_ZVAL(value);
			ZVAL_ZVAL(value, *zv, 1, 0);
			zend_hash_update(Z_OBJPROP_P(instance), name_key, value);
			efree(name);
		} else {
		}
	}

#undef PHP_PROTOCOLBUFFERS_PROCESS_BOOL

	return 0;
}

int php_protocolbuffers_field_descriptor_get_name(zval *instance, char **retval, int *len TSRMLS_DC)
{
	zval *result = NULL;

	if (php_protocolbuffers_read_protected_property(instance, ZEND_STRS("name"), &result TSRMLS_CC)) {
		*retval = Z_STRVAL_P(result);
		*len    = Z_STRLEN_P(result)+1;
		return 1;
	} else {
		return 0;
	}

	 return 0;
}

static void php_protocolbuffers_field_descriptor_get_property(INTERNAL_FUNCTION_PARAMETERS, char *property, size_t property_length, int retval_type)
{
	zval *result = NULL, *instance = getThis();

	if (php_protocolbuffers_read_protected_property(instance, property, property_length, &result TSRMLS_CC)) {
		if (retval_type == IS_STRING) {
			RETVAL_STRING(Z_STRVAL_P(result));
		} else if (retval_type == IS_LONG) {
			RETVAL_LONG(Z_LVAL_P(result));
		} else if (retval_type == IS_TRUE||retval_type == IS_FALSE) {
			RETVAL_BOOL(Z_BVAL_P(result));
		} else {
			RETVAL_ZVAL(result, 1, 0);
		}
	} else {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "%s does not have values", property);
	}
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_field_descriptor___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_field_descriptor_get_type, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_field_descriptor_set_type, 0, 0, 1)
	ZEND_ARG_INFO(0, wiretype)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_field_descriptor_get_default, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_field_descriptor_set_default, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

/* {{{ proto ProtocolBuffers_FieldDescriptor ProtocolBuffersFieldDescriptor::__construct(array $params)
*/
PHP_METHOD(protocolbuffers_field_descriptor, __construct)
{
	zval *params = NULL, *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"|a", &params) == FAILURE) {
		return;
	}

	if (params) {
#if PHP_VERSION_ID < 50300
		zend_hash_apply_with_arguments(Z_ARRVAL_P(params), (apply_func_args_t)php_protocolbuffers_field_descriptor_process_params, 1, &instance);
#else
		zend_hash_apply_with_arguments(Z_ARRVAL_P(params) TSRMLS_CC, (apply_func_args_t)php_protocolbuffers_field_descriptor_process_params, 1, &instance);
#endif
		if (EG(exception)) {
			return;
		}
	}
}
/* }}} */

/* {{{ proto long ProtocolBuffersFieldDescriptor::getType()
*/
PHP_METHOD(protocolbuffers_field_descriptor, getType)
{
	php_protocolbuffers_field_descriptor_get_property(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRS("type"), IS_LONG);
}
/* }}} */

/* {{{ proto mixed ProtocolBuffersFieldDescriptor::getDefault()
*/
PHP_METHOD(protocolbuffers_field_descriptor, getDefault)
{
	php_protocolbuffers_field_descriptor_get_property(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRS("default"), 0);
}
/* }}} */

/* {{{ proto string ProtocolBuffersFieldDescriptor::getName()
*/
PHP_METHOD(protocolbuffers_field_descriptor, getName)
{
	php_protocolbuffers_field_descriptor_get_property(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRS("name"), IS_STRING);
}
/* }}} */

/* {{{ proto long ProtocolBuffersFieldDescriptor::isExtension()
*/
PHP_METHOD(protocolbuffers_field_descriptor, isExtension)
{
	php_protocolbuffers_field_descriptor_get_property(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRS("extension"), IS_TRUE);
}
/* }}} */

static zend_function_entry php_protocolbuffers_field_descriptor_methods[] = {
	PHP_ME(protocolbuffers_field_descriptor, __construct,  arginfo_protocolbuffers_field_descriptor___construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(protocolbuffers_field_descriptor, getName, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_field_descriptor, getType,  arginfo_protocolbuffers_field_descriptor_get_type, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_field_descriptor, getDefault, arginfo_protocolbuffers_field_descriptor_get_default, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_field_descriptor, isExtension, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

void php_protocolbuffers_filed_descriptor_class(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "ProtocolBuffersFieldDescriptor", php_protocolbuffers_field_descriptor_methods);
	php_protocol_buffers_field_descriptor_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	PHP_PROTOCOLBUFFERS_REGISTER_NS_CLASS_ALIAS(PHP_PROTOCOLBUFFERS_NAMESPACE, "FieldDescriptor", php_protocol_buffers_field_descriptor_class_entry);
}
