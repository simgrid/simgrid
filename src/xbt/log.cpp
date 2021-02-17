/* log - a generic logging facility in the spirit of log4j                  */

/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/xbt/log_private.hpp"
#include "src/xbt_modinter.h"
#include "xbt/asserts.h"
#include "xbt/dynar.h"
#include "xbt/str.h"
#include "xbt/string.hpp"

#include <algorithm>
#include <array>
#include <mutex>
#include <string>
#include <vector>

int xbt_log_no_loc = 0; /* if set to true (with --log=no_loc), file localization will be omitted (for tesh tests) */
static std::recursive_mutex* log_cat_init_mutex = nullptr;

xbt_log_appender_t xbt_log_default_appender = nullptr; /* set in log_init */
xbt_log_layout_t xbt_log_default_layout     = nullptr; /* set in log_init */

struct xbt_log_setting_t {
  std::string catname;
  std::string fmt;
  e_xbt_log_priority_t thresh = xbt_log_priority_uninitialized;
  int additivity              = -1;
  xbt_log_appender_t appender = nullptr;
};

// This function is here to avoid static initialization order fiasco
static auto& xbt_log_settings()
{
  static std::vector<xbt_log_setting_t> value;
  return value;
}

constexpr std::array<const char*, 8> xbt_log_priority_names{
    {"NONE", "TRACE", "DEBUG", "VERBOSE", "INFO", "WARNING", "ERROR", "CRITICAL"}};

s_xbt_log_category_t _XBT_LOGV(XBT_LOG_ROOT_CAT) = {
    nullptr /*parent */,
    nullptr /* firstChild */,
    nullptr /* nextSibling */,
    "root",
    "The common ancestor for all categories",
    0 /*initialized */,
    xbt_log_priority_uninitialized /* threshold */,
    0 /* isThreshInherited */,
    nullptr /* appender */,
    nullptr /* layout */,
    0 /* additivity */
};

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(log, xbt, "Loggings from the logging mechanism itself");

/* create the default appender and install it in the root category,
   which were already created (damnit. Too slow little beetle) */
void xbt_log_preinit(void)
{
  xbt_log_default_appender             = xbt_log_appender_stream(stderr);
  xbt_log_default_layout               = xbt_log_layout_simple_new(nullptr);
  _XBT_LOGV(XBT_LOG_ROOT_CAT).appender = xbt_log_default_appender;
  _XBT_LOGV(XBT_LOG_ROOT_CAT).layout = xbt_log_default_layout;
  log_cat_init_mutex                   = new std::recursive_mutex();
}

static void xbt_log_help();
static void xbt_log_help_categories();

void xbt_log_init(int *argc, char **argv)
{
  unsigned help_requested = 0;  /* 1: logs; 2: categories */
  int j                   = 1;
  int parse_args          = 1; // Stop parsing the parameters once we found '--'

  xbt_log_control_set("xbt_help.app:stdout xbt_help.threshold:VERBOSE xbt_help.fmt:%m%n");

  /* Set logs and init log submodule */
  for (int i = 1; i < *argc; i++) {
    if (strcmp("--", argv[i]) == 0) {
      parse_args = 0;
      argv[j++]  = argv[i]; // Keep the '--' for sg_config
    } else if (parse_args && strncmp(argv[i], "--log=", strlen("--log=")) == 0) {
      char* opt = strchr(argv[i], '=');
      opt++;
      xbt_log_control_set(opt);
      XBT_DEBUG("Did apply '%s' as log setting", opt);
    } else if (parse_args && strcmp(argv[i], "--help-logs") == 0) {
      help_requested |= 1U;
    } else if (parse_args && strcmp(argv[i], "--help-log-categories") == 0) {
      help_requested |= 2U;
    } else {
      argv[j++] = argv[i];
    }
  }
  if (j < *argc) {
    argv[j] = nullptr;
    *argc = j;
  }

  if (help_requested) {
    if (help_requested & 1)
      xbt_log_help();
    if (help_requested & 2)
      xbt_log_help_categories();
    exit(0);
  }
}

