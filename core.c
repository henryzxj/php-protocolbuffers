#include "protocolbuffers.h"

#include "core.h"
#include "serializer.h"
#include "json_serializer.h"
#include "unknown_field.h"
#include "unknown_field_set.h"

static char *single_property_name_default = "_properties";
static int single_property_name_default_len = sizeof("_properties");
static char *unknown_property_name_default = "_unknown";
static int unknown_property_name_default_len = sizeof("_unknown");

const char* php_protocolbuffers_decode_message(INTERNAL_FUNCTION_PARAMETERS, const char *data, const char *data_end, php_protocolbuffers_scheme_container *container, int native_scalars, zval *result);

int php_protocolbuffers_read_protected_property(zval *instance, char *name, size_t name_len, zval **result TSRMLS_DC)
{
	zval *tmp;
	int retval = 0;

	zend_string *prop_name_key = zend_mangle_property_name((char*)"*", 1, name, name_len, 0);
	if ((tmp=zend_hash_find(Z_OBJPROP_P(instance), prop_name_key)) != NULL) {
		*result = tmp;
		retval = 1;
	}
	zend_string_release(prop_name_key);
	return retval;
}

int php_protocolbuffers_set_protected_property(zval *instance, char *name, size_t name_len, zval *value TSRMLS_DC)
{
	zend_string *prop_name_key = zend_mangle_property_name((char*)"*", 1, name, name_len, 0);
	zend_hash_update(Z_OBJPROP_P(instance), prop_name_key, value);
	zend_string_release(prop_name_key);
	return 1;
}

char *php_protocolbuffers_get_default_single_property_name()
{
	return single_property_name_default;
}

int php_protocolbuffers_get_default_single_property_name_len()
{
	return single_property_name_default_len;
}

char *php_protocolbuffers_get_default_unknown_property_name()
{
	return unknown_property_name_default;
}

int php_protocolbuffers_get_default_unknown_property_name_len()
{
	return unknown_property_name_default_len;
}

void php_protocolbuffers_scheme_container_init(php_protocolbuffers_scheme_container *container)
{
	memset(container, '\0', sizeof(php_protocolbuffers_scheme_container));

	container->size = 0;
	container->scheme = NULL;
	container->use_single_property = 0;
	container->process_unknown_fields = 0;
	container->use_wakeup_and_sleep = 0;
	container->extension_cnt = 0;
	container->extensions = NULL;
	container->use_wakeup_and_sleep = 0;
	container->single_property_h = 0;
	container->orig_single_property_name     = php_protocolbuffers_get_default_single_property_name();
	container->orig_single_property_name_len = php_protocolbuffers_get_default_single_property_name_len();


	zend_string *property_name_key = zend_mangle_property_name(
		(char*)"*",
		1,
		(char*)container->orig_single_property_name,
		container->orig_single_property_name_len,
		0
	);

	container->single_property_name = ZSTR_VAL(property_name_key);
	container->single_property_name_len = ZSTR_LEN(property_name_key);

	container->single_property_h = zend_inline_hash_func(container->single_property_name, container->single_property_name_len);
	zend_string_release(property_name_key);
}

int php_protocolbuffers_get_scheme_container(zend_string *klass, php_protocolbuffers_scheme_container **result TSRMLS_DC)
{
	return php_protocolbuffers_get_scheme_container_ex(klass, 1, result TSRMLS_CC);
}

