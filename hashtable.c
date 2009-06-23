/*
Copyright (c) 2009 by Pejman Attar, Yoann Canal, Juliusz Chroboczek and
                      Gabriel Kerneis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "parse.h"
#include "hashtable.h"
#include "torrent.h"

uint32_t
hash(unsigned char *key, int ht_size)
{
    uint32_t *seed = (uint32_t *)key;
    return (seed[0] ^ seed[1] ^ seed[2] ^ seed[3] ^ seed[4]) % ht_size;
}


hashtable *
ht_create(int size)
{
    hashtable *ht = malloc(sizeof(hashtable));
    if(!ht) return NULL;

    ht->size = size;
    ht->table = calloc(size, sizeof(struct torrent *));
    if(!ht->table) {
        free(ht);
        return NULL;
    }

    return ht;
}


void
ht_insert(hashtable *ht, struct torrent *hte)
{
    /* unsigned char *key, void *value) */
    uint32_t h = hash(hte->info_hash, ht->size);

    if(ht->table[h])
      hte->next = ht->table[h];
    else
      hte->next = NULL;

    ht->table[h] = hte;
}


struct torrent *
ht_get(hashtable *ht, unsigned char *key)
{
    uint32_t h = hash(key, ht->size);
    struct torrent *hte = ht->table[h];
    while(hte) {
        if(!memcmp(key, hte->info_hash, 20))
            return hte;
        hte = hte->next;
    }
    return NULL;
}
