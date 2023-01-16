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


#ifndef SERVICES_SHELL_CORSHELL_H_
#define SERVICES_SHELL_CORSHELL_H_

#include "system_config.h"


#include <stdint.h>
#include <stddef.h>


/*===========================================================================*/
/* Client pre-compile time settings.                                         */
/*===========================================================================*/

#define CORSHELL_NEWLINE            "\r\n"

#ifndef CORSHELL_STRSUB_USE
#define CORSHELL_STRSUB_USE         0
#endif

/*===========================================================================*/
/* Constants                                                                 */
/*===========================================================================*/

#define CORSHELL_OUT_NULL               (0)
#define CORSHELL_OUT_STD                (1)
#define CORSHELL_OUT_ERR                (2)
#define CORSHELL_OUT_SYS                (3)
#define CORSHELL_IN_STD                 (4)


#define CORSHELL_CMD_E_OK               (0)
#define CORSHELL_CMD_E_FAIL             (-1)
#define CORSHELL_CMD_E_NOT_FOUND        (-2)
#define CORSHELL_CMD_E_PARMS            (-3)
#define CORSHELL_CMD_E_BUSY             (-4)
#define CORSHELL_CMD_E_WOULD_BLOCK      (-5)
#define CORSHELL_CMD_E_NOT_IMPL         (-6)
#define CORSHELL_CMD_E_NOT_READY        (-7)
#define CORSHELL_CMD_E_MEMORY           (-8)
#define CORSHELL_CMD_E_BREAK            (-9)
#define CORSHELL_CMD_E_CANCEL           (-10)
#define CORSHELL_CMD_E_EXIST            (-11)

#define CORSHELL_ARGC_MAX               14
#define CORSHELL_PRINT_BUFFER_SIZE      384
#define CORSHELL_LINE_SIZE_MAX          256
#define CORSHELL_LINE_STRSUB_SIZE_MAX   320

#ifdef CFG_PORT_POSIX
#define ALIGN           __attribute__ ((aligned (32)))
#else
#define ALIGN
#endif


/*===========================================================================*/
/* Macros.                                                                   */
/*===========================================================================*/

#define CORSHELL_MALLOC(size)           malloc (size)
#define CORSHELL_FREE(mem)              free (mem)


/*===========================================================================*/
/* Client data structures and types.                                         */
/*===========================================================================*/

typedef int32_t (*CORSHELL_OUT_FP)(void* /*ctx*/, uint32_t /*out*/, const char* /*str*/);
typedef int32_t (*CORSHELL_CMD_FP)(void* /*ctx*/, CORSHELL_OUT_FP /*sip_out*/, char** /*argv*/, int /*argc*/);


typedef struct ALIGN CORSHELL_CMD_S {
    const char*         cmd ;
    CORSHELL_CMD_FP     fp ;
    const char*         usage ;
} CORSHELL_CMD_T ;


typedef struct CORSHELL_CMD_LIST_S {
    struct CORSHELL_CMD_LIST_S* next ;
    void *                      service ;
    const CORSHELL_CMD_T *      cmds;
    uint32_t                    cnt;
} CORSHELL_CMD_LIST_T ;



#define CORSHELL_CMD_LIST_START(name, service) \
        const CORSHELL_CMD_T    _corshell_##name[] ; \
        CORSHELL_CMD_LIST_T     _corshell_##name##_list = { 0, service, _corshell_##name } ; \
        const CORSHELL_CMD_T    _corshell_##name[] = {
#define CORSHELL_CMD_LIST(name, function, usage)    { name, function, usage },
#define CORSHELL_CMD_LIST_END() {0, 0, 0}   } ;

#define CORSHELL_CMD_LIST_INSTALL(name) \
        extern CORSHELL_CMD_LIST_T  _corshell_##name##_list ; \
        corshell_install (&_corshell_##name##_list) ;

#if 1
#define CORSHELL_CMD_DECL(name, function, usage)        \
    const CORSHELL_CMD_T    \
    __coralcmd_##function   ALIGN \
    __attribute__((used))  \
     __attribute__((section(".rodata.cmds." #function))) =      \
    { name,             \
    function,                       \
    usage                   \
    }
#else
#define CORSHELL_CMD_DECL(name, function, usage)
#endif

typedef struct CORSHELL_OUT_S {
    void*               ctx ;
    uint32_t            flags ;
    CORSHELL_OUT_FP     shell_out ;
} CORSHELL_OUT_T ;


/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

    /*===========================================================================*/
    /* corshell  interface.                                                    */
    /*===========================================================================*/

    extern int32_t      corshell_init (void) ;

    extern size_t       corshell_cmd_split(char *buffer, size_t len, char *argv[], size_t argv_size) ;
    extern int32_t      corshell_cmd_run (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
    extern int32_t      corshell_script_run (void* ctx, CORSHELL_OUT_FP shell_out, const char* name, char* start, int length) ;
    extern const char*  corshell_script_get_last_error () ;
    extern int32_t      corshell_script_clear_last_error () ;

    extern uint32_t     corshell_cmd_help (char *buffer, size_t len) ;
    extern uint32_t     corshell_install (CORSHELL_CMD_LIST_T * list) ;
    extern uint32_t     corshell_uninstall (CORSHELL_CMD_LIST_T * list) ;

    extern int32_t      corshell_scan_int (const char * str, uint32_t * val) ;
    extern int32_t      corshell_print(void* ctx, uint32_t out, CORSHELL_OUT_FP shell_out, const char * fmtstr, ...) ;
    extern int32_t      corshell_ts_print(void* ctx, uint32_t out, CORSHELL_OUT_FP shell_out, const char * fmtstr, ...) ;
    extern int32_t      corshell_print_table(void* ctx, uint32_t out, CORSHELL_OUT_FP shell_out, const char * left, int32_t tabright, const char * fmtstr, ...) ;



#ifdef __cplusplus
}
#endif

#endif /* SERVICES_SHELL_CORSHELL_H_ */
