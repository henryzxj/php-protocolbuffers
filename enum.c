#include "protocolbuffers.h"
#include "enum.h"

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_enum_is_valid, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_protocolbuffers_enum_get_name, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()


/* {{{ proto void ProtocolBuffersEnum::__construct()
*/
PHP_METHOD(protocolbuffers_enum, __construct)
{
}
/* }}} */

/* {{{ proto bool ProtocolBuffersEnum::isValid(long $value)
*/
PHP_METHOD(protocolbuffers_enum, isValid)
{
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 3)
	zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "ProtocolBuffersEnum::isValid can't work under PHP 5.3. please consider upgrading your PHP");
	return;
#else
	long value;
	zval *result;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"l", &value) == FAILURE) {
		return;
	}

	zend_class_entry *called_scope = zend_get_called_scope(execute_data);
	if (zend_call_method_with_0_params(NULL, called_scope, NULL, "getenumdescriptor", result)) {
		zval *values, *entry;
		HashPosition pos;

		if (!instanceof_function_ex(Z_OBJCE_P(result), php_protocol_buffers_enum_descriptor_class_entry, 0 TSRMLS_CC)) {
			zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "ProtocolBuffersEnum::getEnumDescriptor returns unexpected value.");
			zval_ptr_dtor(result);
			return;
		}

		php_protocolbuffers_read_protected_property(result, ZEND_STRS("values"), &values TSRMLS_CC);
		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(values), &pos);
		while ((entry=zend_hash_get_current_data_ex(Z_ARRVAL_P(values), &pos)) != NULL) {
			if (Z_LVAL_P(entry) == value) {
				RETVAL_TRUE;
				break;
			}
			zend_hash_move_forward_ex(Z_ARRVAL_P(values), &pos);
		}
		zval_ptr_dtor(result);
		RETVAL_FALSE;
	} else {
			zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "cannot call ProtocolBuffersEnum::getEnumDescriptor.");
			return;
	}
#endif
}
/* }}} */

/* {{{ proto string ProtocolBuffersEnum::getName(long $value)
*/
PHP_METHOD(protocolbuffers_enum, getName)
{
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 3)
	zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "ProtocolBuffersEnum::getName can't work under PHP 5.3. please consider upgrading your PHP");
	return;
#else
	long value;
	zval *result;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"l", &value) == FAILURE) {
		return;
	}
	zend_class_entry *called_scope = zend_get_called_scope(execute_data);
	if (zend_call_method_with_0_params(NULL, called_scope, NULL, "getenumdescriptor", result)) {
		zval *values, *entry;
		HashPosition pos;

		if (!instanceof_function_ex(Z_OBJCE_P(result), php_protocol_buffers_enum_descriptor_class_entry, 0 TSRMLS_CC)) {
			zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "ProtocolBuffersEnum::getEnumDescriptor returns unexpected value.");
			zval_ptr_dtor(result);
			return;
		}

		php_protocolbuffers_read_protected_property(result, ZEND_STRS("values"), &values TSRMLS_CC);
		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(values), &pos);
		while ((entry=zend_hash_get_current_data_ex(Z_ARRVAL_P(values), &pos)) != NULL) {
			if (Z_LVAL_P(entry) == value) {
				char *key;
				uint key_len;
				ulong index;
				zend_string *key_name = zend_string_init(key,key_len,0);

				zend_hash_get_current_key_ex(Z_ARRVAL_P(values), &key_name, &index, &pos);
				RETURN_STRINGL(key, key_len);
				break;
			}
			zend_hash_move_forward_ex(Z_ARRVAL_P(values), &pos);
		}
		zval_ptr_dtor(result);
		RETVAL_FALSE;
	} else {
			zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC, "cannot call ProtocolBuffersEnum::getEnumDescriptor.");
			return;
	}
#endif
}
/* }}} */


static zend_function_entry protocolbuffers_enum_methods[] = {
	PHP_ME(protocolbuffers_enum, __construct, NULL, ZEND_ACC_PRIVATE | ZEND_ACC_CTOR)
	PHP_ME(protocolbuffers_enum, isValid, arginfo_protocolbuffers_enum_is_valid, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_ME(protocolbuffers_enum, getName, arginfo_protocolbuffers_enum_get_name, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	PHP_FE_END
};

void php_protocolbuffers_enum_class(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "ProtocolBuffersEnum", protocolbuffers_enum_methods);
	php_protocol_buffers_enum_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	PHP_PROTOCOLBUFFERS_REGISTER_NS_CLASS_ALIAS(PHP_PROTOCOLBUFFERS_NAMESPACE, "Enum", php_protocol_buffers_enum_class_entry);
	zend_class_implements(php_protocol_buffers_enum_class_entry TSRMLS_CC, 1, php_protocol_buffers_enum_describable_class_entry);

	php_protocol_buffers_enum_class_entry->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

}
