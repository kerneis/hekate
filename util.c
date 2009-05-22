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
