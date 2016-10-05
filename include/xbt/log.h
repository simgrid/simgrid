/* log - a generic logging facility in the spirit of log4j                  */

/* Copyright (c) 2004-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup XBT_log
 *  @brief A generic logging facility in the spirit of log4j (grounding feature)
 *
 */

/** \defgroup XBT_log_cats Existing log categories
 *  \ingroup XBT_log
 *  \brief (automatically extracted) 
 *
 *  This is the list of all existing log categories in SimGrid.
 *  This list is automatically extracted from the source code by the tools/doxygen/xbt_log_extract_hierarchy.pl utility.
 *
 *  It should thus contain every categories that are defined in the SimGrid library. 
 *  If you want to see the one defined in your code in addition, provide `--help-logs` on the command line of your simulator.
 */

/* XBT_LOG_MAYDAY: define this to replace the logging facilities with basic
   printf function. Useful to debug the logging facilities themselves, or to not make source analysis tools mad */
//#define XBT_LOG_MAYDAY

#ifndef _XBT_LOG_H_
#define _XBT_LOG_H_

#include "xbt/misc.h"
#include <stdarg.h>
#include <stddef.h>             /* NULL */
SG_BEGIN_DECL()
/**\brief Log priorities
 * \ingroup XBT_log
 *
 * The different existing priorities.
*/
typedef enum {
  //! @cond
  xbt_log_priority_none = 0,           /** used internally (don't poke with)*/
  //! @endcond
  xbt_log_priority_trace = 1,          /**< enter and return of some functions */
  xbt_log_priority_debug = 2,          /**< crufty output  */
  xbt_log_priority_verbose = 3,        /**< verbose output for the user wanting more */
  xbt_log_priority_info = 4,           /**< output about the regular functionning */
  xbt_log_priority_warning = 5,        /**< minor issue encountered */
  xbt_log_priority_error = 6,          /**< issue encountered */
  xbt_log_priority_critical = 7,       /**< major issue encountered */

  xbt_log_priority_infinite = 8,       /**< value for XBT_LOG_STATIC_THRESHOLD to not log */

  //! @cond
  xbt_log_priority_uninitialized = -1  /* used internally (don't poke with) */
  //! @endcond
} e_xbt_log_priority_t;

/*
 * define NLOG to disable at compilation time any logging request
 * define NDEBUG to disable at compilation time any logging request of priority below VERBOSE
 */

/**
 * @def XBT_LOG_STATIC_THRESHOLD
 * @ingroup XBT_log
 *
 * All logging requests with priority < XBT_LOG_STATIC_THRESHOLD are disabled at compile time, i.e., compiled out.
 */
#ifdef NLOG
#  define XBT_LOG_STATIC_THRESHOLD xbt_log_priority_infinite
#else

#  ifdef NDEBUG
#    define XBT_LOG_STATIC_THRESHOLD xbt_log_priority_verbose
#  else                         /* !NLOG && !NDEBUG */

#    ifndef XBT_LOG_STATIC_THRESHOLD
#      define XBT_LOG_STATIC_THRESHOLD xbt_log_priority_none
#    endif                      /* !XBT_LOG_STATIC_THRESHOLD */
#  endif                        /* NDEBUG */
#endif                          /* !defined(NLOG) */

/* Transforms a category name to a global variable name. */
#define _XBT_LOGV(cat) _XBT_LOG_CONCAT(_simgrid_log_category__, cat)
#define _XBT_LOGV_CTOR(cat) _XBT_LOG_CONCAT2(_XBT_LOGV(cat), __constructor__)
#define _XBT_LOG_CONCAT(x, y) x ## y
#define _XBT_LOG_CONCAT2(x, y) _XBT_LOG_CONCAT(x, y)
/* Apparently, constructor priorities are not supported by gcc on Macs */
#if defined(__GNUC__) && defined(__APPLE__)
#  define _XBT_LOGV_CTOR_ATTRIBUTE
#else
#  define _XBT_LOGV_CTOR_ATTRIBUTE _XBT_GNUC_CONSTRUCTOR(600)
#endif

