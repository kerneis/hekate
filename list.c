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

    for(tmp=trackers; tmp; tmp=tmp->next){
        if(strcmp(tmp->url, url)==0){
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

piece*
add_piece(piece *l, int64_t offset, int begin, int length)
{
    piece *p =  malloc(sizeof(piece));

    p->offset = offset;
    p->begin  = begin;
    p->length = length;
    p->next = l;

    return p;
}

piece*
remove_piece(piece *l, int offset, int begin, int length)
{
    /*    piece *prec = NULL, *tmp = pl;
    piece * p;
    if(!pl)
	return NULL;
    
    while(!tmp){
        p = tmp -> elmt;

        if(p -> offset == offset &&
           p -> begin  == begin &&
           p -> length == length){
            if(prec == NULL)
                return pl-> next;
            else
                prec -> next = tmp ->next;
            break;
        }
        prec = tmp;
        tmp = tmp -> next;
        }
        return pl;*/
    return NULL;
}
