#ifndef LIST_H
#define LIST_H

#include "hashtable.h"


typedef struct to_list{
    ht_torrent *elmt;
    struct to_list *next;
} to_list;

typedef struct pieces_list pieces_list;

struct pieces_list{
    piece *elmt;
    pieces_list * next;
};


typedef struct flist flist;

struct flist{
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
    pieces_list * list;
};

typedef struct tr_list{
    char *url;
    to_list *head;
    struct tr_list *next;
} tr_list;

void tr_insert(ht_torrent * t, char *url);
pieces_list* add_piece(pieces_list * l, int64_t offset, int begin, int length);
pieces_list* remove_piece(pieces_list *pl, int offset, int begin, int length);

#endif
