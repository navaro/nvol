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


#ifndef __MHEAP_H__
#define __MHEAP_H__


#include <stdint.h>



typedef enum  {
	HEAP_SPACE_NONE = 0,
	HEAP_SPACE_LOCAL,
	HEAP_SPACE_DMA,
	HEAP_SPACE,
	HEAP_SPACE_LAST
} heapspace ;



/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

	extern unsigned int     heap_init (void) ;

	extern void *           heap_malloc(heapspace heap, uint32_t bytes) ;
	extern void *           heap_calloc(heapspace heap, uint32_t bytes, uint32_t size) ;
	extern void             heap_free(heapspace heap, void * mem) ;
	extern void *           heap_realloc(heapspace heap, void * mem, uint32_t bytes) ;


#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#define                 heap_usable_size(mem)           mspace_usable_size(mem)

#endif /* __MHEAP_H__ */
/** @} */
