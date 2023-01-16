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
#include <common/heap.h>
#include <common/debug.h>
#include "dictionary.h"


typedef struct dlist *  (*DICTIONARY_KEYVAL_ALLOC_T)(struct dictionary * /* dict */, const char * /* s */, unsigned int /* keysize */, unsigned int /* valuesize */) ;
typedef void            (*DICTIONARY_KEYVAL_FREE_T)(struct dictionary * /* dict */, struct dlist * /* np */) ;
typedef unsigned int    (*DICTIONARY_KEY_HASH_T)(struct dictionary * /* dict */, const char * /* s */, unsigned int /* keysize */) ;
typedef unsigned int    (*DICTIONARY_KEY_CMP_T)(struct dictionary * /* dict */, struct dlist * /* np */, const char * /* s */, unsigned int /* keysize */) ;
typedef const char*     (*DICTIONARY_KEY_T)(struct dictionary * /* dict */, struct dlist * /* np */) ;
typedef char*           (*DICTIONARY_VALUE_T)(struct dictionary * /* dict */, struct dlist * /* np */) ;


struct dictionary_keyval {
    DICTIONARY_KEYVAL_ALLOC_T       set ;
    DICTIONARY_KEYVAL_FREE_T        free ;
    DICTIONARY_KEY_HASH_T           hash ;
    DICTIONARY_KEY_CMP_T            cmp ;
    DICTIONARY_KEY_T                key ;
    DICTIONARY_VALUE_T              value ;
} ;

struct dictionary {
    const struct dictionary_keyval * key ;
    unsigned int                    hashsize ;
    unsigned int                    keyspec ;
    unsigned int                    count ;
    heapspace                       heap ;
    struct dlist *                  hashtab[]; /* pointer table */
} ;


static struct dlist *
dictionary_str_keyval_alloc(struct dictionary * dict, const char *s, unsigned int keysize, unsigned int valuesize) /* make a duplicate of s */
{
    char *p;
    struct dlist * np ;

    if (s == 0) return 0 ;
    np = (struct dlist *) DICTIONARY_MALLOC(dict->heap,
            sizeof(struct dlist) +
            sizeof(unsigned int) +
            valuesize);
    if (!np) return 0 ;
    if (keysize == 0) keysize = strlen(s) ;
    p = (char *) DICTIONARY_MALLOC(dict->heap, keysize+1); /* +1 for ?\0? */
    if (!p ) {
        DICTIONARY_FREE(dict->heap, np);
        return 0;
    }
   strncpy(p, s, keysize);
   p[keysize] = '\0' ;
   np->keyval[0] = (uintptr_t)p ;
   return np ;
}

static void
dictionary_str_keyval_free(struct dictionary * dict, struct dlist *np) /* make a duplicate of s */
{
    DICTIONARY_FREE (dict->heap, (char *)np->keyval[0]) ;
    DICTIONARY_FREE (dict->heap, np) ;
}

static struct dlist *
dictionary_const_str_keyval_alloc(struct dictionary * dict, const char *s, unsigned int keysize, unsigned int valuesize) /* make a duplicate of s */
{
   struct dlist * np ;

    if (s == 0) return 0 ;
    np = (struct dlist *) DICTIONARY_MALLOC(dict->heap,
            sizeof(struct dlist) +
            sizeof(unsigned int) +
            valuesize);
    if (!np) return 0 ;
    np->keyval[0] = (uintptr_t)s ;
    return np ;
}

static void
dictionary_const_str_keyval_free(struct dictionary * dict, struct dlist *np) /* make a duplicate of s */
{
    DICTIONARY_FREE(dict->heap, np);
}

/* hash: form hash value for string */
static unsigned int
dictionary_str_key_hash(struct dictionary * dict, const char *s, unsigned int keysize)
{
    unsigned hashval;
    if (s == 0) return 0 ;
    if (keysize == 0) keysize = strlen(s) ;
    for (hashval = 0; (*s != '\0') && keysize; s++, keysize--) {
      hashval = *s + 31 * hashval;
    }
    return hashval % dict->hashsize;
}

