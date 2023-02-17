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
#if CFG_STRTAB_USE
#include "strtab.h"
#include <string.h>
#include <stdio.h>
#include <common/errordef.h>
#include <common/debug.h>
#include <nvram/nvol3.h>
#include <drivers/ramdrv.h>
#include <shell/corshell.h>
#if CFG_STRSUB_USE
#include <common/strsub.h>
#endif


typedef struct NVOL3_STRTAB_S {
    NVOL3_RECORD_HEAD_T     head ;
    uint32_t        key ;
    char            value[STRTAB_LENGT_MAX] ;

} NVOL3_STRTAB_T ;


NVOL3_UINT_INSTANCE_DECL(_repo3_string, \
                            ramdrv_read, ramdrv_write, ramdrv_erase, \
                            NVOL3_STRTAB_START, \
                            NVOL3_STRTAB_START + NVOL3_STRTAB_SECTOR_SIZE, \
                            NVOL3_STRTAB_SECTOR_SIZE, \
                            STRTAB_LENGT_MAX, \
                            0, \
                            17, \
                            0, \
                            NVOL3_SECTOR_VERSION) ;

static NVOL3_STRTAB_T       _strtab_buffer ;


#define STRTAB_LOCK_INIT()
#define STRTAB_LOCK()
#define STRTAB_UNLOCK()

#if CFG_STRSUB_USE
static int32_t strtab_strsub_cb(STRSUB_REPLACE_CB cb, const char * str, size_t len, uint32_t offset, uintptr_t arg) ;
static STRSUB_HANDLER_T _strtab_strsub ;
#endif

#define STRTAB_IS_LOADED()          (_repo3_string.dict ? 1 : 0)

/**
 * @brief       One time initialisation
 * @return      status
 */
int32_t
strtab_init(void)
{
    STRTAB_LOCK_INIT();
#if CFG_STRSUB_USE
    strsub_install_handler(0, StrsubToken3, &_strtab_strsub, strtab_strsub_cb) ;
#endif
    return EOK ;
}

/**
 * @brief       Start and load the string table lookup table.
 * @note        If it is not a valid string table it will be reset.
 * @return      status
 */
int32_t
strtab_start(void)
{
    int32_t status = 0 ;
    if (nvol3_validate(&_repo3_string) != EOK) {
        nvol3_reset (&_repo3_string) ;
    }
    status = nvol3_load (&_repo3_string) ;
    CORSHELL_CMD_LIST_INSTALL(strtab) ;


    return status ;
}

/**
 * @brief       Unload the string table and free all resources.
 * @return      status
 */
void
strtab_stop(void)
{
    DBG_CHECKV_T(STRTAB_IS_LOADED(), "strtab not loaded") ;
    CORSHELL_CMD_LIST_UNINSTALL(strtab) ;
    nvol3_unload (&_repo3_string) ;
}

/**
 * @brief       Reset the string table.
 * @return      status
 */
int32_t
strtab_erase(void)
{
    int32_t status = 0 ;
    DBG_CHECK_T(STRTAB_IS_LOADED(),  E_UNEXP, "strtab not loaded") ;
    status = nvol3_reset (&_repo3_string) ;
    return status ;
}

/**
 * @brief       Return the status of the entry in the stringtable
 * @return      status - EOK or E_NOTFOUND
 */
int32_t
strtab_valid (STRTAB_KEY_T key)
{
    DBG_CHECK_T(STRTAB_IS_LOADED(),  E_UNEXP, "strtab not loaded") ;
    return nvol3_record_status (&_repo3_string, (const char*)&key) ;
}

/**
 * @brief      get the value length
 * @param[in]   key
 * @return      length or < 0 (status)
 */
int32_t
strtab_length(STRTAB_KEY_T key)
{
    int32_t res ;
    DBG_CHECK_T(STRTAB_IS_LOADED(),  E_UNEXP, "strtab not loaded") ;
    STRTAB_LOCK() ;
    res = nvol3_record_key_and_data_length  (&_repo3_string, (const char*)&key) ;
    if (res > (int)sizeof(uint16_t)*2) {
        res -= (int)sizeof(uint16_t)*2 ;
    } else if (res > 0) {
        res = 0 ;
    }

    STRTAB_UNLOCK() ;

    return res ;

}

/**
 * @brief      get the value
 * @param[in]   id
 * @param[out]  value
 * @param[in]   length
 * @return      length or < 0 (status)
 */
