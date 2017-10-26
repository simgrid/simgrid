/* cunit - A little C Unit facility                                         */

/* Copyright (c) 2005-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is partially inspirated from the OSSP ts (Test Suite Library)       */
/* At some point we should use https://github.com/google/googletest instead */

#include "src/internal_config.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include <xbt/cunit.h>
#include <xbt/ex.hpp>
#include <xbt/string.hpp>

#include "xbt/sysdep.h"         /* bvprintf */
#include "xbt/dynar.h"

#define STRLEN 1024

/* collection of all suites */
static xbt_dynar_t _xbt_test_suites = nullptr;
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
xbt_test_unit_t _xbt_test_current_unit = nullptr;

/* test suite test log */
class s_xbt_test_log {
public:
  s_xbt_test_log(std::string text, std::string file, int line)
      : text_(std::move(text)), file_(std::move(file)), line_(line)
  {
  }
  void dump() const;

  std::string text_;
  std::string file_;
  int line_;
};

void s_xbt_test_log::dump() const
{
  fprintf(stderr, "      log %p(%s:%d)=%s\n", this, this->file_.c_str(), this->line_, this->text_.c_str());
}

/* test suite test check */
class s_xbt_test_test {
public:
  s_xbt_test_test(std::string title, std::string file, int line)
      : title_(std::move(title)), file_(std::move(file)), line_(line)
  {
  }
  void dump() const;

  std::string title_;
  bool failed_           = false;
  bool expected_failure_ = false;
  bool ignored_          = false;
  std::string file_;
  int line_;
  std::vector<s_xbt_test_log> logs_;
};

void s_xbt_test_test::dump() const
{
  fprintf(stderr, "    test %p(%s:%d)=%s (%s)\n", this, this->file_.c_str(), this->line_, this->title_.c_str(),
          this->failed_ ? "failed" : "not failed");
  for (s_xbt_test_log const& log : this->logs_)
    log.dump();
}

/* test suite test unit */
class s_xbt_test_unit {
public:
  s_xbt_test_unit(std::string name, std::string title, ts_test_cb_t func)
      : name_(std::move(name)), title_(std::move(title)), func_(func)
  {
  }
  void dump() const;

  std::string name_;
  std::string title_;
  ts_test_cb_t func_;
  std::vector<s_xbt_test_test> tests_;

  bool enabled_    = true;
  int nb_tests_    = 0;
  int test_failed_ = 0;
  int test_ignore_ = 0;
  int test_expect_ = 0;
};

void s_xbt_test_unit::dump() const
{
  fprintf(stderr, "  UNIT %s: %s (%s)\n", this->name_.c_str(), this->title_.c_str(),
          (this->enabled_ ? "enabled" : "disabled"));
  if (this->enabled_) {
    for (s_xbt_test_test const& test : this->tests_)
      test.dump();
  }
}

/* test suite */
struct s_xbt_test_suite {
  int enabled;
  const char *name;
  char *title;
  std::vector<s_xbt_test_unit> units;

  int nb_tests;
  int nb_units;
  int test_failed;
  int test_ignore;
  int test_expect;
  int unit_failed;
  int unit_ignore;
  int unit_disabled;
};

/* destroy test suite */
static void xbt_test_suite_free(void *s)
{
  xbt_test_suite_t suite = *(xbt_test_suite_t *) s;

  if (suite == nullptr)
    return;
  xbt_free(suite->title);
  delete suite;
}

/** @brief retrieve a testsuite from name, or create a new one */
xbt_test_suite_t xbt_test_suite_by_name(const char *name, const char *fmt, ...)
{
  if (_xbt_test_suites == nullptr) {
    _xbt_test_suites = xbt_dynar_new(sizeof(xbt_test_suite_t), xbt_test_suite_free);
  } else {
    xbt_test_suite_t suite;
    unsigned int it_suite;
    xbt_dynar_foreach(_xbt_test_suites, it_suite, suite)
      if (not strcmp(suite->name, name))
        return suite;
  }

  xbt_test_suite_t suite = new s_xbt_test_suite{};
  va_list ap;
  va_start(ap, fmt);
  suite->title = bvprintf(fmt, ap);
  va_end(ap);
  suite->name    = name;
  suite->enabled = 1;

  xbt_dynar_push(_xbt_test_suites, &suite);

  return suite;
}

