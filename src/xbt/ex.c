/* ex - Exception Handling                                                  */

/*  Copyright (c) 2005-2010 The SimGrid team                                */
/*  Copyright (c) 2002-2004 Ralf S. Engelschall <rse@engelschall.com>       */
/*  Copyright (c) 2002-2004 The OSSP Project <http://www.ossp.org/>         */
/*  Copyright (c) 2002-2004 Cable & Wireless <http://www.cw.com/>           */
/*  All rights reserved.                                                    */

/* This code is inspirated from the OSSP version (as retrieved back in 2004)*/
/* It was heavily modified to fit the SimGrid framework.                    */

/* The OSSP version has the following copyright notice:
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
**  THIS SOFTWARE IS PROVIDED `AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 */

/* The extensions made for the SimGrid project can either be distributed    */
/* under the same license, or under the LGPL v2.1                           */

#include <stdio.h>
#include <stdlib.h>

#include "portable.h"           /* execinfo when available */
#include "xbt/ex.h"
#include "xbt/str.h"
#include "xbt/module.h"         /* xbt_binary_name */
#include "xbt_modinter.h"       /* backtrace initialization headers */
#include "xbt/synchro.h"        /* xbt_thread_self */

#include "gras/Virtu/virtu_interface.h" /* gras_os_myname */
#include "xbt/ex_interface.h"

#undef HAVE_BACKTRACE
#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)
# define HAVE_BACKTRACE 1       /* Hello linux box */
#endif

#if defined(WIN32) && defined(_M_IX86) && !defined(__GNUC__)
# define HAVE_BACKTRACE 1       /* Hello x86 windows box */
#endif


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ex, xbt, "Exception mecanism");

/* default __ex_ctx callback function */
ex_ctx_t *__xbt_ex_ctx_default(void)
{
  /* Don't scream: this is a default which is never used (so, yes,
     there is one setjump container by running entity).

     This default gets overriden in xbt/xbt_os_thread.c so that it works in
     real life and in simulation when using threads to implement the simulation
     processes (ie, with pthreads and on windows).

     It also gets overriden in xbt/context.c when using ucontextes (as well as
     in Java for now, but after the java overhaul, it will get cleaned out)
   */
  static ex_ctx_t ctx = XBT_CTX_INITIALIZER;

  return &ctx;
}

/* Change raw libc symbols to file names and line numbers */
void xbt_ex_setup_backtrace(xbt_ex_t * e);

void xbt_backtrace_display(xbt_ex_t * e)
{
  xbt_ex_setup_backtrace(e);

#ifdef HAVE_BACKTRACE
  if (e->used == 0) {
    fprintf(stderr, "(backtrace not set)\n");
  } else {
    int i;

    fprintf(stderr, "Backtrace (displayed in thread %p):\n",
            (void *) xbt_thread_self());
    for (i = 1; i < e->used; i++)       /* no need to display "xbt_display_backtrace" */
      fprintf(stderr, "---> %s\n", e->bt_strings[i] + 4);
  }

  /* don't fool xbt_ex_free with uninitialized msg field */
  e->msg = NULL;
  e->remote = 0;
  xbt_ex_free(*e);
#else

  ERROR0("No backtrace on this arch");
#endif
}

/** \brief show the backtrace of the current point (lovely while debuging) */
void xbt_backtrace_display_current(void)
{
  xbt_ex_t e;
  xbt_backtrace_current(&e);
  xbt_backtrace_display(&e);
}

#if defined(HAVE_EXECINFO_H) && defined(HAVE_POPEN) && defined(ADDR2LINE)
# include "backtrace_linux.c"
#elif (defined(WIN32) && defined (_M_IX86)) && !defined(__GNUC__)
# include "backtrace_windows.c"
#else
# include "backtrace_dummy.c"
#endif

/** @brief shows an exception content and the associated stack if available */
void xbt_ex_display(xbt_ex_t * e)
{
  char *thrower = NULL;

  if (e->remote)
    thrower = bprintf(" on host %s(%d)", e->host, e->pid);

  fprintf(stderr,
          "** SimGrid: UNCAUGHT EXCEPTION received on %s(%d): category: %s; value: %d\n"
          "** %s\n"
          "** Thrown by %s()%s\n",
          gras_os_myname(), (*xbt_getpid) (),
          xbt_ex_catname(e->category), e->value, e->msg,
          e->procname, thrower ? thrower : " in this process");
  CRITICAL1("%s", e->msg);

  if (!e->remote && !e->bt_strings)
    xbt_ex_setup_backtrace(e);

#ifdef HAVE_BACKTRACE
  /* We have everything to build neat backtraces */
  {
    int i;

    fprintf(stderr, "\n");
    for (i = 0; i < e->used; i++)
      fprintf(stderr, "%s\n", e->bt_strings[i]);

  }
#else
  fprintf(stderr, " at %s:%d:%s (no backtrace available on that arch)\n",
          e->file, e->line, e->func);
#endif
}


/* default __ex_terminate callback function */
void __xbt_ex_terminate_default(xbt_ex_t * e)
{
  xbt_ex_display(e);

  abort();
}

