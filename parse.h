#ifndef PARSE_H
#define PARSE_H

#include <inttypes.h>
#include "sha1.h"

#define BUF_SIZE 512
#define SET_BASE_SIZE 8

typedef enum benc_type{
  INT,
  STRING,
  LIST,
  DICT
} benc_type;

typedef struct benc benc;

struct benc {
  benc_type type;
  unsigned char hash[20];
  union {
    int64_t i;
    char *s;
    struct {
      benc **l;
      int size;
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
  struct sha1_ctx sha1;
  char hashing;
} buffer;

buffer * open_buffer(const char * pathname);
void close_buffer(buffer * b);      
benc * parsing(buffer * b);
#endif