int php_protocolbuffers_get_scheme_container_ex(zend_string *klass, int throws_exception, php_protocolbuffers_scheme_container **result TSRMLS_DC)
{
	php_protocolbuffers_scheme_container *cn;
	if ((cn=zend_hash_find_ptr(PBG(messages), klass)) == NULL) {
		zval ret;
		zend_class_entry *ce = NULL;
		if ((ce=(zend_class_entry *)zend_lookup_class(klass)) == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_get_scheme_container failed. %s does find", ZSTR_VAL(klass));
			return 1;
		}

		if (zend_call_method(NULL, ce, NULL, ZEND_STRL("getdescriptor"), &ret, 0, NULL, NULL)) {
			if (Z_TYPE_P(&ret) == IS_ARRAY) {
				/* TODO(chobie): move this block after release. */
				zval_ptr_dtor(&ret);
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_get_scheme_container no longer support array based descriptor");
				return 1;
			} else if (Z_TYPE_P(&ret) == IS_OBJECT) {
				zend_class_entry *entry;
				php_protocolbuffers_descriptor *desc;

				entry = Z_OBJCE_P(&ret);
				if (entry == php_protocol_buffers_descriptor_class_entry) {
					desc = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_descriptor, &ret);
					desc->free_container = 1;
					zend_hash_add_ptr(PBG(messages), klass, desc->container);
				} else {
					zend_throw_exception_ex(php_protocol_buffers_invalid_protocolbuffers_exception_class_entry, 0 TSRMLS_CC, "getDescriptor returns unexpected class");
					if (&ret != NULL) {
						zval_ptr_dtor(&ret);
					}
					return 1;
				}

				zval_ptr_dtor(&ret);
				*result = desc->container;
				return 0;
			} else {
				if (&ret != NULL) {
					zval_ptr_dtor(&ret);
				}
				zend_throw_exception_ex(php_protocol_buffers_invalid_protocolbuffers_exception_class_entry, 0 TSRMLS_CC, "getDescriptor returns unexpected class");
				return 1;
			}
		} else {
			if (EG(exception)) {
				PHP_PROTOCOLBUFFERS_EXCEPTION_ERROR(EG(exception));
			} else {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_get_scheme_container failed. %s does not have getDescriptor method", ZSTR_VAL(klass));
			}
			return 1;
		}
	} else {
		//php_protocolbuffers_scheme_container_init(cn);//add by henryzxj,is that correct
		*result = cn;
	}

	//*result = container;

	return 0;
}

void php_protocolbuffers_process_unknown_field(INTERNAL_FUNCTION_PARAMETERS, php_protocolbuffers_scheme_container *container, zval *result, zval *dz, int tag, int wiretype, int64_t value)
{
	char *unknown_name = {0};
	int unknown_name_len = 0;
	zval *un = NULL;
	unknown_value *val;
	php_protocolbuffers_unknown_field *p = NULL;
	zend_string *unknown_name_key;
	if (container->use_single_property > 0) {
		unknown_name = php_protocolbuffers_get_default_unknown_property_name();
		unknown_name_len = php_protocolbuffers_get_default_unknown_property_name_len();
		unknown_name_key = zend_string_init(unknown_name,unknown_name_len,0);
	} else {
		unknown_name_key = zend_mangle_property_name((char*)"*", 1, (char*)php_protocolbuffers_get_default_unknown_property_name(), php_protocolbuffers_get_default_unknown_property_name_len(), 0);
		unknown_name = ZSTR_VAL(unknown_name_key);
		unknown_name_len = ZSTR_LEN(unknown_name_key);
	}
	HashTable *hresult = Z_OBJPROP_P(result);
	if ((un=zend_hash_find(hresult, unknown_name_key)) !=NULL) {
		if (php_protocolbuffers_unknown_field_get_field(INTERNAL_FUNCTION_PARAM_PASSTHRU, un, tag, unknown_name, unknown_name_len, &p)) {
			val = (unknown_value*)emalloc(sizeof(val));
			val->varint = value;

			zend_hash_next_index_insert(p->ht, (void *)val);
		} else {
			object_init_ex(dz, php_protocol_buffers_unknown_field_class_entry);
			php_protocolbuffers_unknown_field_properties_init(dz TSRMLS_CC);
			php_protocolbuffers_unknown_field_set_number(dz, tag TSRMLS_CC);
			php_protocolbuffers_unknown_field_set_type(dz, wiretype TSRMLS_CC);

			p = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_unknown_field, dz);

			val = (unknown_value*)emalloc(sizeof(unknown_value));
			val->varint = value;
			zend_hash_next_index_insert(p->ht, (void *)val);

			php_protocolbuffers_unknown_field_set_add_field(INTERNAL_FUNCTION_PARAM_PASSTHRU, un, tag, dz);
		}
	}

	if (container->use_single_property < 1) {
		efree(unknown_name);
	}
	zend_string_release(unknown_name_key);
}

