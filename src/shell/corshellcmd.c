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



#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "corshell.h"

static int32_t corshell_version (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t corshell_date (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;

CORSHELL_CMD_DECL(  "version", corshell_version, "" );
CORSHELL_CMD_DECL(  "date", corshell_date, "" );


int32_t corshell_version (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    printf ("corshell version %s - %s\r\n", __DATE__, __TIME__) ;
    return CORSHELL_CMD_E_OK ;
}

int32_t corshell_date (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("%d-%02d-%02d %02d:%02d:%02d\r\n",
       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
       tm.tm_hour, tm.tm_min, tm.tm_sec);

    return CORSHELL_CMD_E_OK ;
}

