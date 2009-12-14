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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <ctype.h> /* isdigit */
#include <inttypes.h> /* int64_t */
#include <openssl/sha.h>

#include "parse.h"
#include "hashtable.h"

benc *
make_benc(benc_type type)
{
    benc *r;

    r = calloc(1, sizeof(benc));

    switch (r->type = type) {
        case INT:
        case STRING:
            break;
         case LIST:
         case DICT:
            r->set.l = malloc(SET_BASE_SIZE*sizeof(benc *));
            r->size = SET_BASE_SIZE;
            r->set.used = 0;
    }
    return r;
}

inline void
add_set (benc *b, benc *obj)
{
    assert(b->type == LIST || b->type == DICT);
    if(b->size == b->set.used) {
        b->set.l = realloc(b->set.l, 2*b->size*sizeof(benc *));
        b->size *= 2;
    }
    b->set.l[b->set.used] = obj;
    b->set.used++;
}


buffer *
open_buffer(const char *pathname)
{
    int fd;
    buffer *b;

    if(!(fd = open(pathname, O_RDONLY|O_NONBLOCK)))
        { perror("open_buffer)open"); return NULL; }

    if(!(b = malloc(sizeof(buffer))))
        { perror("open_buffer)malloc"); close(fd); return NULL; }

    b->fd = fd;
    b->cur = 0;
    b->eof = -1;
    b->buf = malloc(BUF_SIZE);
    b->hashing = 0;

    return b;
}

void
close_buffer(buffer *b)
{
    close(b->fd);
    free(b->buf);
    free(b);
}

inline void
start_hashing(buffer *b)
{
    SHA1_Init(&b->sha1);
    b->hashing = 1;
}

inline void
stop_hashing(buffer *b, unsigned char *result)
{
    SHA1_Final(result, &b->sha1);
    b->hashing = 0;
}

/* shall we return an int to allow error codes? */
/*cps*/ char
get_byte(buffer *b)
{
    int rc=0, i = 0;
    char c;

    if(b->cur == 0) {
       while(i < BUF_SIZE) {
           rc = read(b->fd, b->buf+i, BUF_SIZE-rc);
           if(rc == -1) {
               if(errno != EAGAIN) {
                   perror("read (in get_byte)");
                   exit(1);
               }
           }
           else if(rc == 0) {
               b->eof = i;
               break;
           }
           else
               i += rc;
           /*cpc_yield;*/
       }
    }
    if(b->cur == b->eof)
        return '\0';
    c = b->buf[b->cur];
    b->cur = (b->cur + 1) % BUF_SIZE;
    if(b->hashing)
        SHA1_Update(&b->sha1, &c, 1);
    return c;
}

#define eof_buffer( b ) (b->cur == b->eof)

/*cps*/ char *
get_string(buffer *b, int64_t n)
{
    int64_t i;
    char *s = malloc((size_t)(n+1));
    for(i=0; i<n; i++) {
        if(eof_buffer(b)) { /* This should not happen. */
            printf("Warning: EOF in get_string\n");
            s = realloc(s, (size_t)(i+1)); /* XXX */
            break;
        }
        s[i] = get_byte(b);
    }
    s[i] = '\0';
    return s;
}

/*cps*/ int64_t
parse_int(buffer *b, char c, char end)
{
    int64_t r = 0;
    char negative = 0;

    if(c == '-') {
        negative = 1;
        c = get_byte(b);
    }
    while(c != end) {
        if(!isdigit(c)){
            fprintf(stderr, "(parse_int)bad written torrent\n");
            exit(EXIT_FAILURE); /* XXX fail more gracefully? this includes eof */
        }
        r = r * 10LL + ((int64_t)(c - '0'));
        c = get_byte(b);
    }
    return (negative ? -r : r);
}

/*cps*/ void
parse_list (buffer *b, benc *r)
{
    benc *tmp;

    while((tmp = parsing(b)) != NULL)
        add_set(r, tmp);
    r->size = r->set.used;
}

/*cps*/ void
parse_dict (buffer *b, benc *r)
{
    benc *key, *value;

    while((key = parsing(b)) != NULL) {
        assert(key->type == STRING);
        add_set(r, key);
        if((!b->hashing) && !strcmp(key->s, "info")) {
            start_hashing(b);
            value = parsing(b);
            r->hash = malloc(20);
            if(!r->hash) {
                perror("(parse_dict)malloc");
                return;
            }
            stop_hashing(b, r->hash);
        } else
            value = parsing(b);
        add_set(r,value);
    }
    r->size = r->set.used;
}

/*cps*/ benc *
parsing(buffer *b)
{
    int64_t n;
    char c;

    benc *r = NULL;

    if(eof_buffer(b))
        return r;

    switch (c=get_byte(b)) {
        case 'i':
            r = make_benc(INT);
            r->i = parse_int(b,get_byte(b),'e');
            break;
        case 'd':
            /* printf("dict!\n"); */
            r = make_benc(DICT);
            parse_dict(b, r);
            return r;
        case 'l':
            /* printf("list!\n"); */
            r = make_benc(LIST);
            parse_list(b, r);
            return r;
        case 'e': /* end of a list or dict: return NULL */
            /* printf("end!\n"); */
            break;
        default:
            if(!isdigit(c)) {
                printf("error with char: %c (%x) at %d\n", c, c,
                        b->cur);
                break;
            }
            r = make_benc(STRING);
            n = parse_int(b,c,':');
            r->size = n;
            r->s = get_string(b,n);
            /* printf("string of length %lld\n", (long long int)n); */
    }
    return r;
}

void
free_benc(benc *node)
{
    int i;
    switch(node->type) {
        case INT:
            break;
        case STRING:
            if( node->s )
                free(node->s);
            break;
        case LIST:
        case DICT:
            for(i=0; i<node->set.used; i++)
                free_benc(node->set.l[i]);
            free(node->set.l);
            break;
    }
    free(node);
}
