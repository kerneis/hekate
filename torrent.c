/*
Copyright (c) 2009-2010 by Pejman Attar, Yoann Canal, Juliusz Chroboczek and
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>


#include "parse.h"
#include "torrent.h"

static int
concat_path(struct file *f, char *curr_path, benc *l)
{
    int i, pos, size;
    char *path, *tmp_path;

    pos = strlen(curr_path);
    size = (pos + 1) > 256 ? (pos + 1) : 256;
    path = malloc(size);
    if(!path) {
        perror("(concat_path)malloc");
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
                perror("(concat_path)realloc");
                free(path);
                return -1;
            }
        }
        memcpy(path + pos, l->set.l[i]->s, l->set.l[i]->size);
        pos += l->set.l[i]->size;
    }
    path[pos++] = '\0';
    tmp_path = realloc(path, pos);
    if(tmp_path)
        path = tmp_path;

    f->path = path;
    return 0;
}

void
free_torrent(struct torrent *t)
{
    int i;
    struct file *f;

    if(!t)
        return;

    if(t->info_hash)
        free(t->info_hash);

    if(t->tracker_url)
        free(t->tracker_url);

    if(t->files) {
        for(i = 0; i < t->num_files; i++) {
            f = t->files[i];
            if(f) {
                free(f->path);
                free(f);
            }
        }
    }

    free(t->files);
    free(t);
}

static int
parse_files(struct torrent *elmt, char *curr_path, benc *raw)
{
    int i, j, k, rc;
    uint64_t offset;
    benc *dico;
    struct file **tmp_files;
    struct file *f;

    offset = 0;
    k = 0;
    for(i=0; i<raw->size; i++) {
        dico = raw->set.l[i];
        if(dico->type != DICT) return -2;

        f = malloc(sizeof(struct file));
        if(!f) {
            perror("(parse_files)malloc");
            return -1;
        }

        f->offset = offset;
        f->map = NULL;

        for(j = 0; j < dico->size; j += 2) {
            if((dico->set.l[j])->type != STRING) {
                return -2;
            }

            if(strcmp((dico->set.l[j])->s, "length") == 0 &&
               (dico->set.l[j+1])->type == INT) {
                if(dico->set.l[j+1]->i < 0)
                    return -1;
                f->length = dico->set.l[j+1]->i;
            } else if(strcmp((dico->set.l[j])->s, "path") == 0 &&
                      (dico->set.l[j+1])->type == LIST) {
                rc = concat_path(f, curr_path, dico->set.l[j+1]);
                if(rc<0) return rc;
            }
        }

        if(f->length > 0) {
            offset += f->length;
            elmt->files[k++] = f;
        }
        else {
            elmt->num_files--;
            free(f->path);
            free(f);
        }
    }

    if(i != k) {
        tmp_files = realloc(elmt->files, elmt->num_files*sizeof(struct file *));
        if(tmp_files)
            elmt->files = tmp_files;
    }

    return 0;
}


parse_info(struct torrent *elmt, char *curr_path, benc *raw)
static int
{
    int i, rc, path_length;
    char *path = NULL;
    benc *multi_files = NULL;

    for(i = 0; i < raw->size; i += 2) {
        if((raw->set.l[i])->type != STRING)
            continue;
            
        if(strcmp((raw->set.l[i])->s, "length") == 0 &&
           (raw->set.l[i+1])->type == INT) {
            if(raw->set.l[i+1]->i < 0)
                return -1;

            elmt->files = malloc(sizeof(struct file *));
            if(!elmt->files) {
                perror("(parse_info)malloc files");
                return -1;
            }
            elmt->files[0] = calloc(1, sizeof(struct file));
            if(!elmt->files[0]){
                perror("(parse_info)calloc file");
                return -1;
            }
            
            elmt->num_files = 1;
            elmt->files[0]->length = raw->set.l[i+1]->i;
        } else if(strcmp((raw->set.l[i])->s, "files") == 0 &&
                  (raw->set.l[i+1])->type == LIST) {
            elmt->num_files = raw->set.l[i+1]->size;
            elmt->files = calloc(elmt->num_files, sizeof(struct file *));
            if(!elmt->files){
                perror("(parse_info)calloc files");
                return -1;
            }
            /* we need to know p_length */
            multi_files = raw->set.l[i+1];
        } else if(strcmp((raw->set.l[i])->s, "name") == 0 &&
                  (raw->set.l[i+1])->type == STRING) {
            path_length = raw->set.l[i+1]->size + strlen(curr_path) + 2;
            path = malloc(path_length);
            if(!path) {
                perror("(parse_info)malloc");
                return -1;
            }
            snprintf(path, path_length, "%s/%s",
                     curr_path, (raw->set.l[i+1])->s);
            
            /* in single file case: name of the only file */
            if(!multi_files) {
                elmt->files[0]->path = path;
            }
        } else if(strcmp((raw->set.l[i])->s, "piece length") == 0 &&
                  (raw->set.l[i+1])->type == INT) {
            if(raw->set.l[i+1]->i < 0)
                return -1;
            elmt->p_length = (raw->set.l[i+1])->i;
            
            /* now we can compute chunks offsets */
            if(multi_files) {
                rc = parse_files(elmt, path, multi_files);
                free(path);
                if(rc<0) return rc;
            }
        } else if(strcmp((raw->set.l[i])->s, "pieces") == 0 &&
                  (raw->set.l[i+1])->type == STRING) {
            assert(raw->set.l[i+1]->size >= 0);
            elmt->num_chunks = raw->set.l[i+1]->size/20;
        } else if(strcmp((raw->set.l[i])->s, "private") == 0 &&
                  (raw->set.l[i+1])->type == INT) {
            elmt->private = !!(raw->set.l[i+1])->i;
        }
    }
    return 0;
}

