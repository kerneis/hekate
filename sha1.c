/* SHA1 routines */
/* Written by David Madore <david.madore@ens.fr> */
/* Public domain (2001/05/12) */
/* This version last modified 2001/05/12 */

/* Note: these routines do not depend on endianness. */

/* === The header === */

/* Put this in sha1.h if you don't like having everything in one big
 * file. */

#ifndef _DMADORE_SHA1_H
#define _DMADORE_SHA1_H

struct sha1_ctx {
  /* The five chaining variables */
  unsigned long buf[5];
  /* Count number of message bits */
  unsigned long bits[2];
  /* Data being fed in */
  unsigned long in[16];
  /* Our position within the 512 bits (always between 0 and 63) */
  int b;
};

void SHA1_transform (unsigned long buf[5], const unsigned long in[16]);
void SHA1_start (struct sha1_ctx *context);
void SHA1_feed (struct sha1_ctx *context, unsigned char inb);
void SHA1_stop (struct sha1_ctx *context, unsigned char digest[20]);

#endif /* not defined _DMADORE_SHA1_H */

/* === The implementation === */

#define F1(x, y, z) ((x & y) | ((~x) & z))
#define F2(x, y, z) (x ^ y ^ z)
#define F3(x, y, z) ((x & y) | (x & z) | (y & z))
#define F4(x, y, z) (x ^ y ^ z)

#define SHA1STEP(f, data) \
        { unsigned long temp; \
          temp = e + f (b, c, d) + data + ((a<<5) | (a>>27)); \
          e=d;  d=c;  c=((b<<30)&0xffffffffUL)|(b>>2); \
          b=a;  a=temp&0xffffffffUL; }

void
SHA1_transform (unsigned long buf[5], const unsigned long in[16])
{
  register unsigned long a, b, c, d, e;
  unsigned long w[80];
  int i;

  a = buf[0];  b = buf[1];  c = buf[2];  d = buf[3];  e = buf[4];
  for ( i=0 ; i<16 ; i++ )
    w[i] = in[i];
  for ( ; i<80 ; i++ )
    {
      w[i] = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
      w[i] = ((w[i]<<1)&0xffffffffUL) | (w[i]>>31);
    }
  for ( i=0 ; i<20 ; i++ )
    SHA1STEP (F1, w[i] + 0x5a827999UL);
  for ( ; i<40 ; i++ )
    SHA1STEP (F2, w[i] + 0x6ed9eba1UL);
  for ( ; i<60 ; i++ )
    SHA1STEP (F3, w[i] + 0x8f1bbcdcUL);
  for ( ; i<80 ; i++ )
    SHA1STEP (F4, w[i] + 0xca62c1d6UL);
  buf[0] += a;  buf[1] += b;  buf[2] += c;  buf[3] += d;  buf[4] += e;
  buf[0] &= 0xffffffffUL;  buf[1] &= 0xffffffffUL;
  buf[2] &= 0xffffffffUL;  buf[3] &= 0xffffffffUL;  buf[4] &= 0xffffffffUL;
}

#undef F1
#undef F2
#undef F3
#undef F4
#undef SHA1STEP

void
SHA1_start (struct sha1_ctx *ctx)
{
  int i;

  ctx->buf[0] = 0x67452301UL;
  ctx->buf[1] = 0xefcdab89UL;
  ctx->buf[2] = 0x98badcfeUL;
  ctx->buf[3] = 0x10325476UL;
  ctx->buf[4] = 0xc3d2e1f0UL;
  ctx->bits[0] = 0;
  ctx->bits[1] = 0;
  for ( i=0 ; i<16 ; i++ )
    ctx->in[i] = 0;
  ctx->b = 0;
}

void
SHA1_feed (struct sha1_ctx *ctx, unsigned char inb)
{
  int i;
  unsigned long temp;

  ctx->in[ctx->b/4] <<= 8;
  ctx->in[ctx->b/4] |= inb;
  if ( ++ctx->b >= 64 )
    {
      SHA1_transform (ctx->buf, ctx->in);
      ctx->b = 0;
      for ( i=0 ; i<16 ; i++ )
        ctx->in[i] = 0;
    }
  temp = ctx->bits[0];
  ctx->bits[0] += 8;
  ctx->bits[0] &= 0xffffffffUL;
  if ( temp > ctx->bits[0] )
    ctx->bits[1]++;
}

void
SHA1_stop (struct sha1_ctx *ctx, unsigned char digest[20])
{
  int i;
  unsigned long bits[2];

  for ( i=0 ; i<2 ; i++ )
    bits[i] = ctx->bits[i];
  SHA1_feed (ctx, 0x80);
  for ( ; ctx->b!=56 ; )
    SHA1_feed (ctx, 0);
  for ( i=1 ; i>=0 ; i-- )
    {
      SHA1_feed (ctx, (bits[i]>>24)&0xff);
      SHA1_feed (ctx, (bits[i]>>16)&0xff);
      SHA1_feed (ctx, (bits[i]>>8)&0xff);
      SHA1_feed (ctx, bits[i]&0xff);
    }
  for ( i=0 ; i<5 ; i++ )
    {
      digest[4*i] = (ctx->buf[i]>>24)&0xff;
      digest[4*i+1] = (ctx->buf[i]>>16)&0xff;
      digest[4*i+2] = (ctx->buf[i]>>8)&0xff;
      digest[4*i+3] = ctx->buf[i]&0xff;
    }
}
