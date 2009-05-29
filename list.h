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

#ifndef LIST_H
#define LIST_H

#include "hashtable.h"


typedef struct to_list{
    struct torrent *elmt;
    struct to_list *next;
} to_list;

typedef struct tr_list{
    char *url;
    to_list *head;
    struct tr_list *next;
} tr_list;

typedef struct chunk {
    int64_t offset;
    int begin;
    int length;
    struct chunk *next;
} chunk;

typedef struct peer {
    int stream_writer;
    struct torrent *t;
    int credit;
    struct chunk *list;
} peer;


void tr_insert(struct torrent *t, char *url);
struct chunk *add_chunk(struct chunk *l, int64_t offset, int begin, int length);
struct chunk *remove_chunk(struct chunk *l, int offset, int begin, int length);

#endif
