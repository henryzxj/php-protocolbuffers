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

#ifndef PHP_PROTOCOLBUFFERS_UNKNOWN_FIELD_SET_H
#define PHP_PROTOCOLBUFFERS_UNKNOWN_FIELD_SET_H

void php_protocolbuffers_unknown_field_set_class(TSRMLS_D);

void php_protocolbuffers_unknown_field_set_add_field(INTERNAL_FUNCTION_PARAMETERS, zval *instance, int number, zval *field);

int php_protocolbuffers_unknown_field_get_field(INTERNAL_FUNCTION_PARAMETERS, zval *instance, int number, char *name, int name_len, php_protocolbuffers_unknown_field **f);

void php_protocolbuffers_unknown_field_clear(INTERNAL_FUNCTION_PARAMETERS, zval *instance);

void php_protocolbuffers_unknown_field_set_properties_init(zval *object TSRMLS_DC);

static zend_object_handlers php_protocolbuffers_unknown_field_set_object_handlers;
#endif