/* the externally visible API */
XBT_EXPORT_NO_IMPORT(ex_ctx_cb_t) __xbt_ex_ctx = &__xbt_ex_ctx_default;
XBT_EXPORT_NO_IMPORT(ex_term_cb_t) __xbt_ex_terminate =
  &__xbt_ex_terminate_default;


     void xbt_ex_free(xbt_ex_t e)
{
  int i;

  if (e.msg)
    free(e.msg);
  if (e.remote) {
    free(e.procname);
    free(e.file);
    free(e.func);
    free(e.host);
  }

  if (e.bt_strings) {
    for (i = 0; i < e.used; i++)
      free((char *) e.bt_strings[i]);
    free((char **) e.bt_strings);
  }
  /* memset(e,0,sizeof(xbt_ex_t)); */
}

/** \brief returns a short name for the given exception category */
const char *xbt_ex_catname(xbt_errcat_t cat)
{
  switch (cat) {
  case unknown_error:
    return "unknown_err";
  case arg_error:
    return "invalid_arg";
  case mismatch_error:
    return "mismatch";
  case not_found_error:
    return "not found";
  case system_error:
    return "system_err";
  case network_error:
    return "network_err";
  case timeout_error:
    return "timeout";
  case thread_error:
    return "thread_err";
  default:
    return "INVALID_ERR";
  }
}


#ifdef SIMGRID_TEST
#include <stdio.h>
#include "xbt/ex.h"

XBT_TEST_SUITE("xbt_ex", "Exception Handling");

XBT_TEST_UNIT("controlflow", test_controlflow, "basic nested control flow")
{
  xbt_ex_t ex;
  volatile int n = 1;

  xbt_test_add0("basic nested control flow");

  TRY {
    if (n != 1)
      xbt_test_fail1("M1: n=%d (!= 1)", n);
    n++;
    TRY {
      if (n != 2)
        xbt_test_fail1("M2: n=%d (!= 2)", n);
      n++;
      THROW0(unknown_error, 0, "something");
    }
    CATCH(ex) {
      if (n != 3)
        xbt_test_fail1("M3: n=%d (!= 3)", n);
      n++;
      xbt_ex_free(ex);
    }
    n++;
    TRY {
      if (n != 5)
        xbt_test_fail1("M2: n=%d (!= 5)", n);
      n++;
      THROW0(unknown_error, 0, "something");
    }
    CATCH(ex) {
      if (n != 6)
        xbt_test_fail1("M3: n=%d (!= 6)", n);
      n++;
      RETHROW;
      n++;
    }
    xbt_test_fail1("MX: n=%d (shouldn't reach this point)", n);
  }
  CATCH(ex) {
    if (n != 7)
      xbt_test_fail1("M4: n=%d (!= 7)", n);
    n++;
    xbt_ex_free(ex);
  }
  if (n != 8)
    xbt_test_fail1("M5: n=%d (!= 8)", n);
}

XBT_TEST_UNIT("value", test_value, "exception value passing")
{
  xbt_ex_t ex;

  TRY {
    THROW0(unknown_error, 2, "toto");
  }
  CATCH(ex) {
    xbt_test_add0("exception value passing");
    if (ex.category != unknown_error)
      xbt_test_fail1("category=%d (!= 1)", ex.category);
    if (ex.value != 2)
      xbt_test_fail1("value=%d (!= 2)", ex.value);
    if (strcmp(ex.msg, "toto"))
      xbt_test_fail1("message=%s (!= toto)", ex.msg);
    xbt_ex_free(ex);
  }
}

XBT_TEST_UNIT("variables", test_variables, "variable value preservation")
{
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

XBT_TEST_UNIT("cleanup", test_cleanup, "cleanup handling")
{
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
  }
  CATCH(ex) {
    if (v1 != 5678)
      xbt_test_fail1("v1 = %d (!= 5678)", v1);
    if (!(ex.category == 1 && ex.value == 2 && !strcmp(ex.msg, "blah")))
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
static char *mallocex(int size)
{
  return NULL;
}

#define SMALLAMOUNT 10
#define TOOBIG 100000000

#if 0                           /* this contains syntax errors, actually */
static void bad_example(void)
{
  struct {
    char *first;
  } *globalcontext;
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
    if (cp3 != NULL)
      free(cp3);
    if (cp2 != NULL)
      free(cp2);
    if (cp1 != NULL)
      free(cp1);
  }
  CATCH(ex) {
    printf("cp3=%s", cp3);
    RETHROW;
  }
  /* end_of_bad_example */
}
#endif
typedef struct {
  char *first;
} global_context_t;

static void good_example(void)
{
  global_context_t *global_context = malloc(sizeof(global_context_t));
  xbt_ex_t ex;

  /* GOOD_EXAMPLE */
  {                             /*01 */
    char *volatile /*03 */ cp1 = NULL /*02 */ ;
    char *volatile /*03 */ cp2 = NULL /*02 */ ;
    char *volatile /*03 */ cp3 = NULL /*02 */ ;
    TRY {
      cp1 = mallocex(SMALLAMOUNT);
      global_context->first = cp1;
      cp1 = NULL /*05 give away */ ;
      cp2 = mallocex(TOOBIG);
      cp3 = mallocex(SMALLAMOUNT);
      strcpy(cp1, "foo");
      strcpy(cp2, "bar");
    } CLEANUP {                 /*04 */
      printf("cp3=%s", cp3 == NULL /*02 */ ? "" : cp3);
      if (cp3 != NULL)
        free(cp3);
      if (cp2 != NULL)
        free(cp2);
      /*05 cp1 was given away */
    }
    CATCH(ex) {
      /*05 global context untouched */
      RETHROW;
    }
  }
  /* end_of_good_example */
}
#endif /* SIMGRID_TEST */