static void log_cat_exit(const s_xbt_log_category_t* cat)
{
  if (cat->appender) {
    if (cat->appender->free_)
      cat->appender->free_(cat->appender);
    xbt_free(cat->appender);
  }
  if (cat->layout) {
    if (cat->layout->free_)
      cat->layout->free_(cat->layout);
    xbt_free(cat->layout);
  }

  for (auto const* child = cat->firstChild; child != nullptr; child = child->nextSibling)
    log_cat_exit(child);
}

void xbt_log_postexit(void)
{
  XBT_VERB("Exiting log");
  delete log_cat_init_mutex;
  log_cat_exit(&_XBT_LOGV(XBT_LOG_ROOT_CAT));
}

/* Size of the static string in which we build the log string */
static constexpr size_t XBT_LOG_STATIC_BUFFER_SIZE = 2048;
/* Minimum size of the dynamic string in which we build the log string
   (should be greater than XBT_LOG_STATIC_BUFFER_SIZE) */
static constexpr size_t XBT_LOG_DYNAMIC_BUFFER_SIZE = 4096;

void _xbt_log_event_log(xbt_log_event_t ev, const char *fmt, ...)
{
  const xbt_log_category_s* cat = ev->cat;

  xbt_assert(ev->priority >= 0, "Negative logging priority naturally forbidden");
  xbt_assert(static_cast<size_t>(ev->priority) < xbt_log_priority_names.size(),
             "Priority %d is greater than the biggest allowed value", ev->priority);

  while (true) {
    const s_xbt_log_appender_t* appender = cat->appender;

    if (appender != nullptr) {
      xbt_assert(cat->layout, "No valid layout for the appender of category %s", cat->name);

      /* First, try with a static buffer */
      bool done = false;
      std::array<char, XBT_LOG_STATIC_BUFFER_SIZE> buff;
      ev->buffer      = buff.data();
      ev->buffer_size = buff.size();
      va_start(ev->ap, fmt);
      done = cat->layout->do_layout(cat->layout, ev, fmt);
      va_end(ev->ap);
      ev->buffer = nullptr; // Calm down, static analyzers, this pointer to local array won't leak out of the scope.
      if (done) {
        appender->do_append(appender, buff.data());
      } else {
        /* The static buffer was too small, use a dynamically expanded one */
        ev->buffer_size = XBT_LOG_DYNAMIC_BUFFER_SIZE;
        ev->buffer      = static_cast<char*>(xbt_malloc(ev->buffer_size));
        while (true) {
          va_start(ev->ap, fmt);
          done = cat->layout->do_layout(cat->layout, ev, fmt);
          va_end(ev->ap);
          if (done)
            break; /* Got it */
          ev->buffer_size *= 2;
          ev->buffer = static_cast<char*>(xbt_realloc(ev->buffer, ev->buffer_size));
        }
        appender->do_append(appender, ev->buffer);
        xbt_free(ev->buffer);
      }
    }

    if (!cat->additivity)
      break;
    cat = cat->parent;
  }
}

/* NOTE:
 *
 * The standard logging macros use _XBT_LOG_ISENABLED, which calls _xbt_log_cat_init().  Thus, if we want to avoid an
 * infinite recursion, we can not use the standard logging macros in _xbt_log_cat_init(), and in all functions called
 * from it.
 *
 * To circumvent the problem, we define the macro DISABLE_XBT_LOG_CAT_INIT() to hide the real _xbt_log_cat_init(). The
 * macro has to be called at the beginning of the affected functions.
 */
static int fake_xbt_log_cat_init(xbt_log_category_t, e_xbt_log_priority_t)
{
  return 0;
}
#define DISABLE_XBT_LOG_CAT_INIT()                                                                                     \
  int (*_xbt_log_cat_init)(xbt_log_category_t, e_xbt_log_priority_t) XBT_ATTRIB_UNUSED = fake_xbt_log_cat_init

