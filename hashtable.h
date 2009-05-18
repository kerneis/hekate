#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "parse.h"

typedef struct ht_torrent{
    /* hashtable fields */
    unsigned char key[20];
    struct ht_torrent *next;

    /* file path */
    char *path;

    /* length of the file in bits */
    int64_t f_length;

    /* pieces size in bytes */
    int64_t p_length;

    unsigned char *info_hash;

    void *map;
} ht_torrent;

typedef struct hashtable{
    int size;
    ht_torrent **table;
}hashtable;


hashtable * ht_create(int size);
unsigned char * ht_insert(hashtable *ht, ht_torrent *hte);
void * ht_get(hashtable *ht, unsigned char *key);

int ht_load(hashtable *table, char *curr_path, benc *raw);
int ht_info_load(ht_torrent *elmt, char *curr_path, benc *raw);
#endif
