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

#ifndef __ERRORDEF_H__
#define __ERRORDEF_H__

#define CORAL_ERROR				-4300

#define EOK           			0
#define EFAIL          			-1


#define E_INVALID           	(CORAL_ERROR-2)

#define E_PARM           		(CORAL_ERROR-3)

#define E_SRCH           		(CORAL_ERROR-4)

#define E_EMPTY           		(CORAL_ERROR-5)

#define E_IO             		(CORAL_ERROR-6)

#define E_ALIGN          		(CORAL_ERROR-7)

#define E_CRC            		(CORAL_ERROR-8)

#define E_NOMEM          		(CORAL_ERROR-9)

#define E_BUSY           		(CORAL_ERROR-10)

#define E_INVAL          		(CORAL_ERROR-11)

#define E_TIMEOUT       		(CORAL_ERROR-12)

#define E_NOIMPL       			(CORAL_ERROR-13)

#define E_UNEXP       			(CORAL_ERROR-14)

#define E_FMT       			(CORAL_ERROR-15)

#define E_FULL       			(CORAL_ERROR-16)

#define E_NOTFOUND       		(CORAL_ERROR-17)

#define E_EOF       			(CORAL_ERROR-18)

#define E_BOF       			(CORAL_ERROR-19)

#define E_CANCELED       		(CORAL_ERROR-20)

#define E_EDELETED       		(CORAL_ERROR-21)

#define E_VERSION       		(CORAL_ERROR-22)

#define E_EXIST       			(CORAL_ERROR-23)

#define E_NOTAVAIL       		(CORAL_ERROR-24)

#define E_VALIDATIONRANGE		(CORAL_ERROR-25)

#define E_VALIDATIONFUNCTION	(CORAL_ERROR-26)

#define E_ENTRYNOTFOUND			(CORAL_ERROR-27)

#define E_AUTH					(CORAL_ERROR-28)

#define	E_NOTALLOW				(CORAL_ERROR-29)

#define	E_ADDRRANGE				(CORAL_ERROR-30)

#define	E_CORRUPT				(CORAL_ERROR-31)

#define	E_FILE					(CORAL_ERROR-32)

#define	E_VALIDATION			(CORAL_ERROR-33)

#define	E_CONNECTION			(CORAL_ERROR-34)

#define E_UNKNOWN       		(CORAL_ERROR-63)


#endif /* __ERRORDEF_H__ */

/*@}*/