static unsigned int
dictionary_str_key_cmp(struct dictionary * dict, struct dlist *np, const char *s, unsigned int keysize) /* make a duplicate of s */
{
    char* p = (char*)np->keyval[0] ;
    if (s == p) return 1 ;// NULL key
    if (keysize == 0) keysize = strlen(s) ;
    if ((strncmp(s, p, keysize) == 0) && (p[keysize] == 0)) {
        return 1 ;
    }

    return 0 ;
}

static const char*
dictionary_str_key(struct dictionary * dict, struct dlist *np)
{
    return (const char*)np->keyval[0] ;
}


static char*
dictionary_str_value(struct dictionary * dict, struct dlist *np) /* make a duplicate of s */
{
    return (char*)&np->keyval[1] ;
}


/* hash: form hash value for uint16 */
static struct dlist *
dictionary_uint_keyval_alloc(struct dictionary * dict, const char *s, unsigned int keysize, unsigned int valuesize) /* make a duplicate of s */
{
    struct dlist * np ;

    np = (struct dlist *) DICTIONARY_MALLOC(dict->heap,
            sizeof(struct dlist) +
            sizeof(uintptr_t) +
            valuesize);
    if (!np) return 0 ;
    np->keyval[0] =  *((unsigned int*)s)  ;
    return np ;
}

static void
dictionary_uint_keyval_free(struct dictionary * dict, struct dlist *np) /* make a duplicate of s */
{
    DICTIONARY_FREE (dict->heap, np) ;
    return ;
}

/* hash: form hash value for uint16 */
static unsigned
dictionary_uint_key_hash(struct dictionary * dict, const char *s, unsigned int keysize)
{
    return *((unsigned int*)s) % dict->hashsize ;
}


static unsigned int
dictionary_uint_key_cmp(struct dictionary * dict, struct dlist *np, const char *s, unsigned int keysize) /* make a duplicate of s */
{
    if ((unsigned int)np->keyval[0] == *((unsigned int*)s)) {
        return 1 ;
    }

    return 0 ;
}

static struct dlist *
dictionary_binary_keyval_alloc(struct dictionary * dict, const char *s, unsigned int keysize, unsigned int valuesize) /* make a duplicate of s */
{
    struct dlist * np ;
    uint16_t len = dict->keyspec & 0xFFFF ;

    np = (struct dlist *) DICTIONARY_MALLOC(dict->heap,
            sizeof(struct dlist) +
            len * sizeof(unsigned int) +
            valuesize);
    if (!np) return 0 ;
    memcpy(np->keyval, s, len*sizeof(uint32_t)) ;
    return np ;
}

static void
dictionary_binary_keyval_free(struct dictionary * dict, struct dlist *np) /* make a duplicate of s */
{
    DICTIONARY_FREE (dict->heap, np) ;
    return ;
}

/* hash: form hash value for uint16 */
static unsigned
dictionary_binary_key_hash(struct dictionary * dict, const char *s, unsigned int keysize)
{
    unsigned int  * pkey = (unsigned int*)s ;
    uint16_t len = dict->keyspec & 0xFFFF ;
    uint16_t i ;
    uint32_t hash = 0 ;

    for (i=0; i<len; i++) {
        hash += pkey[i] ;
    }
    return hash % dict->hashsize ;
}


static unsigned int
dictionary_binary_key_cmp(struct dictionary * dict, struct dlist *np, const char *s, unsigned int keysize) /* make a duplicate of s */
{
    unsigned int  * pkey = (unsigned int*)s ;
    unsigned int  * pkeyval = (unsigned int*)np->keyval ;
    uint16_t len = dict->keyspec & 0xFFFF ;
    uint16_t i ;
    uint32_t cmp = 1 ;

    for (i=0; i<len; i++) {
        if (pkey[i] != pkeyval[i]) {
            cmp = 0 ;
            break ;
        }
    }

    return cmp ;
}

const char*
dictionary_key(struct dictionary * dict, struct dlist *np)
{
    return (const char*)&np->keyval[0] ;
}

char*
dictionary_value(struct dictionary * dict, struct dlist *np)
{
    unsigned int  * pkeyval = (unsigned int*)np->keyval ;
    return (char*)&pkeyval[dict->keyspec & 0xFFFF] ;
}

