/* $Id$ */

/* cunit - A little C Unit facility                                         */

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is partially inspirated from the OSSP ts (Test Suite Library)       */

#include "portable.h"

#include "xbt/sysdep.h"         /* vasprintf */
#include "xbt/cunit.h"
#include "xbt/dynar.h"

/* collection of all suites */
static xbt_dynar_t _xbt_test_suites = NULL;
/* global statistics */
static int _xbt_test_nb_tests = 0;
static int _xbt_test_test_failed = 0;
static int _xbt_test_test_ignore = 0;
static int _xbt_test_test_expect = 0;

static int _xbt_test_nb_units = 0;
static int _xbt_test_unit_failed = 0;
static int _xbt_test_unit_ignore = 0;
static int _xbt_test_unit_disabled = 0;

static int _xbt_test_nb_suites = 0;
static int _xbt_test_suite_failed = 0;
static int _xbt_test_suite_ignore = 0;
static int _xbt_test_suite_disabled = 0;

/* Context */
xbt_test_unit_t _xbt_test_current_unit = NULL;


/* test suite test log */
typedef struct s_xbt_test_log {
  char *text;
  const char *file;
  int line;
} *xbt_test_log_t;

static void xbt_test_log_dump(xbt_test_log_t log)
{
  if (log)
    fprintf(stderr, "      log %p(%s:%d)=%s\n", log, log->file, log->line,
            log->text);
  else
    fprintf(stderr, "      log=NULL\n");
}

/* test suite test check */
typedef struct s_xbt_test_test {
  char *title;
  int failed;
  int expected_failure;
  int ignored;
  const char *file;
  int line;
  xbt_dynar_t logs;
} *xbt_test_test_t;

static void xbt_test_test_dump(xbt_test_test_t test)
{
  if (test) {
    xbt_test_log_t log;
    unsigned int it_log;
    fprintf(stderr, "    test %p(%s:%d)=%s (%s)\n",
            test, test->file, test->line, test->title,
            test->failed ? "failed" : "not failed");
    xbt_dynar_foreach(test->logs, it_log, log)
      xbt_test_log_dump(log);
  } else
    fprintf(stderr, "    test=NULL\n");
}

/* test suite test unit */
struct s_xbt_test_unit {
  int enabled;
  char *name;
  char *title;
  ts_test_cb_t func;
  const char *file;
  int line;
  xbt_dynar_t tests;            /* of xbt_test_test_t */

  int nb_tests;
  int test_failed, test_ignore, test_expect;
};

static void xbt_test_unit_dump(xbt_test_unit_t unit)
{
  if (unit) {
    xbt_test_test_t test;
    unsigned int it_test;
    fprintf(stderr, "  UNIT %s: %s (%s)\n",
            unit->name, unit->title,
            (unit->enabled ? "enabled" : "disabled"));
    if (unit->enabled)
      xbt_dynar_foreach(unit->tests, it_test, test)
        xbt_test_test_dump(test);
  } else {
    fprintf(stderr, "  unit=NULL\n");
  }
}

/* test suite */
struct s_xbt_test_suite {
  int enabled;
  const char *name;
  char *title;
  xbt_dynar_t units;            /* of xbt_test_unit_t */

  int nb_tests, nb_units;
  int test_failed, test_ignore, test_expect;
  int unit_failed, unit_ignore, unit_disabled;
};

/* destroy test suite */
static void xbt_test_suite_free(void *s)
{
  xbt_test_suite_t suite = *(xbt_test_suite_t *) s;

  if (suite == NULL)
    return;
  xbt_dynar_free(&suite->units);
  free(suite->title);
  free(suite);
}

static void xbt_test_unit_free(void *unit)
{
  xbt_test_unit_t u = *(xbt_test_unit_t *) unit;
  /* name is static */
  free(u->title);
  xbt_dynar_free(&u->tests);
  free(u);
}

static void xbt_test_test_free(void *test)
{
  xbt_test_test_t t = *(xbt_test_test_t *) test;
  free(t->title);
  xbt_dynar_free(&(t->logs));
  free(t);
}

static void xbt_test_log_free(void *log)
{
  xbt_test_log_t l = *(xbt_test_log_t *) log;
  free(l->text);
  free(l);
}