int32_t
strtab_get(STRTAB_KEY_T key, char* value, int length)
{
    int32_t res ;
    DBG_CHECK_T(STRTAB_IS_LOADED(),  E_UNEXP, "strtab not loaded") ;
    if (!value) return E_PARM ;
    STRTAB_LOCK() ;
    _strtab_buffer.key = key ;

    if ((res = nvol3_record_get(&_repo3_string,
              (NVOL3_RECORD_T*)&_strtab_buffer)) > (int)sizeof(uint16_t)*2) {

        res -= sizeof(uint16_t)*2 ;
        if (res < length) {
            length = res ;
        }
        strncpy(value, (char*)_strtab_buffer.value, length) ;

    } else if (res >= 0) {
        res = E_INVAL ;

    }
    STRTAB_UNLOCK() ;

    return res ;
}

/**
 * @brief      set the value
 * @param[in]   id
 * @param[in]   value
 * @param[in]   length
 * @return      length or < 0 (status)
 */
int32_t
strtab_set(STRTAB_KEY_T key, const char* value, int length)
{
    int32_t res ;
    DBG_CHECK_T(STRTAB_IS_LOADED(),  E_UNEXP, "strtab not loaded") ;
    if (length > STRTAB_LENGT_MAX) {
        return E_PARM ;
    }
    STRTAB_LOCK() ;
    _strtab_buffer.key = key ;
    strncpy((char*)_strtab_buffer.value, value, length) ;
    res =  nvol3_record_set (&_repo3_string, (NVOL3_RECORD_T*)&_strtab_buffer, length + sizeof(uint16_t)*2) ;
    STRTAB_UNLOCK() ;

    return res ;
}

/*
 * Simple iterator. Should only be used by one client at a time!
 */
static NVOL3_ITERATOR_T _strtab_it ;

static int32_t
strtab_cmp (const char * first, const char * second)
{
    return  *((uint16_t*)first) - *((uint16_t*)second) ;
}

int32_t
strtab_first (STRTAB_KEY_T* key, char* value, int length)
{
    int32_t res ;
    (void)strtab_cmp ;
    DBG_CHECK_T(STRTAB_IS_LOADED(),  E_UNEXP, "strtab not loaded") ;
    STRTAB_LOCK() ;
    if ((res = nvol3_record_first (&_repo3_string,
               (NVOL3_RECORD_T*)&_strtab_buffer, &_strtab_it, strtab_cmp)) >
               (int)sizeof(uint16_t)*2) {
        res -= sizeof(uint16_t)*2 ;
        if (res < length) {
            length = res ;
        }
        strncpy(value, (char*)_strtab_buffer.value, length) ;
        *key = *(STRTAB_KEY_T*)dictionary_get_key(_repo3_string.dict, _strtab_it.it.np) ;


    }
    STRTAB_UNLOCK() ;
    return res ;
}

int32_t
strtab_next (STRTAB_KEY_T* key, char* value, int length)
{
    int32_t res ;
    DBG_CHECK_T(STRTAB_IS_LOADED(),  E_UNEXP, "strtab not loaded") ;
    STRTAB_LOCK() ;
    if ((res = nvol3_record_next (&_repo3_string, (NVOL3_RECORD_T*)&_strtab_buffer, &_strtab_it)) > (int)sizeof(uint16_t)*2) {
        res -= sizeof(uint16_t)*2 ;
        if (res < length) {
            length = res ;
        }
        strncpy(value, (char*)_strtab_buffer.value, length) ;
        //*key = (STRTAB_KEY_T)_strtab_it.it.np->key ;
        *key = *(STRTAB_KEY_T*)dictionary_get_key(_repo3_string.dict, _strtab_it.it.np) ;

    }
    STRTAB_UNLOCK() ;
    return res ;
}

void
strtab_log_status (void)
{
    if (STRTAB_IS_LOADED()) {
        nvol3_entry_log_status (&_repo3_string, 1) ;
    }

}


int32_t
strtab_strsub_cb(STRSUB_REPLACE_CB cb, const char * str, size_t len, uint32_t offset, uintptr_t arg)
{
    int32_t res = E_INVAL ;

    STRTAB_LOCK() ;

    if (sscanf(str, "%u", (unsigned int*)&_strtab_buffer.key)) {

        if ((res = nvol3_record_get(&_repo3_string, (NVOL3_RECORD_T*)&_strtab_buffer)) > (int)sizeof(uint16_t)*2) {

            res -= sizeof(uint16_t)*2 ;
            res = cb (_strtab_buffer.value, res, offset, arg) ;

        } else if (res >= 0) {
            res = E_INVAL ;

        }

    }


    STRTAB_UNLOCK() ;

    return res ;
}


#endif /* CFG_STRTAB_USE */
