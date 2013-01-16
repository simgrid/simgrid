/* cunit - A little C Unit facility                                         */

/* Copyright (c) 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is partially inspirated from the OSSP ts (Test Suite Library)       */

#ifndef _XBT_CUNIT_H_
#define _XBT_CUNIT_H_

#include "xbt/sysdep.h"         /* XBT_GNU_PRINTF */
#include "xbt/ex.h"

SG_BEGIN_DECL()

/* note that the internals of testall, that follow, are not publicly documented */

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
XBT_PUBLIC(void) xbt_test_suite_push(xbt_test_suite_t suite,
                                     const char *name, ts_test_cb_t func,
                                     const char *fmt, ...);

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

XBT_PUBLIC(int) xbt_test_run(char *selection, int verbosity);
/* Show information about the selection of tests */
XBT_PUBLIC(void) xbt_test_dump(char *selection);
/* Cleanup the mess */
XBT_PUBLIC(void) xbt_test_exit(void);


/** 
 * @addtogroup XBT_cunit
 * @brief Unit test mechanism (to test a set of functions)
 *  
 * This module is mainly intended to allow the tests of SimGrid
 * itself and may lack the level of genericity that you would expect
 * as a user. Only use it in external projects at your own risk (but
 * it work rather well for us). We play with the idea of migrating
 * to an external solution for our unit tests, possibly offering
 * more features, but having absolutely no dependencies is a nice
 * feature of SimGrid (and this code is sufficient to cover our
 * needs, actually, so why should we bother switching?)
 * 
 * Note that if you want to test a full binary (such as an example),
 * you want to use our integration testing mechanism, not our unit
 * testing one. Please refer to Section \ref
 * inside_cmake_addtest_integration
 * 
 * Some more information on our unit testing is available in Section @ref inside_cmake_addtest_unit.  
 * 
 * All code intended to be executed as a unit test will be extracted
 * by a script (tools/sg_unit_extract.pl), and must thus be protected
 * between preprocessor definitions, as follows. Note that
 * SIMGRID_TEST string must appear on the endif line too for the
 * script to work, and that this script does not allow to have more
 * than one suite per file. For now, but patches are naturally
 * welcome.
 * 
@verbatim
#ifdef SIMGRID_TEST

<your code>

#endif  // SIMGRID_TEST
@endverbatim
 * 
 *
 * @{ 
 */
/** @brief Provide informations about the suite declared in this file
 *  @hideinitializer
 * 
 * Actually, this macro is not used by C, but by the script
 * extracting the test units, but that should be transparent for you.
 *
 * @param suite_name the short name of this suite, to be used in the --tests argument of testall afterward. Avoid spaces and any other strange chars
 * @param suite_title instructive title that testall should display when your suite is run
 */
#define XBT_TEST_SUITE(suite_name,suite_title)

/** @brief Declare a new test units (containing individual tests)
 *  @hideinitializer
 *
 * @param name the short name that will be used in test all to enable/disable this test
 * @param func a valid function name that will be used to contain all code of this unit
 * @param title human informative description of your test (displayed in testall)
 */
#define XBT_TEST_UNIT(name,func,title)    \
    void func(void);  /*prototype*/       \
    void func(void)


/* test operations */
XBT_PUBLIC(void) _xbt_test_add(const char *file, int line, const char *fmt,
                               ...) _XBT_GNUC_PRINTF(3, 4);
XBT_PUBLIC(void) _xbt_test_fail(const char *file, int line,
                                const char *fmt, ...) _XBT_GNUC_PRINTF(3,
                                                                       4);
XBT_PUBLIC(void) _xbt_test_log(const char *file, int line, const char *fmt,
                               ...) _XBT_GNUC_PRINTF(3, 4);
/** @brief Declare that a new test begins (printf-like parameters, describing the test) 
 *  @hideinitializer */
#define xbt_test_add(...)       _xbt_test_add(__FILE__, __LINE__, __VA_ARGS__)
/** @brief Declare that the lastly started test failed (printf-like parameters, describing failure cause) 
 *  @hideinitializer */