/* The root of the category hierarchy. */
#define XBT_LOG_ROOT_CAT   root

/* The whole tree of categories is connected by setting the address of the parent category as a field of the child one.
 * This is normally done at the first use of the category.
 *
 * It is however necessary to make this connections as early as possible, if we want the category to be listed by
 * --help-log-categories.
 *
 * When possible, the initializations takes place automatically before the start of main().  It's the case when
 * compiling with gcc.
 *
 * For the other cases, you can use the XBT_LOG_CONNECT(cat) macro to force early initialization.  See, for example,
 * in xbt/log.c, the function xbt_log_connect_categories().
 */

#define XBT_LOG_CONNECT(cat)                    \
  if (1) {                                      \
    extern void _XBT_LOGV_CTOR(cat)(void);      \
    _XBT_LOGV_CTOR(cat)();                      \
  } else ((void)0)

/* XBT_LOG_NEW_SUBCATEGORY_helper:
 * Implementation of XBT_LOG_NEW_SUBCATEGORY, which must declare "extern parent" in addition to avoid an extra
 * declaration of root when XBT_LOG_NEW_SUBCATEGORY is called by XBT_LOG_NEW_CATEGORY */
#define XBT_LOG_NEW_SUBCATEGORY_helper(catName, parent, desc)           \
  SG_BEGIN_DECL()                                                       \
  extern void _XBT_LOGV_CTOR(catName)(void) _XBT_LOGV_CTOR_ATTRIBUTE; \
  void _XBT_LOGV_CTOR(catName)(void)                                    \
  {                                                                     \
    XBT_LOG_EXTERNAL_CATEGORY(catName);                                 \
    if (!_XBT_LOGV(catName).initialized) {                              \
      _xbt_log_cat_init(&_XBT_LOGV(catName), xbt_log_priority_uninitialized); \
    }                                                                   \
  }                                                                     \
  SG_END_DECL()                                                         \
  XBT_EXPORT_NO_IMPORT(s_xbt_log_category_t) _XBT_LOGV(catName) = {     \
    &_XBT_LOGV(parent),                                                 \
    NULL /* firstChild */,                                              \
    NULL /* nextSibling */,                                             \
    #catName,                                                           \
    desc,                                                               \
    0 /*initialized */,                                                 \
    xbt_log_priority_uninitialized /* threshold */,                     \
    1 /* isThreshInherited */,                                          \
    NULL /* appender */,                                                \
    NULL /* layout */,                                                  \
    1 /* additivity */                                                  \
  }

/**
 * \ingroup XBT_log
 * \param catName name of new category
 * \param parent father of the new category in the tree
 * \param desc string describing the purpose of this category
 * \hideinitializer
 *
 * Defines a new subcategory of the parent. 
 */
#define XBT_LOG_NEW_SUBCATEGORY(catName, parent, desc)    \
  XBT_LOG_EXTERNAL_CATEGORY(parent);                      \
  XBT_LOG_NEW_SUBCATEGORY_helper(catName, parent, desc)   \

/**
 * \ingroup XBT_log  
 * \param catName name of new category
 * \param desc string describing the purpose of this category
 * \hideinitializer
 *
 * Creates a new subcategory of the root category.
 */
# define XBT_LOG_NEW_CATEGORY(catName,desc)  \
   XBT_LOG_NEW_SUBCATEGORY_helper(catName, XBT_LOG_ROOT_CAT, desc)

/**
 * \ingroup XBT_log  
 * \param cname name of the cat
 * \hideinitializer
 *
 * Indicates which category is the default one.
 */

