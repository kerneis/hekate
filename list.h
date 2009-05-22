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

typedef struct chunk {
    int64_t offset;
    int begin;
    int length;
    struct chunk *next;
} chunk;

typedef struct peer {
    int stream_writer;
    ht_torrent *t;
    int credit;
    struct chunk *list;
} peer;


void tr_insert(ht_torrent *t, char *url);
struct chunk *add_chunk(struct chunk *l, int64_t offset, int begin, int length);
struct chunk *remove_chunk(struct chunk *l, int offset, int begin, int length);

#endif
