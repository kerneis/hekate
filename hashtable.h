#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "parse.h"

struct file {
    int64_t offset;
    int64_t length;
    char *path;
    char *map;
};

typedef struct ht_torrent {
    /* hashtable fields */
    unsigned char key[20];
    struct ht_torrent *next;

    /* pieces size in bytes */
    int64_t p_length;
    int32_t num_chunks;

    int num_files;
    struct file **files;
    unsigned char *info_hash;
} ht_torrent;

typedef struct hashtable{
    int size;
    ht_torrent **table;
}hashtable;


hashtable * ht_create(int size);
unsigned char * ht_insert(hashtable *ht, ht_torrent *hte);
ht_torrent * ht_get(hashtable *ht, unsigned char *key);

int ht_load(hashtable *table, char *curr_path, benc *raw);
int ht_info_load(ht_torrent *elmt, char *curr_path, benc *raw);
#endif
