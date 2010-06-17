/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 179 "xbt/xbt_sha.c" 
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
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