#if defined(XBT_LOG_MAYDAY) /*|| defined (NLOG) * turning logging off */
# define XBT_LOG_DEFAULT_CATEGORY(cname)
#else
# define XBT_LOG_DEFAULT_CATEGORY(cname) \
   static xbt_log_category_t _XBT_LOGV(default) XBT_ATTRIB_UNUSED = &_XBT_LOGV(cname)
#endif

/**
 * \ingroup XBT_log  
 * \param cname name of the cat
 * \param desc string describing the purpose of this category
 * \hideinitializer
 *
 * Creates a new subcategory of the root category and makes it the default (used by macros that don't explicitly
 * specify a category).
 */
# define XBT_LOG_NEW_DEFAULT_CATEGORY(cname,desc)        \
    XBT_LOG_NEW_CATEGORY(cname,desc);                   \
    XBT_LOG_DEFAULT_CATEGORY(cname)

/**
 * \ingroup XBT_log  
 * \param cname name of the cat
 * \param parent name of the parent
 * \param desc string describing the purpose of this category
 * \hideinitializer
 *
 * Creates a new subcategory of the parent category and makes it the default
 * (used by macros that don't explicitly specify a category).
 */
#define XBT_LOG_NEW_DEFAULT_SUBCATEGORY(cname, parent, desc) \
    XBT_LOG_NEW_SUBCATEGORY(cname, parent, desc);            \
    XBT_LOG_DEFAULT_CATEGORY(cname)

/**
 * \ingroup XBT_log  
 * \param cname name of the cat
 * \hideinitializer
 *
 * Indicates that a category you'll use in this file (e.g., to get subcategories of it) really lives in another file.
 */

#define XBT_LOG_EXTERNAL_CATEGORY(cname) \
   extern s_xbt_log_category_t _XBT_LOGV(cname)

/**
 * \ingroup XBT_log
 * \param cname name of the cat
 * \hideinitializer
 *
 * Indicates that the default category of this file was declared in another file.
 */

#define XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(cname) \
   XBT_LOG_EXTERNAL_CATEGORY(cname);\
   XBT_LOG_DEFAULT_CATEGORY(cname)

/* Functions you may call */

XBT_PUBLIC(void) xbt_log_control_set(const char *cs);

/* Forward declarations */
typedef struct xbt_log_appender_s  s_xbt_log_appender_t;
typedef struct xbt_log_appender_s* xbt_log_appender_t;
typedef struct xbt_log_layout_s  s_xbt_log_layout_t;
typedef struct xbt_log_layout_s* xbt_log_layout_t;
typedef struct xbt_log_event_s  s_xbt_log_event_t;
typedef struct xbt_log_event_s* xbt_log_event_t;
typedef struct xbt_log_category_s  s_xbt_log_category_t;
typedef struct xbt_log_category_s* xbt_log_category_t;

/* Do NOT access any members of this structure directly. FIXME: move to private? */

struct xbt_log_category_s {
  xbt_log_category_t parent;
  xbt_log_category_t firstChild;
  xbt_log_category_t nextSibling;
  const char *name;
  const char *description;
  int initialized;
  int threshold;
  int isThreshInherited;
  xbt_log_appender_t appender;
  xbt_log_layout_t layout;
  int additivity;
};

struct xbt_log_event_s {
  xbt_log_category_t cat;
  e_xbt_log_priority_t priority;
  const char *fileName;
  const char *functionName;
  int lineNum;
  va_list ap;
  char *buffer;
  int buffer_size;
};

/**
 * \ingroup XBT_log_implem
 * \param cat the category (not only its name, but the variable)
 * \param thresholdPriority the priority
 *
 * Programatically alters a category's threshold priority (don't use).
 */
XBT_PUBLIC(void) xbt_log_threshold_set(xbt_log_category_t cat, e_xbt_log_priority_t thresholdPriority);

/**
 * \ingroup XBT_log_implem  
 * \param cat the category (not only its name, but the variable)
 * \param app the appender
 *
 * Programatically sets the category's appender. (the preferred interface is through xbt_log_control_set())
 */