static void xbt_test_suite_dump(xbt_test_suite_t suite)
{
  if (suite) {
    fprintf(stderr, "TESTSUITE %s: %s (%s)\n", suite->name, suite->title, suite->enabled ? "enabled" : "disabled");
    if (suite->enabled) {
      for (s_xbt_test_unit const& unit : suite->units)
        unit.dump();
    }
  } else {
    fprintf(stderr, "TESTSUITE IS NULL!\n");
  }
}

/* add test case to test suite */
void xbt_test_suite_push(xbt_test_suite_t suite, const char *name, ts_test_cb_t func, const char *fmt, ...)
{
  xbt_assert(suite);
  xbt_assert(func);
  xbt_assert(fmt);

  va_list ap;
  va_start(ap, fmt);
  suite->units.emplace_back(name, simgrid::xbt::string_vprintf(fmt, ap), func);
  va_end(ap);
}

/* run test one suite */
static int xbt_test_suite_run(xbt_test_suite_t suite, int verbosity)
{
  if (suite == nullptr)
    return 0;

  /* suite title pretty-printing */
  char suite_title[81];
  int suite_len = strlen(suite->title);

  xbt_assert(suite_len < 68, "suite title \"%s\" too long (%d should be less than 68", suite->title, suite_len);

  suite_title[0] = ' ';
  for (int i = 1; i < 79; i++)
    suite_title[i] = '=';
  suite_title[79]  = '\n';
  suite_title[80]  = '\0';

  snprintf(suite_title + 40 - (suite_len + 4) / 2, 81 - (40 - (suite_len + 4) / 2), "[ %s ]", suite->title);
  suite_title[40 + (suite_len + 5) / 2] = '=';
  if (not suite->enabled)
    snprintf(suite_title + 70, 11, " DISABLED ");
  fprintf(stderr, "\n%s\n", suite_title);

  if (suite->enabled) {
    /* iterate through all tests */
    for (s_xbt_test_unit& unit : suite->units) {
      /* init unit case counters */
      unit.nb_tests_    = 0;
      unit.test_ignore_ = 0;
      unit.test_failed_ = 0;
      unit.test_expect_ = 0;

      /* display unit title */
      char* cp = bprintf(" Unit: %s ......................................"
                         "......................................",
                         unit.title_.c_str());
      cp[70] = '\0';
      fprintf(stderr, "%s", cp);
      free(cp);

      /* run the test case function */
      _xbt_test_current_unit = &unit;
      if (unit.enabled_)
        unit.func_();

      /* iterate through all performed tests to determine status */
      for (s_xbt_test_test const& test : unit.tests_) {
        if (test.ignored_) {
          unit.test_ignore_++;
        } else {
          unit.nb_tests_++;

          if ((test.failed_ && not test.expected_failure_) || (not test.failed_ && test.expected_failure_))
            unit.test_failed_++;
          if (test.expected_failure_)
            unit.test_expect_++;
        }
      }
      /* Display whether this unit went well */
      if (unit.test_failed_ > 0 || unit.test_expect_ || (verbosity && unit.nb_tests_ > 0)) {
        /* some tests failed (or were supposed to), so do detailed reporting of test case */
        if (unit.test_failed_ > 0) {
          fprintf(stderr, ".. failed\n");
        } else if (unit.nb_tests_) {
          fprintf(stderr, "...... ok\n");       /* successful, but show about expected */
        } else {
          fprintf(stderr, ".... skip\n");       /* shouldn't happen, but I'm a bit lost with this logic */
        }
        for (s_xbt_test_test const& test : unit.tests_) {
          std::string file = test.file_;
          int line         = test.line_;
          const char* resname;
          if (test.ignored_)
            resname = " SKIP";
          else if (test.expected_failure_) {
            if (test.failed_)
              resname = "EFAIL";
            else
              resname = "EPASS";
          } else {
            if (test.failed_)
              resname = " FAIL";
            else
              resname = " PASS";
          }
          fprintf(stderr, "      %s: %s [%s:%d]\n", resname, test.title_.c_str(), file.c_str(), line);

          if ((test.expected_failure_ && not test.failed_) || (not test.expected_failure_ && test.failed_)) {
            for (s_xbt_test_log const& log : test.logs_) {
              file = (log.file_.empty() ? file : log.file_);
              line = (log.line_ == 0 ? line : log.line_);
              fprintf(stderr, "             %s:%d: %s\n", file.c_str(), line, log.text_.c_str());
            }
          }
        }
        fprintf(stderr, "    Summary: %d of %d tests failed", unit.test_failed_, unit.nb_tests_);
        if (unit.test_ignore_) {
          fprintf(stderr, " (%d tests ignored)\n", unit.test_ignore_);
        } else {
          fprintf(stderr, "\n");
        }
      } else if (not unit.enabled_) {
        fprintf(stderr, " disabled\n"); /* no test were run */
      } else if (unit.nb_tests_) {
        fprintf(stderr, "...... ok\n"); /* successful */
      } else {
        fprintf(stderr, ".... skip\n"); /* no test were run */
      }

      /* Accumulate test counts into the suite */
      suite->nb_tests += unit.nb_tests_;
      suite->test_failed += unit.test_failed_;
      suite->test_ignore += unit.test_ignore_;
      suite->test_expect += unit.test_expect_;

      _xbt_test_nb_tests += unit.nb_tests_;
      _xbt_test_test_failed += unit.test_failed_;
      _xbt_test_test_ignore += unit.test_ignore_;
      _xbt_test_test_expect += unit.test_expect_;

      /* What's the conclusion of this test anyway? */
      if (unit.nb_tests_) {
        suite->nb_units++;
        if (unit.test_failed_)
          suite->unit_failed++;
      } else if (not unit.enabled_) {
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
  } else if (not suite->enabled) {
    _xbt_test_suite_disabled++;
  } else {
    _xbt_test_suite_ignore++;
  }

  /* print test suite summary */
  if (suite->enabled) {
    int first = 1; /* for result pretty printing */

    fprintf(stderr," =====================================================================%s\n",
            (suite->nb_units ? (suite->unit_failed ? "== FAILED" : "====== OK") :
                               (suite->unit_disabled ? " DISABLED" : "==== SKIP")));
    fprintf(stderr, " Summary: Units: %.0f%% ok (%d units: ", suite->nb_units
            ? ((1 - (double) suite->unit_failed / (double) suite->nb_units) * 100.0) : 100.0, suite->nb_units);

    if (suite->nb_units != suite->unit_failed) {
      fprintf(stderr, "%s%d ok", (first ? "" : ", "), suite->nb_units - suite->unit_failed);
      first = 0;
    }
    if (suite->unit_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "), suite->unit_failed);
      first = 0;
    }
    if (suite->unit_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : ", "), suite->unit_ignore);
      first = 0;
    }
    if (suite->unit_disabled) {
      fprintf(stderr, "%s%d disabled", (first ? "" : ", "), suite->unit_disabled);
    }
    fprintf(stderr, ")\n          Tests: %.0f%% ok (%d tests: ", suite->nb_tests
            ? ((1 - (double) suite->test_failed / (double) suite->nb_tests) * 100.0) : 100.0, suite->nb_tests);

    first = 1;
    if (suite->nb_tests != suite->test_failed) {
      fprintf(stderr, "%s%d ok", (first ? "" : ", "), suite->nb_tests - suite->test_failed);
      first = 0;
    }
    if (suite->test_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "), suite->test_failed);
      first = 0;
    }
    if (suite->test_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : "; "), suite->test_ignore);
      first = 0;
    }
    if (suite->test_expect) {
      fprintf(stderr, "%s%d expected to fail", (first ? "" : "; "), suite->test_expect);
    }
    fprintf(stderr, ")\n");
  }
  return suite->unit_failed;
}

