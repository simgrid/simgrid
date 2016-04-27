/* cunit - A little C Unit facility                                         */

/* Copyright (c) 2005-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is partially inspired from the OSSP ts (Test Suite Library)         */

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
XBT_PUBLIC(xbt_test_suite_t) xbt_test_suite_new(const char *name, const char *fmt, ...);
XBT_PUBLIC(xbt_test_suite_t) xbt_test_suite_by_name(const char *name, const char *fmt, ...);
XBT_PUBLIC(void) xbt_test_suite_dump(xbt_test_suite_t suite);
XBT_PUBLIC(void) xbt_test_suite_push(xbt_test_suite_t suite, const char *name, ts_test_cb_t func, const char *fmt, ...);

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
 * @brief Unit testing implementation (see @ref inside_tests_add_units)
 *  
 * This module is mainly intended to allow the tests of SimGrid itself and may lack the level of genericity that you
 * would expect as a user. Only use it in external projects at your own risk (but it works rather well for us). We play
 * with the idea of migrating to an external solution for our unit tests, possibly offering more features, but having
 * absolutely no dependencies is a nice feature of SimGrid (and this code is sufficient to cover our needs, actually,
 * so why should we bother switching?)
 * 
 * Unit testing is not intended to write integration tests.
 * Please refer to \ref inside_tests_add_integration for that instead.
 *
 * @{ 
 */
/** @brief Provide information about the suite declared in this file
 *  @hideinitializer
 * 
 * Actually, this macro is only used by the script extracting the test units, but that should be transparent for you.
 *
 * @param suite_name the short name of this suite, to be used in the --tests argument of testall afterward. Avoid
 *        spaces and any other strange chars
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
#ifdef __cplusplus
#define XBT_TEST_UNIT(name,func,title)    \
    extern "C" void func(void);  /*prototype*/ \
    void func(void)
#else
#define XBT_TEST_UNIT(name,func,title)    \
    void func(void);  /*prototype*/       \
    void func(void)
#endif

/* test operations */
XBT_PUBLIC(void) _xbt_test_add(const char *file, int line, const char *fmt, ...) XBT_ATTRIB_PRINTF(3, 4);
XBT_PUBLIC(void) _xbt_test_fail(const char *file, int line, const char *fmt, ...) XBT_ATTRIB_PRINTF(3, 4);
XBT_PUBLIC(void) _xbt_test_log(const char *file, int line, const char *fmt, ...) XBT_ATTRIB_PRINTF(3, 4);
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
 * - If provided more parameters, the first one is a condition, and the other ones are printf-like arguments that are
 *   to be displayed when the condition fails.
 */
#define xbt_test_assert(...)    _XBT_IF_ONE_ARG(_xbt_test_assert_ARG1,  \
                                                _xbt_test_assert_ARGN,  \
                                                __VA_ARGS__)(__VA_ARGS__)
#define _xbt_test_assert_ARG1(cond)      _xbt_test_assert_CHECK(cond, "%s", #cond)
#define _xbt_test_assert_ARGN(cond, ...) _xbt_test_assert_CHECK(cond, __VA_ARGS__)
#define _xbt_test_assert_CHECK(cond, ...)                       \
  do { if (!(cond)) xbt_test_fail(__VA_ARGS__); } while (0)
/** @brief Report some details to help debugging when the test fails (shown only on failure)
 *  @hideinitializer  */
#define xbt_test_log(...)       _xbt_test_log(__FILE__, __LINE__, __VA_ARGS__)

/** @brief Declare that the lastly started test failed because of the provided exception */
XBT_PUBLIC(void) xbt_test_exception(xbt_ex_t e);

/** @brief Declare that the lastly started test was expected to fail (and actually failed) */
XBT_PUBLIC(void) xbt_test_expect_failure(void);
/** @brief Declare that the lastly started test should be skipped today */
XBT_PUBLIC(void) xbt_test_skip(void);

/** @} */

SG_END_DECL()
#endif                          /* _XBT_CUNIT_H_ */
