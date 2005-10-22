/*
**  OSSP ts - Test Suite Library
**  Copyright (c) 2001-2004 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2001-2004 The OSSP Project <http://www.ossp.org/>
**  Copyright (c) 2001-2004 Cable & Wireless <http://www.cw.com/>
**
**  This file is part of OSSP ts, a small test suite library which
**  can be found at http://www.ossp.org/pkg/lib/ts/.
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
**  ts.c: test suite library
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "gras_config.h"

#include "xbt/testsuite.h"

/* embedded ring data structure library */
#define RING_ENTRY(elem) \
    struct { elem *next; elem *prev; }
#define RING_HEAD(elem) \
    struct { elem *next; elem *prev; }
#define RING_SENTINEL(hp, elem, link) \
    (elem *)((char *)(hp) - ((size_t)(&((elem *)0)->link)))
#define RING_FIRST(hp) \
    (hp)->next
#define RING_LAST(hp) \
    (hp)->prev
#define RING_NEXT(ep, link) \
    (ep)->link.next
#define RING_PREV(ep, link) \
    (ep)->link.prev
#define RING_INIT(hp, elem, link) \
    do { RING_FIRST((hp)) = RING_SENTINEL((hp), elem, link); \
         RING_LAST((hp))  = RING_SENTINEL((hp), elem, link); } while (0)
#define RING_EMPTY(hp, elem, link) \
    (RING_FIRST((hp)) == RING_SENTINEL((hp), elem, link))
#define RING_ELEM_INIT(ep, link) \
    do { RING_NEXT((ep), link) = (ep); \
         RING_PREV((ep), link) = (ep); } while (0)
#define RING_SPLICE_BEFORE(lep, ep1, epN, link) \
    do { RING_NEXT((epN), link) = (lep); \
         RING_PREV((ep1), link) = RING_PREV((lep), link); \
         RING_NEXT(RING_PREV((lep), link), link) = (ep1); \
         RING_PREV((lep), link) = (epN); } while (0)
#define RING_SPLICE_TAIL(hp, ep1, epN, elem, link) \
    RING_SPLICE_BEFORE(RING_SENTINEL((hp), elem, link), (ep1), (epN), link)
#define RING_INSERT_TAIL(hp, nep, elem, link) \
    RING_SPLICE_TAIL((hp), (nep), (nep), elem, link)
#define RING_FOREACH(ep, hp, elem, link) \
    for ((ep)  = RING_FIRST((hp)); \
         (ep) != RING_SENTINEL((hp), elem, link); \
         (ep)  = RING_NEXT((ep), link))
#define RING_FOREACH_LA(ep, epT, hp, elem, link) \
    for ((ep)  = RING_FIRST((hp)), (epT) = RING_NEXT((ep), link); \
         (ep) != RING_SENTINEL((hp), elem, link); \
         (ep)  = (epT), (epT) = RING_NEXT((epT), link))

/* test suite test log */
struct tstl_st;
typedef struct tstl_st tstl_t;
struct tstl_st {
    RING_ENTRY(tstl_t) next;
    char              *text;
    const char        *file;
    int                line;
};

/* test suite test check */
struct tstc_st;
typedef struct tstc_st tstc_t;
struct tstc_st {
    RING_ENTRY(tstc_t) next;
    char              *title;
    int                failed;
    const char        *file;
    int                line;
    RING_HEAD(tstl_t)  logs;
};

/* test suite test */
struct ts_test_st {
    RING_ENTRY(ts_test_t)  next;
    char              *title;
    ts_test_cb_t         func;
    const char        *file;
    int                line;
    RING_HEAD(tstc_t)  checks;
};

/* test suite */
struct ts_suite_st {
    char              *title;
    RING_HEAD(ts_test_t)   tests;
};

/* minimal output-independent vprintf(3) variant which supports %{c,s,d,%} only */
static int ts_suite_mvxprintf(char *buffer, size_t bufsize, const char *format, va_list ap)
{
    /* sufficient integer buffer: <available-bits> x log_10(2) + safety */
    char ibuf[((sizeof(int)*8)/3)+10]; 
    char *cp;
    char c;
    int d;
    int n;
    int bytes;

    if (format == NULL)
        return -1;
    bytes = 0;
    while (*format != '\0') {
        if (*format == '%') {
            c = *(format+1);
            if (c == '%') {
                /* expand "%%" */
                cp = &c;
                n = sizeof(char);
            }
            else if (c == 'c') {
                /* expand "%c" */
                c = (char)va_arg(ap, int);
                cp = &c;
                n = sizeof(char);
            }
            else if (c == 's') {
                /* expand "%s" */
                if ((cp = (char *)va_arg(ap, char *)) == NULL)
                    cp = (char*)"(null)";
                n = strlen(cp);
            }
            else if (c == 'd') {
                /* expand "%d" */
                d = (int)va_arg(ap, int);
#ifdef HAVE_SNPRINTF
                snprintf(ibuf, sizeof(ibuf), "%d", d); /* explicitly secure */
#else
                sprintf(ibuf, "%d", d);                /* implicitly secure */
#endif
                cp = ibuf;
                n = strlen(cp);
            }
            else {
                /* any other "%X" */
                cp = (char *)format;
                n  = 2;
            }
            format += 2;
        }
        else {
            /* plain text */
            cp = (char *)format;
            if ((format = strchr(cp, '%')) == NULL)
                format = strchr(cp, '\0');
            n = format - cp;
        }
        /* perform output operation */
        if (buffer != NULL) {
            if (n > bufsize)
                return -1;
            memcpy(buffer, cp, n);
            buffer  += n;
            bufsize -= n;
        }
        bytes += n;
    }
    /* nul-terminate output */
    if (buffer != NULL) {
        if (bufsize == 0)
            return -1;
        *buffer = '\0';
    }
    return bytes;
}

