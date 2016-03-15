/* ex - Exception Handling                                                  */

/* Copyright (c) 2005-2015. The SimGrid Team.
 * All rights reserved.                                                     */

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

#include "src/internal_config.h"           /* execinfo when available */
#include "xbt/ex.h"
#include "xbt/str.h"
#include "xbt/synchro_core.h"
#include "src/xbt_modinter.h"       /* backtrace initialization headers */

#include "src/xbt/ex_interface.h"
#include "simgrid/sg_config.h"  /* Configuration mechanism of SimGrid */

#include "simgrid/simix.h" /* SIMIX_process_self_get_name() */

#undef HAVE_BACKTRACE
#if HAVE_EXECINFO_H && HAVE_POPEN && defined(ADDR2LINE)
# define HAVE_BACKTRACE 1       /* Hello linux box */
#endif

#if defined(_WIN32) && defined(_M_IX86) && !defined(__GNUC__)
# define HAVE_BACKTRACE 1       /* Hello x86 windows box */
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ex, xbt, "Exception mecanism");

XBT_EXPORT_NO_IMPORT(const xbt_running_ctx_t) __xbt_ex_ctx_initializer = XBT_RUNNING_CTX_INITIALIZER;

/* default __ex_ctx callback function */
xbt_running_ctx_t *__xbt_ex_ctx_default(void)
{
  /* Don't scream: this is a default which is never used (so, yes,
     there is one setjump container by running entity).

     This default gets overriden in xbt/xbt_os_thread.c so that it works in
     real life and in simulation when using threads to implement the simulation
     processes (ie, with pthreads and on windows).

     It also gets overriden in xbt/context.c when using ucontexts (as well as
     in Java for now, but after the java overhaul, it will get cleaned out)
   */
  static xbt_running_ctx_t ctx = XBT_RUNNING_CTX_INITIALIZER;

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
    fprintf(stderr, "Backtrace (displayed in process %s):\n", SIMIX_process_self_get_name());
    for (int i = 1; i < e->used; i++)       /* no need to display "xbt_backtrace_display" */
      fprintf(stderr, "---> %s\n", e->bt_strings[i] + 4);
  }

  /* don't fool xbt_ex_free with uninitialized msg field */
  e->msg = NULL;
  xbt_ex_free(*e);
#else
  XBT_ERROR("No backtrace on this arch");
#endif
}

/** \brief show the backtrace of the current point (lovely while debuging) */
void xbt_backtrace_display_current(void)
{
  xbt_ex_t e;
  xbt_backtrace_current(&e);
  xbt_backtrace_display(&e);
}

#if HAVE_EXECINFO_H && HAVE_POPEN && defined(ADDR2LINE)
# include "src/xbt/backtrace_linux.c"
#else
# include "src/xbt/backtrace_dummy.c"
#endif

/** @brief shows an exception content and the associated stack if available */
void xbt_ex_display(xbt_ex_t * e)
{
  char *thrower = NULL;
  if (e->pid != xbt_getpid())
    thrower = bprintf(" on process %d",e->pid);

  fprintf(stderr, "** SimGrid: UNCAUGHT EXCEPTION received on %s(%d): category: %s; value: %d\n"
          "** %s\n"
          "** Thrown by %s()%s\n",
          xbt_binary_name, xbt_getpid(), xbt_ex_catname(e->category), e->value, e->msg,
          e->procname, thrower ? thrower : " in this process");
  XBT_CRITICAL("%s", e->msg);
  xbt_free(thrower);

  if (xbt_initialized==0 || smx_cleaned) {
    fprintf(stderr, "Ouch. SimGrid is not initialized yet, or already closing. No backtrace available.\n");
    return; /* Not started yet or already closing. Trying to generate a backtrace would probably fail */
  }

  if (!e->bt_strings)
    xbt_ex_setup_backtrace(e);

#ifdef HAVE_BACKTRACE
  if (e->used && e->bt_strings) {
    /* We have everything to build neat backtraces */
    int i;
    int cutpath = 0;
    TRY { // We don't want to have an exception while checking how to deal with the one we already have, do we?
      cutpath = sg_cfg_get_boolean("exception/cutpath");
    } CATCH_ANONYMOUS { }

    fprintf(stderr, "\n");
    for (i = 0; i < e->used; i++) {
        
      if (cutpath) {
        char* p = e->bt_strings[i];
        xbt_str_rtrim(p, ":0123456789");
        char* filename = strrchr(p, '/')+1;
        char* end_of_message  = strrchr(p, ' ');

        int length = strlen(p)-strlen(end_of_message);
        char* dest = malloc(length);

        memcpy(dest, &p[0], length);
        dest[length] = 0;

        fprintf(stderr, "%s %s\n", dest, filename);

        free(dest);
      }
      else {
        fprintf(stderr, "%s\n", e->bt_strings[i]);
      }
    }
  } else
#endif
    fprintf(stderr, "\n"
        "**   In %s() at %s:%d\n"
        "**   (no backtrace available)\n", e->func, e->file, e->line);
}

/* default __ex_terminate callback function */
void __xbt_ex_terminate_default(xbt_ex_t * e)
{
  xbt_ex_display(e);
  xbt_abort();
}

/* the externally visible API */
XBT_EXPORT_NO_IMPORT(xbt_running_ctx_fetcher_t) __xbt_running_ctx_fetch = &__xbt_ex_ctx_default;
XBT_EXPORT_NO_IMPORT(ex_term_cb_t) __xbt_ex_terminate = &__xbt_ex_terminate_default;

