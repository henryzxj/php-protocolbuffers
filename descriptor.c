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
#include "descriptor.h"

static zend_object_handlers php_protocolbuffers_descriptor_object_handlers;

static const char *fields_map[] = {
	"DUMMY",
	"TYPE_DOUBLE",
	"TYPE_FLOAT",
	"TYPE_INT64",
	"TYPE_UINT64",
	"TYPE_INT32",
	"TYPE_FIXED64",
	"TYPE_FIXED32",
	"TYPE_BOOL",
	"TYPE_STRING",
	"TYPE_GROUP",
	"TYPE_MESSAGE",
	"TYPE_BYTES",
	"TYPE_UINT32",
	"TYPE_ENUM",
	"TYPE_SFIXED32",
	"TYPE_SFIXED64",
	"TYPE_SINT32",
	"TYPE_SINT64",
};

int php_protocolbuffers_descriptor_properties_init(zval *object TSRMLS_DC)
{
	zval pp;
	array_init(&pp);
	zend_update_property(Z_OBJCE_P(object),object,ZEND_STRL("fields"),&pp);
	zval_ptr_dtor(&pp);
	return 0;
}


static const char* field_type_to_str(int field_type)
{
	if (field_type > 0 && field_type <= MAX_FIELD_TYPE) {
		return fields_map[field_type];
	} else {
		return NULL;
	}
}

static php_protocolbuffers_descriptor *php_protocolbuffers_fetch_object(zend_object *object) {
	return (php_protocolbuffers_descriptor *)((char*)(object) - XtOffsetOf(php_protocolbuffers_descriptor, zo));
}

static void php_protocolbuffers_descriptor_free_storage(zend_object *object TSRMLS_DC)
{
	php_protocolbuffers_descriptor * intern = php_protocolbuffers_fetch_object(object);

	if (intern->name_len > 0) {
		efree(intern->name);
	}

	if (intern->container != NULL && intern->free_container <= 1) {
		int i;

		for (i = 0; i < (intern->container)->size; i++) {
			if ((intern->container)->scheme[i].original_name_key != NULL) {
//				efree((intern->container)->scheme[i].original_name);
				zend_string_release((intern->container)->scheme[i].original_name_key);
			}
			if ((intern->container)->scheme[i].name_key != NULL) {
//				efree((intern->container)->scheme[i].name);
				zend_string_release((intern->container)->scheme[i].name_key);
			}
			if ((intern->container)->scheme[i].mangled_name_key != NULL) {
//				efree((intern->container)->scheme[i].mangled_name);
				zend_string_release((intern->container)->scheme[i].mangled_name_key);
			}
			if ((intern->container)->scheme[i].default_value != NULL) {
				zval_ptr_dtor((intern->container)->scheme[i].default_value);
			}
		}

		if (intern->container->single_property_name != NULL) {
			efree(intern->container->single_property_name);
		}

		if (intern->container->scheme != NULL) {
			efree(intern->container->scheme);
		}

		if (intern->container->extensions != NULL) {
			efree(intern->container->extensions);
		}

		efree(intern->container);
	}
	zend_object_std_dtor(&intern->zo);
}

zend_object *php_protocolbuffers_descriptor_new(zend_class_entry *ce TSRMLS_DC)
{

	PHP_PROTOCOLBUFFERS_STD_CREATE_OBJECT(php_protocolbuffers_descriptor);

	intern->name = NULL;
	intern->name_len = 0;
	intern->free_container = 0;
	intern->container = (php_protocolbuffers_scheme_container*)emalloc(sizeof(php_protocolbuffers_scheme_container));
	php_protocolbuffers_scheme_container_init(intern->container);
	return &intern->zo;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_get_name, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_get_field, 0, 0, 1)
	ZEND_ARG_INFO(0, tag)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_get_fields, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_descriptor_dump, 0, 0, 0)
ZEND_END_ARG_INFO()


/* {{{ proto ProtocolBuffersDescriptor ProtocolBuffersDescriptor::__construct()
*/
PHP_METHOD(protocolbuffers_descriptor, __construct)
{
	/* do not create instance by user. we expect it always created from builder. */
}
/* }}} */

/* {{{ proto string ProtocolBuffersDescriptor::getName()
*/
PHP_METHOD(protocolbuffers_descriptor, getName)
{
	zval *instance = getThis();
	php_protocolbuffers_descriptor *descriptor;

	descriptor = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_descriptor, instance);

	RETURN_STRINGL(descriptor->name, descriptor->name_len);
}
/* }}} */

/* {{{ proto ProtocolBuffersFieldDescriptor ProtocolBuffersDescriptor::getField(int $tag)
*/
PHP_METHOD(protocolbuffers_descriptor, getField)
{
	zval *instance = getThis();
	zval *result;
	long tag = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"l", &tag) == FAILURE) {
		return;
	}

	if (php_protocolbuffers_read_protected_property(instance, ZEND_STRS("fields"), &result TSRMLS_CC)) {
		zval *entry;
		if ((entry=zend_hash_index_find(Z_ARRVAL_P(result), tag)) != NULL) {
			RETVAL_ZVAL(entry, 0, 1);
		}
	}
}
/* }}} */

