/* $Id$ */

/* cunit - A little C Unit facility                                         */

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is partially inspirated from the OSSP ts (Test Suite Library)       */

#ifndef _XBT_CUNIT_H_
#define _XBT_CUNIT_H_

#include "xbt/sysdep.h"    /* XBT_GNU_PRINTF */

/* test suite object type */
typedef struct s_xbt_test_suite *xbt_test_suite_t;

/* test object type */
typedef struct s_xbt_test_unit *xbt_test_unit_t;

/* test callback function type */
typedef void (*ts_test_cb_t)(xbt_test_unit_t);

/* test suite operations */
xbt_test_suite_t xbt_test_suite_new  (const char *fmt, ...);
void             xbt_test_suite_dump (xbt_test_suite_t suite);
void             xbt_test_suite_push (xbt_test_suite_t suite, 
				      ts_test_cb_t func, const char *fmt, ...);

int xbt_test_run  (void);


/* test operations */
void    _xbt_test(xbt_test_unit_t u, const char*file,int line, const char *fmt, ...)_XBT_GNUC_PRINTF(4,5);
#define xbt_test0(fmt)           _xbt_test(_unit,__FILE__,__LINE__,fmt)
#define xbt_test1(fmt,a)         _xbt_test(_unit,__FILE__,__LINE__,fmt,a)
#define xbt_test2(fmt,a,b)       _xbt_test(_unit,__FILE__,__LINE__,fmt,a,b)
#define xbt_test3(fmt,a,b,c)     _xbt_test(_unit,__FILE__,__LINE__,fmt,a,b,c)
#define xbt_test4(fmt,a,b,c,d)   _xbt_test(_unit,__FILE__,__LINE__,fmt,a,b,c,d)
#define xbt_test5(fmt,a,b,c,d,e) _xbt_test(_unit,__FILE__,__LINE__,fmt,a,b,c,d,e)

void    _xbt_test_fail(xbt_test_unit_t u, const char*file,int line, const char *fmt, ...) _XBT_GNUC_PRINTF(4,5);
#define xbt_test_fail0(fmt)           _xbt_test_fail(_unit, __FILE__, __LINE__, fmt)
#define xbt_test_fail1(fmt,a)         _xbt_test_fail(_unit, __FILE__, __LINE__, fmt,a)
#define xbt_test_fail2(fmt,a,b)       _xbt_test_fail(_unit, __FILE__, __LINE__, fmt,a,b)
#define xbt_test_fail3(fmt,a,b,c)     _xbt_test_fail(_unit, __FILE__, __LINE__, fmt,a,b,c)
#define xbt_test_fail4(fmt,a,b,c,d)   _xbt_test_fail(_unit, __FILE__, __LINE__, fmt,a,b,c,d)
#define xbt_test_fail5(fmt,a,b,c,d,e) _xbt_test_fail(_unit, __FILE__, __LINE__, fmt,a,b,c,d,e)

void    _xbt_test_log (xbt_test_unit_t u, const char*file,int line, const char *fmt, ...)_XBT_GNUC_PRINTF(4,5);
#define xbt_test_log0(fmt)           _xbt_test_log(_unit, __FILE__, __LINE__, fmt)
#define xbt_test_log1(fmt,a)         _xbt_test_log(_unit, __FILE__, __LINE__, fmt,a)
#define xbt_test_log2(fmt,a,b)       _xbt_test_log(_unit, __FILE__, __LINE__, fmt,a,b)
#define xbt_test_log3(fmt,a,b,c)     _xbt_test_log(_unit, __FILE__, __LINE__, fmt,a,b,c)
#define xbt_test_log4(fmt,a,b,c,d)   _xbt_test_log(_unit, __FILE__, __LINE__, fmt,a,b,c,d)
#define xbt_test_log5(fmt,a,b,c,d,e) _xbt_test_log(_unit, __FILE__, __LINE__, fmt,a,b,c,d,e)

void _xbt_test_expect_failure(xbt_test_unit_t unit);
#define    xbt_test_expect_failure() _xbt_test_expect_failure(_unit)

void _xbt_test_skip(xbt_test_unit_t unit);
#define    xbt_test_skip() _xbt_test_skip(_unit)

/* test suite short-cut macros */
#define XBT_TEST_UNIT(name) \
    static void name(xbt_test_unit_t _unit)

#endif /* _TS_H_ */