void xbt_ex_free(xbt_ex_t e)
{
  free(e.msg);

  if (e.bt_strings) {
    for (int i = 0; i < e.used; i++)
      free(e.bt_strings[i]);
    free(e.bt_strings);
  }
}

/** \brief returns a short name for the given exception category */
const char *xbt_ex_catname(xbt_errcat_t cat)
{
  switch (cat) {
  case unknown_error:
    return "unknown error";
  case arg_error:
    return "invalid argument";
  case bound_error:
    return "out of bounds";
  case mismatch_error:
    return "mismatch";
  case not_found_error:
    return "not found";
  case system_error:
    return "system error";
  case network_error:
    return "network error";
  case timeout_error:
    return "timeout";
  case cancel_error:
    return "action canceled";
  case thread_error:
    return "thread error";
  case host_error:
    return "host failed";
  case tracing_error:
    return "tracing error";
  case io_error:
    return "io error";
  case vm_error:
    return "vm error";
  }
  return "INVALID ERROR";
}

#ifdef SIMGRID_TEST
#include <stdio.h>
#include "xbt/ex.h"

XBT_TEST_SUITE("xbt_ex", "Exception Handling");

XBT_TEST_UNIT("controlflow", test_controlflow, "basic nested control flow")
{
  xbt_ex_t ex;
  volatile int n = 1;

  xbt_test_add("basic nested control flow");

  TRY {
    if (n != 1)
      xbt_test_fail("M1: n=%d (!= 1)", n);
    n++;
    TRY {
      if (n != 2)
        xbt_test_fail("M2: n=%d (!= 2)", n);
      n++;
      THROWF(unknown_error, 0, "something");
    } CATCH(ex) {
      if (n != 3)
        xbt_test_fail("M3: n=%d (!= 3)", n);
      n++;
      xbt_ex_free(ex);
    }
    n++;
    TRY {
      if (n != 5)
        xbt_test_fail("M2: n=%d (!= 5)", n);
      n++;
      THROWF(unknown_error, 0, "something");
    } CATCH_ANONYMOUS {
      if (n != 6)
        xbt_test_fail("M3: n=%d (!= 6)", n);
      n++;
      RETHROW;
      n++;
    }
    xbt_test_fail("MX: n=%d (shouldn't reach this point)", n);
  } CATCH(ex) {
    if (n != 7)
      xbt_test_fail("M4: n=%d (!= 7)", n);
    n++;
    xbt_ex_free(ex);
  }
  if (n != 8)
    xbt_test_fail("M5: n=%d (!= 8)", n);
}

XBT_TEST_UNIT("value", test_value, "exception value passing")
{
  xbt_ex_t ex;

  TRY {
    THROWF(unknown_error, 2, "toto");
  } CATCH(ex) {
    xbt_test_add("exception value passing");
    if (ex.category != unknown_error)
      xbt_test_fail("category=%d (!= 1)", (int)ex.category);
    if (ex.value != 2)
      xbt_test_fail("value=%d (!= 2)", ex.value);
    if (strcmp(ex.msg, "toto"))
      xbt_test_fail("message=%s (!= toto)", ex.msg);
    xbt_ex_free(ex);
  }
}

XBT_TEST_UNIT("variables", test_variables, "variable value preservation")
{
  xbt_ex_t ex;
  int r1;
  int XBT_ATTRIB_UNUSED r2;
  int v1;
  volatile int v2;

  r1 = r2 = v1 = v2 = 1234;
  TRY {
    r2 = 5678;
    v2 = 5678;
    THROWF(unknown_error, 0, "toto");
  } CATCH(ex) {
    xbt_test_add("variable preservation");
    if (r1 != 1234)
      xbt_test_fail("r1=%d (!= 1234)", r1);
    if (v1 != 1234)
      xbt_test_fail("v1=%d (!= 1234)", v1);
    /* r2 is allowed to be destroyed because not volatile */
    if (v2 != 5678)
      xbt_test_fail("v2=%d (!= 5678)", v2);
    xbt_ex_free(ex);
  }
}

XBT_TEST_UNIT("cleanup", test_cleanup, "cleanup handling")
{
  xbt_ex_t ex;
  volatile int v1;
  int c;

  xbt_test_add("cleanup handling");

  v1 = 1234;
  c = 0;
  TRY {
    v1 = 5678;
    THROWF(1, 2, "blah");
  }
  TRY_CLEANUP {
    if (v1 != 5678)
      xbt_test_fail("v1 = %d (!= 5678)", v1);
    c = 1;
  } CATCH(ex) {
    if (v1 != 5678)
      xbt_test_fail("v1 = %d (!= 5678)", v1);
    if (!(ex.category == 1 && ex.value == 2 && !strcmp(ex.msg, "blah")))
      xbt_test_fail("unexpected exception contents");
    xbt_ex_free(ex);
  }
  if (!c)
    xbt_test_fail("xbt_ex_free not executed");
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

typedef struct {
  char *first;
} global_context_t;

static void good_example(void)
{
  global_context_t *global_context = xbt_malloc(sizeof(global_context_t));

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
    }
    TRY_CLEANUP {               /*04 */
      printf("cp3=%s", cp3 == NULL /*02 */ ? "" : cp3);
      free(cp3);
      free(cp2);
      /*05 cp1 was given away */
    }
    CATCH_ANONYMOUS {
      /*05 global context untouched */
      RETHROW;
    }
  }
  /* end_of_good_example */
}
#endif                          /* SIMGRID_TEST */
