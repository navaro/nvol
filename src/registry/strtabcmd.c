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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "registry.h"
#include "shell/corshell.h"
#include "common/errordef.h"


static int32_t corshell_strtab (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t corshell_strtabchk (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t corshell_strtaberase (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t corshell_strtabstats (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;


CORSHELL_CMD_LIST_START(strtab, 0)
CORSHELL_CMD_LIST(  "strtab", corshell_strtab,  "[idx] [value]")
CORSHELL_CMD_LIST(  "strtabchk", corshell_strtabchk,  "<idx> <value>")
CORSHELL_CMD_LIST(  "strtaberase", corshell_strtaberase,  "")
CORSHELL_CMD_LIST(  "strtabstats", corshell_strtabstats,  "")
CORSHELL_CMD_LIST_END()


int32_t corshell_strtab (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{

    STRTAB_KEY_T key ;
    char* value[STRTAB_LENGT_MAX] ;
    int32_t status ;

    if (argc >= 2) {
        int id = 0 ;
        sscanf(argv[1], "%d", &id) ;
        key = (uint16_t) id ;

    }

    if (argc == 2) {

        if ((status = strtab_get(key, (char*)value, STRTAB_LENGT_MAX)) > 0) {
            corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                    "%s\r\n",
                    value) ;

        } else {
            corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                    "Error %d\r\n",
                    status) ;

         }

    } else if (argc > 2) {



        if ((status = strtab_set(key, (char*)argv[2], strlen(argv[2])+1)) == EOK) {
            corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                    "OK\r\n") ;
        } else {
            corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                    "Error %d\r\n",
                    status) ;

        }


    } else {

        int res ;
        uint32_t cnt = 0 ;
        res = strtab_first (&key, (char*)value, STRTAB_LENGT_MAX) ;

        while (res > 0) {

            corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                    "%.4d   %s\r\n",
                    key, value) ;
            cnt++ ;

            res = strtab_next (&key, (char*)value, STRTAB_LENGT_MAX) ;

        }
        corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
            "\r\n    %d entries found." CORSHELL_NEWLINE, cnt) ;

    }

    return CORSHELL_CMD_E_OK ;
}

int32_t corshell_strtabchk (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    if (argc > 2) {

        STRTAB_KEY_T key = 0 ;
        int32_t status ;

            int id = 0 ;
            sscanf(argv[1], "%d", &id) ;
            key = (uint16_t) id ;

        if (strtab_valid(key) == EOK) {
            shell_out (ctx, CORSHELL_OUT_STD, "Already set\r\n") ;
        } else if ((status = strtab_set(key, (char*)argv[2], strlen((char*)argv[2])+1)) == EOK) {
            shell_out (ctx, CORSHELL_OUT_STD, "    OK\r\n") ;
        } else {
            corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                    "Error %d\r\n",
                    status) ;

        }


    } else   {

        return corshell_strtab (ctx, shell_out, argv, argc) ;

    }

    return CORSHELL_CMD_E_OK ;
}

int32_t corshell_strtaberase (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    int32_t status = strtab_erase () ;

    if (status != EOK) {
        corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                "Error %d\r\n",
                status) ;


    } else {
        shell_out (ctx, CORSHELL_OUT_STD, "    OK\r\n") ;

    }

    return CORSHELL_CMD_E_OK ;
}

static int32_t
corshell_strtabstats (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    registry_log_status () ;
    return CORSHELL_CMD_E_OK ;
}


#endif /* CFG_REGISTRY_USE */
