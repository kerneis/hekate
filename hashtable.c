#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "parse.h"
#include "hashtable.h"

uint
hash(unsigned char *key, int ht_size)
{
    uint *seed = (uint *)key;
    return (seed[0] ^ seed[1] ^ seed[2] ^ seed[3] ^ seed[4]) % ht_size;
}


hashtable *
ht_create(int size)
{
    hashtable * ht = malloc(sizeof(hashtable));
    if(!ht) return NULL;

    ht->size = size;
    ht->table = calloc(size, sizeof(ht_element *));
    if(!ht->table) return NULL;

    return ht;
}


unsigned char *
ht_insert(hashtable * ht, unsigned char *key, void *value)
{
    uint h = hash(key, ht->size);
    ht_element * hte = malloc(sizeof(ht_element));
    if(!hte) return NULL;

    memcpy(hte->key, key, 20);
    if(ht->table[h])
      hte->next = ht->table[h];
    else
      hte->next = NULL;
    hte->content = value;
    
    ht->table[h] = hte;
    return hte->key;
}


void*
ht_get(hashtable * ht, unsigned char *key)
{
    uint h = hash(key, ht->size);
    ht_element * hte = ht->table[h];
    while(hte) {
        if(!memcmp(key, hte->key, 20))
            return hte->content;
        hte = hte->next;
    }
    return NULL;
}


elmt_pos* 
ht_init_scan(hashtable * table)
{
    elmt_pos * res = malloc(sizeof(elmt_pos));
    if(!res) return NULL;

    res->ht = table;
    res->array_pos = 0;
    res->list_pos = NULL;

    return res;
}

void*
ht_get_next(elmt_pos * pos)
{
    int i;
    assert(pos!=NULL);
    
    if(!pos->list_pos && !pos->list_pos->next){
	pos->list_pos = pos->list_pos->next;
	return pos->list_pos;
    }

    for(i=pos->array_pos; i<pos->ht->size; i++){
	if(pos->ht->table[i]){
	    pos->array_pos = i;
	    pos->list_pos = pos->ht->table[i];
	    return pos->ht->table[i]->content;
	}
    }

    free(pos);
    return NULL;
}


int
ht_info_load(ht_torrent * elmt, benc *raw)
{
    int i;
    int64_t j;
    chunk * chunk;
    int64_t chunks_num;
      
    for(i=0; i<raw->set.used; i+=2) {

        /* Use the fact that dictionnary are sorted */

        if((raw->set.l[i])->type != STRING) {
            free_benc(raw);
            return -2;
        }

        if(!strcmp((raw->set.l[i])->s, "name") &&
                (raw->set.l[i+1])->type == STRING ) {
            elmt->path = (raw->set.l[i+1])->s;
            raw->set.l[i+1]->s = NULL;
            continue;
        }

        if(!strcmp((raw->set.l[i])->s, "piece length") &&
                (raw->set.l[i+1])->type == INT) {
            elmt->p_length = (raw->set.l[i+1])->i;
            continue;
        }

        if(!strcmp((raw->set.l[i])->s, "length") &&
                (raw->set.l[i+1])->type == INT ) {
            elmt->f_length = (raw->set.l[i+1])->i;
            continue;
        }

        /* TODO: case files */

        if(!strcmp((raw->set.l[i])->s, "pieces") &&
                (raw->set.l[i+1])->type == STRING ) {

            assert(elmt->p_length>0 && elmt->f_length>0);
            chunks_num = (elmt->f_length-1)/elmt->p_length+1;

            elmt->hash = ht_create(chunks_num);
            if(!elmt->hash) {
                perror("ht_create");
                return -1;
            }

            for(j=0LL; j<chunks_num; j++) {
                chunk = malloc(sizeof(struct chunk));
                if(!chunk) {
                    perror("creating chunk");
                    return -1;
                }
                chunk->offset = j*elmt->p_length;

                if(!ht_insert(elmt->hash, (unsigned char *)(raw->set.l[i+1]->s)+20LL*j, chunk)) {
                    perror("ht_insert");
                    return -1;
                }
            }
        }
    }
    return 0;
}

int
ht_load(hashtable * table, benc *raw)
{
    int i, rc;
    ht_torrent * elmt = malloc(sizeof(ht_torrent));
    if(!elmt) {
        perror("ht_load");
        return -1;
    }
    if(raw->type != DICT) {
        free_benc(raw);
        return -2;
    }

    for(i=0; i<raw->set.used; i+=2) {
        if( (raw->set.l[i])->type != STRING ) {
            free_benc(raw);
            return -2;
        }

        printf("%s\n", raw->set.l[i]->s);

        if(!strcmp((raw->set.l[i])->s, "announce") &&
                (raw->set.l[i+1])->type == STRING ) {
            elmt->tracker = (raw->set.l[i+1])->s;
            raw->set.l[i+1]->s = NULL;
            continue;
        }

        if(!strcmp((raw->set.l[i])->s, "info") &&
                (raw->set.l[i+1])->type == DICT ) {
            rc = ht_info_load(elmt, raw->set.l[i+1]);
            if( rc<0 ) {
                free_benc(raw);
                return rc;
            }
        }
    }

    if(elmt->path == NULL ||
            elmt->tracker == NULL ||
            elmt->f_length == 0 ||
            elmt->p_length == 0 ||
            elmt->hash == NULL ) {
        free_benc(raw);
        return -2;
    }
    if(!(elmt->info_hash=ht_insert(table, raw->hash, elmt))) {
        perror("ht_insert");
        return -1;
    }
    if(!linsert_tracker(elmt)) {
	perror("linsert");
	return -1;
    }

    free_benc(raw);
    return 0;
}
