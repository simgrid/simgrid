/* $Id$ */

/* cunit - A little C Unit facility                                         */

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is partially inspirated from the OSSP ts (Test Suite Library)       */

#include "gras_config.h"

#include "xbt/sysdep.h"    /* vasprintf */
#include "xbt/cunit.h"
#include "xbt/dynar.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(testsuite,xbt,"Test infrastructure");

/* collection of all suites */
static xbt_dynar_t _xbt_test_suites = NULL; 
/* global statistics */
static int _xbt_test_nb_tests = 0;
static int _xbt_test_test_failed = 0;
static int _xbt_test_test_ignore = 0;
static int _xbt_test_test_expect = 0;

static int _xbt_test_nb_units    = 0;
static int _xbt_test_unit_failed = 0;
static int _xbt_test_unit_ignore = 0;

static int _xbt_test_nb_suites    = 0;
static int _xbt_test_suite_failed = 0;
static int _xbt_test_suite_ignore = 0;


/* test suite test log */
typedef struct s_xbt_test_log {
  char              *text;
  const char        *file;
  int                line;
} *xbt_test_log_t;

static void xbt_test_log_dump(xbt_test_log_t log) {
  if (log)
    fprintf(stderr,"      log %p(%s:%d)=%s\n",log,log->file,log->line,log->text);
  else
    fprintf(stderr,"      log=NULL\n");	   
}
static void xbt_test_log_free(xbt_test_log_t log) {
  if (!log)
    return;
  if (log->text)
    free(log->text);
  free(log);
}

/* test suite test check */
typedef struct s_xbt_test_test {
  char        *title;
  int          failed;
  int          expected_failure;
  int          ignored;
  const char  *file;
  int          line;
  xbt_dynar_t  logs;
} *xbt_test_test_t;

static void xbt_test_test_dump(xbt_test_test_t test){
  if (test) {
    xbt_test_log_t log;
    int it_log;
    fprintf(stderr,"    test %p(%s:%d)=%s (%s)\n",
	    test,test->file,test->line,test->title,
	    test->failed?"failed":"not failed");
    xbt_dynar_foreach(test->logs,it_log,log)
      xbt_test_log_dump(log);
  }
  else
    fprintf(stderr,"    test=NULL\n");	   
}

/* test suite test unit */
struct s_xbt_test_unit {
  char        *title;
  ts_test_cb_t func;
  const char  *file;
  int          line;
  xbt_dynar_t  tests; /* of xbt_test_test_t*/

  int nb_tests;
  int test_failed,test_ignore,test_expect;
};

static void xbt_test_unit_dump(xbt_test_unit_t unit) {
  if (unit) {
    xbt_test_test_t test;
    int it_test;
    fprintf(stderr,"  unit %p(%s:%d)=%s (func=%p)\n",
	    unit,unit->file,unit->line,unit->title,unit->file);
    xbt_dynar_foreach(unit->tests,it_test,test)
      xbt_test_test_dump(test);
  } else {
    fprintf(stderr,"  unit=NULL\n");
  }
}

/* test suite */
struct s_xbt_test_suite {
  char       *title;
  xbt_dynar_t units; /* of xbt_test_unit_t */

  int nb_tests,nb_units;
  int test_failed,test_ignore,test_expect;
  int unit_failed,unit_ignore;
};

/* destroy test suite */
static void xbt_test_suite_free(void *s) {
  xbt_test_suite_t suite = *(xbt_test_suite_t*) s;

  if (suite == NULL)
    return;
  xbt_dynar_free(&suite->units);
  free(suite->title);
  free(suite);
}

/** @brief create test suite */
xbt_test_suite_t xbt_test_suite_new(const char *fmt, ...) {
  xbt_test_suite_t suite = xbt_new0(struct s_xbt_test_suite,1);
  va_list ap;

  if (!_xbt_test_suites) 
    _xbt_test_suites = xbt_dynar_new(sizeof(xbt_test_suite_t),&xbt_test_suite_free);

  va_start(ap, fmt);
  vasprintf(&suite->title,fmt, ap);
  suite->units = xbt_dynar_new(sizeof(xbt_test_unit_t), NULL);
  va_end(ap);

  xbt_dynar_push(_xbt_test_suites,&suite);

  return suite;
}

void xbt_test_suite_dump(xbt_test_suite_t suite) {
  if (suite) {
    xbt_test_unit_t unit;
    int it_unit;
    fprintf(stderr,"DUMP suite %s\n",suite->title);
    xbt_dynar_foreach(suite->units,it_unit,unit)
      xbt_test_unit_dump(unit);
  } else {
    fprintf(stderr,"suite=NULL\n");
  }
}