/** @brief create test suite */
xbt_test_suite_t xbt_test_suite_new(const char *name, const char *fmt, ...)
{
  xbt_test_suite_t suite = xbt_new0(struct s_xbt_test_suite, 1);
  va_list ap;
  int vres;

  if (!_xbt_test_suites)
    _xbt_test_suites =
      xbt_dynar_new(sizeof(xbt_test_suite_t), xbt_test_suite_free);

  va_start(ap, fmt);
  vres = vasprintf(&suite->title, fmt, ap);
  suite->units = xbt_dynar_new(sizeof(xbt_test_unit_t), &xbt_test_unit_free);
  va_end(ap);
  suite->name = name;
  suite->enabled = 1;

  xbt_dynar_push(_xbt_test_suites, &suite);

  return suite;
}

/** @brief retrieve a testsuite from name, or create a new one */
xbt_test_suite_t xbt_test_suite_by_name(const char *name, const char *fmt,
                                        ...)
{
  xbt_test_suite_t suite;
  unsigned int it_suite;

  char *bufname;
  va_list ap;
  int vres;

  if (_xbt_test_suites)
    xbt_dynar_foreach(_xbt_test_suites, it_suite, suite)
      if (!strcmp(suite->name, name))
      return suite;

  va_start(ap, fmt);
  vres = vasprintf(&bufname, fmt, ap);
  va_end(ap);
  suite = xbt_test_suite_new(name, bufname, NULL);
  free(bufname);

  return suite;
}

void xbt_test_suite_dump(xbt_test_suite_t suite)
{
  if (suite) {
    xbt_test_unit_t unit;
    unsigned int it_unit;
    fprintf(stderr, "TESTSUITE %s: %s (%s)\n",
            suite->name, suite->title,
            suite->enabled ? "enabled" : "disabled");
    if (suite->enabled)
      xbt_dynar_foreach(suite->units, it_unit, unit)
        xbt_test_unit_dump(unit);
  } else {
    fprintf(stderr, "TESTSUITE IS NULL!\n");
  }
}

/* add test case to test suite */
void xbt_test_suite_push(xbt_test_suite_t suite, const char *name,
                         ts_test_cb_t func, const char *fmt, ...)
{
  xbt_test_unit_t unit;
  va_list ap;
  int vres;

  xbt_assert(suite);
  xbt_assert(func);
  xbt_assert(fmt);

  unit = xbt_new0(struct s_xbt_test_unit, 1);
  va_start(ap, fmt);
  vres = vasprintf(&unit->title, fmt, ap);
  va_end(ap);
  unit->name = (char *) name;
  unit->func = func;
  unit->file = NULL;
  unit->line = 0;
  unit->enabled = 1;
  unit->tests = xbt_dynar_new(sizeof(xbt_test_test_t), xbt_test_test_free);

  xbt_dynar_push(suite->units, &unit);
  return;
}

