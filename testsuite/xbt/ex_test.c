/*
**  OSSP ex - Exception Handling
**  Copyright (c) 2002-2004 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2002-2004 The OSSP Project <http://www.ossp.org/>
**  Copyright (c) 2002-2004 Cable & Wireless <http://www.cw.com/>
**
**  This file is part of OSSP ex, an exception handling library
**  which can be found at http://www.ossp.org/pkg/lib/ex/.
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
**
**  ex_test.c: exception handling test suite
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "ex_test_ts.h"
#include "xbt/ex.h"

TS_TEST(test_controlflow)
{
    ex_t ex;
    volatile int n;

    ts_test_check(TS_CTX, "basic nested control flow");
    n = 1;
    xbt_try {
        if (n != 1)
            ts_test_fail(TS_CTX, "M1: n=%d (!= 1)", n);
        n++;
        xbt_try {
            if (n != 2)
                ts_test_fail(TS_CTX, "M2: n=%d (!= 2)", n);
            n++;
            xbt_throw(0,0,"something");
        }
        xbt_catch (ex) {
            if (n != 3)
                ts_test_fail(TS_CTX, "M3: n=%d (!= 1)", n);
            n++;
            xbt_rethrow;
        }
        ts_test_fail(TS_CTX, "MX: n=%d (expected: not reached)", n);
    }
    xbt_catch (ex) {
        if (n != 4)
            ts_test_fail(TS_CTX, "M4: n=%d (!= 4)", n);
        n++;
    }
    if (n != 5)
        ts_test_fail(TS_CTX, "M5: n=%d (!= 5)", n);
}

TS_TEST(test_value)
{
    ex_t ex;

    xbt_try {
        xbt_throw(1, 2, "toto");
    }
    xbt_catch (ex) {
        ts_test_check(TS_CTX, "exception value passing");
        if (ex.category != 1)
            ts_test_fail(TS_CTX, "category=%d (!= 1)", ex.category);
        if (ex.value != 2)
            ts_test_fail(TS_CTX, "value=%d (!= 2)", ex.value);
        if (strcmp(ex.msg,"toto"))
            ts_test_fail(TS_CTX, "message=%s (!= toto)", ex.msg);
    }
}

TS_TEST(test_variables)
{
    ex_t ex;
    int r1, r2;
    volatile int v1, v2;

    r1 = r2 = v1 = v2 = 1234;
    xbt_try {
        r2 = 5678;
        v2 = 5678;
        xbt_throw(0, 0, 0);
    }
    xbt_catch (ex) {
        ts_test_check(TS_CTX, "variable preservation");
        if (r1 != 1234)
            ts_test_fail(TS_CTX, "r1=%d (!= 1234)", r1);
        if (v1 != 1234)
            ts_test_fail(TS_CTX, "v1=%d (!= 1234)", v1);
        /* r2 is allowed to be destroyed because not volatile */
        if (v2 != 5678)
            ts_test_fail(TS_CTX, "v2=%d (!= 5678)", v2);
    }
}

TS_TEST(test_defer)
{
    ex_t ex;
    volatile int i1 = 0;
    volatile int i2 = 0;
    volatile int i3 = 0;

    ts_test_check(TS_CTX, "exception deferring");
    if (xbt_deferring)
        ts_test_fail(TS_CTX, "unexpected deferring scope");
    xbt_try {
        xbt_defer {
            if (!xbt_deferring)
                ts_test_fail(TS_CTX, "unexpected non-deferring scope");
            xbt_defer {
                i1 = 1;
                xbt_throw(4711, 0, NULL);
                i2 = 2;
                xbt_throw(0, 0, NULL);
                i3 = 3;
                xbt_throw(0, 0, NULL);
            }
            xbt_throw(0, 0, 0);
        }
        ts_test_fail(TS_CTX, "unexpected not occurred deferred throwing");
    }
    xbt_catch (ex) {
        if (ex.category != 4711)
            ts_test_fail(TS_CTX, "caught exception with value %d, expected 4711", ex.value);
    }
    if (i1 != 1)
        ts_test_fail(TS_CTX, "v.i1 not set (expected 1, got %d)", i1);
    if (i2 != 2)
        ts_test_fail(TS_CTX, "v.i2 not set (expected 2, got %d)", i2);
    if (i3 != 3)
        ts_test_fail(TS_CTX, "v.i3 not set (expected 3, got %d)", i3);
}

