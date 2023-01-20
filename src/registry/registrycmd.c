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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "registry.h"
#include "shell/corshell.h"
#include "common/errordef.h"

static int32_t      corshell_reg (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t      corshell_regadd (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t      corshell_regdel (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t      corshell_regstats (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t      corshell_regerase (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t      corshell_regtest (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;


CORSHELL_CMD_LIST_START(registry, 0)
CORSHELL_CMD_LIST("reg", corshell_reg, "[key] [value]")
CORSHELL_CMD_LIST("regadd", corshell_regadd, "<key> <value>")
CORSHELL_CMD_LIST("regdel", corshell_regdel, "<key>")
CORSHELL_CMD_LIST("regstats", corshell_regstats, "")
CORSHELL_CMD_LIST("regerase", corshell_regerase, "")
CORSHELL_CMD_LIST("regtest", corshell_regtest, "[repeat]")
CORSHELL_CMD_LIST_END()



void
reg_print (void* ctx, CORSHELL_OUT_FP shell_out, REGISTRY_KEY_T key, char* value, int length)
{
    char tmp[40] ;
    snprintf(tmp, 40, "%s:", key) ;
    corshell_print_table(ctx, CORSHELL_OUT_STD, shell_out,
            tmp, 24, "%s" CORSHELL_NEWLINE, value) ;
}

static int32_t
reg_show (void* ctx, CORSHELL_OUT_FP shell_out, const char * search, char * value, uint32_t len)
{
    int32_t found = EFAIL ;
    REGISTRY_KEY_T key ;
    int32_t res = registry_first (&key, value, len) ;
    while (res >= 0) {

        if (!search || strstr(key, search)) {

            reg_print (ctx, shell_out, key, value, len) ;

            found = EOK ;
        }
        res = registry_next (&key, value, len) ;

    }

    return found ;
}

static int32_t
corshell_reg (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{

    char value[REGISTRY_VALUE_LENGT_MAX] ;
    int32_t res = CORSHELL_CMD_E_OK ;

    if (argc == 1) {
        res = reg_show (ctx, shell_out, 0, value, REGISTRY_VALUE_LENGT_MAX) ;

    }
    else if (argc == 2) {

        res = registry_value_get (argv[1], value, REGISTRY_VALUE_LENGT_MAX) ;
        if (res > 0) {
            reg_print (ctx, shell_out, argv[1], value, REGISTRY_VALUE_LENGT_MAX) ;

        } else {
            res = reg_show (ctx, shell_out, argv[1], value, REGISTRY_VALUE_LENGT_MAX) ;

        }

    }
    else if (argc == 3) {

        res = registry_value_set (argv[1], argv[2], strlen(argv[2])+ 1) ;

        corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
            "%s" CORSHELL_NEWLINE, res == EOK ? "OK" : "ERR") ;

    }


    return res ;
}

static int32_t
corshell_regadd (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    char value[REGISTRY_VALUE_LENGT_MAX] ;
    int32_t res = CORSHELL_CMD_E_OK ;

    if (argc < 3) {
        return CORSHELL_CMD_E_PARMS ;

    }

    res = registry_value_get (argv[1], value, REGISTRY_VALUE_LENGT_MAX) ;
    if (res >= 0 ) {
        corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
            "registry setting %s exists" CORSHELL_NEWLINE, argv[1]) ;

        return CORSHELL_CMD_E_EXIST ;
    }


    res = registry_value_set (argv[1], argv[2],  strlen (argv[2]) + 1) ;

    corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
        "%s" CORSHELL_NEWLINE, res == EOK ? "OK" : "ERR") ;

    return res ;
}

static int32_t
corshell_regdel (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    int32_t res ;

    if (argc != 2) {
        return CORSHELL_CMD_E_PARMS ;

    }

    res = registry_value_delete (argv[1]) ;
    corshell_print(ctx, CORSHELL_OUT_STD, shell_out, "%s\r\n", res == EOK ? "OK" : "ERR") ;

    return CORSHELL_CMD_E_OK ;
}

static int32_t
corshell_regstats (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    registry_log_status () ;
    return CORSHELL_CMD_E_OK ;
}


static int32_t
corshell_regerase (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    registry_erase () ;
    return CORSHELL_CMD_E_OK ;
}



int32_t
corshell_regtest (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    unsigned int repeat = 100 ;
    char writeval[16] = {0} ;
    char readval[16] = {0} ;
    unsigned int intval = 0 ;


    if (argc > 1) {
        sscanf(argv[1], "%u", &repeat) ;

    }

    corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
              "registry testing %d times...\r\n", repeat) ;

    snprintf(writeval, 20, "%.12u", intval++) ;
    int32_t res = registry_value_set ("___test___", writeval,
                        strlen(writeval)+1) ;

    if (res < 0) {
        corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
                 "create return %d\r\n", res) ;
        return CORSHELL_CMD_E_FAIL ;

    }

    while (repeat--) {
        snprintf(writeval, 20, "%.12u", intval) ;
        int32_t res = registry_value_set ("___test___", writeval,
                            strlen(writeval)+1) ;
        if (res < 0) {
            corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
                     "set return %d\r\n", res) ;
            break ;

        }
        res = registry_value_get ("___test___", readval, 16) ;
        if (res < 0) {
            corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
                     "get return %d\r\n", res) ;
            break ;

        }
        if (strcmp(writeval, readval)) {
            corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
                     "get %s expected %s\r\n", writeval, readval) ;
            break ;

        }

        intval++ ;

    }

    res = registry_value_delete ("___test___") ;
    if (res < 0) {
         corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
                  "delete return %d\r\n", res) ;
         return CORSHELL_CMD_E_FAIL ;

     }

    corshell_print(ctx, CORSHELL_OUT_STD, shell_out,
              "done\r\n") ;


    return CORSHELL_CMD_E_OK ;

}

#endif /* CFG_REGISTRY_USE */
