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

    if(!(to_elmt=malloc(sizeof(to_list)))){
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

    if(!(tmp=malloc(sizeof(tr_list)))){
	perror("(tr_insert)Unable to create trackers list.");
	exit(EXIT_FAILURE);
    }
    to_elmt->next = NULL;
    tmp->url = url;
    tmp->head = to_elmt;
    tmp->next = trackers;
    trackers = tmp;

    return tmp;
}

list*
add(list * l ,void * elmt){
    list * tmp = l;
    
    while( tmp && tmp->next )
	tmp = tmp -> next;
    
    if(!tmp){
	tmp = malloc(sizeof(list));
	tmp -> elmt = elmt;
	tmp -> next = NULL;
	return tmp;
    }
    
    else{
	//tmp -> next == NULL
	tmp -> next = malloc(sizeof(list));
	tmp -> next -> elmt = elmt;
	tmp -> next -> next = NULL;
	return l;
    }
    return l;
}
