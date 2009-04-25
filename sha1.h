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

/* EOF */
