/*
Copyright (c) 2009 by Pejman Attar, Yoann Canal, Juliusz Chroboczek and
                      Gabriel Kerneis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "torrent.h"

int
ht_concat_path(struct file *f, char *curr_path, benc *l)
{
    int i, pos, size;
    char *path, *tmp_path;

    pos = strlen(curr_path);
    size = (pos + 1) > 256 ? (pos + 1) : 256;
    path = malloc(size);
    if(!path) {
        perror("(ht_concat_path)malloc");
        return -1;
    }
    memcpy(path, curr_path, pos);

    for(i = 0; i < l->size; i++) {
        if(l->set.l[i]->type != STRING) {
            free(path);
            return -2;
        }

        path[pos++] = '/';
        while(pos + l->set.l[i]->size > size) {
            size *= 2;
            tmp_path = realloc(path, size);
            if(!tmp_path) {
                perror("(ht_concat_path)realloc");
                free(path);
                return -1;
            }
        }
        memcpy(path + pos, l->set.l[i]->s, l->set.l[i]->size);
        pos += l->set.l[i]->size;
    }
    path[pos++] = '\0';
    path = realloc(path, pos);

    f->path = path;
    return 0;
}

int
ht_files_load(struct torrent *elmt, char *curr_path, benc *raw)
{
    int i, j, c, rc;
    int64_t offset;
    benc *dico;
    struct file *f;

    offset = 0;
    for(i=0; i<raw->size; i++) {
        dico = raw->set.l[i];
        if(dico->type != DICT) return -2;

        f = malloc(sizeof(struct file));
        if(!f) {
            perror("(ht_files_load)malloc");
            return -1;
        }

        f->offset = offset;
        f->map = NULL;

        c = 0;
        for(j=0; j<dico->size; j+=2) {
            if((dico->set.l[j])->type != STRING) {
                return -2;
            }

            switch(c) {
            case 0:
                if(strcmp((dico->set.l[j])->s, "length") == 0 &&
                   (dico->set.l[j+1])->type == INT) {
                    f->length = dico->set.l[j+1]->i;
                    c = 1;
                }
                break;

            case 1:
                if(strcmp((dico->set.l[j])->s, "path") == 0 &&
                   (dico->set.l[j+1])->type == LIST) {
                    rc = ht_concat_path(f, curr_path, dico->set.l[j+1]);
                    if(rc<0) return rc;
                    c = 2;
                }
                break;

            default:
                j = dico->size;
            }
        }

        offset += f->length;
        elmt->files[i] = f;
    }
    return 0;
}


int
ht_info_load(struct torrent *elmt, char *curr_path, benc *raw)
{
    int i, c, rc, path_length;
    char *path = NULL;
    benc *multi_files = NULL;

    c=0; /* Use the fact that dictionnary are sorted */
    for(i=0; i<raw->size; i+=2) {

        if((raw->set.l[i])->type != STRING) {
            return -2;
        }

        switch(c){
        case 0:
            /* single file case */
            if(strcmp((raw->set.l[i])->s, "length") == 0 &&
               (raw->set.l[i+1])->type == INT) {
                elmt->files = malloc(sizeof(struct file *));
                if(!elmt->files){
                    perror("(ht_info_load)malloc files");
                    return -1;
                }
                elmt->files[0] = calloc(1, sizeof(struct file));
                if(!elmt->files[0]){
                    perror("(ht_info_load)malloc files");
                    return -1;
                }

                elmt->num_files = 1;
                elmt->files[0]->length = raw->set.l[i+1]->i;
                c = 1;
            }
            /* multi files case */
            if(strcmp((raw->set.l[i])->s, "files") == 0 &&
               (raw->set.l[i+1])->type == LIST) {
                elmt->num_files = raw->set.l[i+1]->size;
                elmt->files = calloc(elmt->num_files, sizeof(struct file *));
                if(!elmt->files){
                    perror("(ht_info_load)malloc files");
                    return -1;
                }
                /* we need to know p_length */
                multi_files = raw->set.l[i+1];
                c = 1;
            }
            break;

        case 1:
            if(strcmp((raw->set.l[i])->s, "name") == 0 &&
               (raw->set.l[i+1])->type == STRING) {
                path_length = raw->set.l[i+1]->size + strlen(curr_path) + 2;
                path = malloc(path_length);
                if(!path) {
                    perror("(ht_info_load)malloc");
                    return -1;
                }
                snprintf(path, path_length, "%s/%s",
                         curr_path, (raw->set.l[i+1])->s);

                /* in single file case: name of the only file */
                if(!multi_files) {
                    elmt->files[0]->path = path;
                }
                /* in multi files case: path will be the recommended path */
                c = 2;
            }
            break;

        case 2:
            if(strcmp((raw->set.l[i])->s, "piece length") == 0 &&
               (raw->set.l[i+1])->type == INT) {
                elmt->p_length = (raw->set.l[i+1])->i;

                /* now we can compute chunks offsets */
                if(multi_files) {
                    rc = ht_files_load(elmt, path, multi_files);
                    free(path);
                    if(rc<0) return rc;
                }
                c = 3;
            }
            break;

        case 3:
            if(strcmp((raw->set.l[i])->s, "pieces") == 0 &&
               (raw->set.l[i+1])->type == STRING) {
                elmt->num_chunks = raw->set.l[i+1]->size/20;
                c = 4;
            }
            break;

        default:
            i = raw->size; /* leave loop */
        }
    }
    if(c<4) return -2;
    return 0;
}

struct torrent *
ht_load(char *curr_path, benc *raw)
{
    /* XXX free correctement en cas d'erreur... */
    int i, c, rc;
    struct torrent *elmt;

    elmt = calloc(1, sizeof(struct torrent));
    if(!elmt) {
        perror("ht_load");
        goto error;
    }

    if(raw->type != DICT)
        goto torrent_error;

    c=0; /* Use the fact that dictionnary are sorted */
    for(i=0; i<raw->size; i+=2) {
        if((raw->set.l[i])->type != STRING)
            goto torrent_error;

        switch(c) {
        case 0:
            if(strcmp((raw->set.l[i])->s, "announce") == 0 &&
               (raw->set.l[i+1])->type == STRING) {
                elmt->tracker_url = (raw->set.l[i+1])->s;
                raw->set.l[i+1]->s = NULL;
                c++;
            }
            break;

        case 1:
            if(strcmp((raw->set.l[i])->s, "info") == 0 &&
               (raw->set.l[i+1])->type == DICT ) {
                rc = ht_info_load(elmt, curr_path, raw->set.l[i+1]);
                if(rc < -1)
                    goto torrent_error;
                else if(rc < 0)
                    goto error;
                c++;
            }
            break;

        default:
            i = raw->size; /* leave loop */
        }
    }

    /* copy info hash */
    elmt->info_hash = raw->hash;
    raw->hash = NULL;

    /* was the .torrent complete? */
    if(c<2 || !elmt->info_hash)
        goto torrent_error;

    free_benc(raw);
    return elmt;

 torrent_error:
    fprintf(stderr, "Bad .torrent file.\n");
 error:
    /* XXX free correctly */
    free_benc(raw);
    return NULL;
}
