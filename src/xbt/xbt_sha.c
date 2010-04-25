/* xbt_sha.c - SHA1 hash function */

/* Initial version part of iksemel (XML parser for Jabber)
 *   Copyright (C) 2000-2003 Gurer Ozen <madcat@e-kolay.net>. All right reserved.
 *   Distributed under LGPL v2.1, February 1999.
 */

/* Later adapted to fit into SimGrid. Distributed under LGPL v2.1, Feb 1999.*/
/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/hash.h"

struct s_xbt_sha_ {
  unsigned int hash[5];
  unsigned int buf[80];
  int blen;
  unsigned int lenhi, lenlo;
};
static void sha_calculate(xbt_sha_t sha);

/* ************** */
/* User Interface */
/* ************** */

/** @brief constructor */
xbt_sha_t xbt_sha_new(void)
{
  xbt_sha_t sha;

  sha = xbt_new(s_xbt_sha_t, 1);
  xbt_sha_reset(sha);

  return sha;
}

/** @brief destructor */
void xbt_sha_free(xbt_sha_t sha)
{
  free(sha);
}

void xbt_sha_reset(xbt_sha_t sha)
{
  memset(sha, 0, sizeof(s_xbt_sha_t));
  sha->hash[0] = 0x67452301;
  sha->hash[1] = 0xefcdab89;
  sha->hash[2] = 0x98badcfe;
  sha->hash[3] = 0x10325476;
  sha->hash[4] = 0xc3d2e1f0;
}

/* @brief Add some more data to the buffer */
void xbt_sha_feed(xbt_sha_t sha, const unsigned char *data, size_t len)
{
  int i;

  for (i = 0; i < len; i++) {
    sha->buf[sha->blen / 4] <<= 8;
    sha->buf[sha->blen / 4] |= (unsigned int) data[i];
    if ((++sha->blen) % 64 == 0) {
      sha_calculate(sha);
      sha->blen = 0;
    }
    sha->lenlo += 8;
    sha->lenhi += (sha->lenlo < 8);
  }
}

/* finalize computation before displaying the result */
static void xbt_sha_finalize(xbt_sha_t sha)
{
  unsigned char pad[8];
  unsigned char padc;

  pad[0] = (unsigned char) ((sha->lenhi >> 24) & 0xff);
  pad[1] = (unsigned char) ((sha->lenhi >> 16) & 0xff);
  pad[2] = (unsigned char) ((sha->lenhi >> 8) & 0xff);
  pad[3] = (unsigned char) (sha->lenhi & 0xff);
  pad[4] = (unsigned char) ((sha->lenlo >> 24) & 0xff);
  pad[5] = (unsigned char) ((sha->lenlo >> 16) & 0xff);
  pad[6] = (unsigned char) ((sha->lenlo >> 8) & 0xff);
  pad[7] = (unsigned char) (sha->lenlo & 255);

  padc = 0x80;
  xbt_sha_feed(sha, &padc, 1);

  padc = 0x00;
  while (sha->blen != 56)
    xbt_sha_feed(sha, &padc, 1);

  xbt_sha_feed(sha, pad, 8);
}

/** @brief returns the sha hash into a newly allocated buffer (+ reset sha object) */
char *xbt_sha_read(xbt_sha_t sha)
{
  char *res = xbt_malloc(40);
  xbt_sha_print(sha, res);
  return res;
}

/** @brief copy the content sha hash into the @a hash pre-allocated string (and reset buffer) */
void xbt_sha_print(xbt_sha_t sha, char *hash)
{
  int i;

  xbt_sha_finalize(sha);
  for (i = 0; i < 5; i++) {
    sprintf(hash, "%08x", sha->hash[i]);
    hash += 8;
  }
}


/** @brief simply compute a SHA1 hash and copy it to the provided buffer */
void xbt_sha(const char *data, char *hash)
{
  s_xbt_sha_t sha;

  xbt_sha_reset(&sha);
  xbt_sha_feed(&sha, (const unsigned char *) data, strlen(data));

  xbt_sha_print(&sha, hash);
}

/* ********************* */
/* Core of the algorithm */
/* ********************* */

#define SRL(x,y) (((x) << (y)) | ((x) >> (32-(y))))
#define SHA(a,b,f,c) \
  for (i= (a) ; i<= (b) ; i++) { \
    TMP = SRL(A,5) + ( (f) ) + E + sha->buf[i] + (c) ; \
    E = D; \
    D = C; \
    C = SRL(B,30); \
    B = A; \
    A = TMP; \
  }

static void sha_calculate(xbt_sha_t sha)
{
  int i;
  unsigned int A, B, C, D, E, TMP;

  for (i = 16; i < 80; i++)
    sha->buf[i] =
      SRL(sha->buf[i - 3] ^ sha->buf[i - 8] ^ sha->
          buf[i - 14] ^ sha->buf[i - 16], 1);

  A = sha->hash[0];
  B = sha->hash[1];
  C = sha->hash[2];
  D = sha->hash[3];
  E = sha->hash[4];

  SHA(0, 19, ((C ^ D) & B) ^ D, 0x5a827999);
  SHA(20, 39, B ^ C ^ D, 0x6ed9eba1);
  SHA(40, 59, (B & C) | (D & (B | C)), 0x8f1bbcdc);
  SHA(60, 79, B ^ C ^ D, 0xca62c1d6);

  sha->hash[0] += A;
  sha->hash[1] += B;
  sha->hash[2] += C;
  sha->hash[3] += D;
  sha->hash[4] += E;
}

/* ************* */
/* Testing stuff */
/* ************* */
#ifdef SIMGRID_TEST
#include "xbt/hash.h"
#include "portable.h"           /* hexa_str */

static char *mycmp(const char *p1, const char *p2, size_t n)
{
  int i;

  for (i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return bprintf("Differs on %d -- Ox%x", i, p1[i]);
    }
  }
  return xbt_strdup("");
}

static void test_sha(const char *clear, const char *hashed)
{
  char hash[41];
  xbt_sha(clear, hash);

  xbt_test_add1("==== Test with '%s'", clear);
  xbt_test_assert3(!memcmp(hash, hashed, 40), "Wrong sha: %40s!=%40s (%s)",
                   hash, hashed, mycmp(hash, hashed, 40));
}

XBT_TEST_SUITE("hash", "Various hash functions");

XBT_LOG_NEW_DEFAULT_CATEGORY(hash, "Tests of various hash functions ");


XBT_TEST_UNIT("sha", test_crypto_sha, "Test of the sha algorithm")
{
  /* Empty string as test vector */
  test_sha("", "da39a3ee5e6b4b0d3255bfef95601890afd80709");

  /* Some pangram as test vector */
  test_sha("The quick brown fox jumps over the lazy dog",
           "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
  test_sha("Woven silk pyjamas exchanged for blue quartz",
           "da3aff337c810c6470db4dbf0f205c8afc31c442");
  test_sha("Pack my box with five dozen liquor jugs",
           "373ba8be29d4d95708bf7cd43038f4e409dcb439");

}
#endif /* SIMGRID_TEST */
