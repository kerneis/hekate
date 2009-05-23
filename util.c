#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "util.h"

size_t pagesize;

int
prefetch(void *begin, size_t length)
{
    size_t b = (size_t)begin;
    size_t br = b / pagesize * pagesize;
    size_t l = length + (b - br);
    size_t lr = (l + (pagesize - 1)) / pagesize * pagesize;
    return posix_madvise((void*)br, lr, POSIX_MADV_WILLNEED);
}

int
incore(void *begin, size_t length)
{
    size_t b = (size_t)begin;
    size_t br = b / pagesize * pagesize;
    size_t l = length + (b - br);
    size_t lr = (l + (pagesize - 1)) / pagesize * pagesize;
    unsigned char vec[32];
    int i, rc;

    if(lr > 32 * pagesize)
        return -1;

    rc = mincore((void*)br, lr, vec);
    if(rc < 0)
        return -1;

    for(i = 0; i < lr / pagesize; i++)
        if(!(vec[i] & 1))
            return 0;
    return 1;
}
