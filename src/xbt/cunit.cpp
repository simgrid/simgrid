/* cunit - A little C Unit facility                                         */

/* Copyright (c) 2005-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is partially inspirated from the OSSP ts (Test Suite Library)       */
/* At some point we should use https://github.com/google/googletest instead */

#include "src/internal_config.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <xbt/cunit.h>
#include <xbt/ex.hpp>
#include <xbt/string.hpp>

/* output stream to use everywhere */
static std::ostream& _xbt_test_out = std::cerr;

/* collection of all suites */
static std::vector<xbt_test_suite_t> _xbt_test_suites;
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
  _xbt_test_out << "      log " << this << "(" << file_ << ":" << line_ << ")=" << text_ << "\n";
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
  _xbt_test_out << "    test " << this << "(" << file_ << ":" << line_ << ")=" << title_ << " ("
                << (failed_ ? "failed" : "not failed") << ")\n";
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
  _xbt_test_out << "  UNIT " << name_ << ": " << title_ << " (" << (this->enabled_ ? "enabled" : "disabled") << ")\n";
  if (this->enabled_) {
    for (s_xbt_test_test const& test : this->tests_)
      test.dump();
  }
}

/* test suite */
class s_xbt_test_suite {
public:
  s_xbt_test_suite(std::string name, std::string title) : name_(std::move(name)), title_(std::move(title)) {}
  void dump() const;
  void push(s_xbt_test_unit unit) { units_.emplace_back(std::move(unit)); }
  int run(int verbosity);

  std::string name_;
  std::string title_;
  std::vector<s_xbt_test_unit> units_;

  bool enabled_      = true;
  int nb_tests_      = 0;
  int nb_units_      = 0;
  int test_failed_   = 0;
  int test_ignore_   = 0;
  int test_expect_   = 0;
  int unit_failed_   = 0;
  int unit_ignore_   = 0;
  int unit_disabled_ = 0;
};

/** @brief retrieve a testsuite from name, or create a new one */
xbt_test_suite_t xbt_test_suite_by_name(const char *name, const char *fmt, ...)
{
  auto res = std::find_if(begin(_xbt_test_suites), end(_xbt_test_suites),
                          [&name](xbt_test_suite_t const& suite) { return suite->name_ == name; });
  if (res != end(_xbt_test_suites))
    return *res;

  va_list ap;
  va_start(ap, fmt);
  xbt_test_suite_t suite = new s_xbt_test_suite(name, simgrid::xbt::string_vprintf(fmt, ap));
  va_end(ap);

  _xbt_test_suites.push_back(suite);

  return suite;
}

