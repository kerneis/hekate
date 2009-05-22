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
    int64_t offset;
    int begin;
    int length;

    struct piece *next;
} piece;

typedef struct flist{
    int stream_writer;
    ht_torrent *t;
    piece *list;
} flist;


void tr_insert(ht_torrent *t, char *url);
piece * add_piece(piece *l, int64_t offset, int begin, int length);
piece * remove_piece(piece *l, int offset, int begin, int length);

#endif