static void _xbt_log_cat_apply_set(xbt_log_category_t category, const xbt_log_setting_t& setting)
{
  DISABLE_XBT_LOG_CAT_INIT();
  if (setting.thresh != xbt_log_priority_uninitialized) {
    xbt_log_threshold_set(category, setting.thresh);

    XBT_DEBUG("Apply settings for category '%s': set threshold to %s (=%d)",
           category->name, xbt_log_priority_names[category->threshold], category->threshold);
  }

  if (not setting.fmt.empty()) {
    xbt_log_layout_set(category, xbt_log_layout_format_new(setting.fmt.c_str()));

    XBT_DEBUG("Apply settings for category '%s': set format to %s", category->name, setting.fmt.c_str());
  }

  if (setting.additivity != -1) {
    xbt_log_additivity_set(category, setting.additivity);

    XBT_DEBUG("Apply settings for category '%s': set additivity to %s", category->name,
              (setting.additivity ? "on" : "off"));
  }
  if (setting.appender) {
    xbt_log_appender_set(category, setting.appender);
    if (!category->layout)
      xbt_log_layout_set(category, xbt_log_layout_simple_new(nullptr));
    category->additivity = 0;
    XBT_DEBUG("Set %p as appender of category '%s'", setting.appender, category->name);
  }
}

/*
 * This gets called the first time a category is referenced and performs the initialization.
 * Also resets threshold to inherited!
 */
int _xbt_log_cat_init(xbt_log_category_t category, e_xbt_log_priority_t priority)
{
  DISABLE_XBT_LOG_CAT_INIT();
  if (category->initialized)
    return priority >= category->threshold;

  if (log_cat_init_mutex != nullptr)
    log_cat_init_mutex->lock();

  XBT_DEBUG("Initializing category '%s' (firstChild=%s, nextSibling=%s)", category->name,
         (category->firstChild ? category->firstChild->name : "none"),
         (category->nextSibling ? category->nextSibling->name : "none"));

  if (category == &_XBT_LOGV(XBT_LOG_ROOT_CAT)) {
    category->threshold = xbt_log_priority_info;
    category->appender = xbt_log_default_appender;
    category->layout = xbt_log_default_layout;
  } else {
    if (!category->parent)
      category->parent = &_XBT_LOGV(XBT_LOG_ROOT_CAT);

    XBT_DEBUG("Set %s (%s) as father of %s ", category->parent->name,
           (category->parent->initialized ? xbt_log_priority_names[category->parent->threshold] : "uninited"),
           category->name);
    xbt_log_parent_set(category, category->parent);

    if (XBT_LOG_ISENABLED(log, xbt_log_priority_debug)) {
      std::string res;
      const xbt_log_category_s* cpp = category->parent->firstChild;
      while (cpp) {
        res += std::string(" ") + cpp->name;
        cpp = cpp->nextSibling;
      }

      XBT_DEBUG("Children of %s:%s; nextSibling: %s", category->parent->name, res.c_str(),
                (category->parent->nextSibling ? category->parent->nextSibling->name : "none"));
    }
  }

  /* Apply the control */
  auto iset = std::find_if(begin(xbt_log_settings()), end(xbt_log_settings()),
                           [category](const xbt_log_setting_t& s) { return s.catname == category->name; });
  if (iset != xbt_log_settings().end()) {
    _xbt_log_cat_apply_set(category, *iset);
    xbt_log_settings().erase(iset);
  } else {
    XBT_DEBUG("Category '%s': inherited threshold = %s (=%d)", category->name,
              xbt_log_priority_names[category->threshold], category->threshold);
  }

  category->initialized = 1;
  if (log_cat_init_mutex != nullptr)
    log_cat_init_mutex->unlock();
  return priority >= category->threshold;
}

void xbt_log_parent_set(xbt_log_category_t cat, xbt_log_category_t parent)
{
  xbt_assert(cat, "NULL category to be given a parent");
  xbt_assert(parent, "The parent category of %s is NULL", cat->name);

  /* if the category is initialized, unlink from current parent */
  if (cat->initialized) {
    xbt_log_category_t *cpp = &cat->parent->firstChild;

    while (*cpp != cat && *cpp != nullptr) {
      cpp = &(*cpp)->nextSibling;
    }

    xbt_assert(*cpp == cat);
    *cpp = cat->nextSibling;
  }

  cat->parent = parent;
  cat->nextSibling = parent->firstChild;

  parent->firstChild = cat;

  if (!parent->initialized)
    _xbt_log_cat_init(parent, xbt_log_priority_uninitialized /* ignored */ );

  cat->threshold = parent->threshold;

  cat->isThreshInherited = 1;
}