XBT_PUBLIC(void) xbt_log_appender_set(xbt_log_category_t cat, xbt_log_appender_t app);
/**
 * \ingroup XBT_log_implem  
 * \param cat the category (not only its name, but the variable)
 * \param lay the layout
 *
 * Programatically sets the category's layout. (the preferred interface is through xbt_log_control_set())
 */
XBT_PUBLIC(void) xbt_log_layout_set(xbt_log_category_t cat, xbt_log_layout_t lay);

/**
 * \ingroup XBT_log_implem  
 * \param cat the category (not only its name, but the variable)
 * \param additivity whether logging actions must be passed to parent.
 *
 * Programatically sets whether the logging actions must be passed to the parent category.
 * (the preferred interface is through xbt_log_control_set())
 */
XBT_PUBLIC(void) xbt_log_additivity_set(xbt_log_category_t cat, int additivity);

/** @brief create a new simple layout 
 *
 * This layout is not as flexible as the pattern one
 */
XBT_PUBLIC(xbt_log_layout_t) xbt_log_layout_simple_new(char *arg);
XBT_PUBLIC(xbt_log_layout_t) xbt_log_layout_format_new(char *arg);
XBT_PUBLIC(xbt_log_appender_t) xbt_log_appender_file_new(char *arg);
XBT_PUBLIC(xbt_log_appender_t) xbt_log_appender2_file_new(char *arg,int roll);

/* ********************************** */
/* Functions that you shouldn't call  */
/* ********************************** */
XBT_PUBLIC(void) _xbt_log_event_log(xbt_log_event_t ev, const char *fmt, ...) XBT_ATTRIB_PRINTF(2, 3);
XBT_PUBLIC(int) _xbt_log_cat_init(xbt_log_category_t category, e_xbt_log_priority_t priority);

#ifdef DLL_EXPORT
XBT_PUBLIC_DATA(s_xbt_log_category_t) _XBT_LOGV(XBT_LOG_ROOT_CAT);
#else
// If we `dllexport` the root log category, MinGW does not want us to take its address with the error:
// > initializer element is not constant
// When using auto-import, MinGW is happy.
// We should handle this for non-root log categories as well.
extern s_xbt_log_category_t _XBT_LOGV(XBT_LOG_ROOT_CAT);
#endif

extern xbt_log_appender_t xbt_log_default_appender;
extern xbt_log_layout_t xbt_log_default_layout;

/* ********************** */
/* Public functions again */
/* ********************** */

/**
 * \ingroup XBT_log 
 * \param catName name of the category
 * \param priority minimal priority to be enabled to return true (must be #e_xbt_log_priority_t)
 * \hideinitializer
 *
 * Returns true if the given priority is enabled for the category.
 * If you have expensive expressions that are computed outside of the log command and used only within it, you should
 * make its evaluation conditional using this macro.
 */
#define XBT_LOG_ISENABLED(catName, priority) \
            _XBT_LOG_ISENABLEDV(_XBT_LOGV(catName), priority)

/*
 * Helper function that implements XBT_LOG_ISENABLED.
 *
 * NOTES
 * First part is a compile-time constant.
 * Call to xbt_log_cat_init only happens once.
 */
#define _XBT_LOG_ISENABLEDV(catv, priority)                  \
       (priority >= XBT_LOG_STATIC_THRESHOLD                 \
        && ((catv).initialized || _xbt_log_cat_init(&(catv), priority)) \
        && priority >= (catv).threshold)

/*
 * Internal Macros
 * Some kludge macros to ease maintenance. See how they're used below.
 *
 * IMPLEMENTATION NOTE: To reduce the parameter passing overhead of an enabled message, the many parameters passed to
 * the logging function are packed in a structure. Since these values will be usually be passed to at least 3 functions,
 * this is a win.
 * It also allows adding new values (such as a timestamp) without breaking code.
 * Setting the LogEvent's valist member is done inside _log_logEvent.
 */