static void apply_selection(char *selection)
{
  /* for the parsing */
  char *sel = selection;
  int done = 0;
  char dir[STRLEN]; /* the directive */
  /* iterators */
  unsigned int it_suite;
  xbt_test_suite_t suite;

  char suitename[STRLEN];
  char unitname[STRLEN];

  if (not selection || selection[0] == '\0')
    return;

  /* First apply the selection */
  while (not done) {
    int enabling = 1;

    char *p = strchr(sel, ',');
    if (p) {
      snprintf(dir, STRLEN, "%.*s", (int)(p - sel), sel);
      sel = p + 1;
    } else {
      snprintf(dir, STRLEN, "%s", sel);
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
      snprintf(suitename, STRLEN, "%.*s", (int)(p - dir), dir);
      snprintf(unitname, STRLEN, "%s", p + 1);
    } else {
      snprintf(suitename, STRLEN, "%s", dir);
      unitname[0] = '\0';
    }

    /* Deal with the specific case of 'all' pseudo serie */
    if (not strcmp("all", suitename)) {
      xbt_assert(unitname[0] == '\0', "The 'all' pseudo-suite does not accept any unit specification\n");

      xbt_dynar_foreach(_xbt_test_suites, it_suite, suite) {
        for (s_xbt_test_unit& unit : suite->units) {
          unit.enabled_ = enabling;
        }
        suite->enabled = enabling;
      }
    } else {
      unsigned int it;
      for (it = 0; it < xbt_dynar_length(_xbt_test_suites); it++) {
        xbt_test_suite_t thissuite =
            xbt_dynar_get_as(_xbt_test_suites, it, xbt_test_suite_t);
        if (not strcmp(suitename, thissuite->name)) {
          /* Do not disable the whole suite when we just want to disable a child */
          if (enabling || (unitname[0] == '\0'))
            thissuite->enabled = enabling;

          if (unitname[0] == '\0') {
            for (s_xbt_test_unit& unit : thissuite->units) {
              unit.enabled_ = enabling;
            }
          } else {              /* act on one child only */
            /* search relevant unit */
            auto unit = std::find_if(begin(thissuite->units), end(thissuite->units),
                                     [&unitname](s_xbt_test_unit const& unit) { return unit.name_ == unitname; });
            if (unit == end(thissuite->units))
              xbt_die("Suite '%s' has no unit of name '%s'. Cannot apply the selection\n", suitename, unitname);
            unit->enabled_ = enabling;
          }                     /* act on childs (either all or one) */

          break;                /* found the relevant serie. We are happy */
        }
      }                         /* search relevant series */
      xbt_assert(it != xbt_dynar_length(_xbt_test_suites),
                 "No suite of name '%s' found. Cannot apply the selection\n", suitename);
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

int xbt_test_run(char *selection, int verbosity)
{
  apply_selection(selection);

  if (_xbt_test_suites) {
    unsigned int it_suite;
    xbt_test_suite_t suite;
    int first = 1;

    /* Run all the suites */
    xbt_dynar_foreach(_xbt_test_suites, it_suite, suite)
      xbt_test_suite_run(suite, verbosity);

    /* Display some more statistics */
    fprintf(stderr, "\n\n TOTAL: Suites: %.0f%% ok (%d suites: ",_xbt_test_nb_suites
            ? ((1 - (double) _xbt_test_suite_failed / (double) _xbt_test_nb_suites) * 100.0)
            : 100.0, _xbt_test_nb_suites);
    if (_xbt_test_nb_suites != _xbt_test_suite_failed) {
      fprintf(stderr, "%d ok", _xbt_test_nb_suites - _xbt_test_suite_failed);
      first = 0;
    }
    if (_xbt_test_suite_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "), _xbt_test_suite_failed);
      first = 0;
    }

    if (_xbt_test_suite_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : ", "), _xbt_test_suite_ignore);
    }
    fprintf(stderr, ")\n        Units:  %.0f%% ok (%d units: ", _xbt_test_nb_units
            ? ((1 - (double) _xbt_test_unit_failed / (double) _xbt_test_nb_units) * 100.0) : 100.0, _xbt_test_nb_units);
    first = 1;
    if (_xbt_test_nb_units != _xbt_test_unit_failed) {
      fprintf(stderr, "%d ok", _xbt_test_nb_units - _xbt_test_unit_failed);
      first = 0;
    }
    if (_xbt_test_unit_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "), _xbt_test_unit_failed);
      first = 0;
    }
    if (_xbt_test_unit_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : ", "), _xbt_test_unit_ignore);
    }
    fprintf(stderr, ")\n        Tests:  %.0f%% ok (%d tests: ", _xbt_test_nb_tests
            ? ((1 - (double) _xbt_test_test_failed / (double) _xbt_test_nb_tests) * 100.0) : 100.0, _xbt_test_nb_tests);
    first = 1;
    if (_xbt_test_nb_tests != _xbt_test_test_failed) {
      fprintf(stderr, "%d ok", _xbt_test_nb_tests - _xbt_test_test_failed);
      first = 0;
    }
    if (_xbt_test_test_failed) {
      fprintf(stderr, "%s%d failed", (first ? "" : ", "), _xbt_test_test_failed);
      first = 0;
    }
    if (_xbt_test_test_ignore) {
      fprintf(stderr, "%s%d ignored", (first ? "" : ", "), _xbt_test_test_ignore);
      first = 0;
    }
    if (_xbt_test_test_expect) {
      fprintf(stderr, "%s%d expected to fail", (first ? "" : ", "), _xbt_test_test_expect);
    }

    fprintf(stderr, ")\n");
  } else {
    fprintf(stderr, "No unit to run!\n");
    _xbt_test_unit_failed++;
  }
  return _xbt_test_unit_failed;
}