/* run test one suite */
static int xbt_test_suite_run(xbt_test_suite_t suite)
{
  xbt_test_unit_t unit;
  xbt_test_test_t test;
  xbt_test_log_t log;

  const char *file;
  int line;
  char *cp;
  unsigned int it_unit, it_test, it_log;

  int first = 1;                /* for result pretty printing */
  int vres;

  if (suite == NULL)
    return 0;

  /* suite title pretty-printing */
  {
    char suite_title[80];
    int suite_len = strlen(suite->title);
    int i;

    xbt_assert2(suite_len < 68,
                "suite title \"%s\" too long (%d should be less than 68",
                suite->title, suite_len);

    suite_title[0] = ' ';
    for (i = 1; i < 79; i++)
      suite_title[i] = '=';
    suite_title[i++] = '\n';
    suite_title[79] = '\0';

    sprintf(suite_title + 40 - (suite_len + 4) / 2, "[ %s ]", suite->title);
    suite_title[40 + (suite_len + 5) / 2] = '=';
    if (!suite->enabled)
      sprintf(suite_title + 70, " DISABLED ");
    fprintf(stderr, "\n%s\n", suite_title);
  }

  if (suite->enabled) {
    /* iterate through all tests */
    xbt_dynar_foreach(suite->units, it_unit, unit) {
      /* init unit case counters */
      unit->nb_tests = 0;
      unit->test_ignore = 0;
      unit->test_failed = 0;
      unit->test_expect = 0;

      /* display unit title */
      vres = asprintf(&cp, " Unit: %s ......................................"
                      "......................................", unit->title);
      cp[70] = '\0';
      fprintf(stderr, "%s", cp);
      free(cp);

      /* run the test case function */
      _xbt_test_current_unit = unit;
      if (unit->enabled)
        unit->func();

      /* iterate through all performed tests to determine status */
      xbt_dynar_foreach(unit->tests, it_test, test) {
        if (test->ignored) {
          unit->test_ignore++;
        } else {
          unit->nb_tests++;

          if (test->failed && !test->expected_failure)
            unit->test_failed++;
          if (!test->failed && test->expected_failure)
            unit->test_failed++;
          if (test->expected_failure)
            unit->test_expect++;
        }
      }


      /* Display whether this unit went well */
      if (unit->test_failed > 0 || unit->test_expect) {
        /* some tests failed (or were supposed to), so do detailed reporting of test case */
        if (unit->test_failed > 0) {
          fprintf(stderr, ".. failed\n");
        } else if (unit->nb_tests) {
          fprintf(stderr, "...... ok\n");       /* successful, but show about expected */
        } else {
          fprintf(stderr, ".... skip\n");       /* shouldn't happen, but I'm a bit lost with this logic */
        }
        xbt_dynar_foreach(unit->tests, it_test, test) {
          file = (test->file != NULL ? test->file : unit->file);
          line = (test->line != 0 ? test->line : unit->line);
          fprintf(stderr, "      %s: %s [%s:%d]\n",
                  (test->ignored ? " SKIP"
                   : (test->expected_failure
                      ? (test->failed ? "EFAIL" : "EPASS") : (test->failed ?
                                                              " FAIL" :
                                                              " PASS"))),
                  test->title, file, line);

          if ((test->expected_failure && !test->failed)
              || (!test->expected_failure && test->failed)) {
            xbt_dynar_foreach(test->logs, it_log, log) {
              file = (log->file != NULL ? log->file : file);
              line = (log->line != 0 ? log->line : line);
              fprintf(stderr, "             %s:%d: %s\n",
                      file, line, log->text);

            }
          }
        }
        fprintf(stderr, "    Summary: %d of %d tests failed",
                unit->test_failed, unit->nb_tests);
        if (unit->test_ignore) {
          fprintf(stderr, " (%d tests ignored)\n", unit->test_ignore);
        } else {
          fprintf(stderr, "\n");
        }

      } else if (!unit->enabled) {
        fprintf(stderr, " disabled\n"); /* no test were run */
      } else if (unit->nb_tests) {
        fprintf(stderr, "...... ok\n"); /* successful */
      } else {
        fprintf(stderr, ".... skip\n"); /* no test were run */
      }

      /* Accumulate test counts into the suite */
      suite->nb_tests += unit->nb_tests;
      suite->test_failed += unit->test_failed;
      suite->test_ignore += unit->test_ignore;
      suite->test_expect += unit->test_expect;

      _xbt_test_nb_tests += unit->nb_tests;
      _xbt_test_test_failed += unit->test_failed;
      _xbt_test_test_ignore += unit->test_ignore;
      _xbt_test_test_expect += unit->test_expect;

      /* What's the conclusion of this test anyway? */
      if (unit->nb_tests) {
        suite->nb_units++;
        if (unit->test_failed)
          suite->unit_failed++;
      } else if (!unit->enabled) {
        suite->unit_disabled++;
      } else {
        suite->unit_ignore++;
      }
    }
  }
  _xbt_test_nb_units += suite->nb_units;
  _xbt_test_unit_failed += suite->unit_failed;
  _xbt_test_unit_ignore += suite->unit_ignore;
  _xbt_test_unit_disabled += suite->unit_disabled;

  if (suite->nb_units) {
    _xbt_test_nb_suites++;
    if (suite->test_failed)
      _xbt_test_suite_failed++;
  } else if (!suite->enabled) {
    _xbt_test_suite_disabled++;
  } else {
    _xbt_test_suite_ignore++;
  }


  /* print test suite summary */
  if (suite->enabled) {

    fprintf(stderr,
            " =====================================================================%s\n",
            (suite->nb_units
             ? (suite->unit_failed ? "== FAILED" : "====== OK")
             : (suite->unit_disabled ? " DISABLED" : "==== SKIP")));
    fprintf(stderr, " Summary: Units: %.0f%% ok (%d units: ",
            suite->nb_units
            ? ((1 -
                (double) suite->unit_failed / (double) suite->nb_units) *
               100.0) : 100.0, suite->nb_units);

    if (suite->nb_units != suite->unit_failed) {
      fprintf(stderr, "%s%d ok", (first ? "" : ", "),
              suite->nb_units - suite->unit_failed);
      first = 0;
    }
    if (suite->unit_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "), suite->unit_failed);
      first = 0;
    }
    if (suite->unit_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : ", "),
              suite->unit_ignore);
      first = 0;
    }
    if (suite->unit_disabled) {
      fprintf(stderr, "%s%d disabled", (first ? "" : ", "),
              suite->unit_disabled);
      first = 0;
    }
    fprintf(stderr, ")\n          Tests: %.0f%% ok (%d tests: ",
            suite->nb_tests
            ? ((1 -
                (double) suite->test_failed / (double) suite->nb_tests) *
               100.0) : 100.0, suite->nb_tests);

    first = 1;
    if (suite->nb_tests != suite->test_failed) {
      fprintf(stderr, "%s%d ok", (first ? "" : ", "),
              suite->nb_tests - suite->test_failed);
      first = 0;
    }
    if (suite->test_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "), suite->test_failed);
      first = 0;
    }
    if (suite->test_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : "; "),
              suite->test_ignore);
      first = 0;
    }
    if (suite->test_expect) {
      fprintf(stderr, "%s%d expected to fail", (first ? "" : "; "),
              suite->test_expect);
      first = 0;
    }
    fprintf(stderr, ")\n");
  }
  return suite->unit_failed;
}