void php_protocolbuffers_process_unknown_field_bytes(INTERNAL_FUNCTION_PARAMETERS, php_protocolbuffers_scheme_container *container, zval *result, int tag, int wiretype, uint8_t *bytes, int length)
{
	char *unknown_name = {0};
	int unknown_name_len = 0;
	zval dz, *unknown_fieldset = NULL;
	unknown_value *val = NULL;
	php_protocolbuffers_unknown_field *p = NULL;
	uint8_t *buffer = NULL;
	zend_string *unknown_name_key=NULL;
	if (container->use_single_property > 0) {
		unknown_name     = php_protocolbuffers_get_default_unknown_property_name();
		unknown_name_len = php_protocolbuffers_get_default_unknown_property_name_len();
		unknown_name_key = zend_string_init(unknown_name,unknown_name_len,0);
	} else {
		unknown_name_key = zend_mangle_property_name((char*)"*", 1, (char*)php_protocolbuffers_get_default_unknown_property_name(), php_protocolbuffers_get_default_unknown_property_name_len(), 0);
		unknown_name = ZSTR_VAL(unknown_name_key);
		unknown_name_len = ZSTR_LEN(unknown_name_key);
	}

	HashTable *hresult = Z_OBJPROP_P(result);
	if ((unknown_fieldset=zend_hash_find(hresult, unknown_name_key)) != NULL) {
		if (Z_OBJCE_P(unknown_fieldset) != php_protocol_buffers_unknown_field_set_class_entry) {
			return;
		}

		if (php_protocolbuffers_unknown_field_get_field(INTERNAL_FUNCTION_PARAM_PASSTHRU, unknown_fieldset, tag, unknown_name, unknown_name_len, &p)) {
			val = (unknown_value*)emalloc(sizeof(unknown_value));
			buffer = (uint8_t*)emalloc(length);
			memcpy(buffer, bytes, length);

			val->buffer.val = buffer;
			val->buffer.len = length;

			zend_hash_next_index_insert(p->ht, (void *)val);
		} else {
//			MAKE_STD_ZVAL(dz);
			object_init_ex(&dz, php_protocol_buffers_unknown_field_class_entry);
			php_protocolbuffers_unknown_field_properties_init(&dz TSRMLS_CC);
			php_protocolbuffers_unknown_field_set_number(&dz, tag TSRMLS_CC);
			php_protocolbuffers_unknown_field_set_type(&dz, wiretype TSRMLS_CC);

			p = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_unknown_field, &dz);

			val = (unknown_value*)emalloc(sizeof(unknown_value));
			buffer = (uint8_t*)emalloc(length);
			memcpy(buffer, bytes, length);
			val->buffer.val = buffer;
			val->buffer.len = length;
			zend_hash_next_index_insert_ptr(p->ht, (void *)val);

			php_protocolbuffers_unknown_field_set_add_field(INTERNAL_FUNCTION_PARAM_PASSTHRU, unknown_fieldset, tag, &dz);
		}
	}

	zend_string_release(unknown_name_key);
	if (container->use_single_property < 1) {
		efree(unknown_name);
	}
}

void php_protocolbuffers_format_string(zval *result, pbf *payload, int native_scalars TSRMLS_DC)
{
	char __buffer[64] = {0};
	char *buffer = __buffer;
	int free = 0;
	size_t size = 0;

	if (native_scalars == 1) {
		switch (payload->type) {
			case TYPE_SINT32:
			case TYPE_INT32:
				ZVAL_LONG(result, payload->value.int32);
				break;
			case TYPE_UINT32:
				ZVAL_LONG(result, payload->value.uint32);
				break;
			case TYPE_UINT64:
				ZVAL_LONG(result, payload->value.uint64);
				break;
			case TYPE_SINT64:
			case TYPE_INT64:
				ZVAL_LONG(result, payload->value.int64);
				break;
			break;
			case TYPE_FLOAT:
				ZVAL_DOUBLE(result, payload->value.f);
				break;
			case TYPE_DOUBLE:
				ZVAL_DOUBLE(result, payload->value.d);
				break;
			break;
			default:
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "the type %d does not support. maybe this is bug", payload->type);
				return;
			break;
		}
	} else {
		switch (payload->type) {
			case TYPE_SINT32:
			case TYPE_INT32:
				size = snprintf(buffer, sizeof(__buffer), "%d", payload->value.int32);
			break;
			case TYPE_UINT32:
				size = snprintf(buffer, sizeof(__buffer), "%u", payload->value.uint32);
			break;
			case TYPE_UINT64:
				size = snprintf(buffer, sizeof(__buffer), "%llu" , payload->value.uint64);
			break;
			case TYPE_SINT64:
			case TYPE_INT64:
				size = snprintf(buffer, sizeof(__buffer), "%lld" , payload->value.int64);
			break;
			case TYPE_FLOAT:{
				char *p = 0;
				free = 1;

				// Note: this is safe
				buffer = emalloc(MAX_LENGTH_OF_DOUBLE + EG(precision) + 1);
				size = zend_sprintf(buffer, "%f", payload->value.f);

				// remove trailing zeros
				p = strchr(buffer,'.');
				if(p != NULL) {
					while(*++p);

					while('0' == *--p){
						*p = '\0';
						size--;
					}

					if(*p == '.'){
						*p = '\0';
						size--;
					}
					break;
				}
				}
			break;
			case TYPE_DOUBLE:{
				free = 1;
				/// Note: this is safe
				buffer = emalloc(MAX_LENGTH_OF_DOUBLE + EG(precision) + 1);
				size = zend_sprintf(buffer, "%.*G", (int)EG(precision), payload->value.d);
			}
			break;
			default:
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "the type %d does not support. maybe this is bug", payload->type);
				return;
			break;
		}

		if (buffer != NULL) {
			zend_string *buff = zend_string_init(buffer,size,0);
			ZVAL_STR(result,buff);
			zend_string_release(buff);
		}
		if (free) {
			efree(buffer);
		}
	}
}