/* add test case to test suite */
void xbt_test_suite_push(xbt_test_suite_t suite, ts_test_cb_t func, const char *fmt, ...) {
  xbt_test_unit_t unit;
  va_list ap;
  
  xbt_assert(suite);
  xbt_assert(func);
  xbt_assert(fmt);

  unit = xbt_new(struct s_xbt_test_unit,1);
  va_start(ap, fmt);
  vasprintf(&unit->title, fmt, ap);
  va_end(ap);
  unit->func = func;
  unit->file = NULL;
  unit->line = 0;
  unit->tests = xbt_dynar_new(sizeof(xbt_test_test_t), NULL);
  
  xbt_dynar_push(suite->units, &unit);
  return;
}

/* run test one suite */
static int xbt_test_suite_run(xbt_test_suite_t suite) {
  xbt_test_unit_t unit;
  xbt_test_test_t test;
  xbt_test_log_t log;

  const char *file;
  int line;
  char *cp;
  int it_unit,it_test,it_log;

  if (suite == NULL)
    return 0;

  /* iterate through all tests to see how much failed */
  xbt_dynar_foreach(suite->units, it_unit, unit) {
    /* init unit case counters */
    unit->nb_tests = 0;
    unit->test_ignore = 0;
    unit->test_failed = 0;
    unit->test_expect = 0;

    /* run the test case function */
    unit->func(unit);
  
    /* iterate through all performed tests to determine status */
    xbt_dynar_foreach(unit->tests,it_test, test) {
      if (test->ignored) {
	unit->test_ignore++;
      } else {
	unit->nb_tests++;

	if ( test->failed && !test->expected_failure) unit->test_failed++;
	if (!test->failed &&  test->expected_failure) unit->test_failed++;
	if (test->expected_failure)
	  unit->test_expect++;
      }
    }
    
    /* Accumulate test counts into the suite */
    suite->nb_tests    += unit->nb_tests;
    suite->test_failed += unit->test_failed;
    suite->test_ignore += unit->test_ignore;
    suite->test_expect += unit->test_expect;

    _xbt_test_nb_tests    += unit->nb_tests;
    _xbt_test_test_failed += unit->test_failed;
    _xbt_test_test_ignore += unit->test_ignore;
    _xbt_test_test_expect += unit->test_expect;
    
    /* What's the conclusion of this test anyway? */
    if (unit->nb_tests) {
      suite->nb_units++;
      if (unit->test_failed)
	suite->unit_failed++;
    } else {
      suite->unit_ignore++;
    }
  }
  _xbt_test_nb_units    += suite->nb_units;
  _xbt_test_unit_failed += suite->unit_failed;
  _xbt_test_unit_ignore += suite->unit_ignore;

  if (suite->nb_units) {
    _xbt_test_nb_suites++;
    if (suite->test_failed)
      _xbt_test_suite_failed++;
  } else {
    _xbt_test_suite_ignore++;
  }

  /* suite title pretty-printing */
  {
    char suite_title[80];
    int suite_len=strlen(suite->title);
    int i;

    xbt_assert2(suite_len<70,"suite title \"%s\" too long (%d should be less than 70",
		suite->title,suite_len);
    
    suite_title[0]=' ';
    for (i=1;i<79;i++)
      suite_title[i]='=';
    suite_title[i]='\0';

    sprintf(suite_title + 40 - (suite_len+4)/2, "[ %s ]", suite->title);
    suite_title[40 + (suite_len+4)/2] = '=';

    fprintf(stderr, "\n%s  %s\n",suite_title,
	    (suite->nb_units?(suite->unit_failed?"FAILED":"OK"):"SKIP"));
    
  }
  
  /* iterate through all test cases to display details */
  xbt_dynar_foreach(suite->units, it_unit, unit) {
    asprintf(&cp," Unit: %s ........................................"
	     "........................................", unit->title);
    cp[72] = '\0';
    fprintf(stderr, "%s", cp);
    free(cp);
    
    if (unit->test_failed > 0 || unit->test_expect) {
      /* some tests failed (or were supposed to), so do detailed reporting of test case */
      if (unit->test_failed > 0) {
	fprintf(stderr, " failed\n");
      } else if (unit->nb_tests) {
	fprintf(stderr, ".... ok\n"); /* successful, but show about expected */
      } else {
	fprintf(stderr, ".. skip\n"); /* shouldn't happen, but I'm a bit lost with this logic */
      }
      xbt_dynar_foreach(unit->tests,it_test, test) {
	file = (test->file != NULL ? test->file : unit->file);
	line = (test->line != 0    ? test->line : unit->line);
	fprintf(stderr, "      %s: %s [%s:%d]\n", 
		(test->ignored?" SKIP":(test->expected_failure?(test->failed?"EFAIL":"EPASS"):
					                         (test->failed?" FAIL":" PASS"))),
		test->title, file, line);

	xbt_dynar_foreach(test->logs,it_log,log) {
	  file = (log->file != NULL ? log->file : file);
	  line = (log->line != 0    ? log->line : line);
	    fprintf(stderr, "             %s:%d: %s\n", 
		    file, line,log->text);

	}
      }
      fprintf(stderr, "    Summary: %d of %d tests failed",unit->test_failed, unit->nb_tests);
      if (unit->test_ignore) {
	fprintf(stderr," (%d tests ignored)\n",unit->test_ignore);
      } else {
	fprintf(stderr,"\n");
      }
    } else if (unit->nb_tests) {
      fprintf(stderr, ".... ok\n"); /* successful */
    } else {
      fprintf(stderr, ".. skip\n"); /* no test were run */
    }
  }
  
  /* print test suite summary */
  fprintf(stderr, " ==============================================================================\n");
  fprintf(stderr, " Summary: Units: %.0f%% ok (%d units: ", 
	  suite->nb_units?((1-(double)suite->unit_failed/(double)suite->nb_units)*100.0):100.0,
	  suite->nb_units);
  int first=1;
  if (suite->nb_units != suite->unit_failed) {
    fprintf(stderr, "%s%d ok",(first?"":", "),suite->nb_units - suite->unit_failed);
    first = 0;
  }
  if (suite->unit_failed) {
    fprintf(stderr, "%s%d failed",(first?"":", "),suite->unit_failed);
    first = 0;
  }
  if (suite->unit_ignore) {
    fprintf(stderr, "%s%d ignored",(first?"":", "),suite->unit_ignore);
    first = 0;
  }
  fprintf(stderr,")\n          Tests: %.0f%% ok (%d tests: ",
	  suite->nb_tests?((1-(double)suite->test_failed/(double)suite->nb_tests)*100.0):100.0,
	  suite->nb_tests);

  first=1;
  if (suite->nb_tests != suite->test_failed) {
    fprintf(stderr, "%s%d ok",(first?"":", "),suite->nb_tests - suite->test_failed);
    first = 0;
  }
  if (suite->test_failed) {
    fprintf(stderr, "%s%d failed",(first?"":", "),suite->test_failed);
    first = 0;
  }
  if (suite->test_ignore) {
    fprintf(stderr, "%s%d ignored",(first?"":"; "),suite->test_ignore);
    first = 0;
  }
  if (suite->test_expect) {
    fprintf(stderr, "%s%d expected to fail",(first?"":"; "),suite->test_expect);
    first = 0;
  }
  fprintf(stderr,")\n");

  return suite->unit_failed;
}