void xbt_test_exit()
{
  xbt_dynar_free(&_xbt_test_suites);
}

/* annotate test case with test */
void _xbt_test_add(const char *file, int line, const char *fmt, ...)
{
  xbt_test_unit_t unit = _xbt_test_current_unit;
  xbt_assert(unit);

  va_list ap;
  va_start(ap, fmt);
  unit->tests_.emplace_back(simgrid::xbt::string_vprintf(fmt, ap), file, line);
  va_end(ap);
}

/* annotate test case with log message and failure */
void _xbt_test_fail(const char *file, int line, const char *fmt, ...)
{
  xbt_test_unit_t unit = _xbt_test_current_unit;
  xbt_assert(unit);
  xbt_assert(not _xbt_test_current_unit->tests_.empty(), "Test failed even before being declared (broken unit: %s)",
             unit->title_.c_str());

  s_xbt_test_test& test = unit->tests_.back();
  va_list ap;
  va_start(ap, fmt);
  test.logs_.emplace_back(simgrid::xbt::string_vprintf(fmt, ap), file, line);
  va_end(ap);

  test.failed_ = true;
}

void xbt_test_exception(xbt_ex_t e)
{
  _xbt_test_fail(e.throwPoint().file, e.throwPoint().line, "Exception %s raised: %s", xbt_ex_catname(e.category), e.what());
}

