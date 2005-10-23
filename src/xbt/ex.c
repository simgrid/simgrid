/*
**  OSSP ex - Exception Handling (modified to fit into SimGrid)
**  Copyright (c) 2005 Martin Quinson
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
**  ex.c: exception handling (compiler part)
*/

#include <stdio.h>
#include <stdlib.h>

#include "portable.h" /* execinfo when available */
#include "xbt/ex.h"

/* default __ex_ctx callback function */
ex_ctx_t *__xbt_ex_ctx_default(void) {
    static ex_ctx_t ctx = XBT_CTX_INITIALIZER;

    return &ctx;
}

/** @brief shows an exception content and the associated stack if available */
void xbt_ex_display(xbt_ex_t *e)  {

  fprintf(stderr,
	  "** SimGrid: UNCAUGHT EXCEPTION: category: %s; value: %d\n"
	  "** %s\n"
	  "** Thrown by %s%s%s at %s:%d:%s\n",
	  xbt_ex_catname(e->category), e->value, e->msg,
	  e->procname, (e->host?"@":""),(e->host?e->host:"localhost"),
	  e->file,e->line,e->func);

#ifdef HAVE_EXECINFO_H
 {
  char **strings;
  int i;

  fprintf(stderr,"** Backtrace:\n");
  strings = backtrace_symbols (e->bt, e->used);

  for (i = 0; i < e->used; i++)
     printf ("   %s\n", strings[i]);

  free (strings);
 }
#endif
}


/* default __ex_terminate callback function */
void __xbt_ex_terminate_default(xbt_ex_t *e)  {
  xbt_ex_display(e);

  abort();
}

/* the externally visible API */
ex_ctx_cb_t  __xbt_ex_ctx       = &__xbt_ex_ctx_default;
ex_term_cb_t __xbt_ex_terminate = &__xbt_ex_terminate_default;

void xbt_ex_free(xbt_ex_t e) {
  if (e.msg) free(e.msg);
  free(e.procname);
}

/** \brief returns a short name for the given exception category */
const char * xbt_ex_catname(xbt_errcat_t cat) {
  switch (cat) {
  case unknown_error:   return  "unknown_err";
  case arg_error:       return "invalid_arg";
  case mismatch_error:  return "mismatch";
  case not_found_error: return "not found";
  case system_error:    return "system_err";
  case network_error:   return "network_err";
  case timeout_error:   return "timeout";
  case thread_error:    return "thread_err";
  default:              return "INVALID_ERR";
  }
}

#ifndef HAVE_EXECINFO_H
/* dummy implementation. We won't use the result, but ex.h needs it to be defined */
int backtrace (void **__array, int __size) {
  return 0;
}

#endif

#ifdef SIMGRID_TEST
#include "xbt/ex.h"

XBT_TEST_SUITE("xbt_ex","Exception Handling");

XBT_TEST_UNIT("controlflow",test_controlflow, "basic nested control flow") {
    xbt_ex_t ex;
    volatile int n=1;

    xbt_test_add0("basic nested control flow");

    TRY {
        if (n != 1)
            xbt_test_fail1("M1: n=%d (!= 1)", n);
        n++;
        TRY {
            if (n != 2)
                xbt_test_fail1("M2: n=%d (!= 2)", n);
            n++;
            THROW0(unknown_error,0,"something");
        } CATCH (ex) {
            if (n != 3)
                xbt_test_fail1("M3: n=%d (!= 1)", n);
            n++;
            RETHROW;
        }
        xbt_test_fail1("MX: n=%d (shouldn't reach this point)", n);
    }
    CATCH(ex) {
        if (n != 4)
            xbt_test_fail1("M4: n=%d (!= 4)", n);
        n++;
        xbt_ex_free(ex);
    }
    if (n != 5)
        xbt_test_fail1("M5: n=%d (!= 5)", n);
}

