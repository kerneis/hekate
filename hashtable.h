#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "parse.h"

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
unsigned char * ht_insert(hashtable *ht, unsigned char *key, void *value);
void * ht_get(hashtable *ht, unsigned char *key);


typedef struct ht_torrent{
    /* file path */
    char *path;

    /* length of the file in bits */
    int64_t f_length;

    /* pieces size in bytes */
    int64_t p_length;

    unsigned char *info_hash;
} ht_torrent;


int ht_load(hashtable *table, char *curr_path, benc *raw);
int ht_info_load(ht_torrent *elmt, char *curr_path, benc *raw);
#endif
