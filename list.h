#ifndef LIST_H
#define LIST_H

#include "hashtable.h"


typedef struct to_list{
    ht_torrent *elmt;
    struct to_list *next;
} to_list;


typedef struct tr_list{
    char *url;
    to_list *head;
    struct tr_list *next;
} tr_list;

void tr_insert(ht_torrent * t, char *url);
#endif