/* Logging Macros */

#ifdef XBT_LOG_MAYDAY
# define XBT_CLOG(cat, prio, ...) \
  _XBT_IF_ONE_ARG(_XBT_CLOG_ARG1, _XBT_CLOG_ARGN, __VA_ARGS__)(__VA_ARGS__)
# define _XBT_CLOG_ARG1(f) \
  fprintf(stderr,"%s:%d:\n" f, __FILE__, __LINE__)
# define _XBT_CLOG_ARGN(f, ...) \
  fprintf(stderr,"%s:%d:\n" f, __FILE__, __LINE__, __VA_ARGS__)
# define XBT_LOG(...) XBT_CLOG(0, __VA_ARGS__)
#else

// This code is duplicated to remove one level of indirection, working around a MSVC bug
// See: http://stackoverflow.com/questions/9183993/msvc-variadic-macro-expansion

# define XBT_CLOG(category, prio, ...) \
  do {                                                                  \
    if (_XBT_LOG_ISENABLEDV((category), prio)) {                        \
      s_xbt_log_event_t _log_ev;                                        \
      _log_ev.cat = &(category);                                        \
      _log_ev.priority = (prio);                                        \
      _log_ev.fileName = __FILE__;                                      \
      _log_ev.functionName = __func__;                             \
      _log_ev.lineNum = __LINE__;                                       \
      _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
    }                                                                   \
  }  while (0)

# define XBT_LOG(prio,...) \
  do {                                                                  \
    if (_XBT_LOG_ISENABLEDV((*_simgrid_log_category__default), prio)) { \
      s_xbt_log_event_t _log_ev;                                        \
      _log_ev.cat = _simgrid_log_category__default;                     \
      _log_ev.priority = (prio);                                        \
      _log_ev.fileName = __FILE__;                                      \
      _log_ev.functionName = __func__;                             \
      _log_ev.lineNum = __LINE__;                                       \
      _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
    }                                                                   \
  }  while (0)
#endif

/** @ingroup XBT_log
 *  @hideinitializer
 * \param categ the category on which to log
 * \param ... the format string and its arguments
 *  @brief Log an event at the DEBUG priority on the specified category with these args.
 */