/* minimal vasprintf(3) variant which supports %{c,s,d} only */
static char *ts_suite_mvasprintf(const char *format, va_list ap)
{
    char *buffer;
    int n;
    va_list ap2;

    if (format == NULL)
        return NULL;
    va_copy(ap2, ap);
    if ((n = ts_suite_mvxprintf(NULL, 0, format, ap)) == -1)
        return NULL;
    if ((buffer = (char *)malloc(n+1)) == NULL)
        return NULL;
    ts_suite_mvxprintf(buffer, n+1, format, ap2);
    return buffer;
}

/* minimal asprintf(3) variant which supports %{c,s,d} only */
static char *ts_suite_masprintf(const char *format, ...)
{
    va_list ap;
    char *cp;

    va_start(ap, format);
    cp = ts_suite_mvasprintf(format, ap);
    va_end(ap);
    return cp;
}

/* create test suite */
ts_suite_t *ts_suite_new(const char *fmt, ...)
{
    ts_suite_t *ts;
    va_list ap;

    if ((ts = (ts_suite_t *)malloc(sizeof(ts_suite_t))) == NULL)
        return NULL;
    va_start(ap, fmt);
    ts->title = ts_suite_mvasprintf(fmt, ap);
    RING_INIT(&ts->tests, ts_test_t, next);
    va_end(ap);
    return ts;
}

/* add test case to test suite */
void ts_suite_test(ts_suite_t *ts, ts_test_cb_t func, const char *fmt, ...)
{
    ts_test_t *tst;
    va_list ap;

    if (ts == NULL || func == NULL || fmt == NULL)
        return;
    if ((tst = (ts_test_t *)malloc(sizeof(ts_test_t))) == NULL)
        return;
    RING_ELEM_INIT(tst, next);
    va_start(ap, fmt);
    tst->title = ts_suite_mvasprintf(fmt, ap);
    va_end(ap);
    tst->func = func;
    tst->file = NULL;
    tst->line = 0;
    RING_INIT(&tst->checks, tstc_t, next);
    RING_INSERT_TAIL(&ts->tests, tst, ts_test_t, next);
    return;
}

/* run test suite */
int ts_suite_run(ts_suite_t *ts)
{
    ts_test_t *tst;
    tstc_t *tstc;
    tstl_t *tstl;
    int total_tests, total_tests_suite_failed;
    int total_checks, total_checks_failed;
    int test_checks, test_checks_failed;
    const char *file;
    int line;
    char *cp;

    if (ts == NULL)
        return 0;

    /* init total counters */
    total_tests         = 0;
    total_tests_suite_failed  = 0;
    total_checks        = 0;
    total_checks_failed = 0;

    fprintf(stdout, "\n");
    fprintf(stdout, " Test Suite: %s\n", ts->title);
    fprintf(stdout, " __________________________________________________________________\n");
    fprintf(stdout, "\n");
    fflush(stdout);

    /* iterate through all test cases */
    RING_FOREACH(tst, &ts->tests, ts_test_t, next) {
        cp = ts_suite_masprintf(" Test: %s ........................................"
                                "........................................", tst->title);
        cp[60] = '\0';
        fprintf(stdout, "%s", cp);
        free(cp);
        fflush(stdout);

        /* init test case counters */
        test_checks        = 0;
        test_checks_failed = 0;

        /* run the test case function */
        tst->func(tst);

        /* iterate through all performed checks to determine status */
        RING_FOREACH(tstc, &tst->checks, tstc_t, next) {
            test_checks++;
            if (tstc->failed)
                test_checks_failed++;
        }

        if (test_checks_failed > 0) {
            /* some checks failed, so do detailed reporting of test case */
            fprintf(stdout, " FAILED\n");
            fprintf(stdout, "       Ops, %d/%d checks failed! Detailed report follows:\n",
                    test_checks_failed, test_checks);
            RING_FOREACH(tstc, &tst->checks, tstc_t, next) {
                file = (tstc->file != NULL ? tstc->file : tst->file);
                line = (tstc->line != 0    ? tstc->line : tst->line);
                if (file != NULL)
                    fprintf(stdout, "       Check: %s [%s:%d]\n", tstc->title, file, line);
                else
                    fprintf(stdout, "       Check: %s\n", tstc->title);
                RING_FOREACH(tstl, &tstc->logs, tstl_t, next) {
                    file = (tstl->file != NULL ? tstl->file : file);
                    line = (tstl->line != 0    ? tstl->line : line);
                    if (file != NULL)
                        fprintf(stdout, "              Log: %s [%s:%d]\n", tstl->text, file, line);
                    else
                        fprintf(stdout, "              Log: %s\n", tstl->text);
                }
            }
        }
        else {
            /* test case ran successfully */
            fprintf(stdout, ".... OK\n");
        }
        fflush(stdout);

        /* accumulate counters */
        total_checks += test_checks;
        total_tests++;
        if (test_checks_failed > 0) {
            total_checks_failed += test_checks_failed;
            total_tests_suite_failed++;
        }
    }

    /* print test suite summary */
    fprintf(stdout, " __________________________________________________________________\n");
    fprintf(stdout, "\n");
    fprintf(stdout, " Test Summary: %d tests (%d ok, %d failed), %d checks (%d ok, %d failed)\n", 
            total_tests, (total_tests - total_tests_suite_failed), total_tests_suite_failed, 
            total_checks, (total_checks - total_checks_failed), total_checks_failed); 
    if (total_tests_suite_failed > 0)
        fprintf(stdout, " Test Suite: FAILED\n");
    else
        fprintf(stdout, " Test Suite: OK\n");
    fprintf(stdout, "\n");
    fflush(stdout);

    return total_checks_failed;
}