static void _set_inherited_thresholds(const s_xbt_log_category_t* cat)
{
  xbt_log_category_t child = cat->firstChild;

  for (; child != nullptr; child = child->nextSibling) {
    if (child->isThreshInherited) {
      if (cat != &_XBT_LOGV(log))
        XBT_VERB("Set category threshold of %s to %s (=%d)",
              child->name, xbt_log_priority_names[cat->threshold], cat->threshold);
      child->threshold = cat->threshold;
      _set_inherited_thresholds(child);
    }
  }
}

void xbt_log_threshold_set(xbt_log_category_t cat, e_xbt_log_priority_t threshold)
{
  cat->threshold = threshold;
  cat->isThreshInherited = 0;

  _set_inherited_thresholds(cat);
}

static xbt_log_setting_t _xbt_log_parse_setting(const char *control_string)
{
  const char *orig_control_string = control_string;
  xbt_log_setting_t set;

  if (!*control_string)
    return set;
  XBT_DEBUG("Parse log setting '%s'", control_string);

  control_string += strspn(control_string, " ");
  const char* name = control_string;
  control_string += strcspn(control_string, ".:= ");
  const char* option = control_string;
  control_string += strcspn(control_string, ":= ");
  const char* value = control_string;

  xbt_assert(*option == '.' && (*value == '=' || *value == ':'), "Invalid control string '%s'", orig_control_string);

  size_t name_len = option - name;
  ++option;
  size_t option_len = value - option;
  ++value;

  if (strncmp(option, "threshold", option_len) == 0) {
    XBT_DEBUG("New priority name = %s", value);
    int i;
    for (i = 0; i < xbt_log_priority_infinite; i++) {
      if (strcasecmp(value, xbt_log_priority_names[i]) == 0) {
        XBT_DEBUG("This is priority %d", i);
        break;
      }
    }

    if(i<XBT_LOG_STATIC_THRESHOLD){
      fprintf(stderr, "Priority '%s' (in setting '%s') is above allowed priority '%s'.\n\n"
                      "Compiling SimGrid with -DNDEBUG forbids the levels 'trace' and 'debug'\n"
                      "while -DNLOG forbids any logging, at any level.",
              value, name, xbt_log_priority_names[XBT_LOG_STATIC_THRESHOLD]);
      exit(1);
    }else if (i < xbt_log_priority_infinite) {
      set.thresh = (e_xbt_log_priority_t)i;
    } else {
      throw std::invalid_argument(simgrid::xbt::string_printf(
          "Unknown priority name: %s (must be one of: trace,debug,verbose,info,warning,error,critical)", value));
    }
  } else if (strncmp(option, "additivity", option_len) == 0) {
    set.additivity = (strcasecmp(value, "ON") == 0 || strcasecmp(value, "YES") == 0 || strcmp(value, "1") == 0);
  } else if (strncmp(option, "appender", option_len) == 0) {
    if (strncmp(value, "file:", 5) == 0) {
      set.appender = xbt_log_appender_file_new(value + 5);
    } else if (strncmp(value, "rollfile:", 9) == 0) {
      set.appender = xbt_log_appender2_file_new(value + 9, 1);
    } else if (strncmp(value, "splitfile:", 10) == 0) {
      set.appender = xbt_log_appender2_file_new(value + 10, 0);
    } else if (strcmp(value, "stderr") == 0) {
      set.appender = xbt_log_appender_stream(stderr);
    } else if (strcmp(value, "stdout") == 0) {
      set.appender = xbt_log_appender_stream(stdout);
    } else {
      throw std::invalid_argument(simgrid::xbt::string_printf("Unknown appender log type: '%s'", value));
    }
  } else if (strncmp(option, "fmt", option_len) == 0) {
    set.fmt = std::string(value);
  } else {
    xbt_die("Unknown setting of the log category: '%.*s'", static_cast<int>(option_len), option);
  }
  set.catname = std::string(name, name_len);

  XBT_DEBUG("This is for cat '%s'", set.catname.c_str());

  return set;
}

