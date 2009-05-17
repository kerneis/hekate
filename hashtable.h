#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "parse.h"

typedef unsigned int uint;

typedef struct ht_element{
    unsigned char key[20];
    struct ht_element *next;
    void *content;
}ht_element;

typedef struct hashtable{
    int size;
    ht_element **table;
}hashtable;

hashtable * ht_create(int size);
unsigned char * ht_insert(hashtable * ht, unsigned char *key, void *value);
void * ht_get(hashtable * ht, unsigned char *key);


typedef struct ht_torrent{
    /* file path */
    char * path;

    /* length of the file in bits */
    int64_t f_length;

    /* pieces size in bytes */
    int64_t p_length;

    /* hastable containing the sha1/offset_in_file for each chunk */
    hashtable * hash;

    unsigned char * info_hash;

} ht_torrent;


typedef struct piece{
    /* TO-DO: add a file field for multi-files torrents */
    /* offset associated with a chunk in a file */
    int64_t offset;
    int begin;
    int length;

}piece;

int ht_load(hashtable * table, char *curr_path, benc *raw);
int ht_info_load(ht_torrent * elmt, char *curr_path, benc *raw);
#endif
