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
#include <cpc/compatibility.h>
#include <Ws2tcpip.h>
#include <wincrypt.h>

#include "util.h"
/* #include <Bcrypt.h> MinGW hasn't Bcrypt.h */

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

long
random(void)
{
    long number;
    random_bytes(&number, sizeof(number));
    return number;
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

    closesocket(s);

    return rc;

 fail:
    save = errno;
    closesocket(s);
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
        errno = WSAEAFNOSUPPORT;
        return -1;
    }
}

int
find_global_address(int af, void *addr, int *addr_len)
{
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    int rc;

    debugf(5, "Try to get address of www.transmissionbt.com\n");
    /* This should be a name with both IPv4 and IPv6 addresses. */
    rc = get_name_source_address( af, "www.transmissionbt.com",
                                  (struct sockaddr*)&ss, &ss_len );
    debugf(5, "Failed. Try to get address of www.ietf.org\n");
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

/* Note that this is different from GNU's strndup(3). */
char *
strdup_n(const char *buf, int n)
{
    char *s;
    s = malloc(n + 1);
    if(s == NULL)
        return NULL;
    memcpy(s, buf, n);
    s[n] = '\0';
    return s;
}

char*
vsprintf_a(const char *f, va_list args)
{
    int n, size;
    char buf[64];
    char *string;
    va_list args_copy;

    va_copy(args_copy, args);
    n = vsnprintf(buf, 64, f, args_copy);
    if(n >= 0 && n < 64) {
        return strdup_n(buf, n);
    }
    if(n >= 64)
        size = n + 1;
    else
        size = 96;

    while(1) {
        string = malloc(size);
        if(!string)
            return NULL;
        va_copy(args_copy, args);
        n = vsnprintf(string, size, f, args_copy);
        if(n >= 0 && n < size)
            return string;
        else if(n >= size)
            size = n + 1;
        else
            size = size * 3 / 2;
        free(string);
        if(size > 16 * 1024)
            return NULL;
    }
    /* NOTREACHED */
}

char*
sprintf_a(const char *f, ...)
{
    char *s;
    va_list args;
    va_start(args, f);
    s = vsprintf_a(f, args);
    va_end(args);
    return s;
}

int
random_bytes(void *buffer, size_t size)
{
    static HCRYPTPROV hProv = NULL;
    int rc;
    if(hProv) goto key_container_acquired;
    rc = CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0);
    if(rc) goto key_container_acquired;
    if(GetLastError() != NTE_BAD_KEYSET) {
        print_error("Cannot acquire cryptographic service handle");
        goto cannot_have_cryptographic_service_handle;
    }
    rc = CryptAcquireContext(&hProv,NULL,NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);

 key_container_acquired:
    rc = CryptGenRandom(hProv, size, buffer);
    return rc ? size : -1;
 cannot_have_cryptographic_service_handle:
    for(rc = 0; rc < size; rc ++)
        *(unsigned char*)(buffer + rc) = (unsigned char) (rand() % 0xFF);
    return 0;
}
