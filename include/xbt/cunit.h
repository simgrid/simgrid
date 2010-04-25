/* cunit - A little C Unit facility                                         */

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is partially inspirated from the OSSP ts (Test Suite Library)       */

#ifndef _XBT_CUNIT_H_
#define _XBT_CUNIT_H_

#include "xbt/sysdep.h"         /* XBT_GNU_PRINTF */
#include "xbt/ex.h"

SG_BEGIN_DECL()

/* test suite object type */
     typedef struct s_xbt_test_suite *xbt_test_suite_t;

/* test object type */
     typedef struct s_xbt_test_unit *xbt_test_unit_t;

/* test callback function type */
     typedef void (*ts_test_cb_t) (void);

/* test suite operations */
XBT_PUBLIC(xbt_test_suite_t) xbt_test_suite_new(const char *name,
                                                const char *fmt, ...);
XBT_PUBLIC(xbt_test_suite_t) xbt_test_suite_by_name(const char *name,
                                                    const char *fmt, ...);
XBT_PUBLIC(void) xbt_test_suite_dump(xbt_test_suite_t suite);
XBT_PUBLIC(void) xbt_test_suite_push(xbt_test_suite_t suite, const char *name,
                                     ts_test_cb_t func, const char *fmt, ...);

/* Run all the specified tests. what_to_do allows to disable some tests.
 * It is a coma (,) separated list of directives. They are applied from left to right.
 *
 * Each of them of form:
 * 
 * [-|+]suitename[:unitname[:testname]]
 * 
 * * First char: 
 *   if it's a '-', the directive disables something
 *   if it's a '+', the directive enables something
 *   By default, everything is enabled, but you can disable a suite and reenable some parts
 * * Suitename: the suite on which the directive acts
 * * unitname: if given, the unit on which the directive acts. If not, acts on any units.
 * * testname: if given, the test on which the directive acts. If not, acts on any tests.
 */

XBT_PUBLIC(int) xbt_test_run(char *selection);
/* Show information about the selection of tests */
XBT_PUBLIC(void) xbt_test_dump(char *selection);
/* Cleanup the mess */
XBT_PUBLIC(void) xbt_test_exit(void);

/* test operations */
XBT_PUBLIC(void) _xbt_test_add(const char *file, int line, const char *fmt,
                               ...) _XBT_GNUC_PRINTF(3, 4);
#define xbt_test_add0(fmt)           _xbt_test_add(__FILE__,__LINE__,fmt)
#define xbt_test_add1(fmt,a)         _xbt_test_add(__FILE__,__LINE__,fmt,a)
#define xbt_test_add2(fmt,a,b)       _xbt_test_add(__FILE__,__LINE__,fmt,a,b)
#define xbt_test_add3(fmt,a,b,c)     _xbt_test_add(__FILE__,__LINE__,fmt,a,b,c)
#define xbt_test_add4(fmt,a,b,c,d)   _xbt_test_add(__FILE__,__LINE__,fmt,a,b,c,d)
#define xbt_test_add5(fmt,a,b,c,d,e) _xbt_test_add(__FILE__,__LINE__,fmt,a,b,c,d,e)

XBT_PUBLIC(void) _xbt_test_fail(const char *file, int line, const char *fmt,
                                ...) _XBT_GNUC_PRINTF(3, 4);
#define xbt_test_fail0(fmt)           _xbt_test_fail(__FILE__, __LINE__, fmt)
#define xbt_test_fail1(fmt,a)         _xbt_test_fail(__FILE__, __LINE__, fmt,a)
#define xbt_test_fail2(fmt,a,b)       _xbt_test_fail(__FILE__, __LINE__, fmt,a,b)
#define xbt_test_fail3(fmt,a,b,c)     _xbt_test_fail(__FILE__, __LINE__, fmt,a,b,c)
#define xbt_test_fail4(fmt,a,b,c,d)   _xbt_test_fail(__FILE__, __LINE__, fmt,a,b,c,d)
#define xbt_test_fail5(fmt,a,b,c,d,e) _xbt_test_fail(__FILE__, __LINE__, fmt,a,b,c,d,e)

#define xbt_test_assert0(cond,fmt)           if(!(cond)) xbt_test_fail0(fmt)
#define xbt_test_assert1(cond,fmt,a)         if(!(cond)) xbt_test_fail1(fmt,a)
#define xbt_test_assert2(cond,fmt,a,b)       if(!(cond)) xbt_test_fail2(fmt,a,b)
#define xbt_test_assert3(cond,fmt,a,b,c)     if(!(cond)) xbt_test_fail3(fmt,a,b,c)
#define xbt_test_assert4(cond,fmt,a,b,c,d)   if(!(cond)) xbt_test_fail4(fmt,a,b,c,d)
#define xbt_test_assert5(cond,fmt,a,b,c,d,e) if(!(cond)) xbt_test_fail5(fmt,a,b,c,d,e)
#define xbt_test_assert(cond)                xbt_test_assert0(cond,#cond)

XBT_PUBLIC(void) _xbt_test_log(const char *file, int line, const char *fmt,
                               ...) _XBT_GNUC_PRINTF(3, 4);
#define xbt_test_log0(fmt)           _xbt_test_log(__FILE__, __LINE__, fmt)
#define xbt_test_log1(fmt,a)         _xbt_test_log(__FILE__, __LINE__, fmt,a)
#define xbt_test_log2(fmt,a,b)       _xbt_test_log(__FILE__, __LINE__, fmt,a,b)
#define xbt_test_log3(fmt,a,b,c)     _xbt_test_log(__FILE__, __LINE__, fmt,a,b,c)
#define xbt_test_log4(fmt,a,b,c,d)   _xbt_test_log(__FILE__, __LINE__, fmt,a,b,c,d)
#define xbt_test_log5(fmt,a,b,c,d,e) _xbt_test_log(__FILE__, __LINE__, fmt,a,b,c,d,e)

XBT_PUBLIC(void) xbt_test_exception(xbt_ex_t e);

XBT_PUBLIC(void) xbt_test_expect_failure(void);
XBT_PUBLIC(void) xbt_test_skip(void);

/* test suite short-cut macros */
#define XBT_TEST_UNIT(name,func,title)    \
    void func(void);  /*prototype*/       \
    void func(void)

SG_END_DECL()
#endif /* _TS_H_ */