const char* php_protocolbuffers_decode_message(INTERNAL_FUNCTION_PARAMETERS, const char *data, const char *data_end, php_protocolbuffers_scheme_container *container, int native_scalars, zval *result)
{
	uint32_t payload = 0, tag = 0, wiretype = 0;
	uint64_t value = 0;
	zval dzz;
	HashTable *hresult;
	pbf __payload = {0};
	php_protocolbuffers_scheme *prior_scheme = NULL;
	zend_class_entry *result_ce = Z_OBJCE_P(result);
	if (container->use_single_property > 0) {
		zval *tmp = NULL;
		if((tmp=zend_hash_str_find(Z_OBJPROP_P(result), container->single_property_name, container->single_property_name_len+1))!=NULL){
			if (Z_TYPE_P(tmp) != IS_ARRAY) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "single property is not an array.");
				return NULL;
			}

			hresult = Z_ARRVAL_P(tmp);
		} else {
			zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "the class does not have property.");
			return NULL;
		}

	} else {
		hresult = Z_OBJPROP_P(result);
	}

	while (data < data_end) {
		php_protocolbuffers_scheme *s = NULL;

		data = ReadVarint32FromArray(data, &payload, data_end);

		if (data == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_decode_message failed. ReadVarint32FromArray returns NULL.");
			return NULL;
		}

		tag      = (payload >> 0x03);
		wiretype = (payload & 0x07);

		if (tag < 1 || tag > ktagmax) {
			return NULL;
		}

		if (wiretype > WIRETYPE_FIXED32) {
			return NULL;
		}

		s = php_protocolbuffers_search_scheme_by_tag(container->scheme, container->size, tag);

		switch (wiretype) {
		case WIRETYPE_VARINT:
		{
			data = ReadVarint64FromArray(data, &value, data_end);
			if (!php_protocolbuffers_process_varint(INTERNAL_FUNCTION_PARAM_PASSTHRU, wiretype, tag, container, s, value, result, native_scalars)) {
				return NULL;
			}
		}
		break;
		case WIRETYPE_FIXED64:
			if (!php_protocolbuffers_process_fixed64(INTERNAL_FUNCTION_PARAM_PASSTHRU, wiretype, tag, container, s, data, result, native_scalars)) {
				return NULL;
			}
			data += 8;
		break;
		case WIRETYPE_LENGTH_DELIMITED:
			data = ReadVarint32FromArray(data, &payload, data_end);

			if ((data+payload) > data_end) {
				return NULL;
			}

			if (s == NULL) {
				if (container->process_unknown_fields > 0) {
					php_protocolbuffers_process_unknown_field_bytes(INTERNAL_FUNCTION_PARAM_PASSTHRU, container, result, tag, wiretype, (uint8_t*)data, payload);
				} else {
					/* skip unknown field */
				}
			} else if (s->type == TYPE_STRING) {
				ZVAL_STRINGL(&dzz, (char*)data, payload);
				php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);

			} else if (s->type == TYPE_BYTES) {
				ZVAL_STRINGL(&dzz, (char*)data, payload);

				php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
			} else if (s->type == TYPE_MESSAGE) {
				const char *n_buffer_end = data + payload;
				zval zz_obj;
				php_protocolbuffers_scheme_container *c_container = NULL;
//				char *name1 = {0};
//				int name_length1 = 0;
//				ulong name_hash = 0;
				zend_string *name_key = NULL;

				php_protocolbuffers_get_scheme_container(s->ce->name, &c_container TSRMLS_CC);

				if (container->use_single_property < 1) {
//					name        = s->mangled_name;
//					name_length = s->mangled_name_len;
//					name_hash   = s->mangled_name_h;
//					name_key = s->mangled_name_key;
					name_key = s->name_key;
				} else {
//					name        = s->name;
//					name_length = s->name_len;
//					name_hash   = s->name_h;
					name_key = s->name_key;
				}
				if (prior_scheme != NULL && prior_scheme == s && !s->repeated) {
					/* NOTE: some protobuf implementation will split child message into each filed. */
					zval *tt;

					if ((tt=zend_hash_find(hresult, name_key)) != NULL) {
//						zz_obj = tt;
						ZVAL_COPY(&zz_obj,tt);
						//Z_TRY_ADDREF(zz_obj);
					} else {
						object_init_ex(&zz_obj, s->ce);
						php_protocolbuffers_properties_init(&zz_obj, s->ce TSRMLS_CC);
					}
				} else {
					object_init_ex(&zz_obj, s->ce);
					php_protocolbuffers_properties_init(&zz_obj, s->ce TSRMLS_CC);

					if (instanceof_function_ex(s->ce, php_protocol_buffers_message_class_entry, 0 TSRMLS_CC)) {
						php_protocolbuffers_message *m;
						m = PHP_PROTOCOLBUFFERS_GET_OBJECT(php_protocolbuffers_message, &zz_obj);

						// point to parent.
						ZVAL_ZVAL(m->container, result, 0, 0);
					}
				}

				//php_protocolbuffers_decode_message(INTERNAL_FUNCTION_PARAM_PASSTHRU, data, n_buffer_end, c_container, PBG(native_scalars), &zz_obj);

				if (c_container->use_wakeup_and_sleep > 0) {
					php_protocolbuffers_execute_wakeup(&zz_obj, c_container TSRMLS_CC);
				}

				if (s->repeated) {
					if (!zend_hash_exists(hresult, name_key)) {
						zval arr;

						array_init(&arr);

//						Z_TRY_ADDREF(zz_obj);
						//Z_ADDREF_P(&zz_obj);
						zend_hash_next_index_insert(Z_ARRVAL(arr), &zz_obj);
//						Z_TRY_ADDREF(arr);
						//Z_ADDREF_P(&arr);
						zend_update_property(result_ce,result,ZSTR_VAL(name_key),ZSTR_LEN(name_key),&arr);
						zval_dtor(&arr);
					} else {
						zval *arr2 = NULL;

						if ((arr2=zend_hash_find(hresult, name_key)) != NULL) {
							//Z_ADDREF_P(&zz_obj);
							zend_hash_next_index_insert(Z_ARRVAL_P(arr2), &zz_obj);
						}
					}
				} else {
					zend_update_property(result_ce,result,ZSTR_VAL(name_key),ZSTR_LEN(name_key),&zz_obj);
				}

				zval_ptr_dtor(&zz_obj);
			} else if (s->packed) {
				const char *packed_data_end, *last_data_offset;

				last_data_offset = data;
				packed_data_end = data + payload;
				do {
					switch (s->type) {
						case TYPE_DOUBLE:
						{
							uint64_t _v;
							double d;
							memcpy(&_v, data, 8);

							d = decode_double(_v);

							__payload.type = TYPE_DOUBLE;__payload.value.d = d;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);
							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);

							data += 8;
						}
						break;
						case TYPE_FLOAT:
						{
							uint32_t _v;
							float a = 0;

							memcpy(&_v, data, 4);
							a = decode_float(_v);


							__payload.type = TYPE_DOUBLE;__payload.value.d = a;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);

							data += 4;
						}
						break;
						case TYPE_INT64:
						{
							uint64_t v2;
							data = ReadVarint64FromArray(data, &v2, data_end);


							__payload.type = TYPE_INT64;__payload.value.int64 = v2;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
						}
						break;
						case TYPE_UINT64:
						{
							uint64_t v2;
							data = ReadVarint64FromArray(data, &v2, data_end);


							__payload.type = TYPE_UINT64;__payload.value.uint64 = v2;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
						}
						break;
						break;
						case TYPE_INT32:
							data = ReadVarint32FromArray(data, &payload, data_end);


							__payload.type = TYPE_INT32;__payload.value.int32 = (int32_t)payload;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
						break;
						case TYPE_FIXED64:
						{
							uint64_t l;
							memcpy(&l, data, 8);

							__payload.type = TYPE_UINT64;__payload.value.uint64 = l;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
							data += 8;
						}
						break;
						case TYPE_FIXED32:
						{
							uint32_t l = 0;
							memcpy(&l, data, 4);

							__payload.type = TYPE_UINT32;__payload.value.uint32 = l;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);

							data += 4;
						}
						break;
						case TYPE_BOOL:
							data = ReadVarint32FromArray(data, &payload, data_end);

							ZVAL_BOOL(&dzz, payload);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
						break;
						case TYPE_UINT32:
							data = ReadVarint32FromArray(data, &payload, data_end);

							__payload.type = TYPE_UINT32;__payload.value.uint32 = (uint32_t)payload;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
						break;
						case TYPE_ENUM:
							data = ReadVarint32FromArray(data, &payload, data_end);

							ZVAL_LONG(&dzz, (int32_t)payload);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
						break;
						case TYPE_SFIXED32:
						{
							int32_t l = 0;

							memcpy(&l, data, 4);

							__payload.type = TYPE_INT32;__payload.value.int32 = l;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
							data += 4;
						}
						break;
						case TYPE_SFIXED64:
						{
							int64_t l;
							memcpy(&l, data, 8);

							__payload.type = TYPE_INT64;__payload.value.int64 = l;
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);

							data += 8;
						}
						break;
						case TYPE_SINT32:
							data = ReadVarint32FromArray(data, &payload, data_end);

							__payload.type = TYPE_INT32;__payload.value.int32 = (int32_t)zigzag_decode32(payload);
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
						break;
						case TYPE_SINT64:
						{
							uint64_t v2;
							data = ReadVarint64FromArray(data, &v2, data_end);


							__payload.type = TYPE_SINT64;__payload.value.int64 = (int64_t)zigzag_decode64(v2);
							php_protocolbuffers_format_string(&dzz, &__payload, native_scalars TSRMLS_CC);

							php_protocolbuffers_decode_add_value_and_consider_repeated(container, s, result, &dzz TSRMLS_CC);
						}
						break;
					}

					if (last_data_offset == data) {
						php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_decode_message: detect infinite loop when processing repeated packed field.");
						break;
					}
					last_data_offset = data;
				} while(data < packed_data_end);
				break;
			}

			data += payload;
		break;
		case WIRETYPE_FIXED32: {
			if (!php_protocolbuffers_process_fixed32(INTERNAL_FUNCTION_PARAM_PASSTHRU, wiretype, tag, container, s, data, result, native_scalars)) {
				return NULL;
			}
			data += 4;
		}
		break;
		}
		prior_scheme = s;
		php_var_dump(result,1);
	}

	return data;
}


