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

#include "ex_test_ts.h"
#include "xbt/ex.h"

TS_TEST(test_controlflow)
{
    ex_t ex;
    volatile int n;

    ts_test_check(TS_CTX, "basic nested control flow");
    n = 1;
    ex_try {
        if (n != 1)
            ts_test_fail(TS_CTX, "M1: n=%d (!= 1)", n);
        n++;
        ex_try {
            if (n != 2)
                ts_test_fail(TS_CTX, "M2: n=%d (!= 2)", n);
            n++;
            ex_throw(0, 0, 0);
        }
        ex_catch (ex) {
            if (n != 3)
                ts_test_fail(TS_CTX, "M3: n=%d (!= 1)", n);
            n++;
            ex_rethrow;
        }
        ts_test_fail(TS_CTX, "MX: n=%d (expected: not reached)", n);
    }
    ex_catch (ex) {
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

    ex_try {
        ex_throw(1, 2, 3);
    }
    ex_catch (ex) {
        ts_test_check(TS_CTX, "exception value passing");
        if (ex.ex_class != (void *)1)
            ts_test_fail(TS_CTX, "ex_class=0x%lx (!= 1)", (long)ex.ex_class);
        if (ex.ex_object != (void *)2)
            ts_test_fail(TS_CTX, "ex_object=0x%lx (!= 2)", (long)ex.ex_object);
        if (ex.ex_value != (void *)3)
            ts_test_fail(TS_CTX, "ex_value=0x%lx (!= 3)", (long)ex.ex_value);
    }
}

TS_TEST(test_variables)
{
    ex_t ex;
    int r1, r2;
    volatile int v1, v2;

    r1 = r2 = v1 = v2 = 1234;
    ex_try {
        r2 = 5678;
        v2 = 5678;
        ex_throw(0, 0, 0);
    }
    ex_catch (ex) {
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
    if (ex_deferring)
        ts_test_fail(TS_CTX, "unexpected deferring scope");
    ex_try {
        ex_defer {
            if (!ex_deferring)
                ts_test_fail(TS_CTX, "unexpected non-deferring scope");
            ex_defer {
                i1 = 1;
                ex_throw(0, 0, 4711);
                i2 = 2;
                ex_throw(0, 0, 0);
                i3 = 3;
                ex_throw(0, 0, 0);
            }
            ex_throw(0, 0, 0);
        }
        ts_test_fail(TS_CTX, "unexpected not occurred deferred throwing");
    }
    ex_catch (ex) {
        if ((long)ex.ex_value != (long)4711)
            ts_test_fail(TS_CTX, "caught exception with value %d, expected 4711", (long)ex.ex_value);
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
    if (ex_shielding)
        ts_test_fail(TS_CTX, "unexpected shielding scope");
    if (ex_catching)
        ts_test_fail(TS_CTX, "unexpected catching scope");
    ex_try {
        ex_shield {
            if (!ex_shielding)
                ts_test_fail(TS_CTX, "unexpected non-shielding scope");
            ex_throw(0, 0, 0);
        }
        if (ex_shielding)
            ts_test_fail(TS_CTX, "unexpected shielding scope");
        if (!ex_catching)
            ts_test_fail(TS_CTX, "unexpected non-catching scope");
    }
    ex_catch (ex) {
        ts_test_fail(TS_CTX, "unexpected exception catched");
        if (ex_catching)
            ts_test_fail(TS_CTX, "unexpected catching scope");
    }
    if (ex_catching)
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
    ex_try {
        v1 = 5678;
        ex_throw(1, 2, 3);
    }
    ex_cleanup {
        if (v1 != 5678)
            ts_test_fail(TS_CTX, "v1 = %d (!= 5678)", v1);
        c = 1;
    }
    ex_catch (ex) {
        if (v1 != 5678)
            ts_test_fail(TS_CTX, "v1 = %d (!= 5678)", v1);
        if (!(ex.ex_class == (void *)1 && ex.ex_object == (void *)2 && ex.ex_value == (void *)3))
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

