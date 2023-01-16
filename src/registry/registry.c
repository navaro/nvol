/*
    Copyright (C) 2015-2023, Navaro, All Rights Reserved
    SPDX-License-Identifier: MIT

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 */

#include "system_config.h"
#if CFG_REGISTRY_USE
#include "registry.h"
#include "common/errordef.h"
#include "common/debug.h"
#include "nvram/nvol3.h"
#include "shell/corshell.h"
#if CFG_STRSUB_USE
#include "common/strsub.h"
#endif


NVOL3_INSTANCE_DECL(_regdef_nvol3_entry, \
		STORAGE_NVOL3_REGISTRY_START, \
		STORAGE_NVOL3_REGISTRY_START + STORAGE_NVOL3_REGISTRY_SECTOR_SIZE, \
		STORAGE_NVOL3_REGISTRY_SECTOR_SIZE, \
		REGISTRY_KEY_LENGTH /*key_size*/, \
		DICTIONARY_KEYSPEC_BINARY(6) /*key_type (24 character string)*/, \
		101 /*hashsize*/, \
		REGISTRY_VALUE_LENGT_MAX /*data_size*/, \
		0 /*local_size*/, \
		0 /*tallie*/, \
		NVOL3_SECTOR_VERSION /*version*/) ;



typedef struct NVOL3_REGISTRY_S {
	NVOL3_RECORD_HEAD_T 	head ; /* 8 bytes */
	char 					key[REGISTRY_KEY_LENGTH] ; /* 24 bytes */
	char					value[REGISTRY_VALUE_LENGT_MAX] ; /* 224 bytes */

} NVOL3_REGISTRY_T ;  /* 256 bytes */

#define REGISTRY_KEY_TYPE_LEN		((int32_t)(REGISTRY_KEY_LENGTH))

NVOL3_REGISTRY_T _registry_value ;
char _registry_key[REGISTRY_KEY_LENGTH+1] ;


#if CFG_STRSUB_USE
static int32_t registry_strsub_cb(STRSUB_REPLACE_CB cb, const char * str, size_t len, uint32_t offset, uintptr_t arg) ;
static STRSUB_HANDLER_T _registry_strsub ;
#endif

static inline void
_setkey (NVOL3_REGISTRY_T* entry, REGISTRY_KEY_T key)
{
	strncpy (entry->key, key, REGISTRY_KEY_LENGTH) ;
}


int32_t
registry_init (void)
{
	return EOK ;
}

int32_t
registry_start (void)
{
	int32_t status = 0 ;

	if (nvol3_validate(&_regdef_nvol3_entry) != EOK) {
		DBG_MESSAGE_REGISTRY( DBG_MESSAGE_SEVERITY_REPORT, "REG   : : resetting _regdef_nvol3_entry")
		status = nvol3_reset (&_regdef_nvol3_entry) ;

	} else {
		status = nvol3_load (&_regdef_nvol3_entry) ;

	}

#if CFG_STRSUB_USE
	strsub_install_handler(0, StrsubToken1, &_registry_strsub, registry_strsub_cb) ;
#endif

	CORSHELL_CMD_LIST_INSTALL(registry) ;

	return status ;
}

int32_t
registry_erase (void)
{
	int32_t status = 0 ;
	status = nvol3_reset (&_regdef_nvol3_entry) ;

	return status ;
}

int32_t
registry_value_delete (REGISTRY_KEY_T id)
{
	int32_t res = EFAIL;
	DBG_CHECK_T(id, E_PARM, "registry_value_delete id") ;

	_setkey (&_registry_value, id) ;
    res = nvol3_record_delete(&_regdef_nvol3_entry, (NVOL3_RECORD_T*)&_registry_value) ;

	return res ;
}

bool
registry_value_valid (REGISTRY_KEY_T id)
{
	bool res = false ;
	DBG_CHECK_T(id, E_PARM, "registry_value_valid id") ;

	_setkey (&_registry_value, id) ;
    if (nvol3_record_get(&_regdef_nvol3_entry, (NVOL3_RECORD_T*)&_registry_value) > REGISTRY_KEY_TYPE_LEN) {
    	res = true ;
	}

	return res ;

}

int32_t
registry_value_length (REGISTRY_KEY_T id)
{
	int32_t res ;
	DBG_CHECK_T(id, E_PARM, "registry_value_length id") ;
	_setkey (&_registry_value, id) ;
	res = nvol3_record_key_and_data_length  (&_regdef_nvol3_entry, (const char*)_registry_value.key) ;
	if (res > REGISTRY_KEY_TYPE_LEN) {
		res -= REGISTRY_KEY_TYPE_LEN ;
	} else {
		res = 0 ;
	}

	return res ;
}

