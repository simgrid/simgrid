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
**  ts.h: test suite library API
*/

#ifndef _TS_H_
#define _TS_H_

/* test suite object type */
struct ts_suite_st;
typedef struct ts_suite_st ts_suite_t;

/* test object type */
struct ts_test_st;
typedef struct ts_test_st ts_test_t;

/* test callback function type */
typedef void (*ts_test_cb_t)(ts_test_t *);

/* test suite operations */
ts_suite_t *ts_suite_new  (const char *fmt, ...);
void        ts_suite_test (ts_suite_t *s, ts_test_cb_t func, const char *fmt, ...);
int         ts_suite_run  (ts_suite_t *s);
void        ts_suite_free (ts_suite_t *s);

/* test operations */
ts_test_t  *ts_test_ctx   (ts_test_t *t, const char *file, int line);
void        ts_test_check (ts_test_t *t, const char *fmt, ...);
void        ts_test_fail  (ts_test_t *t, const char *fmt, ...);
void        ts_test_log   (ts_test_t *t, const char *fmt, ...);

/* test suite short-cut macros */
#define TS_TEST(name) \
    static void name(ts_test_t *_t)
#define TS_CTX \
    ts_test_ctx(_t, __FILE__, __LINE__)

#endif /* _TS_H_ */