/* destroy test suite */
void ts_suite_free(ts_suite_t *ts)
{
    ts_test_t *tst, *tstT;
    tstc_t *tstc, *tstcT;
    tstl_t *tstl, *tstlT;

    if (ts == NULL)
        return;
    RING_FOREACH_LA(tst, tstT, &ts->tests, ts_test_t, next) {
        RING_FOREACH_LA(tstc, tstcT, &tst->checks, tstc_t, next) {
            RING_FOREACH_LA(tstl, tstlT, &tstc->logs, tstl_t, next) {
                free(tstl->text);
            }
            free(tstc->title);
            free(tstc);
        }
        free(tst->title);
        free(tst);
    }
    free(ts->title);
    free(ts);
    return;
}

/* annotate test case with file name and line number */
ts_test_t *ts_test_ctx(ts_test_t *tst, const char *file, int line)
{
    if (tst != NULL && file != NULL) {
        tst->file = file;
        tst->line = line;
    }
    return tst;
}

/* annotate test case with check */
void ts_test_check(ts_test_t *tst, const char *fmt, ...)
{
    tstc_t *tstc;
    va_list ap;

    if (tst == NULL || fmt == NULL)
        return;
    if ((tstc = (tstc_t *)malloc(sizeof(tstc_t))) == NULL)
        return;
    va_start(ap, fmt);
    RING_ELEM_INIT(tstc, next);
    tstc->title = ts_suite_mvasprintf(fmt, ap);
    tstc->failed = 0;
    tstc->file = tst->file;
    tstc->line = tst->line;
    RING_INIT(&tstc->logs, tstl_t, next);
    RING_INSERT_TAIL(&tst->checks, tstc, tstc_t, next);
    va_end(ap);
    return;
}

/* annotate test case with log message and failure */
void ts_test_fail(ts_test_t *tst, const char *fmt, ...)
{
    tstc_t *tstc;
    tstl_t *tstl;
    va_list ap;

    if (tst == NULL || fmt == NULL)
        return;
    if ((tstl = (tstl_t *)malloc(sizeof(tstl_t))) == NULL)
        return;
    va_start(ap, fmt);
    tstl->text = ts_suite_mvasprintf(fmt, ap);
    tstl->file = tst->file;
    tstl->line = tst->line;
    RING_ELEM_INIT(tstl, next);
    tstc = RING_LAST(&tst->checks);
    RING_INSERT_TAIL(&tstc->logs, tstl, tstl_t, next);
    tstc->failed = 1;
    va_end(ap);
    return;
}

/* annotate test case with log message only */
void ts_test_log(ts_test_t *tst, const char *fmt, ...)
{
    tstc_t *tstc;
    tstl_t *tstl;
    va_list ap;

    if (tst == NULL || fmt == NULL)
        return;
    if ((tstl = (tstl_t *)malloc(sizeof(tstl_t))) == NULL)
        return;
    va_start(ap, fmt);
    tstl->text = ts_suite_mvasprintf(fmt, ap);
    tstl->file = tst->file;
    tstl->line = tst->line;
    RING_ELEM_INIT(tstl, next);
    tstc = RING_LAST(&tst->checks);
    RING_INSERT_TAIL(&tstc->logs, tstl, tstl_t, next);
    va_end(ap);
    return;
}