void s_xbt_test_suite::dump() const
{
  _xbt_test_out << "TESTSUITE " << name_ << ": " << title_ << " (" << (this->enabled_ ? "enabled" : "disabled")
                << ")\n";
  if (this->enabled_) {
    for (s_xbt_test_unit const& unit : this->units_)
      unit.dump();
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
  s_xbt_test_unit unit(name, simgrid::xbt::string_vprintf(fmt, ap), func);
  va_end(ap);
  suite->push(unit);
}

/* run test one suite */
int s_xbt_test_suite::run(int verbosity)
{
  /* suite title pretty-printing */
  int suite_len = this->title_.length();
  xbt_assert(suite_len < 68, "suite title \"%s\" too long (%d should be less than 68", this->title_.c_str(), suite_len);

  std::string suite_title = " ";
  suite_title.resize(40 - (suite_len + 4) / 2, '=');
  suite_title += std::string("[ ") + this->title_ + " ]";
  suite_title.resize(79, '=');
  if (not this->enabled_)
    suite_title.replace(70, std::string::npos, " DISABLED");
  _xbt_test_out << "\n" << suite_title << "\n";

  if (this->enabled_) {
    /* iterate through all tests */
    for (s_xbt_test_unit& unit : this->units_) {
      /* init unit case counters */
      unit.nb_tests_    = 0;
      unit.test_ignore_ = 0;
      unit.test_failed_ = 0;
      unit.test_expect_ = 0;

      /* display unit title */
      std::string cp = std::string(" Unit: ") + unit.title_ + " ";
      cp.resize(70, '.');
      _xbt_test_out << cp;

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
          _xbt_test_out << ".. failed\n";
        } else if (unit.nb_tests_) {
          _xbt_test_out << "...... ok\n"; /* successful, but show about expected */
        } else {
          _xbt_test_out << ".... skip\n"; /* shouldn't happen, but I'm a bit lost with this logic */
        }
        for (s_xbt_test_test const& test : unit.tests_) {
          std::string file = test.file_;
          int line         = test.line_;
          std::string resname;
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
          _xbt_test_out << "      " << resname << ": " << test.title_ << " [" << file << ":" << line << "]\n";

          if ((test.expected_failure_ && not test.failed_) || (not test.expected_failure_ && test.failed_)) {
            for (s_xbt_test_log const& log : test.logs_) {
              file = (log.file_.empty() ? file : log.file_);
              line = (log.line_ == 0 ? line : log.line_);
              _xbt_test_out << "             " << file << ":" << line << ": " << log.text_ << "\n";
            }
          }
        }
        _xbt_test_out << "    Summary: " << unit.test_failed_ << " of " << unit.nb_tests_ << " tests failed";
        if (unit.test_ignore_) {
          _xbt_test_out << " (" << unit.test_ignore_ << " tests ignored)\n";
        } else {
          _xbt_test_out << "\n";
        }
      } else if (not unit.enabled_) {
        _xbt_test_out << " disabled\n"; /* no test were run */
      } else if (unit.nb_tests_) {
        _xbt_test_out << "...... ok\n"; /* successful */
      } else {
        _xbt_test_out << ".... skip\n"; /* no test were run */
      }

      /* Accumulate test counts into the suite */
      this->nb_tests_ += unit.nb_tests_;
      this->test_failed_ += unit.test_failed_;
      this->test_ignore_ += unit.test_ignore_;
      this->test_expect_ += unit.test_expect_;

      _xbt_test_nb_tests += unit.nb_tests_;
      _xbt_test_test_failed += unit.test_failed_;
      _xbt_test_test_ignore += unit.test_ignore_;
      _xbt_test_test_expect += unit.test_expect_;

      /* What's the conclusion of this test anyway? */
      if (unit.nb_tests_) {
        this->nb_units_++;
        if (unit.test_failed_)
          this->unit_failed_++;
      } else if (not unit.enabled_) {
        this->unit_disabled_++;
      } else {
        this->unit_ignore_++;
      }
    }
  }
  _xbt_test_nb_units += this->nb_units_;
  _xbt_test_unit_failed += this->unit_failed_;
  _xbt_test_unit_ignore += this->unit_ignore_;
  _xbt_test_unit_disabled += this->unit_disabled_;

  if (this->nb_units_) {
    _xbt_test_nb_suites++;
    if (this->test_failed_)
      _xbt_test_suite_failed++;
  } else if (not this->enabled_) {
    _xbt_test_suite_disabled++;
  } else {
    _xbt_test_suite_ignore++;
  }

  /* print test suite summary */
  if (this->enabled_) {
    bool first = true; /* for result pretty printing */

    _xbt_test_out << " ====================================================================="
                  << (this->nb_units_ ? (this->unit_failed_ ? "== FAILED" : "====== OK")
                                      : (this->unit_disabled_ ? " DISABLED" : "==== SKIP"))
                  << "\n";
    _xbt_test_out.setf(std::ios::fixed);
    _xbt_test_out.precision(0);
    _xbt_test_out << " Summary: Units: "
                  << (this->nb_units_ ? ((1 - (double)this->unit_failed_ / (double)this->nb_units_) * 100.0) : 100.0)
                  << "% ok (" << this->nb_units_ << " units: ";
    if (this->nb_units_ != this->unit_failed_) {
      _xbt_test_out << (first ? "" : ", ") << (this->nb_units_ - this->unit_failed_) << " ok";
      first = false;
    }
    if (this->unit_failed_) {
      _xbt_test_out << (first ? "" : ", ") << this->unit_failed_ << " failed";
      first = false;
    }
    if (this->unit_ignore_) {
      _xbt_test_out << (first ? "" : ", ") << this->unit_ignore_ << " ignored";
      first = false;
    }
    if (this->unit_disabled_) {
      _xbt_test_out << (first ? "" : ", ") << this->unit_disabled_ << " disabled";
    }
    _xbt_test_out << ")\n          Tests: "
                  << (this->nb_tests_ ? ((1 - (double)this->test_failed_ / (double)this->nb_tests_) * 100.0) : 100.0)
                  << "% ok (" << this->nb_tests_ << " tests: ";
    first = true;
    if (this->nb_tests_ != this->test_failed_) {
      _xbt_test_out << (first ? "" : ", ") << (this->nb_tests_ - this->test_failed_) << " ok";
      first = false;
    }
    if (this->test_failed_) {
      _xbt_test_out << (first ? "" : ", ") << this->test_failed_ << " failed";
      first = false;
    }
    if (this->test_ignore_) {
      _xbt_test_out << (first ? "" : "; ") << this->test_ignore_ << " ignored";
      first = false;
    }
    if (this->test_expect_) {
      _xbt_test_out << (first ? "" : "; ") << this->test_expect_ << " expected to fail";
    }
    _xbt_test_out << ")\n";
  }
  return this->unit_failed_;
}

