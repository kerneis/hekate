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

#ifndef PARSE_H
#define PARSE_H

#include <inttypes.h>
#include <openssl/sha.h>

#define BUF_SIZE 512
#define SET_BASE_SIZE 8

typedef enum benc_type{
  TOKEN_INT,
  TOKEN_STRING,
  TOKEN_LIST,
  TOKEN_DICT
} benc_type;

typedef struct benc benc;

struct benc {
  benc_type type;
  unsigned char *hash;
  int size;
  union {
    int64_t i;
    char *s;
    struct {
      benc **l;
      int used;
    } set;
  };
};

void free_benc(benc * node);

typedef struct buffer {
  char * buf;
  int fd;
  int cur;
  int eof;
  SHA_CTX sha1;
  char hashing;
} buffer;

buffer * open_buffer(const char * pathname);
void close_buffer(buffer * b);
benc * parsing(buffer * b);
#endif