static void apply_selection(char *selection)
{
  /* for the parsing */
  char *sel = selection;
  char *p;
  int done = 0;
  char dir[1024];               /* the directive */
  /* iterators */
  unsigned int it_suite;
  xbt_test_suite_t suite;
  xbt_test_unit_t unit;
  unsigned int it_unit;

  char suitename[512];
  char unitname[512];

  if (!selection || selection[0] == '\0')
    return;

  /*printf("Test selection: %s\n", selection); */

  /* First apply the selection */
  while (!done) {
    int enabling = 1;

    p = strchr(sel, ',');
    if (p) {
      strncpy(dir, sel, p - sel);
      dir[p - sel] = '\0';
      sel = p + 1;
    } else {
      strcpy(dir, sel);
      done = 1;
    }

    if (dir[0] == '-') {
      enabling = 0;
      memmove(dir, dir + 1, strlen(dir));
    }
    if (dir[0] == '+') {
      enabling = 1;
      memmove(dir, dir + 1, strlen(dir));
    }

    p = strchr(dir, ':');
    if (p) {
      strcpy(unitname, p + 1);
      strncpy(suitename, dir, p - dir);
      suitename[p - dir] = '\0';
    } else {
      strcpy(suitename, dir);
      unitname[0] = '\0';
    }
    /*fprintf(stderr,"Seen %s (%s; suite=%s; unit=%s)\n",
       dir,enabling?"enabling":"disabling", suitename, unitname); */

    /* Deal with the specific case of 'all' pseudo serie */
    if (!strcmp("all", suitename)) {
      if (unitname[0] != '\0') {
        fprintf(stderr,
                "The 'all' pseudo-suite does not accept any unit specification\n");
        exit(1);
      }

      xbt_dynar_foreach(_xbt_test_suites, it_suite, suite) {
        xbt_dynar_foreach(suite->units, it_unit, unit) {
          unit->enabled = enabling;
        }
        suite->enabled = enabling;
      }
    } else {
      unsigned int it;
      for (it = 0; it < xbt_dynar_length(_xbt_test_suites); it++) {
        xbt_test_suite_t thissuite =
          xbt_dynar_get_as(_xbt_test_suites, it, xbt_test_suite_t);
        if (!strcmp(suitename, thissuite->name)) {
          /* Do not disable the whole suite when we just want to disable a child */
          if (enabling || (unitname[0] == '\0'))
            thissuite->enabled = enabling;

          if (unitname[0] == '\0') {
            xbt_dynar_foreach(thissuite->units, it_unit, unit) {
              unit->enabled = enabling;
            }
          } else {              /* act on one child only */
            unsigned int it2_unit;
            /* search it, first (we won't reuse it for external loop which gets broken) */
            for (it2_unit = 0; it2_unit < xbt_dynar_length(thissuite->units);
                 it2_unit++) {
              xbt_test_unit_t thisunit =
                xbt_dynar_get_as(thissuite->units, it2_unit, xbt_test_unit_t);
              if (!strcmp(thisunit->name, unitname)) {
                thisunit->enabled = enabling;
                break;
              }
            }                   /* search relevant unit */
            if (it2_unit == xbt_dynar_length(thissuite->units)) {
              fprintf(stderr,
                      "Suite '%s' has no unit of name '%s'. Cannot apply the selection\n",
                      suitename, unitname);
              exit(1);
            }
          }                     /* act on childs (either all or one) */

          break;                /* found the relevant serie. We are happy */
        }
      }                         /* search relevant series */
      if (it == xbt_dynar_length(_xbt_test_suites)) {
        fprintf(stderr,
                "No suite of name '%s' found. Cannot apply the selection\n",
                suitename);
        exit(1);
      }
    }

  }
}