#define xbt_test_fail(...)      _xbt_test_fail(__FILE__, __LINE__, __VA_ARGS__)
/** @brief The lastly started test is actually an assert
 *  @hideinitializer 
 * 
 * - If provided a uniq parameter, this is assumed to be a condition that is expected to be true
 * - If provided more parameters, the first one is a condition, and the other ones are printf-like arguments that are to be displayed when the condition fails.
 */
#define xbt_test_assert(...)    _XBT_IF_ONE_ARG(_xbt_test_assert_ARG1,  \
                                                _xbt_test_assert_ARGN,  \
                                                __VA_ARGS__)(__VA_ARGS__)
#define _xbt_test_assert_ARG1(cond)      _xbt_test_assert_CHECK(cond,   \
                                                                "%s", #cond)
#define _xbt_test_assert_ARGN(cond, ...) _xbt_test_assert_CHECK(cond,   \
                                                                __VA_ARGS__)
#define _xbt_test_assert_CHECK(cond, ...)                       \
  do { if (!(cond)) xbt_test_fail(__VA_ARGS__); } while (0)
#define xbt_test_log(...)       _xbt_test_log(__FILE__, __LINE__, __VA_ARGS__)

/** @brief Declare that the lastly started test failed because of the provided exception */
XBT_PUBLIC(void) xbt_test_exception(xbt_ex_t e);

/** @brief Declare that the lastly started test was expected to fail (and actually failed) */
XBT_PUBLIC(void) xbt_test_expect_failure(void);
/** @brief Declare that the lastly started test should be skiped today */
XBT_PUBLIC(void) xbt_test_skip(void);

/** @} */

#ifdef XBT_USE_DEPRECATED

/* Kept for backward compatibility. */

#define xbt_test_add0(...)      xbt_test_add(__VA_ARGS__)
#define xbt_test_add1(...)      xbt_test_add(__VA_ARGS__)
#define xbt_test_add2(...)      xbt_test_add(__VA_ARGS__)
#define xbt_test_add3(...)      xbt_test_add(__VA_ARGS__)
#define xbt_test_add4(...)      xbt_test_add(__VA_ARGS__)
#define xbt_test_add5(...)      xbt_test_add(__VA_ARGS__)

#define xbt_test_fail0(...)     xbt_test_fail(__VA_ARGS__)
#define xbt_test_fail1(...)     xbt_test_fail(__VA_ARGS__)
#define xbt_test_fail2(...)     xbt_test_fail(__VA_ARGS__)
#define xbt_test_fail3(...)     xbt_test_fail(__VA_ARGS__)
#define xbt_test_fail4(...)     xbt_test_fail(__VA_ARGS__)
#define xbt_test_fail5(...)     xbt_test_fail(__VA_ARGS__)

#define xbt_test_assert0(...)   xbt_test_assert(__VA_ARGS__)
#define xbt_test_assert1(...)   xbt_test_assert(__VA_ARGS__)
#define xbt_test_assert2(...)   xbt_test_assert(__VA_ARGS__)
#define xbt_test_assert3(...)   xbt_test_assert(__VA_ARGS__)
#define xbt_test_assert4(...)   xbt_test_assert(__VA_ARGS__)
#define xbt_test_assert5(...)   xbt_test_assert(__VA_ARGS__)

#define xbt_test_log0(...)      xbt_test_log(__VA_ARGS__)
#define xbt_test_log1(...)      xbt_test_log(__VA_ARGS__)
#define xbt_test_log2(...)      xbt_test_log(__VA_ARGS__)
#define xbt_test_log3(...)      xbt_test_log(__VA_ARGS__)
#define xbt_test_log4(...)      xbt_test_log(__VA_ARGS__)
#define xbt_test_log5(...)      xbt_test_log(__VA_ARGS__)

#endif

SG_END_DECL()
#endif                          /* _TS_H_ */