int php_protocolbuffers_jsonserialize(INTERNAL_FUNCTION_PARAMETERS, int throws_exception, zend_class_entry *ce, zval *klass, zval **result)
{
	int err = 0;
	zval *tmp = NULL;
	php_protocolbuffers_scheme_container *container;

	err = php_protocolbuffers_get_scheme_container_ex(ce->name, throws_exception, &container TSRMLS_CC);
	if (err) {
		if (EG(exception)) {
			return err;
		} else {
			php_protocolbuffers_raise_error_or_exception(php_protocolbuffers_get_exception_base(TSRMLS_C), E_ERROR, throws_exception, "php_protocolbuffers_get_scheme_container failed. %s does not have getDescriptor method", ce->name);
			return err;
		}
	}

//	MAKE_STD_ZVAL(tmp);
	array_init(tmp);

	if (php_protocolbuffers_encode_jsonserialize(klass, container, throws_exception, &tmp TSRMLS_CC) != 0) {
		zval_ptr_dtor(tmp);
		return 1;
	}

	*result = tmp;
	return err;
}

int php_protocolbuffers_encode(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce, zval *klass)
{
	int err = 0;
	php_protocolbuffers_serializer *ser = NULL;
	php_protocolbuffers_scheme_container *container;

	err = php_protocolbuffers_get_scheme_container(ce->name, &container TSRMLS_CC);
	if (err) {
		if (EG(exception)) {
			return 1;
		} else {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_get_scheme_container failed. %s does not have getDescriptor method", ce->name);
			return 1;
		}
	}

	if (php_protocolbuffers_encode_message(INTERNAL_FUNCTION_PARAM_PASSTHRU, klass, container, &ser)) {
		return 1;
	}

	if (ser == NULL) {
		return 1;
	}

	if (ser->buffer_size > 0) {
		RETVAL_STRINGL((char*)ser->buffer, ser->buffer_size);
	}

	php_protocolbuffers_serializer_destroy(ser);
	return 0;
}

