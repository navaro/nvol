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


#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdint.h>
#include <stdarg.h>

#include "system_config.h"


#define DBG_STATIC_ASSERT(condition) typedef char DBG_STATIC_ASSERT__LINE__[ (condition) ? 1 : -1];


#define     ANSI_CLEAR      // "\e[0m"      // ANSI 'clear' color code
#define     ANSI_GREEN      // "\e[32m" // ANSI 'green' color code
#define     ANSI_YELLOW     // "\e[33m" // ANSI 'yellow' color code
#define     ANSI_RED        // "\e[31m" // ANSI 'red' color code

#define DBG_MESSAGE_SEVERITY_NEVER                          (0)
#define DBG_MESSAGE_SEVERITY_ASSERT                         (1)
#define DBG_MESSAGE_SEVERITY_ERROR                          (2)
#define DBG_MESSAGE_SEVERITY_WARNING                        (3)
#define DBG_MESSAGE_SEVERITY_REPORT                         (4)
#define DBG_MESSAGE_SEVERITY_LOG                            (5)
#define DBG_MESSAGE_SEVERITY_INFO                           (6)
#define DBG_MESSAGE_SEVERITY_DEBUG                          (7)
#define DBG_MESSAGE_SEVERITY_VERBOSE                        (8)

#define DBG_MESSAGE_SEVERITY_MASK                           (0x0F)



#define DBG_MESSAGE_FLAG_NO_TIMESTAMP                       (SVC_LOGGER_FLAGS_NO_TIMESTAMP)
#define DBG_MESSAGE_FLAG_NO_FORMATTING                      (SVC_LOGGER_FLAGS_NO_FORMATTING)
#define DBG_MESSAGE_FLAG_PROGRESS                           (DBG_MESSAGE_FLAG_NO_TIMESTAMP|DBG_MESSAGE_FLAG_NO_FORMATTING|SVC_LOGGER_FLAGS_PROGRESS)

#define DBG_MESSAGE_MASK(severity, facility)                (severity, (1<<facility))

#define DBG_MESSAGE_WOULD_LOG(type, severity)               (((type) & DBG_MESSAGE_SEVERITY_MASK) <= severity)
#define DBG_MESSAGE_GET_SEVERITY(type)                      (type & DBG_MESSAGE_SEVERITY_MASK)
#define DBG_MESSAGE_LOGGER_TYPE(severity, flags)                            ((severity) | (flags))



#if CFG_PLATFORM_SVC_LOGGER

extern uint32_t         logger_type_log (uint32_t type, const char *str, ...) ;

#define DBG_VMESSAGE_T(type, facility, fmt_str, args)       {  svc_logger_type_vlog(type, facility, fmt_str, args) ; }
#define DBG_MESSAGE_T(type, facility, fmt_str, ...)         {  svc_logger_type_log(type, facility, fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_ERROR(type, facility, fmt_str, ...)   {  if (((type) & DBG_MESSAGE_SEVERITY_MASK) <= DBG_MESSAGE_SEVERITY_ERROR)      svc_logger_type_log(type, facility, fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_WARNING(type, facility, fmt_str, ...) {  if (((type) & DBG_MESSAGE_SEVERITY_MASK) <= DBG_MESSAGE_SEVERITY_WARNING)    svc_logger_type_log(type, facility, fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_REPORT(type, facility, fmt_str, ...)  {  if (((type) & DBG_MESSAGE_SEVERITY_MASK) <= DBG_MESSAGE_SEVERITY_REPORT)     svc_logger_type_log(type, facility, fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_LOG(type, facility, fmt_str, ...)     {  if (((type) & DBG_MESSAGE_SEVERITY_MASK) <= DBG_MESSAGE_SEVERITY_LOG)        svc_logger_type_log(type, facility, fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_DEBUG(type, facility, fmt_str, ...)   {  if (((type) & DBG_MESSAGE_SEVERITY_MASK) <= DBG_MESSAGE_SEVERITY_DEBUG)      svc_logger_type_log(type, facility, fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_ASSERT(type, facility, fmt_str, ...)  {  if (((type) & DBG_MESSAGE_SEVERITY_MASK) <= DBG_MESSAGE_SEVERITY_ASSERT)     svc_logger_type_log(type, facility, fmt_str, ##__VA_ARGS__) ; }
#define DBG_ASSERT_T(cond, fmt_str, ...)                    {  if (!(cond)) { dbg_assert(fmt_str, ##__VA_ARGS__)  ; }}
#define DBG_ASSERT_ISR_T(cond, fmt_str, ...)                {  if (!(cond)) {  dbg_assert(fmt_str, ##__VA_ARGS__)  ; }}