int xbt_test_run(void) {
  
  if (_xbt_test_suites) {
    int it_suite;
    xbt_test_suite_t suite;
    int first=1;
    
    /* Run all the suites */
    xbt_dynar_foreach(_xbt_test_suites,it_suite,suite) 
      xbt_test_suite_run(suite);

    /* Display some more statistics */
    fprintf(stderr,"\n\n TOTAL: Suites: %.0f%% ok (%d suites: ",
	    _xbt_test_nb_suites
	      ? ((1-(double)_xbt_test_suite_failed/(double)_xbt_test_nb_suites)*100.0)
	      : 100.0,
	    _xbt_test_nb_suites);
    if (_xbt_test_nb_suites != _xbt_test_suite_failed) {
      fprintf(stderr, "%d ok",_xbt_test_nb_suites - _xbt_test_suite_failed);
      first = 0;
    }
    if (_xbt_test_suite_failed) {
      fprintf(stderr, "%s%d failed",(first?"":", "),_xbt_test_suite_failed);
      first = 0;
    }
    
    if (_xbt_test_suite_ignore) {
      fprintf(stderr, "%s%d ignored",(first?"":", "),_xbt_test_suite_ignore);
      first = 0;
    }
    fprintf(stderr,")\n        Units:  %.0f%% ok (%d units:  ",
	    _xbt_test_nb_units?((1-(double)_xbt_test_unit_failed/(double)_xbt_test_nb_units)*100.0):100.0,
	    _xbt_test_nb_units);
    first=1;
    if (_xbt_test_nb_units != _xbt_test_unit_failed) {
      fprintf(stderr, "%s%d ok",(first?"":", "),_xbt_test_nb_units - _xbt_test_unit_failed);
      first = 0;
    }
    if (_xbt_test_unit_failed) {
      fprintf(stderr, "%s%d failed",(first?"":", "),_xbt_test_unit_failed);
      first = 0;
    }
    if (_xbt_test_unit_ignore) {
      fprintf(stderr, "%s%d ignored",(first?"":", "),_xbt_test_unit_ignore);
      first = 0;
    }
    fprintf(stderr,")\n        Tests:  %.0f%% ok (%d tests:  ",
	    _xbt_test_nb_tests?((1-(double)_xbt_test_test_failed/(double)_xbt_test_nb_tests)*100.0):100.0,
	    _xbt_test_nb_tests);
    first=1;
    if (_xbt_test_nb_tests != _xbt_test_test_failed) {
      fprintf(stderr, "%s%d ok",(first?"":", "),_xbt_test_nb_tests - _xbt_test_test_failed);
      first = 0;
    }
    if (_xbt_test_test_failed) {
      fprintf(stderr, "%s%d failed",(first?"":", "),_xbt_test_test_failed);
      first = 0;
    }
    if (_xbt_test_test_ignore) {
      fprintf(stderr, "%s%d ignored",(first?"":", "),_xbt_test_test_ignore);
      first = 0;
    }
    if (_xbt_test_test_expect) {
      fprintf(stderr, "%s%d expected to fail",(first?"":", "),_xbt_test_test_expect);
    }
    
    fprintf(stderr,")\n");
  } else {
    fprintf(stderr,"No unit to run!\n");
    _xbt_test_unit_failed++;
  }
  return _xbt_test_unit_failed;
}