struct torrent *
parse_torrent(char *curr_path, benc *raw)
{
    int i, rc;
    struct torrent *elmt;

    elmt = calloc(1, sizeof(struct torrent));
    if(!elmt) {
        perror("(parse_torrent)calloc");
        goto internal_error;
    }

    if(raw->type != DICT)
        goto torrent_error;

    for(i=0; i < raw->size; i += 2) {
        if((raw->set.l[i])->type != STRING)
            continue;

        if(strcmp((raw->set.l[i])->s, "announce") == 0 &&
           (raw->set.l[i+1])->type == STRING) {
            elmt->tracker_url = (raw->set.l[i+1])->s;
            raw->set.l[i+1]->s = NULL;
        } else if(strcmp((raw->set.l[i])->s, "info") == 0 &&
                  (raw->set.l[i+1])->type == DICT ) {
            rc = parse_info(elmt, curr_path, raw->set.l[i+1]);
            if(rc < -1)
                goto torrent_error;
            else if(rc < 0)
                goto internal_error;
        }
    }

    /* copy info hash */
    elmt->info_hash = raw->hash;
    raw->hash = NULL;

    free_benc(raw);
    return elmt;

 internal_error:
    fprintf(stderr, "Internal error parsing a torrent in %s\n", curr_path);
    free_benc(raw);
    free_torrent(elmt);
    return NULL;

 torrent_error:
    fprintf(stderr, "Bad .torrent file in %s\n", curr_path);
    free_benc(raw);
    free_torrent(elmt);
    return NULL;
}

extern void debugf(int level, const char *format, ...);

int
validate_torrent(struct torrent *t)
{
    int i;
    struct stat s;

    for(i = 0; i < t->num_files; i++) {
        if(stat(t->files[i]->path, &s) < 0 ||
           s.st_size != t->files[i]->length)
            goto fail;
    }

    return 1;

  fail:
    debugf(2, "Validation failed for file %s\n", t->files[i]->path);
    return 0;
}