/* {{{ proto ProtocolBuffersFieldDescriptor ProtocolBuffersDescriptor::getFields()
*/
PHP_METHOD(protocolbuffers_descriptor, getFields)
{
	zval *instance = getThis();
	zval *result;

	if (php_protocolbuffers_read_protected_property(instance, ZEND_STRS("fields"), &result TSRMLS_CC)) {
		RETVAL_ZVAL(result, 0, 1);
	}
}
/* }}} */

/* {{{ proto void ProtocolBuffersDescriptor::dump()
*/
PHP_METHOD(protocolbuffers_descriptor, dump)
{
	zval *instance = getThis();
	php_protocolbuffers_descriptor *descriptor;
	int n = 0;
	php_protocolbuffers_scheme *ischeme;


	descriptor = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_descriptor, instance);

	php_printf("{\n");
	if (descriptor->name_len > 0) {
		php_printf("  \"name\": \"%s\",\n", descriptor->name);
	} else {
		php_printf("  \"name\": \"\",\n");
	}

	php_printf("  \"fields\": {\n");
	if (descriptor->container->size > 0) {
		for (n = 0; n < descriptor->container->size; n++) {
			ischeme = &(descriptor->container->scheme[n]);
			php_printf("    \"%d\": {\n", ischeme->tag);
			php_printf("      type: \"%s\",\n", field_type_to_str(ischeme->type));
			php_printf("      name: \"%s\",\n", ZSTR_VAL(ischeme->name_key));
			if (ischeme->type == TYPE_MESSAGE && ischeme->ce != NULL) {
				php_printf("      message: \"%s\",\n", ischeme->ce->name);
			}
			php_printf("      repeated: %s,\n", (ischeme->repeated == 1) ? "true" : "false");
			php_printf("      packed: %s\n", (ischeme->packed == 1) ? "true" : "false");
			php_printf("    }");

			if (n+1 < descriptor->container->size) {
				php_printf(",\n");
			} else {
				php_printf("\n");
			}

		}
	}
	php_printf("  }\n");

	php_printf("  \"extension_ranges\": {\n");
	if (descriptor->container->extension_cnt > 0) {
		php_protocolbuffers_extension_range *range;
		for (n = 0; n < descriptor->container->extension_cnt; n++) {
			range = &(descriptor->container->extensions[n]);

			php_printf("    \"%d\": {\n", n);
			php_printf("      begin: %d,\n", range->begin);
			php_printf("      end: %d,\n", range->end);
			php_printf("    }");

			if (n+1 < descriptor->container->extension_cnt) {
				php_printf(",\n");
			} else {
				php_printf("\n");
			}
		}
	}
	php_printf("  }\n");

	php_printf("}\n");
}
/* }}} */

static zend_function_entry php_protocolbuffers_descriptor_methods[] = {
	PHP_ME(protocolbuffers_descriptor, __construct, arginfo_protocolbuffers_descriptor___construct, ZEND_ACC_PRIVATE)
	PHP_ME(protocolbuffers_descriptor, getName,     arginfo_protocolbuffers_descriptor_get_name,    ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_descriptor, getField,    arginfo_protocolbuffers_descriptor_get_field,   ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_descriptor, getFields,   arginfo_protocolbuffers_descriptor_get_fields,  ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_descriptor, dump,        arginfo_protocolbuffers_descriptor_dump,        ZEND_ACC_PUBLIC)
	PHP_FE_END
};

void php_protocolbuffers_descriptor_class(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "ProtocolBuffersDescriptor", php_protocolbuffers_descriptor_methods);
	php_protocol_buffers_descriptor_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	php_protocol_buffers_descriptor_class_entry->create_object = php_protocolbuffers_descriptor_new;

	//zend_declare_property_null(php_protocol_buffers_descriptor_class_entry, ZEND_STRL("fields"), ZEND_ACC_PROTECTED TSRMLS_CC);

	PHP_PROTOCOLBUFFERS_REGISTER_NS_CLASS_ALIAS(PHP_PROTOCOLBUFFERS_NAMESPACE, "Descriptor", php_protocol_buffers_descriptor_class_entry);

	memcpy(&php_protocolbuffers_descriptor_object_handlers, zend_get_std_object_handlers(), sizeof(php_protocolbuffers_descriptor_object_handlers));
	php_protocolbuffers_descriptor_object_handlers.offset = XtOffsetOf(php_protocolbuffers_descriptor, zo);
	php_protocolbuffers_descriptor_object_handlers.free_obj = (zend_object_free_obj_t)php_protocolbuffers_descriptor_free_storage;
}
