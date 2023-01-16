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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>

#define DEBUG_MSG_SIZE 			128
#define DEBUG_SERIAL_DRIVER		&SD3


static char debug_msg[DEBUG_MSG_SIZE] ;

int
dbg_vprintf (const char *format_str, va_list	args)
{
	int32_t count = vsnprintf ((char*)debug_msg, DEBUG_MSG_SIZE, (char*)format_str, args) ;

	debug_msg [DEBUG_MSG_SIZE-1] = '\0';
	printf ("%s\r\n", (uint8_t*)debug_msg) ;

    return count ;

}

/**
 * @brief   	debug_printf
 * @details		prints a formatted string to the debug output.
 * @note
 *
 * @param[in] format	format string to print to debug output
 *
 * @return              number of characters printed
 *
 * @debug
 */
int
dbg_printf (const char *format, ...)
{
	va_list			args;

    va_start (args, format) ;

    return dbg_vprintf (format, args) ;
}

/**
 * @brief   	debug_put
 * @details		prints a string to the debug output.
 * @note
 *
 * @param[in] str	 string to print to debug output
 *
 *
 * @debug
 */
void
dbg_put (const char *str)
{

	printf (str) ;

    return  ;
}

void
dbg_assert (const char *format, ...)
{
	va_list			args;

    va_start (args, format) ;

    dbg_vprintf (format, args) ;

    assert (0) ;


}