static void apply_selection(char *selection)
{
  if (not selection || selection[0] == '\0')
    return;

  /* for the parsing */
  std::string sel = selection;
  bool done       = false;
  std::string dir; /* the directive */
  std::string suitename;
  std::string unitname;

  /* First apply the selection */
  size_t p0 = 0;
  while (not done) {
    bool enabling = true;

    size_t p = sel.find(',', p0);
    if (p != std::string::npos) {
      dir = sel.substr(p0, p - p0);
      p0  = p + 1;
    } else {
      dir  = sel.substr(p0);
      done = true;
    }

    if (dir[0] == '-') {
      enabling = false;
      dir.erase(0, 1);
    }
    if (dir[0] == '+') {
      enabling = true;
      dir.erase(0, 1);
    }

    p = dir.find(':');
    if (p != std::string::npos) {
      suitename = dir.substr(0, p);
      unitname  = dir.substr(p + 1);
    } else {
      suitename = dir;
      unitname  = "";
    }

    /* Deal with the specific case of 'all' pseudo serie */
    if (suitename == "all") {
      xbt_assert(unitname.empty(), "The 'all' pseudo-suite does not accept any unit specification\n");

      for (xbt_test_suite_t& suite : _xbt_test_suites) {
        for (s_xbt_test_unit& unit : suite->units_) {
          unit.enabled_ = enabling;
        }
        suite->enabled_ = enabling;
      }
    } else {
      bool suitefound = false;
      for (xbt_test_suite_t& thissuite : _xbt_test_suites) {
        if (suitename == thissuite->name_) {
          /* Do not disable the whole suite when we just want to disable a child */
          if (enabling || unitname.empty())
            thissuite->enabled_ = enabling;

          if (unitname.empty()) {
            for (s_xbt_test_unit& unit : thissuite->units_) {
              unit.enabled_ = enabling;
            }
          } else {              /* act on one child only */
            /* search relevant unit */
            auto unit = std::find_if(begin(thissuite->units_), end(thissuite->units_),
                                     [&unitname](s_xbt_test_unit const& unit) { return unit.name_ == unitname; });
            if (unit == end(thissuite->units_))
              xbt_die("Suite '%s' has no unit of name '%s'. Cannot apply the selection\n", suitename.c_str(),
                      unitname.c_str());
            unit->enabled_ = enabling;
          }                     /* act on childs (either all or one) */
          suitefound = true;
          break;                /* found the relevant serie. We are happy */
        }
      }                         /* search relevant series */
      xbt_assert(suitefound, "No suite of name '%s' found. Cannot apply the selection\n", suitename.c_str());
    }
  }
}

