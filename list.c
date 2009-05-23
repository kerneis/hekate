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
