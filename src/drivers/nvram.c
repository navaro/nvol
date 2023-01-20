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
#include <string.h>
#include <stdint.h>
#include <common/errordef.h>

#include "nvram.h"


#if !CFG_PLATFORM_SPIFLASH
#define NVRAM_SIZE      (       \
        NVOL3_REGISTRY_SECTOR_SIZE*NVOL3_REGISTRY_SECTOR_COUNT \
        )
static uint8_t          _nvram_test[NVRAM_SIZE] PLATFORM_SECTION_NOINIT ;
#endif

int32_t
nvram_init (void)
{

    return EOK;
}

int32_t
nvram_start (void)
{

    return EOK ;
}

int32_t
nvram_stop (void)
{

    return EOK ;
}


#if !CFG_PLATFORM_SPIFLASH
int32_t
nvram_erase (uint32_t addr_start, uint32_t addr_end)
{
    if (addr_end < addr_start) return E_PARM ;
    if (addr_start >= NVRAM_SIZE) return E_PARM ;
    if (addr_end >= NVRAM_SIZE) {
        addr_end = NVRAM_SIZE - 1 ;
    }
    memset ((void*)(_nvram_test + addr_start), 0xFF, addr_end - addr_start) ;

    return EOK ;
}

int32_t
nvram_write (uint32_t addr, uint32_t len, uint8_t * data)
{
    uint32_t i ;
    if (addr >= NVRAM_SIZE) return E_PARM ;
    if (addr + len >= NVRAM_SIZE) return E_PARM ;

    for (i=0; i<len; i++) {
        _nvram_test[i+addr] &= data[i] ;
    }


    // memcpy ((void*)(_nvram_test + addr), data, len) ;

    return EOK ;
}

int32_t
nvram_read (uint32_t addr, uint32_t len, uint8_t * data)
{
    if (addr >= NVRAM_SIZE) return E_PARM ;
    if (addr + len >= NVRAM_SIZE) return E_PARM ;

    memcpy (data, (void*)(_nvram_test + addr), len) ;

    return EOK ;
}
#endif

