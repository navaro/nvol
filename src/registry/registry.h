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

#ifndef __REGISTRY_REGISTRY_H__
#define __REGISTRY_REGISTRY_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define DBG_MESSAGE_REGISTRY(severity, fmt_str, ...)    	DBG_MESSAGE_T_REPORT (DBG_MESSAGE_LOGGER_TYPE(severity,0), 0, fmt_str, ##__VA_ARGS__)
#define DBG_ASSERT_REGISTRY                               	DBG_ASSERT_T
#define DBG_CHECK_REGISTRY                               	DBG_ASSERT_T

/*===========================================================================*/
/* Constants.                                                                */
/*===========================================================================*/

#define REGISTRY_KEY_LENGTH					24
#define REGISTRY_VALUE_LENGT_MAX			224

/*===========================================================================*/
/* Data structures and types.                                                */
/*===========================================================================*/

typedef const char* REGISTRY_KEY_T ;


/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

	int32_t		registry_init (void) ;
	int32_t		registry_start (void) ;
	int32_t		registry_erase (void) ;

	bool 		registry_value_valid (REGISTRY_KEY_T id) ;
	int32_t 	registry_value_length (REGISTRY_KEY_T id) ;
	int32_t		registry_value_get (REGISTRY_KEY_T id, char* value, unsigned int length) ;
	int32_t		registry_value_set (REGISTRY_KEY_T id, const char* value,  unsigned int length) ;
	int32_t 	registry_value_delete (REGISTRY_KEY_T id) ;

	int32_t		registry_first (REGISTRY_KEY_T* key, char* value, int length) ;
	int32_t		registry_next (REGISTRY_KEY_T* key, char* value, int length) ;

	void		registry_log_status (void) ;



#ifdef __cplusplus
}
#endif

#endif /* __REGISTRY_REGISTRY_H__ */