void xbt_test_dump(char *selection)
{
  apply_selection(selection);

  if (_xbt_test_suites) {
    unsigned int it_suite;
    xbt_test_suite_t suite;

    xbt_dynar_foreach(_xbt_test_suites, it_suite, suite)
      xbt_test_suite_dump(suite);
  } else {
    printf(" No suite defined.");
  }
}

int xbt_test_run(char *selection)
{
  apply_selection(selection);

  if (_xbt_test_suites) {
    unsigned int it_suite;
    xbt_test_suite_t suite;
    int first = 1;

    /* Run all the suites */
    xbt_dynar_foreach(_xbt_test_suites, it_suite, suite)
      xbt_test_suite_run(suite);

    /* Display some more statistics */
    fprintf(stderr, "\n\n TOTAL: Suites: %.0f%% ok (%d suites: ",
            _xbt_test_nb_suites
            ? ((1 -
                (double) _xbt_test_suite_failed /
                (double) _xbt_test_nb_suites) * 100.0)
            : 100.0, _xbt_test_nb_suites);
    if (_xbt_test_nb_suites != _xbt_test_suite_failed) {
      fprintf(stderr, "%d ok", _xbt_test_nb_suites - _xbt_test_suite_failed);
      first = 0;
    }
    if (_xbt_test_suite_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "),
              _xbt_test_suite_failed);
      first = 0;
    }

    if (_xbt_test_suite_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : ", "),
              _xbt_test_suite_ignore);
      first = 0;
    }
    fprintf(stderr, ")\n        Units:  %.0f%% ok (%d units: ",
            _xbt_test_nb_units
            ? ((1 -
                (double) _xbt_test_unit_failed /
                (double) _xbt_test_nb_units) * 100.0) : 100.0,
            _xbt_test_nb_units);
    first = 1;
    if (_xbt_test_nb_units != _xbt_test_unit_failed) {
      fprintf(stderr, "%s%d ok", (first ? "" : ", "),
              _xbt_test_nb_units - _xbt_test_unit_failed);
      first = 0;
    }
    if (_xbt_test_unit_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "),
              _xbt_test_unit_failed);
      first = 0;
    }
    if (_xbt_test_unit_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : ", "),
              _xbt_test_unit_ignore);
      first = 0;
    }
    fprintf(stderr, ")\n        Tests:  %.0f%% ok (%d tests: ",
            _xbt_test_nb_tests
            ? ((1 -
                (double) _xbt_test_test_failed /
                (double) _xbt_test_nb_tests) * 100.0) : 100.0,
            _xbt_test_nb_tests);
    first = 1;
    if (_xbt_test_nb_tests != _xbt_test_test_failed) {
      fprintf(stderr, "%s%d ok", (first ? "" : ", "),
              _xbt_test_nb_tests - _xbt_test_test_failed);
      first = 0;
    }
    if (_xbt_test_test_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "),
              _xbt_test_test_failed);
      first = 0;
    }
    if (_xbt_test_test_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : ", "),
              _xbt_test_test_ignore);
      first = 0;
    }
    if (_xbt_test_test_expect) {
      fprintf(stderr, "%s%d expected to fail", (first ? "" : ", "),
              _xbt_test_test_expect);
    }

    fprintf(stderr, ")\n");
  } else {
    fprintf(stderr, "No unit to run!\n");
    _xbt_test_unit_failed++;
  }
  return _xbt_test_unit_failed;
}