static struct dlist *
dict_remove (struct dictionary * dict, const char *s, unsigned int keysize) {
    struct dlist *np;
    struct dlist *prev = 0 ;
    unsigned hashval = dict->key->hash (dict, s, keysize) ;
    for (np = dict->hashtab[hashval]; np != 0; np = np->next) {
        if (dict->key->cmp(dict, np, s, keysize)) {
          break ; /* found */
        }
        prev = np ;
    }
    if (np) {
        dict->count-- ;
        if (prev) {
            prev->next = np->next ;
        }
        else {
            dict->hashtab[hashval] = np->next ;
        }
    }
    return np;

}

/* dict_lookup: look for s in hashtab */
static struct dlist *
dict_lookup (struct dictionary * dict, const char *key, unsigned int keysize)
{
    struct dlist *np;
    for (np = dict->hashtab[dict->key->hash(dict, key, keysize)]; np != 0; np = np->next) {
        if (dict->key->cmp(dict, np, key, keysize)) {
          return np; /* found */
        }
    }
    return 0; /* not found */
}

struct dictionary *
dictionary_init (heapspace heap, unsigned int keyspec, unsigned int hashsize)
{
    static const struct dictionary_keyval str_key = {
            &dictionary_str_keyval_alloc,
            &dictionary_str_keyval_free,
            &dictionary_str_key_hash,
            &dictionary_str_key_cmp,
            &dictionary_str_key,
            &dictionary_str_value

    };
    static const struct dictionary_keyval const_str_key = {
            &dictionary_const_str_keyval_alloc,
            &dictionary_const_str_keyval_free,
            &dictionary_str_key_hash,
            &dictionary_str_key_cmp,
            &dictionary_str_key,
            &dictionary_str_value

    };
    static const struct dictionary_keyval uint_key = {
            &dictionary_uint_keyval_alloc,
            &dictionary_uint_keyval_free,
            &dictionary_uint_key_hash,
            &dictionary_uint_key_cmp,
            &dictionary_key,
            &dictionary_value

    };

    static const struct dictionary_keyval binary_key = {
            &dictionary_binary_keyval_alloc,
            &dictionary_binary_keyval_free,
            &dictionary_binary_key_hash,
            &dictionary_binary_key_cmp,
            &dictionary_key,
            &dictionary_value

    };


    struct dictionary * dict ; 
#define DICTSIZE(hashsize) (sizeof(struct dictionary) + sizeof(struct dlist *) * hashsize)
     dict =   (struct dictionary *) DICTIONARY_MALLOC(heap, DICTSIZE(hashsize)) ;
     if (dict) {
        memset (dict,0,DICTSIZE(hashsize)) ;

        dict->keyspec = keyspec ;

        if ((keyspec >> 16) == DICTIONARY_KEYTYPE_BINARY) {
            dict->key = &binary_key ;

        } else if (keyspec == DICTIONARY_KEYSPEC_UINT) {
            dict->key = &uint_key ;

        } else if (keyspec == DICTIONARY_KEYSPEC_CONST_STRING) {
            dict->key = &const_str_key ;

        } else {
            dict->key = &str_key ;

        }

        dict->hashsize = hashsize ;
        dict->heap = heap ;
     }
    return dict ;
}

/* install: put (name, defn) in hashtab */
struct dlist*
dictionary_install_size(struct dictionary * dict, const char *key, unsigned int keysize, unsigned int valuesize)
{
    struct dlist *np;
    unsigned hashval;
     if ((np = dict_lookup(dict, key, keysize)) == 0) { /* not found */
        np = dict->key->set(dict, key, keysize, valuesize) ;
        if (np == 0) return 0;
        hashval = dict->key->hash(dict, key, keysize);
        np->next = dict->hashtab[hashval];
        dict->hashtab[hashval] = np;
        dict->count++ ;
    }

    return np ;
}

/* install: put (name, defn) in hashtab */
struct dlist*
dictionary_replace(struct dictionary * dict, const char *key, unsigned int keysize, const char *value, unsigned int valuesize)
{
    struct dlist*  np = dictionary_install_size(dict, key,  keysize,  valuesize) ;
    if (!np) return 0 ;
    char* p = dict->key->value(dict, np);
    memcpy (p, value, valuesize) ;
    return np ;
}