int php_protocolbuffers_decode(INTERNAL_FUNCTION_PARAMETERS, const char *data,long data_len, zend_string *klass)
{
	zval obj;
	php_protocolbuffers_scheme_container *container;
	const char *data_end, *res;
	int err = 0;
	zend_class_entry *ce = NULL;

	if (data_len < 1) {
		zend_throw_exception_ex(php_protocol_buffers_invalid_protocolbuffers_exception_class_entry, 0 TSRMLS_CC, "passed variable seems null");
		return 1;
	}

	err = php_protocolbuffers_get_scheme_container(klass, &container TSRMLS_CC);
	if (err) {
		if (EG(exception)) {
			// do nothing
		} else {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_get_scheme_container failed. %s does not have getDescriptor method", ZSTR_VAL(klass));
		}
		return 1;
	}

	data_end = data + data_len;

	if (PBG(classes)) {
		/* Memo: fast lookup */
		if ((ce=zend_hash_find_ptr(PBG(classes), klass)) == NULL) {
			ce = zend_lookup_class(klass);
			if (ce != NULL) {
				zend_hash_update_ptr(PBG(classes), klass, ce);
			} else {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "class lookup failed. %s does exist", ZSTR_VAL(klass));
				return 1;
			}
		}
	}

	object_init_ex(&obj, ce);
	php_protocolbuffers_properties_init(&obj, ce TSRMLS_CC);
	/* add unknown fields */
	if (container->process_unknown_fields > 0) {
		zval *un = NULL, unknown;
		//char *unknown_name = {0};
		//int unknown_name_len = 0;

		object_init_ex(&unknown, php_protocol_buffers_unknown_field_set_class_entry);
		php_protocolbuffers_unknown_field_set_properties_init(&unknown TSRMLS_CC);

		zend_string *unknown_name_key = zend_mangle_property_name((char*)"*", 1, (char*)"_unknown", sizeof("_unknown"), 0);
		if ((un=zend_hash_find(Z_OBJPROP_P(&obj), unknown_name_key)) == NULL) {
			zend_hash_update(Z_OBJPROP_P(&obj), unknown_name_key, &unknown);
		} else {
			if (Z_TYPE_P(un) == IS_NULL) {
				zend_hash_update(Z_OBJPROP_P(&obj), unknown_name_key, &unknown);
			} else {
				efree(&unknown);
			}
		}
		zend_string_release(unknown_name_key);
	}

	res = php_protocolbuffers_decode_message(INTERNAL_FUNCTION_PARAM_PASSTHRU, data, data_end, container, PBG(native_scalars), &obj);
	if (res == NULL) {
		if (Z_TYPE(obj) != IS_NULL) {
			zval_ptr_dtor(&obj);
		}
		zend_throw_exception_ex(php_protocol_buffers_invalid_protocolbuffers_exception_class_entry, 0 TSRMLS_CC, "passed variable contains malformed byte sequence. or it contains unsupported tag");
		return 0;
	}

	if (container->use_wakeup_and_sleep > 0) {
		php_protocolbuffers_execute_wakeup(&obj, container TSRMLS_CC);
	}
	RETVAL_ZVAL(&obj,0,1);
	return 0;
}