int32_t
registry_value_get (REGISTRY_KEY_T id, char* value, unsigned int length)
{
	int32_t res ;
	//DBG_CHECK_T(value, E_PARM, "registry_value_get val") ;
	DBG_CHECK_T(id, E_PARM, "registry_value_get id") ;

	_setkey (&_registry_value, id) ;
	memset(value, 0, length) ;
    if ((res = nvol3_record_get(&_regdef_nvol3_entry, (NVOL3_RECORD_T*)&_registry_value)) > REGISTRY_KEY_TYPE_LEN) {
    	res -= REGISTRY_KEY_TYPE_LEN ;
       	if (value && (length > 0)) {
			res = (int)length <= res ? (int)length : res ;
			memcpy(value, _registry_value.value, res) ;
       	}
	} else if (res >= 0) {
		res = E_INVAL ;

	}

    return res ;
}

int32_t
registry_value_set (REGISTRY_KEY_T id, const char* value, unsigned int length)
{
	int32_t res ;
	DBG_CHECK_T(value, E_PARM, "registry_value_set val") ;
	DBG_CHECK_T(id, E_PARM, "registry_value_set id") ;
	if (length > REGISTRY_VALUE_LENGT_MAX) return E_PARM ;
 	_setkey (&_registry_value, id) ;
    memcpy(_registry_value.value, value, length) ;

    res =  nvol3_record_set (&_regdef_nvol3_entry, (NVOL3_RECORD_T*)&_registry_value, length + REGISTRY_KEY_TYPE_LEN) ;


	return res ;

}


static NVOL3_ITERATOR_T _registry_it ;

int32_t
registry_first (REGISTRY_KEY_T* key, char* value, int length)
{
	int32_t res ;
	DBG_CHECK_T(value, E_PARM, "registry_first val") ;
	DBG_CHECK_T(key, E_PARM, "registry_first id") ;
	if ((res = nvol3_record_first (&_regdef_nvol3_entry, (NVOL3_RECORD_T*)&_registry_value, &_registry_it)) > REGISTRY_KEY_TYPE_LEN) {
		res -= REGISTRY_KEY_TYPE_LEN ;
		if (res < length) {
			length = res ;
		}
		memcpy(value, (char*)_registry_value.value, length) ;
		strncpy(_registry_key, _registry_value.key, REGISTRY_KEY_LENGTH) ;
		_registry_key[REGISTRY_KEY_LENGTH] = '\0' ;
		*key = _registry_key  ;

	} else if (res >= 0) {
		res = E_INVAL ;

	}

	return res ;
}

int32_t
registry_next (REGISTRY_KEY_T* key, char* value, int length)
{
	int32_t res ;
	DBG_CHECK_T(value, E_PARM, "registry_next val") ;
	DBG_CHECK_T(key, E_PARM, "registry_next id") ;
	if ((res = nvol3_record_next (&_regdef_nvol3_entry, (NVOL3_RECORD_T*)&_registry_value, &_registry_it)) > REGISTRY_KEY_TYPE_LEN) {
		res -= REGISTRY_KEY_TYPE_LEN ;
		if (res < length) {
			length = res ;
		}
		memcpy(value, (char*)_registry_value.value, length) ;
		strncpy(_registry_key, _registry_value.key, REGISTRY_KEY_LENGTH) ;
		_registry_key[REGISTRY_KEY_LENGTH] = '\0' ;
		*key = _registry_key  ;

	} else if (res >= 0) {
		res = E_INVAL ;

	}

	return res ;
}



void
registry_log_status (void)
{
	nvol3_entry_log_status (&_regdef_nvol3_entry, 1) ;
}



#if CFG_STRSUB_USE
int32_t
registry_strsub_cb (STRSUB_REPLACE_CB cb, const char * str, size_t len, uint32_t offset, uintptr_t arg)
{
	int32_t res  ;



	memset (&_registry_value, 0, sizeof(_registry_value)) ;
	strncpy (_registry_value.key, str, len) ;

    if ((res = nvol3_record_get(&_regdef_nvol3_entry, (NVOL3_RECORD_T*)&_registry_value)) > REGISTRY_KEY_TYPE_LEN) {
    	res -= REGISTRY_KEY_TYPE_LEN ;
    	res = cb (_registry_value.value, res, offset, arg) ;

 	} else if (res >= 0) {
		res = E_INVAL ;

	}

    return res ;
}
#endif

#endif /* CFG_REGISTRY_USE */
