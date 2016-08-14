#include "protocolbuffers.h"
#include "enum_descriptor_builder.h"

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_enum_descriptor_builder_add_value, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, force_add)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_enum_descriptor_builder_build, 0, 0, 0)
ZEND_END_ARG_INFO()

/* {{{ proto void ProtocolBuffersDescriptorBuilder::__construct()
*/
PHP_METHOD(protocolbuffers_enum_descriptor_builder, __construct)
{
	zval fields;

//	MAKE_STD_ZVAL(fields);
	array_init(&fields);

	php_protocolbuffers_set_protected_property(getThis(), ZEND_STRL("values"), &fields TSRMLS_CC);
	zval_ptr_dtor(&fields);
}
/* }}} */

/* {{{ proto void ProtocolBuffersDescriptorBuilder::addValue(ProtocolBuffers\EnumValueDescriptor $value[, bool $force = false])
*/
PHP_METHOD(protocolbuffers_enum_descriptor_builder, addValue)
{
	zval *instance = getThis();
	zval *value, *fields, *name;
	zend_bool force = 0;
//	char *property;
	int property_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"O|b", &value, php_protocol_buffers_enum_value_descriptor_class_entry, &force) == FAILURE) {
		return;
	}

	zend_string *property_key = zend_mangle_property_name((char*)"*", 1, (char*)ZEND_STRL("values"), 0);
	if ((fields=zend_hash_find(Z_OBJPROP_P(instance), property_key)) != NULL) {
		if (php_protocolbuffers_read_protected_property(value, ZEND_STRL("name"), &name TSRMLS_CC)) {
			zend_string *name_key = zend_string_init(Z_STRVAL_P(name), Z_STRLEN_P(name),0);
			if (zend_hash_exists(Z_ARRVAL_P(fields), name_key)) {
				if (force < 1) {
					zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "name `%s` has already registered.", Z_STRVAL_P(name));
				}
				zend_string_release(property_key);
				zend_string_release(name_key);
				return;
			}
		}

		Z_ADDREF_P(value);
		zend_string *name_key = zend_string_init(Z_STRVAL_P(name), Z_STRLEN_P(name)+1,0);
		zend_hash_update(Z_ARRVAL_P(fields), name_key, value);
		zend_string_release(name_key);
	}
	zend_string_release(property_key);

}
/* }}} */

/* {{{ proto ProtocolBuffers\EnumDescriptor ProtocolBuffersDescriptorBuilder::build()
*/
PHP_METHOD(protocolbuffers_enum_descriptor_builder, build)
{
	zval *result, *container, *fields, *entry, *key, *value;
	char *property;
	int property_len;
	HashPosition pos;

//	MAKE_STD_ZVAL(result);
	object_init_ex(result, php_protocol_buffers_enum_descriptor_class_entry);

//	MAKE_STD_ZVAL(container);
	array_init(container);

	zend_string *property_key = zend_mangle_property_name((char*)"*", 1, ZEND_STRL("values"), 0);
	if ((fields=zend_hash_find(Z_OBJPROP_P(getThis()), property_key)) != NULL) {
		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(fields), &pos);
		while ((entry=zend_hash_get_current_data_ex(Z_ARRVAL_P(fields), &pos)) != NULL) {
			zval *tmp = NULL;

//			MAKE_STD_ZVAL(tmp);

			php_protocolbuffers_read_protected_property(entry, ZEND_STRL("name"), &key TSRMLS_CC);
			php_protocolbuffers_read_protected_property(entry, ZEND_STRL("value"), &value TSRMLS_CC);

			ZVAL_ZVAL(tmp, value, 1, 0);
			zend_string *key_name = zend_string_init(Z_STRVAL_P(key),Z_STRLEN_P(key),0);
			zend_hash_update(Z_ARRVAL_P(container), key_name, tmp);
			zend_hash_move_forward_ex(Z_ARRVAL_P(fields), &pos);
		}
	}
	efree(property);

	php_protocolbuffers_set_protected_property(result, ZEND_STRL("values"), container TSRMLS_CC);
	RETURN_ZVAL(result, 0, 1);
}
/* }}} */


static zend_function_entry protocolbuffers_enum_descriptor_builder_methods[] = {
	PHP_ME(protocolbuffers_enum_descriptor_builder, __construct, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(protocolbuffers_enum_descriptor_builder, addValue,  arginfo_protocolbuffers_enum_descriptor_builder_add_value, ZEND_ACC_PUBLIC)
	PHP_ME(protocolbuffers_enum_descriptor_builder, build,     arginfo_protocolbuffers_enum_descriptor_builder_build, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

void php_protocolbuffers_enum_descriptor_builder_class(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "ProtocolBuffersEnumDescriptorBuilder", protocolbuffers_enum_descriptor_builder_methods);
	php_protocol_buffers_enum_descriptor_builder_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	PHP_PROTOCOLBUFFERS_REGISTER_NS_CLASS_ALIAS(PHP_PROTOCOLBUFFERS_NAMESPACE, "EnumDescriptorBuilder", php_protocol_buffers_enum_descriptor_builder_class_entry);

	//zend_declare_property_null(php_protocol_buffers_enum_descriptor_builder_class_entry, ZEND_STRL("values")-1, ZEND_ACC_PROTECTED TSRMLS_CC);
}