void xbt_test_dump(char *selection)
{
  apply_selection(selection);

  if (not _xbt_test_suites.empty()) {
    for (xbt_test_suite_t suite : _xbt_test_suites)
      suite->dump();
  } else {
    _xbt_test_out << " No suite defined.";
  }
}

int xbt_test_run(char *selection, int verbosity)
{
  apply_selection(selection);

  if (not _xbt_test_suites.empty()) {
    bool first = true;

    /* Run all the suites */
    for (xbt_test_suite_t& suite : _xbt_test_suites)
      if (suite)
        suite->run(verbosity);

    /* Display some more statistics */
    _xbt_test_out.setf(std::ios::fixed);
    _xbt_test_out.precision(0);
    _xbt_test_out << "\n\n TOTAL: Suites: "
                  << (_xbt_test_nb_suites ? ((1 - (double)_xbt_test_suite_failed / (double)_xbt_test_nb_suites) * 100.0)
                                          : 100.0)
                  << "% ok (" << _xbt_test_nb_suites << " suites: ";
    if (_xbt_test_nb_suites != _xbt_test_suite_failed) {
      _xbt_test_out << (_xbt_test_nb_suites - _xbt_test_suite_failed) << " ok";
      first = false;
    }
    if (_xbt_test_suite_failed) {
      _xbt_test_out << (first ? "" : ", ") << _xbt_test_suite_failed << " failed";
      first = false;
    }

    if (_xbt_test_suite_ignore) {
      _xbt_test_out << (first ? "" : ", ") << _xbt_test_suite_ignore << " ignored";
    }
    _xbt_test_out << ")\n        Units:  "
                  << (_xbt_test_nb_units ? ((1 - (double)_xbt_test_unit_failed / (double)_xbt_test_nb_units) * 100.0)
                                         : 100.0)
                  << "% ok (" << _xbt_test_nb_units << " units: ";
    first = true;
    if (_xbt_test_nb_units != _xbt_test_unit_failed) {
      _xbt_test_out << (_xbt_test_nb_units - _xbt_test_unit_failed) << " ok";
      first = false;
    }
    if (_xbt_test_unit_failed) {
      _xbt_test_out << (first ? "" : ", ") << _xbt_test_unit_failed << " failed";
      first = false;
    }
    if (_xbt_test_unit_ignore) {
      _xbt_test_out << (first ? "" : ", ") << _xbt_test_unit_ignore << " ignored";
    }
    _xbt_test_out << ")\n        Tests:  "
                  << (_xbt_test_nb_tests ? ((1 - (double)_xbt_test_test_failed / (double)_xbt_test_nb_tests) * 100.0)
                                         : 100.0)
                  << "% ok (" << _xbt_test_nb_tests << " tests: ";
    first = true;
    if (_xbt_test_nb_tests != _xbt_test_test_failed) {
      _xbt_test_out << (_xbt_test_nb_tests - _xbt_test_test_failed) << " ok";
      first = false;
    }
    if (_xbt_test_test_failed) {
      _xbt_test_out << (first ? "" : ", ") << _xbt_test_test_failed << " failed";
      first = false;
    }
    if (_xbt_test_test_ignore) {
      _xbt_test_out << (first ? "" : ", ") << _xbt_test_test_ignore << " ignored";
      first = false;
    }
    if (_xbt_test_test_expect) {
      _xbt_test_out << (first ? "" : ", ") << _xbt_test_test_expect << " expected to fail";
    }

    _xbt_test_out << ")\n";
  } else {
    _xbt_test_out << "No unit to run!\n";
    _xbt_test_unit_failed++;
  }
  return _xbt_test_unit_failed;
}

void xbt_test_exit()
{
  for (xbt_test_suite_t suite : _xbt_test_suites)
    delete suite;
  _xbt_test_suites.clear();
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