#ifdef NDEBUG
#define DBG_CHECK_T(cond, ret, fmtstr, ...)                 { if (!(cond)) { return ret ; } }
#define DBG_CHECKV_T(cond, fmtstr, ...)                     { if (!(cond)) { return  ; } }
#else
#define DBG_CHECK_T(cond, ret, fmt_str, ...)                {  if (!(cond)) {  svc_logger_type_log(DBG_MESSAGE_SEVERITY_ERROR , 0, fmt_str, ##__VA_ARGS__ ) ; return ret ; }}
#define DBG_CHECKV_T(cond, fmt_str, ...)                    {  if (!(cond)) {  svc_logger_type_log(DBG_MESSAGE_SEVERITY_ERROR , 0, fmt_str, ##__VA_ARGS__ ) ; }}
#endif

#else

extern int              dbg_printf (const char *format, ...)  ;
extern int              dbg_vprintf (const char *format, va_list args);
extern void             dbg_assert (const char *format, ...)  ;


#define DBG_VMESSAGE_T(type, facility, fmt_str, args)       {  debug_vprintf(fmt_str, args) ; }
#define DBG_MESSAGE_T(type, facility, fmt_str, ...)         {  debug_printf(fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_ERROR(type, facility, fmt_str, ...)   {  if (DBG_MESSAGE_GET_SEVERITY(type) <= DBG_MESSAGE_SEVERITY_ERROR)    dbg_printf(fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_WARNING(type, facility, fmt_str, ...) {  if (DBG_MESSAGE_GET_SEVERITY(type) <= DBG_MESSAGE_SEVERITY_WARNING)  dbg_printf(fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_REPORT(type, facility, fmt_str, ...)  {  if (DBG_MESSAGE_GET_SEVERITY(type) <= DBG_MESSAGE_SEVERITY_REPORT)   dbg_printf(fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_LOG(type, facility, fmt_str, ...)     {  if (DBG_MESSAGE_GET_SEVERITY(type) <= DBG_MESSAGE_SEVERITY_LOG)      dbg_printf(fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_DEBUG(type, facility, fmt_str, ...)   {  if (DBG_MESSAGE_GET_SEVERITY(type) <= DBG_MESSAGE_SEVERITY_DEBUG)    dbg_printf(fmt_str, ##__VA_ARGS__) ; }
#define DBG_MESSAGE_T_ASSERT(type, facility, fmt_str, ...)  {  if (DBG_MESSAGE_GET_SEVERITY(type) <= DBG_MESSAGE_SEVERITY_ASSERT)   dbg_printf(fmt_str, ##__VA_ARGS__) ; }
#define DBG_ASSERT_T(cond, fmt_str, ...)                    {  if (!(cond)) {  dbg_assert(fmt_str, ##__VA_ARGS__)  ; }}

#ifdef NDEBUG
#define DBG_CHECK_T(cond, ret, fmtstr, ...)                 { if (!(cond)) { return ret ; } }
#define DBG_CHECKV_T(cond, fmtstr, ...)                     { if (!(cond)) { return  ; } }
#else
#define DBG_CHECK_T(cond, ret, fmt_str, ...)                {  if (!(cond)) {  dbg_printf(fmt_str, ##__VA_ARGS__ ) ; return ret ; }}
#define DBG_CHECKV_T(cond, fmt_str, ...)                    {  if (!(cond)) {  dbg_printf(fmt_str, ##__VA_ARGS__ ) ; }}
#endif


#endif



#endif /* __DEBUG_H__ */
