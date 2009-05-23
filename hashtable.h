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

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "parse.h"

struct file {
    int64_t offset;
    int64_t length;
    char *path;
    char *map;
};

typedef struct ht_torrent {
    /* hashtable fields */
    unsigned char key[20];
    struct ht_torrent *next;

    /* pieces size in bytes */
    int64_t p_length;
    int32_t num_chunks;

    int num_files;
    struct file **files;
    unsigned char *info_hash;
} ht_torrent;

typedef struct hashtable{
    int size;
    ht_torrent **table;
}hashtable;


hashtable * ht_create(int size);
unsigned char * ht_insert(hashtable *ht, ht_torrent *hte);
ht_torrent * ht_get(hashtable *ht, unsigned char *key);

int ht_load(hashtable *table, char *curr_path, benc *raw);
int ht_info_load(ht_torrent *elmt, char *curr_path, benc *raw);
#endif