/* install: put (name, defn) in hashtab */
struct dlist*
dictionary_lookup(struct dictionary * dict, const char *key, unsigned int keysize, const char *value, unsigned int valuesize)
{
    struct dlist *np;
    unsigned hashval;
     if ((np = dict_lookup(dict, key, keysize)) == 0) { /* not found */
        np = dict->key->set(dict, key, keysize, valuesize) ;
        if (np == 0) return 0;
        hashval = dict->key->hash(dict, key, keysize);
        np->next = dict->hashtab[hashval];
        dict->hashtab[hashval] = np;
        dict->count++ ;
        char* p = dict->key->value(dict, np);
        memcpy (p, value, valuesize) ;

    }

    return np ;
}


struct dlist*
dictionary_get(struct dictionary * dict, const char *key, unsigned int keysize)
{
    return dict_lookup(dict, key, keysize) ;
}

unsigned int
dictionary_remove(struct dictionary * dict, const char *key, unsigned int keysize)
{
    struct dlist *np;

    if ((np = dict_remove(dict, key, keysize)) == 0) { /* not found */
        return 0 ;
    } 

    dict->key->free (dict, np) ;

    return 1;
}

void
dictionary_remove_all(struct dictionary * dict, void (*cb)(struct dictionary *, struct dlist*, uint32_t), uint32_t parm)
{
    struct dlist *np;
    unsigned  i ;
    for (i=0; i<dict->hashsize; i++) {
     for (np = dict->hashtab[i]; np != 0; np = dict->hashtab[i]) {
         if (cb) {
             cb (dict, np, parm) ;
         }
         dict->hashtab[i] = np->next ;
         dict->key->free (dict, np) ;
         dict->count-- ;
            
     }
    }

    DBG_CHECKV_T(dict->count == 0, "UTIL  :A: dictionary_remove_all") ;

}

void
dictionary_destroy(struct dictionary * dict)
{
    dictionary_remove_all (dict, 0, 0) ;

    DICTIONARY_FREE (dict->heap, dict) ;
}

struct dlist*
dictionary_it_first (struct dictionary * dict, struct dictionary_it* it)
{
    it->np = dict->hashtab[0];
    it->idx = 0 ;
    if (it->np) return it->np ;

    return dictionary_it_next (dict, it) ;
}

struct dlist*
dictionary_it_next (struct dictionary * dict, struct dictionary_it* it)
{
   struct dlist *np;
    unsigned  i ;

    if (it->np && it->np->next) {
        it->np = it->np->next ;
        return it->np ;
    }
     for (i=it->idx+1; i<dict->hashsize; i++) {
         for (np = dict->hashtab[i]; np != 0; ) {
                 it->np = np ;
                 it->idx = i ;
                 return np ;
 
         }
     }

     return 0 ;

}

struct dlist*
dictionary_it_at (struct dictionary * dict, const char *key, unsigned int len, struct dictionary_it* it)
{
    it->idx = dict->key->hash(dict, key, len) ;
    it->np = dict_lookup(dict, key, len) ;
    return it->np ;
}

struct dlist*
dictionary_it_get (struct dictionary * dict, struct dictionary_it* it)
{
    return it->np ;

}

const char*
dictionary_get_key (struct dictionary * dict, struct dlist* np)
{
    return dict->key->key (dict, np) ;

}

char*
dictionary_get_value (struct dictionary * dict, struct dlist* np)
{
    return dict->key->value (dict, np) ;

}

unsigned int
dictionary_count (struct dictionary * dict)
{
    return dict->count ;
}

unsigned int
dictionary_hashtab_size (struct dictionary * dict)
{
    return dict->hashsize ;
}

unsigned int
dictionary_hashtab_cnt (struct dictionary * dict, unsigned int idx)
{
    struct dlist *np;
    unsigned int cnt = 0 ;
    for (np = dict->hashtab[idx]; np != 0; np = np->next) {
        cnt++;
    }
    return cnt ;
}