void xbt_test_expect_failure()
{
  xbt_assert(not _xbt_test_current_unit->tests_.empty(),
             "Cannot expect the failure of a test before declaring it (broken unit: %s)",
             _xbt_test_current_unit->title_.c_str());
  _xbt_test_current_unit->tests_.back().expected_failure_ = true;
}

void xbt_test_skip()
{
  xbt_assert(not _xbt_test_current_unit->tests_.empty(), "Test skipped even before being declared (broken unit: %s)",
             _xbt_test_current_unit->title_.c_str());
  _xbt_test_current_unit->tests_.back().ignored_ = true;
}

/* annotate test case with log message only */
void _xbt_test_log(const char *file, int line, const char *fmt, ...)
{
  xbt_test_unit_t unit = _xbt_test_current_unit;
  xbt_assert(unit);
  xbt_assert(not _xbt_test_current_unit->tests_.empty(),
             "Test logged into even before being declared (broken test unit: %s)", unit->title_.c_str());

  va_list ap;
  va_start(ap, fmt);
  unit->tests_.back().logs_.emplace_back(simgrid::xbt::string_vprintf(fmt, ap), file, line);
  va_end(ap);
}

#ifdef SIMGRID_TEST
XBT_TEST_SUITE("cunit", "Testsuite mechanism autotest");

XBT_TEST_UNIT("expect", test_expected_failure, "expected failures")
{
  xbt_test_add("Skipped test");
  xbt_test_skip();

  xbt_test_add("%s %s", "EXPECTED", "FAILURE");
  xbt_test_expect_failure();
  xbt_test_log("%s %s", "Test", "log");
  xbt_test_fail("EXPECTED FAILURE");
}
#endif                          /* SIMGRID_TEST */