static xbt_log_category_t _xbt_log_cat_searchsub(xbt_log_category_t cat, const char* name)
{
  XBT_DEBUG("Search '%s' into '%s' (firstChild='%s'; nextSibling='%s')", name,
         cat->name, (cat->firstChild ? cat->firstChild->name : "none"),
         (cat->nextSibling ? cat->nextSibling->name : "none"));
  if (strcmp(cat->name, name) == 0)
    return cat;

  for (xbt_log_category_t child = cat->firstChild; child != nullptr; child = child->nextSibling) {
    XBT_DEBUG("Dig into %s", child->name);
    xbt_log_category_t res = _xbt_log_cat_searchsub(child, name);
    if (res)
      return res;
  }

  return nullptr;
}

void xbt_log_control_set(const char *control_string)
{
  /* To split the string in commands, and the cursors */
  xbt_dynar_t set_strings;
  char *str;
  unsigned int cpt;

  if (!control_string)
    return;
  XBT_DEBUG("Parse log settings '%s'", control_string);

  /* Special handling of no_loc request, which asks for any file localization to be omitted (for tesh runs) */
  if (strcmp(control_string, "no_loc") == 0) {
    xbt_log_no_loc = 1;
    return;
  }
  /* split the string, and remove empty entries */
  set_strings = xbt_str_split_quoted(control_string);

  if (xbt_dynar_is_empty(set_strings)) {     /* vicious user! */
    xbt_dynar_free(&set_strings);
    return;
  }

  /* Parse each entry and either use it right now (if the category was already created), or store it for further use */
  xbt_dynar_foreach(set_strings, cpt, str) {
    xbt_log_setting_t set  = _xbt_log_parse_setting(str);
    xbt_log_category_t cat = _xbt_log_cat_searchsub(&_XBT_LOGV(XBT_LOG_ROOT_CAT), set.catname.c_str());

    if (cat) {
      XBT_DEBUG("Apply directly");
      _xbt_log_cat_apply_set(cat, set);
    } else {
      XBT_DEBUG("Store for further application");
      XBT_DEBUG("push %p to the settings", &set);
      xbt_log_settings().emplace_back(std::move(set));
    }
  }
  xbt_dynar_free(&set_strings);
}

void xbt_log_appender_set(xbt_log_category_t cat, xbt_log_appender_t app)
{
  if (cat->appender) {
    if (cat->appender->free_)
      cat->appender->free_(cat->appender);
    xbt_free(cat->appender);
  }
  cat->appender = app;
}

void xbt_log_layout_set(xbt_log_category_t cat, xbt_log_layout_t lay)
{
  DISABLE_XBT_LOG_CAT_INIT();
  if (!cat->appender) {
    XBT_VERB ("No appender to category %s. Setting the file appender as default", cat->name);
    xbt_log_appender_set(cat, xbt_log_appender_file_new(nullptr));
  }
  if (cat->layout) {
    if (cat->layout->free_) {
      cat->layout->free_(cat->layout);
    }
    xbt_free(cat->layout);
  }
  cat->layout = lay;
  xbt_log_additivity_set(cat, 0);
}

void xbt_log_additivity_set(xbt_log_category_t cat, int additivity)
{
  cat->additivity = additivity;
}