#define XBT_CDEBUG(categ, ...) \
      do {                                                                  \
        if (XBT_LOG_ISENABLED (categ, xbt_log_priority_debug)) {            \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = &(_XBT_LOGV(categ));                                \
          _log_ev.priority = xbt_log_priority_debug;                        \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the VERB priority on the specified category with these args.
 */
#define XBT_CVERB(categ, ...)  \
      do {                                                                  \
        if (XBT_LOG_ISENABLED (categ, xbt_log_priority_verbose)) {          \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = &(_XBT_LOGV(categ));                                \
          _log_ev.priority = xbt_log_priority_verbose;                      \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the INFO priority on the specified category with these args.
 */
#define XBT_CINFO(categ, ...) \
      do {                                                                  \
        if (XBT_LOG_ISENABLED (categ, xbt_log_priority_info)) {             \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = &(_XBT_LOGV(categ));                                \
          _log_ev.priority = xbt_log_priority_info;                         \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)


/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the WARN priority on the specified category with these args.
 */
#define XBT_CWARN(categ, ...) \
      do {                                                                  \
        if (XBT_LOG_ISENABLED (categ, xbt_log_priority_warning)) {          \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = &(_XBT_LOGV(categ));                                \
          _log_ev.priority = xbt_log_priority_warning;                      \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)


/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the ERROR priority on the specified category with these args.
 */
#define XBT_CERROR(categ, ...) \
      do {                                                                  \
        if (XBT_LOG_ISENABLED (categ, xbt_log_priority_error)) {            \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = &(_XBT_LOGV(categ));                                \
          _log_ev.priority = xbt_log_priority_error;                        \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the CRITICAL priority on the specified category with these args (CCRITICALn exists for any n<10).
 */
#define XBT_CCRITICAL(categ, ...) \
      do {                                                                  \
        if (XBT_LOG_ISENABLED (categ, xbt_log_priority_critical)) {         \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = &(_XBT_LOGV(categ));                                \
          _log_ev.priority = xbt_log_priority_critical;                     \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

/** @ingroup XBT_log
 *  @hideinitializer
 * \param ... the format string and its arguments
 *  @brief Log an event at the DEBUG priority on the default category with these args.
 */
#define XBT_DEBUG(...) \
      do {                                                                  \
        if (_XBT_LOG_ISENABLEDV(*_simgrid_log_category__default,            \
                            xbt_log_priority_debug)) {                  \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = _simgrid_log_category__default;                     \
          _log_ev.priority = xbt_log_priority_debug;                        \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                              \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the VERB priority on the default category with these args.
 */
#define XBT_VERB(...) \
      do {                                                                  \
        if (_XBT_LOG_ISENABLEDV(*_simgrid_log_category__default,            \
                            xbt_log_priority_verbose)) {                \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = _simgrid_log_category__default;                     \
          _log_ev.priority = xbt_log_priority_verbose;                      \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the INFO priority on the default category with these args.
 */
#define XBT_INFO(...) \
      do {                                                                  \
        if (_XBT_LOG_ISENABLEDV(*_simgrid_log_category__default,            \
                            xbt_log_priority_info)) {                   \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = _simgrid_log_category__default;                     \
          _log_ev.priority = xbt_log_priority_info;                         \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the WARN priority on the default category with these args.
 */
#define XBT_WARN(...) \
      do {                                                                  \
        if (_XBT_LOG_ISENABLEDV(*_simgrid_log_category__default,            \
                            xbt_log_priority_warning)) {                \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = _simgrid_log_category__default;                     \
          _log_ev.priority = xbt_log_priority_warning;                      \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the ERROR priority on the default category with these args.
 */
#define XBT_ERROR(...) \
      do {                                                                  \
        if (_XBT_LOG_ISENABLEDV(*_simgrid_log_category__default,            \
                            xbt_log_priority_error)) {                  \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = _simgrid_log_category__default;                     \
          _log_ev.priority = xbt_log_priority_error;                        \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the CRITICAL priority on the default category with these args.
 */
#define XBT_CRITICAL(...) \
      do {                                                                  \
        if (_XBT_LOG_ISENABLEDV(*_simgrid_log_category__default,            \
                            xbt_log_priority_critical)) {               \
          s_xbt_log_event_t _log_ev;                                        \
          _log_ev.cat = _simgrid_log_category__default;                     \
          _log_ev.priority = xbt_log_priority_critical;                     \
          _log_ev.fileName = __FILE__;                                      \
          _log_ev.functionName = __func__;                             \
          _log_ev.lineNum = __LINE__;                                       \
          _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
        }                                                                   \
      }  while (0)

#define _XBT_IN_OUT(...) \
  _XBT_IF_ONE_ARG(_XBT_IN_OUT_ARG1, _XBT_IN_OUT_ARGN, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_IN_OUT_ARG1(fmt) \
  XBT_LOG(xbt_log_priority_trace, fmt, __func__)
#define _XBT_IN_OUT_ARGN(fmt, ...) \
  XBT_LOG(xbt_log_priority_trace, fmt, __func__, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority that we entered in current function, appending a user specified format.
 */
#define XBT_IN(...) _XBT_IN_OUT(">> begin of %s" __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority that we exited the current function, appending a user specified format.
 */
#define XBT_OUT(...) _XBT_IN_OUT("<< end of %s" __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority a message indicating that we reached that point, appending a user specified format.
 */
#define XBT_HERE(...) XBT_LOG(xbt_log_priority_trace, "-- was here" __VA_ARGS__)

SG_END_DECL()
#endif                          /* ! _XBT_LOG_H_ */
