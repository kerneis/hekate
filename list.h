#ifndef LIST_H
#define LIST_H

#include "hashtable.h"

typedef struct list list;

struct list{
    void *elmt;
    struct list *next;
};

list *linsert_tracker(ht_torrent * t);

#endif
