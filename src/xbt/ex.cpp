/* ex - Exception Handling                                                  */

/* Copyright (c) 2005-2017. The SimGrid Team. All rights reserved.          */

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

#include <cstdio>
#include <cstdlib>

#include <xbt/backtrace.hpp>
#include "src/internal_config.h"           /* execinfo when available */
#include "xbt/ex.h"
#include <xbt/ex.hpp>
#include "xbt/log.h"
#include "xbt/log.hpp"
#include "xbt/backtrace.h"
#include "xbt/backtrace.hpp"
#include "src/xbt_modinter.h"       /* backtrace initialization headers */

#include "simgrid/sg_config.h"  /* Configuration mechanism of SimGrid */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_ex, xbt, "Exception mechanism");

// Don't define ~xbt_ex() in ex.hpp.  It is defined here to ensure that there is an unique definition of xt_ex in
// libsimgrid, but not in libsimgrid-java.  Otherwise, sone tests are broken (seen with clang/libc++ on freebsd).
xbt_ex::~xbt_ex() = default;

void _xbt_throw(char* message, xbt_errcat_t errcat, int value, const char* file, int line, const char* func)
{
  xbt_ex e(simgrid::xbt::ThrowPoint(file, line, func), message);
  xbt_free(message);
  e.category = errcat;
  e.value = value;
  throw e;
}

/** @brief shows an exception content and the associated stack if available */
void xbt_ex_display(xbt_ex_t * e)
{
  simgrid::xbt::logException(xbt_log_priority_critical, "UNCAUGHT EXCEPTION", *e);
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
  default:
    return "INVALID ERROR";
  }
  return "INVALID ERROR";
}

#ifdef SIMGRID_TEST
#include "xbt/ex.h"
#include <cstdio>
#include <xbt/ex.hpp>

XBT_TEST_SUITE("xbt_ex", "Exception Handling");

XBT_TEST_UNIT("controlflow", test_controlflow, "basic nested control flow")
{
  xbt_ex_t ex;
  int n = 1;

  xbt_test_add("basic nested control flow");

  try {
    if (n != 1)
      xbt_test_fail("M1: n=%d (!= 1)", n);
    n++;
    try {
      if (n != 2)
        xbt_test_fail("M2: n=%d (!= 2)", n);
      n++;
      THROWF(unknown_error, 0, "something");
    }
    catch (xbt_ex& ex) {
      if (n != 3)
        xbt_test_fail("M3: n=%d (!= 3)", n);
      n++;
    }
    n++;
    try {
      if (n != 5)
        xbt_test_fail("M2: n=%d (!= 5)", n);
      n++;
      THROWF(unknown_error, 0, "something");
    }
    catch(xbt_ex& ex){
      if (n != 6)
        xbt_test_fail("M3: n=%d (!= 6)", n);
      n++;
      throw;
      n++;
    }
    xbt_test_fail("MX: n=%d (shouldn't reach this point)", n);
  }
  catch(xbt_ex& e) {
    if (n != 7)
      xbt_test_fail("M4: n=%d (!= 7)", n);
    n++;
  }
  if (n != 8)
    xbt_test_fail("M5: n=%d (!= 8)", n);
}

XBT_TEST_UNIT("value", test_value, "exception value passing")
{
  try {
    THROWF(unknown_error, 2, "toto");
  }
  catch (xbt_ex& ex) {
    xbt_test_add("exception value passing");
    if (ex.category != unknown_error)
      xbt_test_fail("category=%d (!= 1)", (int)ex.category);
    if (ex.value != 2)
      xbt_test_fail("value=%d (!= 2)", ex.value);
    if (strcmp(ex.what(), "toto"))
      xbt_test_fail("message=%s (!= toto)", ex.what());
  }
}

XBT_TEST_UNIT("variables", test_variables, "variable value preservation")
{
  xbt_ex_t ex;
  int r1;
  XBT_ATTRIB_UNUSED int r2;
  int v1;
  int v2;

  r1 = r2 = v1 = v2 = 1234;
  try {
    r2 = 5678;
    v2 = 5678;
    THROWF(unknown_error, 0, "toto");
  }
  catch(xbt_ex& e) {
    xbt_test_add("variable preservation");
    if (r1 != 1234)
      xbt_test_fail("r1=%d (!= 1234)", r1);
    if (v1 != 1234)
      xbt_test_fail("v1=%d (!= 1234)", v1);
    /* r2 is allowed to be destroyed because not volatile */
    if (v2 != 5678)
      xbt_test_fail("v2=%d (!= 5678)", v2);
  }
}

XBT_TEST_UNIT("cleanup", test_cleanup, "cleanup handling")
{
  int v1;
  int c;

  xbt_test_add("cleanup handling");

  v1 = 1234;
  c = 0;
  try {
    v1 = 5678;
    THROWF(1, 2, "blah");
  }
  catch (xbt_ex& ex) {
    if (v1 != 5678)
      xbt_test_fail("v1 = %d (!= 5678)", v1);
    c = 1;
    if (v1 != 5678)
      xbt_test_fail("v1 = %d (!= 5678)", v1);
    if (not(ex.category == 1 && ex.value == 2 && not strcmp(ex.what(), "blah")))
      xbt_test_fail("unexpected exception contents");
  }
  if (not c)
    xbt_test_fail("xbt_ex_free not executed");
}
#endif                          /* SIMGRID_TEST */