/* annotate test case with test */
void _xbt_test(xbt_test_unit_t unit, const char*file,int line, const char *fmt, ...) {
  xbt_test_test_t test;
  va_list ap;
  
  xbt_assert(unit);
  xbt_assert(fmt);

  test = xbt_new(struct s_xbt_test_test,1);
  va_start(ap, fmt);
  vasprintf(&test->title, fmt, ap);
  va_end(ap);
  test->failed = 0;
  test->expected_failure = 0;
  test->ignored = 0;
  test->file = file;
  test->line = line;
  test->logs = xbt_dynar_new(sizeof(xbt_test_log_t),NULL);
  xbt_dynar_push(unit->tests,&test);
  return;
}

/* annotate test case with log message and failure */
void _xbt_test_fail(xbt_test_unit_t unit,  const char*file,int line,const char *fmt, ...) {
  xbt_test_test_t test;
  xbt_test_log_t log;
  va_list ap;
  
  xbt_assert(unit);
  xbt_assert(fmt);

  log = xbt_new(struct s_xbt_test_log,1);
  va_start(ap, fmt);
  vasprintf(&log->text,fmt, ap);
  va_end(ap);
  log->file = file;
  log->line = line;

  test = xbt_dynar_getlast_as(unit->tests, xbt_test_test_t);
  xbt_dynar_push(test->logs, &log);

  test->failed = 1;
}

void _xbt_test_expect_failure(xbt_test_unit_t unit) {
  xbt_test_test_t test = xbt_dynar_getlast_as(unit->tests,xbt_test_test_t);
  test->expected_failure = 1;
}
void _xbt_test_skip(xbt_test_unit_t unit) {
  xbt_test_test_t test = xbt_dynar_getlast_as(unit->tests,xbt_test_test_t);
  test->ignored = 1;
}

/* annotate test case with log message only */
void _xbt_test_log(xbt_test_unit_t unit, const char*file,int line,const char *fmt, ...) {
  xbt_test_test_t test;
  xbt_test_log_t log;
  va_list ap;

  xbt_assert(unit);
  xbt_assert(fmt);

  log = xbt_new(struct s_xbt_test_log,1);
  va_start(ap, fmt);
  vasprintf(&log->text, fmt, ap);
  va_end(ap);
  log->file = file;
  log->line = line;

  test = xbt_dynar_getlast_as(unit->tests,xbt_test_test_t);
  xbt_dynar_push(test->logs, &log);
}