void xbt_test_exit(void)
{
  xbt_dynar_free(&_xbt_test_suites);
}

/* annotate test case with test */
void _xbt_test_add(const char *file, int line, const char *fmt, ...)
{
  xbt_test_unit_t unit = _xbt_test_current_unit;
  xbt_test_test_t test;
  va_list ap;
  int vres;

  xbt_assert(unit);
  xbt_assert(fmt);

  test = xbt_new0(struct s_xbt_test_test, 1);
  va_start(ap, fmt);
  vres = vasprintf(&test->title, fmt, ap);
  va_end(ap);
  test->failed = 0;
  test->expected_failure = 0;
  test->ignored = 0;
  test->file = file;
  test->line = line;
  test->logs = xbt_dynar_new(sizeof(xbt_test_log_t), xbt_test_log_free);
  xbt_dynar_push(unit->tests, &test);
  return;
}

/* annotate test case with log message and failure */
void _xbt_test_fail(const char *file, int line, const char *fmt, ...)
{
  xbt_test_unit_t unit = _xbt_test_current_unit;
  xbt_test_test_t test;
  xbt_test_log_t log;
  va_list ap;
  int vres;

  xbt_assert(unit);
  xbt_assert(fmt);

  xbt_assert1(xbt_dynar_length(_xbt_test_current_unit->tests),
              "Test failed even before being declared (broken unit: %s)",
              unit->title);

  log = xbt_new(struct s_xbt_test_log, 1);
  va_start(ap, fmt);
  vres = vasprintf(&log->text, fmt, ap);
  va_end(ap);
  log->file = file;
  log->line = line;

  test = xbt_dynar_getlast_as(unit->tests, xbt_test_test_t);
  xbt_dynar_push(test->logs, &log);

  test->failed = 1;
}

void xbt_test_exception(xbt_ex_t e)
{
  _xbt_test_fail(e.file, e.line, "Exception %s raised: %s",
                 xbt_ex_catname(e.category), e.msg);
}

void xbt_test_expect_failure(void)
{
  xbt_test_test_t test;
  xbt_assert1(xbt_dynar_length(_xbt_test_current_unit->tests),
              "Cannot expect the failure of a test before declaring it (broken unit: %s)",
              _xbt_test_current_unit->title);
  test = xbt_dynar_getlast_as(_xbt_test_current_unit->tests, xbt_test_test_t);
  test->expected_failure = 1;
}

void xbt_test_skip(void)
{
  xbt_test_test_t test;

  xbt_assert1(xbt_dynar_length(_xbt_test_current_unit->tests),
              "Test skiped even before being declared (broken unit: %s)",
              _xbt_test_current_unit->title);

  test = xbt_dynar_getlast_as(_xbt_test_current_unit->tests, xbt_test_test_t);
  test->ignored = 1;
}

/* annotate test case with log message only */
void _xbt_test_log(const char *file, int line, const char *fmt, ...)
{
  xbt_test_unit_t unit = _xbt_test_current_unit;
  xbt_test_test_t test;
  xbt_test_log_t log;
  va_list ap;
  int vres;

  xbt_assert(unit);
  xbt_assert(fmt);

  xbt_assert1(xbt_dynar_length(_xbt_test_current_unit->tests),
              "Test logged into even before being declared (broken test unit: %s)",
              unit->title);

  log = xbt_new(struct s_xbt_test_log, 1);
  va_start(ap, fmt);
  vres = vasprintf(&log->text, fmt, ap);
  va_end(ap);
  log->file = file;
  log->line = line;

  test = xbt_dynar_getlast_as(unit->tests, xbt_test_test_t);
  xbt_dynar_push(test->logs, &log);
}




#ifdef SIMGRID_TEST

XBT_TEST_SUITE("cunit", "Testsuite mechanism autotest");

XBT_TEST_UNIT("expect", test_expected_failure, "expected failures")
{
  xbt_test_add0("Skipped test");
  xbt_test_skip();

  xbt_test_add2("%s %s", "EXPECTED", "FAILURE");
  xbt_test_expect_failure();
  xbt_test_log2("%s %s", "Test", "log");
  xbt_test_fail0("EXPECTED FAILURE");
}

#endif /* SIMGRID_TEST */
