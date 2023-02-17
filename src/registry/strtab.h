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



#ifndef SERVICES_REGISTRY_STRTAB_H_
#define SERVICES_REGISTRY_STRTAB_H_

#include <stdint.h>

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

#define STRTAB_LENGT_MAX                            500  // max length of strings saved the strtab


/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

typedef uint16_t STRTAB_KEY_T ;


/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

int32_t     strtab_init(void) ;
int32_t     strtab_start(void) ;
void        strtab_stop(void) ;
int32_t     strtab_erase(void) ;

int32_t     strtab_valid (STRTAB_KEY_T key) ;
int32_t     strtab_length(STRTAB_KEY_T key) ;
int32_t     strtab_get(STRTAB_KEY_T key, char* value, int length) ;
int32_t     strtab_set(STRTAB_KEY_T key, const char* value, int length) ;

int32_t     strtab_first (STRTAB_KEY_T* key, char* value, int length) ;
int32_t     strtab_next (STRTAB_KEY_T* key, char* value, int length) ;

void        strtab_log_status (void) ;

#ifdef __cplusplus
}
#endif

#endif /* SERVICES_REGISTRY_STRTAB_H_ */
