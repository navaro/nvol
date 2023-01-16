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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "corshell.h"
#if CFG_STRSUB_USE
#include "common/strsub.h"
#endif


#define USE_ONERROR         1

int32_t _corshell_status = 0 ;

static int32_t corshell_help (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t corshell_help2 (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t corshell_rem (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;
static int32_t corshell_nop (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc) ;


CORSHELL_CMD_DECL( "help", corshell_help, "[filter]");
CORSHELL_CMD_DECL( "?", corshell_help2, 0);
CORSHELL_CMD_DECL( "rem", corshell_rem, 0);
CORSHELL_CMD_DECL( "nop", corshell_nop, "[res]");


char _corshell_buffer[CORSHELL_PRINT_BUFFER_SIZE]  ;

typedef struct CORSHELL_CMD_LIST_IT_S {
    CORSHELL_CMD_LIST_T*        lst ;
    uint32_t idx ;
} CORSHELL_CMD_LIST_IT_T ;



static CORSHELL_CMD_LIST_T _corshell_static_list = {
        0,
        0,
        0,
        0
};


extern char __corshell_cmds_base__ ;
extern char __corshell_cmds_end__ ;



int32_t
corshell_init (void)
{
    if (((uintptr_t)&__corshell_cmds_end__ - (uintptr_t)&__corshell_cmds_base__) / sizeof(CORSHELL_CMD_T)) {
        _corshell_static_list.cmds = (CORSHELL_CMD_T *)&__corshell_cmds_base__;
        _corshell_static_list.cnt = ((uintptr_t)&__corshell_cmds_end__ - (uintptr_t)&__corshell_cmds_base__) / sizeof(CORSHELL_CMD_T) ;
    }

    return CORSHELL_CMD_E_OK ;
}

const CORSHELL_CMD_T*
_cmd_first (CORSHELL_CMD_LIST_IT_T * it)
{
    it->idx = 0 ;
    it->lst = &_corshell_static_list ;

    return &it->lst->cmds[it->idx] ;
}

const CORSHELL_CMD_T*
_cmd_next (CORSHELL_CMD_LIST_IT_T * it)
{
    it->idx++ ;
    if (
            !it->lst ||
            (it->lst->cnt && (it->idx >= it->lst->cnt)) ||
            (it->lst->cmds[it->idx].cmd == 0)
        ) {
        if (it->lst->next == 0) {
            return 0 ;
        }
        it->lst = it->lst->next ;
        it->idx = 0 ;
    }

    return &it->lst->cmds[it->idx] ;
}

const CORSHELL_CMD_T*
_cmd_get (CORSHELL_CMD_LIST_IT_T * it)
{
    return &it->lst->cmds[it->idx] ;
}

int
_cmd_cmp (CORSHELL_CMD_LIST_IT_T * it1, CORSHELL_CMD_LIST_IT_T * it2)
{
    const CORSHELL_CMD_T * cmd1 = &it1->lst->cmds[it1->idx] ;
    const CORSHELL_CMD_T * cmd2 = &it2->lst->cmds[it2->idx] ;

    return strcmp(cmd1->cmd, cmd2->cmd) ;
}

void
_cmd_help (void* ctx, CORSHELL_OUT_FP shell_out,
        CORSHELL_CMD_LIST_IT_T * it, const char * filter)
{
    const CORSHELL_CMD_T*cmd = _cmd_get(it) ;

    if (cmd->usage) {
        if (!filter || (filter && strstr (cmd->cmd, filter))) {
            corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                    "%s %s" CORSHELL_NEWLINE, cmd->cmd, cmd->usage) ;
        }
    }
}


uint32_t
corshell_install (CORSHELL_CMD_LIST_T * list)
{
    CORSHELL_CMD_LIST_T * l = &_corshell_static_list ;

    while (l->next != 0) {
        l = l->next ;
    }

    list->next = 0 ;
    l->next = list ;

    return CORSHELL_CMD_E_OK ;
}

uint32_t
corshell_uninstall (CORSHELL_CMD_LIST_T * list)
{
    CORSHELL_CMD_LIST_T * l = &_corshell_static_list ;
    CORSHELL_CMD_LIST_T * prev = 0 ;

    for (  ; (l!=0) && (l!=list) ; ) {

        prev = l ;
        l = l->next;

    }

    if ((l == list) && prev) {
            prev->next = l->next ;

    }

    return CORSHELL_CMD_E_OK ;
}


int32_t corshell_print_table (void* ctx, uint32_t out, CORSHELL_OUT_FP shell_out,
        const char * left, int32_t tabright, const char * fmtstr, ...)
{
    va_list         args;
    va_start (args, fmtstr) ;

    int count = snprintf ((char*)_corshell_buffer, CORSHELL_PRINT_BUFFER_SIZE - 3, "%s", (char*)left) ;
    do {
        _corshell_buffer[count++] = ' ' ;
    } while ((count < tabright) && (count < CORSHELL_PRINT_BUFFER_SIZE - 3)) ;
    count += vsnprintf ((char*)&_corshell_buffer[count], CORSHELL_PRINT_BUFFER_SIZE - count, (char*)fmtstr, args) ;
    shell_out (ctx, CORSHELL_OUT_ERR, _corshell_buffer) ;
    return count ;
}

int32_t corshell_print (void* ctx, uint32_t out, CORSHELL_OUT_FP shell_out,
        const char * fmtstr, ...)
{
    va_list         args;
    va_start (args, fmtstr) ;

    int32_t count = vsnprintf ((char*)_corshell_buffer, CORSHELL_PRINT_BUFFER_SIZE, (char*)fmtstr, args) ;
    shell_out (ctx, out, _corshell_buffer) ;

    return count ;
}

#if 0
int32_t corshell_ts_print (void* ctx, uint32_t out, CORSHELL_OUT_FP shell_out,
        const char * fmtstr, ...)
{
    va_list         args;
    va_start (args, fmtstr) ;

    unsigned int now = os_sys_timestamp() ;
    snprintf(_corshell_buffer, CORSHELL_PRINT_BUFFER_SIZE, "[%.6u.%.3u]  ", now/1000, now%1000) ;
    shell_out (ctx, out, _corshell_buffer) ;

    int32_t count = vsnprintf ((char*)_corshell_buffer, CORSHELL_PRINT_BUFFER_SIZE, (char*)fmtstr, args) ;
    shell_out (ctx, out, _corshell_buffer) ;

    return count ;
}
#endif

int32_t corshell_scan_int (const char * str, uint32_t * val)
{
    uint32_t i = 0 ;
    int32_t type = 0;

    if ((str[0] == '+') || (str[0] == '-')) {
        i = 1 ;

    }
    if ((str[0] == '0') && (str[1] == 'x')) {
        type = 1 ;
        i = 2 ;

    }

    while (str[i]) {
        if ((type == 0) && !isdigit((int)str[i])) {
            type = 1 ;

        }
        if ((type == 1) && !isxdigit((int)str[i])) {
            type = 2 ;
            break ;

        }
        i++ ;

    }

    if (type == 0) {
        sscanf(str, "%i", (int*)val) ;

    }
    else if (type == 1) {
        if ((str[0] == '0') && (str[1] == 'x')) {
            sscanf(str, "0x%x", (unsigned int*)val) ;

        } else {
            sscanf(str, "%x", (unsigned int*)val) ;

        }

    } else {
        return CORSHELL_CMD_E_FAIL ;

    }

    return CORSHELL_CMD_E_OK ;
}


int32_t
corshell_cmd_run (void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    //unsigned int i ;
    CORSHELL_CMD_LIST_T * list = 0 ;
    CORSHELL_CMD_LIST_IT_T it ;
    const CORSHELL_CMD_T * cmd ;
    int started  ;

    for (cmd = _cmd_first(&it); cmd; cmd = _cmd_next(&it)) {
        if (*argv[0] == '#') {
            return CORSHELL_CMD_E_OK ;
        }

        if (list != it.lst) {
            list = it.lst ;
            started = -1 ;

        }

        if (strcmp (cmd->cmd, argv[0]) == 0) {
            int32_t res = CORSHELL_CMD_E_OK ;
            int32_t usage = 0 ;
            if ((argc <= 1) || (*argv[1] != '?')) {
#if CFG_PLATFORM_SVC_SERVICES
                if ((started < 0) && list->service) {
                    started = svc_service_status(svc_service_get(list->service))
                            >= SVC_SERVICE_STATUS_STARTED ;
                }
#endif
                if (started != 0) {
                    res = cmd->fp (ctx, shell_out, argv, argc) ;

                } else {
                    res = CORSHELL_CMD_E_NOT_READY ;

                }

            } else {
                usage = 1 ;

            }

            if (cmd->usage && (usage || (res == CORSHELL_CMD_E_PARMS))) {
                corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                        "usage: '%s %s'" CORSHELL_NEWLINE,
                        cmd->cmd, cmd->usage) ;

            }
#if CFG_PLATFORM_SVC_SERVICES
            else if (!started) {
                corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                        "'%s' require service '%s'" CORSHELL_NEWLINE,
                        cmd->cmd, svc_service_name(svc_service_get(list->service))) ;

            }
#endif
            else if (res < CORSHELL_CMD_E_OK) {
                //corshell_print (ctx, CORSHELL_OUT_STD, shell_out, "shell '%s' returned %d!" CORSHELL_NEWLINE,
                //      cmd->cmd, res) ;

            }
            return res ;

        }

    }

    snprintf(_corshell_buffer, CORSHELL_PRINT_BUFFER_SIZE,
            "ERROR: '%s' not found!" CORSHELL_NEWLINE , argv[0]) ;
    shell_out (ctx, CORSHELL_OUT_ERR, _corshell_buffer) ;

    return CORSHELL_CMD_E_NOT_FOUND ;
}

