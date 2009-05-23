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