XBT_TEST_UNIT("value",test_value,"exception value passing") {
    xbt_ex_t ex;

    TRY {
        THROW0(unknown_error, 2, "toto");
    } CATCH(ex) {
        xbt_test_add0("exception value passing");
        if (ex.category != unknown_error)
            xbt_test_fail1("category=%d (!= 1)", ex.category);
        if (ex.value != 2)
            xbt_test_fail1("value=%d (!= 2)", ex.value);
        if (strcmp(ex.msg,"toto"))
            xbt_test_fail1("message=%s (!= toto)", ex.msg);
        xbt_ex_free(ex);
    }
}

XBT_TEST_UNIT("variables",test_variables,"variable value preservation") {
    xbt_ex_t ex;
    int r1, r2;
    volatile int v1, v2;

    r1 = r2 = v1 = v2 = 1234;
    TRY {
        r2 = 5678;
        v2 = 5678;
        THROW0(unknown_error, 0, "toto");
    } CATCH(ex) {
        xbt_test_add0("variable preservation");
        if (r1 != 1234)
            xbt_test_fail1("r1=%d (!= 1234)", r1);
        if (v1 != 1234)
            xbt_test_fail1("v1=%d (!= 1234)", v1);
        /* r2 is allowed to be destroyed because not volatile */
        if (v2 != 5678)
            xbt_test_fail1("v2=%d (!= 5678)", v2);
        xbt_ex_free(ex);
    }
}

XBT_TEST_UNIT("cleanup",test_cleanup,"cleanup handling") {
    xbt_ex_t ex;
    volatile int v1;
    int c;

    xbt_test_add0("cleanup handling");

    v1 = 1234;
    c = 0;
    TRY {
        v1 = 5678;
        THROW0(1, 2, "blah");
    } CLEANUP {
        if (v1 != 5678)
            xbt_test_fail1("v1 = %d (!= 5678)", v1);
        c = 1;
    } CATCH(ex) {
        if (v1 != 5678)
            xbt_test_fail1("v1 = %d (!= 5678)", v1);
        if (!(ex.category == 1 && ex.value == 2 && !strcmp(ex.msg,"blah")))
            xbt_test_fail0("unexpected exception contents");
        xbt_ex_free(ex);
    }
    if (!c)
        xbt_test_fail0("xbt_ex_free not executed");
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
  TRY {
    char *cp1, *cp2, *cp3;
    
    cp1 = mallocex(SMALLAMOUNT);
    globalcontext->first = cp1;
    cp2 = mallocex(TOOBIG);
    cp3 = mallocex(SMALLAMOUNT);
    strcpy(cp1, "foo");
    strcpy(cp2, "bar");
  } CLEANUP {
    if (cp3 != NULL) free(cp3);
    if (cp2 != NULL) free(cp2);
    if (cp1 != NULL) free(cp1);
  } CATCH(ex) {
    printf("cp3=%s", cp3);
    RETHROW;
  }
  /* end_of_bad_example */
}
#endif

static void good_example(void) {
  struct {char*first;} *globalcontext;
  xbt_ex_t ex;

  /* GOOD_EXAMPLE */
  { /*01*/
    char * volatile /*03*/ cp1 = NULL /*02*/;
    char * volatile /*03*/ cp2 = NULL /*02*/;
    char * volatile /*03*/ cp3 = NULL /*02*/;
    TRY {
      cp1 = mallocex(SMALLAMOUNT);
      globalcontext->first = cp1;
      cp1 = NULL /*05 give away*/;
      cp2 = mallocex(TOOBIG);
      cp3 = mallocex(SMALLAMOUNT);
      strcpy(cp1, "foo");
      strcpy(cp2, "bar");
    } CLEANUP { /*04*/
      printf("cp3=%s", cp3 == NULL /*02*/ ? "" : cp3);
      if (cp3 != NULL)
	free(cp3);
      if (cp2 != NULL)
	free(cp2);
      /*05 cp1 was given away */
    } CATCH(ex) {
      /*05 global context untouched */
      RETHROW;
    }
  }
  /* end_of_good_example */
}
#endif /* SIMGRID_TEST */