void php_protocolbuffers_execute_wakeup(zval *obj, php_protocolbuffers_scheme_container *container TSRMLS_DC)
{
	zval fname, *retval_ptr = NULL;
	zend_string *name_key = zend_string_init(ZEND_STRL("__wakeup"),0);
	if (Z_OBJCE_P(obj) != PHP_IC_ENTRY &&
		zend_hash_exists(&Z_OBJCE_P(obj)->function_table, name_key)) {

//			INIT_PZVAL(&fname);
			ZVAL_STRINGL(&fname, "__wakeup", sizeof("__wakeup") -1);

			call_user_function_ex(CG(function_table), obj, &fname, retval_ptr, 0, 0, 1, NULL TSRMLS_CC);
	}

	if (retval_ptr) {
		zval_ptr_dtor(retval_ptr);
	}
	zend_string_release(name_key);
}

void php_protocolbuffers_execute_sleep(zval *obj, php_protocolbuffers_scheme_container *container TSRMLS_DC)
{
	zval fname, *retval_ptr = NULL;
	zend_string *name_key = zend_string_init(ZEND_STRL("__sleep"),0);
	if (Z_OBJCE_P(obj) != PHP_IC_ENTRY &&
		zend_hash_exists(&Z_OBJCE_P(obj)->function_table, name_key)) {

//		INIT_PZVAL(&fname);
		//ZVAL_STRINGL(&fname, ZEND_STRL("__sleep"));
		ZVAL_NEW_STR(&fname,name_key);

		call_user_function_ex(CG(function_table), obj, &fname, retval_ptr, 0, 0, 1, NULL TSRMLS_CC);

		if (retval_ptr) {
			if (Z_TYPE_P(retval_ptr) != IS_ARRAY) {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_execute_sleep failed. __sleep method have to return an array");
				zval_ptr_dtor(retval_ptr);
				retval_ptr = NULL;
			}
		} else {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "php_protocolbuffers_execute_sleep failed. __sleep method have to return an array");
		}
	}
	zend_string_release(name_key);
	if (retval_ptr) {
		zval *entry = NULL;
		HashPosition pos;
		php_protocolbuffers_scheme *scheme;
		int i;

		for (i = 0; i < container->size; i++) {
			scheme = &(container->scheme[i]);
			scheme->skip = 1;
		}

		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(retval_ptr), &pos);
		while ((entry=zend_hash_get_current_data_ex(Z_ARRVAL_P(retval_ptr), &pos)) != NULL) {
			for (i = 0; i < container->size; i++) {
				if (Z_TYPE_P(entry) != IS_STRING) {
					convert_to_string(entry);
				}

				scheme = &(container->scheme[i]);
				if (strcmp(ZSTR_VAL(scheme->name_key), Z_STRVAL_P(entry)) == 0) {
					scheme->skip = 0;
				}
			}

			zend_hash_move_forward_ex(Z_ARRVAL_P(retval_ptr), &pos);
		}

		zval_ptr_dtor(retval_ptr);
	}
}

