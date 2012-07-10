/*
Copyright (c) 2009-2010 by Juliusz Chroboczek

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

#define NO_CPS_PROTO
#include <cpc/cpc_runtime.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <assert.h>

#include "util.h"

int debug_level = 0;
size_t pagesize;

void
debugf(int level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if(debug_level >= level) {
        vfprintf(stderr, format, args);
        fflush(stderr);
    }
    va_end(args);
}

int
prefetch(void *begin, size_t length)
{
    size_t b = (size_t)begin;
    size_t br = b / pagesize * pagesize;
    size_t l = length + (b - br);
    size_t lr = (l + (pagesize - 1)) / pagesize * pagesize;
    assert(length > 0 && length <= LARGE_CHUNK);
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

    rc = mincore((void*)br, l, (void*)vec);
    if(rc < 0)
        return -1;

    for(i = 0; i < lr / pagesize; i++)
        if(!(vec[i] & 1))
            return 0;
    return 1;
}

/* Get the source address used for a given interface address.  Since there
   is no official interface to get this information, we create a connected
   UDP socket (connected UDP... hmm...) and check its source address. */
int
get_source_address(const struct sockaddr *dst, socklen_t dst_len,
                   struct sockaddr *src, socklen_t *src_len)
{
    int s, rc, save;

    s = socket(dst->sa_family, SOCK_DGRAM, 0);
    if(s < 0)
        goto fail;

    /* Since it's a UDP socket, this doesn't actually send any packets. */
    rc = connect(s, dst, dst_len);
    if(rc < 0)
        goto fail;

    rc = getsockname(s, src, src_len);
    if(rc < 0)
        goto fail;

    close(s);

    return rc;

 fail:
    save = errno;
    close(s);
    errno = save;
    return -1;
}

/* Like above, but for a given DNS name. */
int
get_name_source_address(int af, const char *name,
                        struct sockaddr *src, socklen_t *src_len)
{
    struct addrinfo hints, *info, *infop;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_DGRAM;

    rc = getaddrinfo(name, NULL, &hints, &info);
    if(rc != 0) {
        errno = ENOENT;
        return -1;
    }

    rc = -1;
    errno = ENOENT;
    infop = info;
    while(infop) {
        if(infop->ai_addr->sa_family == af) {
            rc = get_source_address(infop->ai_addr, infop->ai_addrlen,
                                    src, src_len);
            if(rc >= 0)
                break;
        }
        infop = infop->ai_next;
    }

    freeaddrinfo(info);
    return rc;
}

/* We all hate NATs. */
int
global_unicast_address(struct sockaddr *sa)
{
    if(sa->sa_family == AF_INET) {
        const unsigned char *a =
            (unsigned char*)&((struct sockaddr_in*)sa)->sin_addr;
        if(a[0] == 0 || a[0] == 127 || a[0] >= 224 ||
           a[0] == 10 || (a[0] == 172 && a[1] >= 16 && a[1] <= 31) ||
           (a[0] == 192 && a[1] == 168))
            return 0;
        return 1;
    } else if(sa->sa_family == AF_INET6) {
        const unsigned char *a =
            (unsigned char*)&((struct sockaddr_in6*)sa)->sin6_addr;
        /* 2000::/3 */
        return (a[0] & 0xE0) == 0x20;
    } else {
        errno = EAFNOSUPPORT;
        return -1;
    }
}

int
find_global_address(int af, void *addr, int *addr_len)
{
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    int rc;

    /* This should be a name with both IPv4 and IPv6 addresses. */
    rc = get_name_source_address( af, "www.transmissionbt.com",
                                  (struct sockaddr*)&ss, &ss_len );
    /* In case Charles removes IPv6 from his website. */
    if( rc < 0 )
        rc = get_name_source_address(  af, "www.ietf.org",
                                      (struct sockaddr*)&ss, &ss_len );

    if( rc < 0 )
        return -1;

    if( !global_unicast_address( (struct sockaddr*)&ss) )
        return -1;

    switch(af) {
    case AF_INET:
        if(*addr_len < 4)
            return -1;
        memcpy(addr, &((struct sockaddr_in*)&ss)->sin_addr, 4);
        *addr_len = 4;
        return 1;
    case AF_INET6:
        if(*addr_len < 16)
            return -1;
        memcpy(addr, &((struct sockaddr_in6*)&ss)->sin6_addr, 16);
        *addr_len = 16;
        return 1;
    default:
        return -1;
    }
}
