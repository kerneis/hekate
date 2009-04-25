#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "parse.h"
#include "hashtable.h"
#include "tracker_conn.h"

list *
linsert_tracker(ht_torrent * t)
{
    list *i, *tmp;
    list *e = malloc(sizeof(list));
    if(!e) return NULL;

    e->elmt = t;
    e->next = NULL;

    i = trackers;
    if(trackers){
	while(!i->next){
	    if(!strcmp(t->tracker, ((ht_torrent *)((list *)(i->elmt))->elmt)->tracker)){
		e->next = ((list *)(i->elmt))->elmt;
		i->elmt = e;
		return i;
	    }
	    i = i->next;
	}
    }

    tmp = malloc(sizeof(list));
    if(!tmp) return NULL;

    tmp->elmt = e;
    tmp->next = trackers;
    trackers = tmp;

    return tmp;
}
