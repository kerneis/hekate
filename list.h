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

typedef struct piece{
    /* TO-DO: add a file field for multi-files torrents */
    /* offset associated with a chunk in a file */
    int64_t offset;
    int begin;
    int length;

    struct piece *next;
} piece;

typedef struct flist{
    /**
       flag == 0 :
         nothing to do write must die
       flag == 1 :  
         the list is not emtpy so writer must countinue
        to execute.
	flag == 2 :
	  Is this realy needed?
     */
    int flag;
    ht_torrent *t;
    piece *list;
} flist;


void tr_insert(ht_torrent * t, char *url);
piece* add_piece(piece *l, int64_t offset, int begin, int length);
piece* remove_piece(piece *l, int offset, int begin, int length);

#endif