static void xbt_log_help()
{
  XBT_HELP(
      "Description of the logging output:\n"
      "\n"
      "   Threshold configuration: --log=CATEGORY_NAME.thres:PRIORITY_LEVEL\n"
      "      CATEGORY_NAME: defined in code with function 'XBT_LOG_NEW_CATEGORY'\n"
      "      PRIORITY_LEVEL: the level to print (trace,debug,verbose,info,warning,error,critical)\n"
      "         -> trace: enter and return of some functions\n"
      "         -> debug: crufty output\n"
      "         -> verbose: verbose output for the user wanting more\n"
      "         -> info: output about the regular functioning\n"
      "         -> warning: minor issue encountered\n"
      "         -> error: issue encountered\n"
      "         -> critical: major issue encountered\n"
      "      The default priority level is 'info'.\n"
      "\n"
      "   Format configuration: --log=CATEGORY_NAME.fmt:FORMAT\n"
      "      FORMAT string may contain:\n"
      "         -> %%%%: the %% char\n"
      "         -> %%n: platform-dependent line separator (LOG4J compatible)\n"
      "         -> %%e: plain old space (SimGrid extension)\n"
      "\n"
      "         -> %%m: user-provided message\n"
      "\n"
      "         -> %%c: Category name (LOG4J compatible)\n"
      "         -> %%p: Priority name (LOG4J compatible)\n"
      "\n"
      "         -> %%h: Hostname (SimGrid extension)\n"
      "         -> %%a: Actor name (SimGrid extension)\n"
      "         -> %%t: Thread \"name\" (LOG4J compatible -- actually the address of the thread in memory)\n"
      "         -> %%i: Process PID (SimGrid extension -- this is a 'i' as in 'i'dea)\n"
      "\n"
      "         -> %%F: file name where the log event was raised (LOG4J compatible)\n"
      "         -> %%l: location where the log event was raised (LOG4J compatible, like '%%F:%%L' -- this is a l as "
      "in 'l'etter)\n"
      "         -> %%L: line number where the log event was raised (LOG4J compatible)\n"
      "         -> %%M: function name (LOG4J compatible -- called method name here of course).\n"
      "\n"
      "         -> %%b: full backtrace (Called %%throwable in LOG4J). Defined only under windows or when using the "
      "GNU libc because\n"
      "                 backtrace() is not defined elsewhere, and we only have a fallback for windows boxes, not "
      "mac ones for example.\n"
      "         -> %%B: short backtrace (only the first line of the %%b). Called %%throwable{short} in LOG4J; "
      "defined where %%b is.\n"
      "\n"
      "         -> %%d: date (UNIX-like epoch)\n"
      "         -> %%r: application age (time elapsed since the beginning of the application)\n"
      "\n"
      "   Category appender: --log=CATEGORY_NAME.app:APPENDER\n"
      "      APPENDER may be:\n"
      "         -> stdout or stderr: standard output streams\n"
      "         -> file:NAME: append to file with given name\n"
      "         -> splitfile:SIZE:NAME: append to files with maximum size SIZE per file.\n"
      "                                 NAME may contain the %% wildcard as a placeholder for the file number.\n"
      "         -> rollfile:SIZE:NAME: append to file with maximum size SIZE.\n"
      "\n"
      "   Category additivity: --log=CATEGORY_NAME.add:VALUE\n"
      "      VALUE:  '0', '1', 'no', 'yes', 'on', or 'off'\n"
      "\n"
      "   Miscellaneous:\n"
      "      --help-log-categories    Display the current hierarchy of log categories.\n"
      "      --log=no_loc             Don't print file names in messages (for tesh tests).\n");
}

static void xbt_log_help_categories_rec(xbt_log_category_t category, const std::string& prefix)
{
  if (!category)
    return;

  std::string this_prefix(prefix);
  std::string child_prefix(prefix);
  if (category->parent) {
    this_prefix  += " \\_ ";
    child_prefix += " |  ";
  }

  std::vector<xbt_log_category_t> cats;
  for (xbt_log_category_t cat = category; cat != nullptr; cat = cat->nextSibling)
    cats.push_back(cat);

  std::sort(begin(cats), end(cats),
            [](const s_xbt_log_category_t* a, const s_xbt_log_category_t* b) { return strcmp(a->name, b->name) < 0; });

  for (auto const& cat : cats) {
    XBT_HELP("%s%s: %s", this_prefix.c_str(), cat->name, cat->description);
    if (cat == cats.back() && category->parent)
      child_prefix[child_prefix.rfind('|')] = ' ';
    xbt_log_help_categories_rec(cat->firstChild, child_prefix);
  }
}

static void xbt_log_help_categories()
{
  XBT_HELP("Current log category hierarchy:");
  xbt_log_help_categories_rec(&_XBT_LOGV(XBT_LOG_ROOT_CAT), "   ");
  XBT_HELP("%s", "");
}