TS_TEST(test_shield)
{
    ex_t ex;

    ts_test_check(TS_CTX, "exception shielding");
    if (xbt_shielding)
        ts_test_fail(TS_CTX, "unexpected shielding scope");
    if (xbt_catching)
        ts_test_fail(TS_CTX, "unexpected catching scope");
    xbt_try {
        xbt_shield {
            if (!xbt_shielding)
                ts_test_fail(TS_CTX, "unexpected non-shielding scope");
            xbt_throw(0, 0, 0);
        }
        if (xbt_shielding)
            ts_test_fail(TS_CTX, "unexpected shielding scope");
        if (!xbt_catching)
            ts_test_fail(TS_CTX, "unexpected non-catching scope");
    }
    xbt_catch (ex) {
        ts_test_fail(TS_CTX, "unexpected exception catched");
        if (xbt_catching)
            ts_test_fail(TS_CTX, "unexpected catching scope");
    }
    if (xbt_catching)
        ts_test_fail(TS_CTX, "unexpected catching scope");
}

TS_TEST(test_cleanup)
{
    ex_t ex;
    volatile int v1;
    int c;

    ts_test_check(TS_CTX, "cleanup handling");

    v1 = 1234;
    c = 0;
    xbt_try {
        v1 = 5678;
        xbt_throw(1, 2, "blah");
    }
    xbt_cleanup {
        if (v1 != 5678)
            ts_test_fail(TS_CTX, "v1 = %d (!= 5678)", v1);
        c = 1;
    }
    xbt_catch (ex) {
        if (v1 != 5678)
            ts_test_fail(TS_CTX, "v1 = %d (!= 5678)", v1);
        if (!(ex.category == 1 && ex.value == 2 && !strcmp(ex.msg,"blah")))
            ts_test_fail(TS_CTX, "unexpected exception contents");
    }
    if (!c)
        ts_test_fail(TS_CTX, "ex_cleanup not executed");
}

int main(int argc, char *argv[])
{
    ts_suite_t *ts;
    int n;

    ts = ts_suite_new("OSSP ex (Exception Handling)");
    ts_suite_test(ts, test_controlflow, "basic nested control flow");
    ts_suite_test(ts, test_value,       "exception value passing");
    ts_suite_test(ts, test_variables,   "variable value preservation");
    ts_suite_test(ts, test_defer,       "exception deferring");
    ts_suite_test(ts, test_shield,      "exception shielding");
    ts_suite_test(ts, test_cleanup,     "cleanup handling");
    n = ts_suite_run(ts);
    ts_suite_free(ts);
    return n;
}


/*
 * The following is the example included in the documentation. It's a good 
 * idea to check its syntax even if we don't try to run it.
 * And actually, it allows to put comments in the code despite doxygen.
 */ 
static char *mallocex(int size) {
  return NULL;
}
#define SMALLAMOUNT 10
#define TOOBIG 100000000

#if 0 /* this contains syntax errors, actually */
static void bad_example(void) {
  struct {char*first;} *globalcontext;
  ex_t ex;

  /* BAD_EXAMPLE */
  xbt_try {
    char *cp1, *cp2, *cp3;
    
    cp1 = mallocex(SMALLAMOUNT);
    globalcontext->first = cp1;
    cp2 = mallocex(TOOBIG);
    cp3 = mallocex(SMALLAMOUNT);
    strcpy(cp1, "foo");
    strcpy(cp2, "bar");
  } xbt_cleanup {
    if (cp3 != NULL) free(cp3);
    if (cp2 != NULL) free(cp2);
    if (cp1 != NULL) free(cp1);
  } xbt_catch(ex) {
    printf("cp3=%s", cp3);
    xbt_rethrow;
  }
  /* end_of_bad_example */
}
#endif

static void good_example(void) {
  struct {char*first;} *globalcontext;
  ex_t ex;

  /* GOOD_EXAMPLE */
  { /*01*/
    char * volatile /*03*/ cp1 = NULL /*02*/;
    char * volatile /*03*/ cp2 = NULL /*02*/;
    char * volatile /*03*/ cp3 = NULL /*02*/;
    xbt_try {
      cp1 = mallocex(SMALLAMOUNT);
      globalcontext->first = cp1;
      cp1 = NULL /*05 give away*/;
      cp2 = mallocex(TOOBIG);
      cp3 = mallocex(SMALLAMOUNT);
      strcpy(cp1, "foo");
      strcpy(cp2, "bar");
    } xbt_cleanup { /*04*/
      printf("cp3=%s", cp3 == NULL /*02*/ ? "" : cp3);
      if (cp3 != NULL)
	free(cp3);
      if (cp2 != NULL)
	free(cp2);
      /*05 cp1 was given away */
    } xbt_catch(ex) {
      /*05 global context untouched */
      xbt_rethrow;
    }
  }
  /* end_of_good_example */
}
