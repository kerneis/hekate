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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "parse.h"
#include "hashtable.h"
#include "tracker_io.h"

void
tr_insert(ht_torrent *t, char *url)
{
    tr_list *tmp;
    to_list *to_elmt;

    to_elmt = malloc(sizeof(to_list));
    if(!to_elmt) {
        perror("(tr_insert)Unable to add torrent in list.");
        return;
    }
    to_elmt->elmt = t;

    for(tmp = trackers; tmp; tmp = tmp->next) {
        if(strcmp(tmp->url, url) == 0) {
            free(url);
            to_elmt->next = tmp->head;
            tmp->head = to_elmt;
            return;
        }
    }

    tmp = malloc(sizeof(tr_list));
    if(!tmp) {
        perror("(tr_insert)Unable to create trackers list.");
        exit(EXIT_FAILURE);
    }
    to_elmt->next = NULL;
    tmp->url = url;
    tmp->head = to_elmt;
    tmp->next = trackers;
    trackers = tmp;
}

struct chunk*
add_chunk(struct chunk *l, int64_t offset, int begin, int length)
{
    struct chunk *c =  malloc(sizeof(struct chunk));

    c->offset = offset;
    c->begin  = begin;
    c->length = length;
    c->next = NULL;

    if(!l)
        return c;
    else {
        struct chunk *ll = l;
        while(ll->next)
            ll = ll->next;
        ll->next = c;
        return l;
    }
}

struct chunk*
remove_chunk(struct chunk* list, int offset, int begin, int length)
{
    struct chunk *prec = NULL, *tmp = list;

    if(!list)
        return NULL;

    while(tmp){
        if(tmp -> offset == offset &&
           tmp -> begin  == begin &&
           tmp -> length == length){
            if(prec == NULL)
                return list-> next;
            else
                prec -> next = tmp ->next;
            break;
        }
        prec = tmp;
        tmp = tmp -> next;
    }

    return list;
}