int php_protocolbuffers_properties_init(zval *object, zend_class_entry *ce TSRMLS_DC)
{
	zval pp;
	int j = 0;
	php_protocolbuffers_scheme_container *container = NULL;
	php_protocolbuffers_scheme *scheme = NULL;
//	HashTable *properties = NULL;

	if (php_protocolbuffers_get_scheme_container(ce->name, &container TSRMLS_CC)) {
		return 1;
	}

//	ALLOC_HASHTABLE(properties);
//	zend_hash_init(properties, 0, NULL, ZVAL_PTR_DTOR, 0);

	if (container->use_single_property > 0) {
		array_init(&pp);
//		zend_string *orig_single_property_name_key = zend_string_init(container->orig_single_property_name, container->orig_single_property_name_len,0);
//		zend_hash_update(properties, orig_single_property_name_key, &pp);
		zend_update_property(ce,object,container->orig_single_property_name, container->orig_single_property_name_len,&pp);
//		zend_string_release(orig_single_property_name_key);
		zval_ptr_dtor(&pp);
	} else {
		for (j = 0; j < container->size; j++) {
			scheme= &container->scheme[j];

			if (scheme->repeated > 0) {
				array_init(&pp);
			} else {
				if(scheme->default_value!=NULL&&Z_TYPE_P(scheme->default_value) != IS_NULL){
					//ZVAL_ZVAL(&pp, scheme->default_value, 1, 0);
					ZVAL_COPY_VALUE(&pp, scheme->default_value);
				}else{
//					if (Z_TYPE_P(scheme->default_value) != IS_NULL) {
//						ZVAL_ZVAL(&pp, scheme->default_value, 1, 0);
//					} else {
						ZVAL_NULL(&pp);
//					}
				}
			}
			//zend_hash_update(properties, scheme->original_name_key, &pp);
			zend_update_property(ce,object,ZSTR_VAL(scheme->original_name_key),ZSTR_LEN(scheme->original_name_key),&pp);
			zval_ptr_dtor(&pp);
		}
	}

//	zend_merge_properties(object, properties);
//	FREE_HASHTABLE(properties);
	return 0;
}