uint32_t
corshell_cmd_help (char *buffer, size_t len)
{
    unsigned offset = 0 ;
    CORSHELL_CMD_LIST_IT_T it ;
    const CORSHELL_CMD_T * cmd ;

    for (cmd = _cmd_first(&it); cmd; cmd = _cmd_next(&it)) {

        if (cmd->usage && (offset + 3 <= len)) {
            unsigned int l = strlen(cmd->usage) + strlen(cmd->cmd)  ;


            if (offset + l + 4 < len) {
                offset += snprintf (&buffer[offset], len - offset,
                        "%s %s\r\n", cmd->cmd, cmd->usage ) ;
            }
            else {
                break ;
            }
        }
    }

    return offset + 1 ;
}

int32_t
corshell_rem(void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{

    return CORSHELL_CMD_E_OK ;
}

int32_t
corshell_nop(void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    int32_t res = CORSHELL_CMD_E_OK ;

    if (argc > 1) {
        corshell_scan_int(argv[1], (uint32_t*)&res) ;

    }

    return res  ;
}

int32_t
corshell_help(void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    CORSHELL_CMD_LIST_IT_T it ;
    int found = 1 ;
    CORSHELL_CMD_LIST_IT_T firstit ;
    CORSHELL_CMD_LIST_IT_T lastit ;
    CORSHELL_CMD_LIST_IT_T nextit ;
    const CORSHELL_CMD_T * cmd ;

    //CORSHELL_CMD_LIST_T*  this = 0 ;
    _cmd_first(&firstit) ;
    _cmd_first(&nextit) ;
    _cmd_first(&lastit) ;

    for (cmd = _cmd_first(&it); cmd; cmd = _cmd_next(&it)) {
        if (_cmd_cmp(&it,&firstit) < 0) {
            firstit = nextit = it ;
        }
        if (_cmd_cmp(&it,&lastit) > 0) {
            lastit = it ;
        }
    }

    do  {

        _cmd_help(ctx, shell_out,
                &nextit, (argc > 1) ? argv[1] : 0) ;

        found = 0 ;
        nextit = lastit ;
        for (cmd = _cmd_first(&it); cmd; cmd = _cmd_next(&it)) {
            if (_cmd_cmp(&it,&firstit) <= 0) {
                continue ;
            }
            if (_cmd_cmp(&it,&nextit) < 0) {
                nextit = it ;
                found = 1 ;
            }

        }
        firstit = nextit ;


    } while (found) ;

    _cmd_help(ctx, shell_out,
            &nextit, (argc > 1) ? argv[1] : 0) ;


    return CORSHELL_CMD_E_OK ;
}

int32_t
corshell_help2(void* ctx, CORSHELL_OUT_FP shell_out, char** argv, int argc)
{
    return corshell_help(ctx, shell_out, argv, argc) ;
}


size_t
corshell_cmd_split(char *buffer, size_t len, char *argv[], size_t argv_size)
{
    char *p, *start_of_word = buffer ;
    int c;
    int strchar ;
    enum states { DULL, IN_WORD, IN_STRING, IN_WORD_STRING } state = DULL;
    size_t argc = 0;
    argv[0] = "" ;

    for (p = buffer; argc < argv_size && *p != '\0' &&
                    ((unsigned int)(p - buffer) < len) ; p++) {

        c = (unsigned char) *p;
        switch (state) {
        case DULL:
            if (isspace(c)) {
                continue;
            }

            if ((c == '"') || (c == '\'')) {
                state = IN_STRING;
                strchar = c ;
                start_of_word = p + 1;
                continue;
            }
            state = IN_WORD;
            start_of_word = p;
            continue;

        case IN_STRING:
            if (c == strchar) {
                *p = 0;
                argv[argc++] = start_of_word;
                state = DULL;
            }
            continue;

        case IN_WORD:
            if (isspace(c)) {
                *p = 0;
                argv[argc++] = start_of_word;
                state = DULL;
            } else if (c == '"') {
                state = IN_WORD_STRING;
            }
            continue;

        case IN_WORD_STRING:
            if (c == '"') {
               state = IN_WORD;
            }
            continue ;
        }

    }

    if (state != DULL && argc < argv_size)
        argv[argc++] = start_of_word;

    return argc;
}


int32_t
corshell_script_clear_last_error ()
{
    _corshell_status = 0 ;
    return CORSHELL_CMD_E_OK ;
}


int32_t
corshell_script_run (void* ctx, CORSHELL_OUT_FP shell_out, const char* name,
                    char* start, int length)
{
    static uint32_t recurse = 0 ;
    int32_t status = CORSHELL_CMD_E_OK ;
    int32_t lasterror = CORSHELL_CMD_E_OK ;
    int32_t cancel ;
    int lineno = 0 ;
    char* line ;
    int len ;
    int i = 0;
    //int i_line_next ;
    //int i_line_error = length ;
    char *argv[CORSHELL_ARGC_MAX];
    int argc ;
    char * current_line  ;
#if CFG_STRSUB_USE
    char * strsub_line  ;
#endif

    enum  {
        stateNormal,
        stateInHandler,
        stateInError,


    } error_state = stateNormal ;


    if (recurse++ == 0) {
//      svc_system_speed(SYSTEM_SVC_SPEED_FAST, SYSTEM_SVC_SPEED_REQUESTOR_SCRIPT) ;

    }

    cancel = shell_out (ctx, CORSHELL_OUT_NULL, 0) ;
    if (cancel < CORSHELL_CMD_E_OK) {
        return cancel ;

    }

    current_line = CORSHELL_MALLOC(CORSHELL_LINE_SIZE_MAX) ;
    if (!current_line) {
        return CORSHELL_CMD_E_MEMORY ;
    }
#if CFG_STRSUB_USE
    strsub_line = CORSHELL_MALLOC(CORSHELL_LINE_STRSUB_SIZE_MAX) ;
    if (!strsub_line) {
        CORSHELL_FREE(current_line) ;
        return CORSHELL_CMD_E_MEMORY ;
    }
#endif

    line = start ;
    while (line) {
        len = 0 ;

        while ((start[i] != '\r') &&
                (start[i] != '\n') &&
                (start[i] != '\0') &&
                (i < length) &&
                (len < CORSHELL_LINE_SIZE_MAX-1)) {
            current_line[len] = start[i] ;
            i++ ; len++ ;
        }
        current_line[len] = '\0' ;
        if (start[i] == '\n') lineno++ ;
        //i_line_next = i ;

#if CFG_STRSUB_USE
        len = strsub_parse_string_to (0, current_line, len, strsub_line, CORSHELL_LINE_STRSUB_SIZE_MAX) ;
        argc = corshell_cmd_split(strsub_line, len, argv, CORSHELL_ARGC_MAX-1);
#else
        argc = corshell_cmd_split(current_line, len, argv, CORSHELL_ARGC_MAX-1);
#endif


        if (argc > 0) {
#if 1
            if (!strcmp (argv[0], ":exit")) {
                corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                        "shell '%s' exit on line %d!" CORSHELL_NEWLINE,
                        name, lineno) ;
                break ;

            } else
#endif
            if (strcmp (argv[0], ":onerror") == 0) {

                if (status >= CORSHELL_CMD_E_OK) {
                    //break ;
                    error_state = stateInError ;
                }

                else {


                    error_state = stateInError ;

                    if (argc > 1) {

                        int shell_errno ;
                        if (    (sscanf(argv[1], "%d", &shell_errno) > 0) &&
                                (status == shell_errno) ) {
                            error_state = stateInHandler ;


                        } else if ((*argv[1] == 'x') || (*argv[1] == '#')) {
                            error_state = stateInHandler ;


                        }


                    } else {
                        error_state = stateInHandler ;

                    }

                    if (error_state == stateInHandler) {
                        corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                                "shell '%s' onerror %d handler on line %d!" CORSHELL_NEWLINE,
                                name, status, lineno) ;

                    }

                }


            } else if (strcmp (argv[0], ":clearerror") == 0) {


                if (error_state > stateNormal) {
                    corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                            "shell '%s' clearerror %d on line %d!" CORSHELL_NEWLINE,
                            name, status, lineno) ;

                    lasterror = CORSHELL_CMD_E_OK ;
                    status = CORSHELL_CMD_E_OK ;
                    _corshell_status = CORSHELL_CMD_E_OK ;


                }
                error_state = stateNormal ;


            } else if (error_state > stateInHandler) {

                // continue until clear error or end of file

            }
            else {

                status = corshell_cmd_run (ctx, shell_out, &argv[0],  argc-0) ;
                if (status < CORSHELL_CMD_E_OK) {

                    error_state = stateInError ;

                    //i_line_error = i_line_next ;
                    if (lineno > 1) {
                        corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                                "shell '%s %s %s' error %d for '%s' on line %d!" CORSHELL_NEWLINE,
                                name, argc>1 ? argv[1] : "", argc>2 ? argv[2] : "",
                                status, argv[0], lineno) ;

                    }

                }



                if (_corshell_status >= CORSHELL_CMD_E_OK) {
                    if (status < CORSHELL_CMD_E_OK) lasterror = status ;
                    cancel = shell_out (ctx, CORSHELL_OUT_NULL, 0) ;
                    if (cancel < CORSHELL_CMD_E_OK) {
                        corshell_print (ctx, CORSHELL_OUT_STD, shell_out,
                                "shell '%s %s %s' cancelled with %d on line %d!" CORSHELL_NEWLINE,
                                name, argc>1 ? argv[1] : "", argc>2 ? argv[2] : "",
                                status, lineno) ;
                        lasterror = status = cancel ;

                    }

                }

            }


        }

        while (((start[i] == '\r') || (start[i] == '\n') || (start[i] == '\0')) && (i < length)) {
            if (start[i] == '\n') lineno++ ;
            i++;
        }
        if (i < length) {
            line = &start[i] ;
        }
        else {
            line = 0 ;
        }
    }

    CORSHELL_FREE(current_line) ;
#if CFG_STRSUB_USE
    CORSHELL_FREE(strsub_line) ;
#endif


    if (recurse) {
        recurse-- ;
    }
    if (!recurse) {
#if CFG_PLATFORM_SVC_SERVICES
        svc_system_speed(SYSTEM_SVC_SPEED_IDLE, SYSTEM_SVC_SPEED_REQUESTOR_SCRIPT) ;
#endif
    }


#if USE_ONERROR
    if (lasterror < 0) {
        _corshell_status = lasterror ;
    }
    return _corshell_status ;
#else
    return status ;
#endif
}
