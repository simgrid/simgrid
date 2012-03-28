/* log - a generic logging facility in the spirit of log4j                  */

/* Copyright (c) 2004-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup XBT_log
 *  @brief A generic logging facility in the spirit of log4j (grounding feature)
 *
 *
 */

/** \defgroup XBT_log_cats Existing log categories
 *  \ingroup XBT_log
 *  \brief (automatically extracted) 
 *     
 *  This is the list of all existing log categories in SimGrid.
 *  This list was automatically extracted from the source code by
 *  the src/xbt_log_extract_hierarchy utility.
 *     
 *  You can thus be certain that it is uptodate, but it may somehow
 *  lack a final manual touch.
 *  Anyway, nothing's perfect ;)
 */

/* XBT_LOG_MAYDAY: define this to replace the logging facilities with basic
   printf function. Useful to debug the logging facilities themselves */
#undef XBT_LOG_MAYDAY
//#define XBT_LOG_MAYDAY

#ifndef _XBT_LOG_H_
#define _XBT_LOG_H_

#include "xbt/misc.h"
#include <stdarg.h>
SG_BEGIN_DECL()
/**\brief Log priorities
 * \ingroup XBT_log
 *
 * The different existing priorities.
*/
typedef enum {
  xbt_log_priority_none = 0,    /* used internally (don't poke with) */
  xbt_log_priority_trace = 1,          /**< enter and return of some functions */
  xbt_log_priority_debug = 2,          /**< crufty output  */
  xbt_log_priority_verbose = 3,        /**< verbose output for the user wanting more */
  xbt_log_priority_info = 4,           /**< output about the regular functionning */
  xbt_log_priority_warning = 5,        /**< minor issue encountered */
  xbt_log_priority_error = 6,          /**< issue encountered */
  xbt_log_priority_critical = 7,       /**< major issue encountered */

  xbt_log_priority_infinite = 8,       /**< value for XBT_LOG_STATIC_THRESHOLD to not log */

  xbt_log_priority_uninitialized = -1   /* used internally (don't poke with) */
} e_xbt_log_priority_t;


/*
 * define NLOG to disable at compilation time any logging request
 * define NDEBUG to disable at compilation time any logging request of priority below INFO
 */


/**
 * @def XBT_LOG_STATIC_THRESHOLD
 * @ingroup XBT_log
 *
 * All logging requests with priority < XBT_LOG_STATIC_THRESHOLD are disabled at
 * compile time, i.e., compiled out.
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
#define _XBT_LOGV(cat)   _XBT_LOG_CONCAT(_simgrid_log_category__, cat)
#define _XBT_LOG_CONCAT(x,y) x ## y

/* The root of the category hierarchy. */
#define XBT_LOG_ROOT_CAT   root

/* The whole tree of categories is connected by setting the address of
 * the parent category as a field of the child one.
 *
 * In strict ansi C, we are allowed to initialize a variable with "a
 * pointer to an lvalue designating an object of static storage
 * duration" [ISO/IEC 9899:1999, Section 6.6].
 * 
 * Unfortunately, Visual C builder does not target any standard
 * compliance, and C99 is not an exception to this unfortunate rule.
 * 
 * So, we work this around by adding a XBT_LOG_CONNECT() macro,
 * allowing to connect a child to its parent. It should be used
 * during the initialization of the code, before the child category
 * gets used.
 * 
 * When compiling with gcc, this is not necessary (XBT_LOG_CONNECT
 * defines to nothing). When compiling with MSVC, this is needed if
 * you don't want to see your child category become a child of root
 * directly.
 */
#if defined(_MSC_VER)
# define _XBT_LOG_PARENT_INITIALIZER(parent) NULL
# define XBT_LOG_CONNECT(child, parent_cat)                             \
  if (1) {                                                              \
    XBT_LOG_EXTERNAL_CATEGORY(child);                                   \
    XBT_LOG_EXTERNAL_CATEGORY(parent_cat);                              \
    _XBT_LOGV(child).parent = &_XBT_LOGV(parent_cat);                   \
    _xbt_log_cat_init(&_XBT_LOGV(child), xbt_log_priority_uninitialized); \
  } else ((void)0)
#else
# define _XBT_LOG_PARENT_INITIALIZER(parent) &_XBT_LOGV(parent)
# define XBT_LOG_CONNECT(child, parent_cat)                             \
  if (1) {                                                              \
    XBT_LOG_EXTERNAL_CATEGORY(child);                                   \
    XBT_LOG_EXTERNAL_CATEGORY(parent_cat);                              \
    xbt_assert(_XBT_LOGV(child).parent == &_XBT_LOGV(parent_cat));      \
    _xbt_log_cat_init(&_XBT_LOGV(child), xbt_log_priority_uninitialized); \
  } else ((void)0)
#endif

/* XBT_LOG_NEW_SUBCATEGORY_helper:
 * Implementation of XBT_LOG_NEW_SUBCATEGORY, which must declare "extern parent" in addition
 * to avoid an extra declaration of root when XBT_LOG_NEW_SUBCATEGORY is called by
 * XBT_LOG_NEW_CATEGORY */
#define XBT_LOG_NEW_SUBCATEGORY_helper(catName, parent, desc) \
    XBT_EXPORT_NO_IMPORT(s_xbt_log_category_t) _XBT_LOGV(catName) = {       \
        _XBT_LOG_PARENT_INITIALIZER(parent),            \
        NULL /* firstChild */,                          \
	NULL /* nextSibling */,                         \
        #catName,                                       \
        desc,                                           \
        0 /*initialized */,                             \
        xbt_log_priority_uninitialized /* threshold */, \
        1 /* isThreshInherited */,                      \
        NULL /* appender */,                            \
	NULL /* layout */,                              \
	1 /* additivity */                              \
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
    extern s_xbt_log_category_t _XBT_LOGV(parent); \
    XBT_LOG_NEW_SUBCATEGORY_helper(catName, parent, desc) \

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

#if defined(XBT_LOG_MAYDAY) || defined(SUPERNOVAE_MODE) /*|| defined (NLOG) * turning logging off */
# define XBT_LOG_DEFAULT_CATEGORY(cname)
#else
# define XBT_LOG_DEFAULT_CATEGORY(cname) \
	 static xbt_log_category_t _XBT_LOGV(default) _XBT_GNUC_UNUSED = &_XBT_LOGV(cname)
#endif

/**
 * \ingroup XBT_log  
 * \param cname name of the cat
 * \param desc string describing the purpose of this category
 * \hideinitializer
 *
 * Creates a new subcategory of the root category and makes it the default
 * (used by macros that don't explicitly specify a category).
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
 * Indicates that a category you'll use in this file (to get subcategories of it, 
 * for example) really lives in another file.
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
typedef struct xbt_log_appender_s s_xbt_log_appender_t,
    *xbt_log_appender_t;
typedef struct xbt_log_layout_s s_xbt_log_layout_t, *xbt_log_layout_t;
typedef struct xbt_log_event_s s_xbt_log_event_t, *xbt_log_event_t;
typedef struct xbt_log_category_s s_xbt_log_category_t,
    *xbt_log_category_t;

/*
 * Do NOT access any members of this structure directly. FIXME: move to private?
 */

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
XBT_PUBLIC(void) xbt_log_threshold_set(xbt_log_category_t cat,
                                       e_xbt_log_priority_t
                                       thresholdPriority);

/**
 * \ingroup XBT_log_implem  
 * \param cat the category (not only its name, but the variable)
 * \param app the appender
 *
 * Programatically sets the category's appender.
 * (the prefered interface is throught xbt_log_control_set())
 *
 */
XBT_PUBLIC(void) xbt_log_appender_set(xbt_log_category_t cat,
                                      xbt_log_appender_t app);
/**
 * \ingroup XBT_log_implem  
 * \param cat the category (not only its name, but the variable)
 * \param lay the layout
 *
 * Programatically sets the category's layout.
 * (the prefered interface is throught xbt_log_control_set())
 *
 */
XBT_PUBLIC(void) xbt_log_layout_set(xbt_log_category_t cat,
                                    xbt_log_layout_t lay);

/**
 * \ingroup XBT_log_implem  
 * \param cat the category (not only its name, but the variable)
 * \param additivity whether logging actions must be passed to parent.
 *
 * Programatically sets whether the logging actions must be passed to 
 * the parent category.
 * (the prefered interface is throught xbt_log_control_set())
 *
 */
XBT_PUBLIC(void) xbt_log_additivity_set(xbt_log_category_t cat,
                                        int additivity);

/** @brief create a new simple layout 
 *
 * This layout is not as flexible as the pattern one
 */
XBT_PUBLIC(xbt_log_layout_t) xbt_log_layout_simple_new(char *arg);
XBT_PUBLIC(xbt_log_layout_t) xbt_log_layout_format_new(char *arg);
XBT_PUBLIC(xbt_log_appender_t) xbt_log_appender_file_new(char *arg);


/* ********************************** */
/* Functions that you shouldn't call  */
/* ********************************** */
XBT_PUBLIC(void) _xbt_log_event_log(xbt_log_event_t ev,
                                    const char *fmt,
                                    ...) _XBT_GNUC_PRINTF(2, 3);

XBT_PUBLIC(int) _xbt_log_cat_init(xbt_log_category_t category,
                                  e_xbt_log_priority_t priority);


XBT_PUBLIC_DATA(s_xbt_log_category_t) _XBT_LOGV(XBT_LOG_ROOT_CAT);


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
 * If you have expensive expressions that are computed outside of the log
 * command and used only within it, you should make its evaluation conditional
 * using this macro.
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
        && (catv.initialized || _xbt_log_cat_init(&catv, priority)) \
        && priority >= catv.threshold)

/*
 * Internal Macros
 * Some kludge macros to ease maintenance. See how they're used below.
 *
 * IMPLEMENTATION NOTE: To reduce the parameter passing overhead of an enabled
 * message, the many parameters passed to the logging function are packed in a
 * structure. Since these values will be usually be passed to at least 3
 * functions, this is a win.
 * It also allows adding new values (such as a timestamp) without breaking
 * code. 
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
# define XBT_CLOG_(catv, prio, ...)                                     \
  do {                                                                  \
    if (_XBT_LOG_ISENABLEDV(catv, prio)) {                              \
      s_xbt_log_event_t _log_ev;                                        \
      _log_ev.cat = &(catv);                                            \
      _log_ev.priority = (prio);                                        \
      _log_ev.fileName = __FILE__;                                      \
      _log_ev.functionName = _XBT_FUNCTION;                             \
      _log_ev.lineNum = __LINE__;                                       \
      _xbt_log_event_log(&_log_ev, __VA_ARGS__);                        \
    }                                                                   \
  }  while (0)
# define XBT_CLOG(cat, prio, ...) XBT_CLOG_(_XBT_LOGV(cat), prio, __VA_ARGS__)
# define XBT_LOG(...) XBT_CLOG_((*_XBT_LOGV(default)), __VA_ARGS__)
#endif

/** @ingroup XBT_log
 *  @hideinitializer
 * \param c the category on which to log
 * \param ... the format string and its arguments
 *  @brief Log an event at the DEBUG priority on the specified category with these args.
 */
#define XBT_CDEBUG(c, ...) XBT_CLOG(c, xbt_log_priority_debug, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the VERB priority on the specified category with these args.
 */
#define XBT_CVERB(c, ...) XBT_CLOG(c, xbt_log_priority_verbose, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the INFO priority on the specified category with these args.
 */
#define XBT_CINFO(c, ...) XBT_CLOG(c, xbt_log_priority_info, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the WARN priority on the specified category with these args.
 */
#define XBT_CWARN(c, ...) XBT_CLOG(c, xbt_log_priority_warning, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the ERROR priority on the specified category with these args.
 */
#define XBT_CERROR(c, ...) XBT_CLOG(c, xbt_log_priority_error, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the CRITICAL priority on the specified category with these args (CCRITICALn exists for any n<10).
 */
#define XBT_CCRITICAL(c, ...) XBT_CLOG(c, xbt_log_priority_critical, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 * \param ... the format string and its arguments
 *  @brief Log an event at the DEBUG priority on the default category with these args.
 */
#define XBT_DEBUG(...) XBT_LOG(xbt_log_priority_debug, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the VERB priority on the default category with these args.
 */
#define XBT_VERB(...) XBT_LOG(xbt_log_priority_verbose, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the INFO priority on the default category with these args.
 */
#define XBT_INFO(...) XBT_LOG(xbt_log_priority_info, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the WARN priority on the default category with these args.
 */
#define XBT_WARN(...) XBT_LOG(xbt_log_priority_warning, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the ERROR priority on the default category with these args.
 */
#define XBT_ERROR(...) XBT_LOG(xbt_log_priority_error, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the CRITICAL priority on the default category with these args.
 */
#define XBT_CRITICAL(...) XBT_LOG(xbt_log_priority_critical, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority that we entered in current function, appending a user specified format.
 */
#define XBT_IN(...) \
  _XBT_IF_ONE_ARG(_XBT_IN_ARG1, _XBT_IN_ARGN, __VA_ARGS__)(__VA_ARGS__)
#define _XBT_IN_ARG1(fmt) \
  XBT_LOG(xbt_log_priority_trace, ">> begin of %s" fmt, _XBT_FUNCTION)
#define _XBT_IN_ARGN(fmt, ...) \
  XBT_LOG(xbt_log_priority_trace, ">> begin of %s" fmt, _XBT_FUNCTION, __VA_ARGS__)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority that we exited the current function.
 */
#define XBT_OUT() XBT_LOG(xbt_log_priority_trace, "<< end of %s", _XBT_FUNCTION)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority a message indicating that we reached that point.
 */
#define XBT_HERE() XBT_LOG(xbt_log_priority_trace, "-- was here")

#ifdef XBT_USE_DEPRECATED

/* Kept for backward compatibility. */

#define CLOG0(...) XBT_CLOG(__VA_ARGS__)
#define CLOG1(...) XBT_CLOG(__VA_ARGS__)
#define CLOG2(...) XBT_CLOG(__VA_ARGS__)
#define CLOG3(...) XBT_CLOG(__VA_ARGS__)
#define CLOG4(...) XBT_CLOG(__VA_ARGS__)
#define CLOG5(...) XBT_CLOG(__VA_ARGS__)
#define CLOG6(...) XBT_CLOG(__VA_ARGS__)
#define CLOG7(...) XBT_CLOG(__VA_ARGS__)
#define CLOG8(...) XBT_CLOG(__VA_ARGS__)
#define CLOG9(...) XBT_CLOG(__VA_ARGS__)
#define CLOG10(...) XBT_CLOG(__VA_ARGS__)

#define CDEBUG0(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG1(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG2(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG3(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG4(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG5(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG6(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG7(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG8(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG9(...) XBT_CDEBUG(__VA_ARGS__)
#define CDEBUG10(...) XBT_CDEBUG(__VA_ARGS__)

#define CVERB0(...) XBT_CVERB(__VA_ARGS__)
#define CVERB1(...) XBT_CVERB(__VA_ARGS__)
#define CVERB2(...) XBT_CVERB(__VA_ARGS__)
#define CVERB3(...) XBT_CVERB(__VA_ARGS__)
#define CVERB4(...) XBT_CVERB(__VA_ARGS__)
#define CVERB5(...) XBT_CVERB(__VA_ARGS__)
#define CVERB6(...) XBT_CVERB(__VA_ARGS__)
#define CVERB7(...) XBT_CVERB(__VA_ARGS__)
#define CVERB8(...) XBT_CVERB(__VA_ARGS__)
#define CVERB9(...) XBT_CVERB(__VA_ARGS__)
#define CVERB10(...) XBT_CVERB(__VA_ARGS__)

#define CINFO0(...) XBT_CINFO(__VA_ARGS__)
#define CINFO1(...) XBT_CINFO(__VA_ARGS__)
#define CINFO2(...) XBT_CINFO(__VA_ARGS__)
#define CINFO3(...) XBT_CINFO(__VA_ARGS__)
#define CINFO4(...) XBT_CINFO(__VA_ARGS__)
#define CINFO5(...) XBT_CINFO(__VA_ARGS__)
#define CINFO6(...) XBT_CINFO(__VA_ARGS__)
#define CINFO7(...) XBT_CINFO(__VA_ARGS__)
#define CINFO8(...) XBT_CINFO(__VA_ARGS__)
#define CINFO9(...) XBT_CINFO(__VA_ARGS__)
#define CINFO10(...) XBT_CINFO(__VA_ARGS__)

#define CWARN0(...) XBT_CWARN(__VA_ARGS__)
#define CWARN1(...) XBT_CWARN(__VA_ARGS__)
#define CWARN2(...) XBT_CWARN(__VA_ARGS__)
#define CWARN3(...) XBT_CWARN(__VA_ARGS__)
#define CWARN4(...) XBT_CWARN(__VA_ARGS__)
#define CWARN5(...) XBT_CWARN(__VA_ARGS__)
#define CWARN6(...) XBT_CWARN(__VA_ARGS__)
#define CWARN7(...) XBT_CWARN(__VA_ARGS__)
#define CWARN8(...) XBT_CWARN(__VA_ARGS__)
#define CWARN9(...) XBT_CWARN(__VA_ARGS__)
#define CWARN10(...) XBT_CWARN(__VA_ARGS__)

#define CERROR0(...) XBT_CERROR(__VA_ARGS__)
#define CERROR1(...) XBT_CERROR(__VA_ARGS__)
#define CERROR2(...) XBT_CERROR(__VA_ARGS__)
#define CERROR3(...) XBT_CERROR(__VA_ARGS__)
#define CERROR4(...) XBT_CERROR(__VA_ARGS__)
#define CERROR5(...) XBT_CERROR(__VA_ARGS__)
#define CERROR6(...) XBT_CERROR(__VA_ARGS__)
#define CERROR7(...) XBT_CERROR(__VA_ARGS__)
#define CERROR8(...) XBT_CERROR(__VA_ARGS__)
#define CERROR9(...) XBT_CERROR(__VA_ARGS__)
#define CERROR10(...) XBT_CERROR(__VA_ARGS__)

#define CCRITICAL0(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL1(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL2(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL3(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL4(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL5(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL6(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL7(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL8(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL9(...) XBT_CCRITICAL(__VA_ARGS__)
#define CCRITICAL10(...) XBT_CCRITICAL(__VA_ARGS__)

#define LOG0(...) XBT_LOG(__VA_ARGS__)
#define LOG1(...) XBT_LOG(__VA_ARGS__)
#define LOG2(...) XBT_LOG(__VA_ARGS__)
#define LOG3(...) XBT_LOG(__VA_ARGS__)
#define LOG4(...) XBT_LOG(__VA_ARGS__)
#define LOG5(...) XBT_LOG(__VA_ARGS__)
#define LOG6(...) XBT_LOG(__VA_ARGS__)
#define LOG7(...) XBT_LOG(__VA_ARGS__)
#define LOG8(...) XBT_LOG(__VA_ARGS__)
#define LOG9(...) XBT_LOG(__VA_ARGS__)
#define LOG10(...) XBT_LOG(__VA_ARGS__)

#define DEBUG0(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG1(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG2(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG3(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG4(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG5(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG6(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG7(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG8(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG9(...) XBT_DEBUG(__VA_ARGS__)
#define DEBUG10(...) XBT_DEBUG(__VA_ARGS__)

#define VERB0(...) XBT_VERB(__VA_ARGS__)
#define VERB1(...) XBT_VERB(__VA_ARGS__)
#define VERB2(...) XBT_VERB(__VA_ARGS__)
#define VERB3(...) XBT_VERB(__VA_ARGS__)
#define VERB4(...) XBT_VERB(__VA_ARGS__)
#define VERB5(...) XBT_VERB(__VA_ARGS__)
#define VERB6(...) XBT_VERB(__VA_ARGS__)
#define VERB7(...) XBT_VERB(__VA_ARGS__)
#define VERB8(...) XBT_VERB(__VA_ARGS__)
#define VERB9(...) XBT_VERB(__VA_ARGS__)
#define VERB10(...) XBT_VERB(__VA_ARGS__)

#define INFO0(...) XBT_INFO(__VA_ARGS__)
#define INFO1(...) XBT_INFO(__VA_ARGS__)
#define INFO2(...) XBT_INFO(__VA_ARGS__)
#define INFO3(...) XBT_INFO(__VA_ARGS__)
#define INFO4(...) XBT_INFO(__VA_ARGS__)
#define INFO5(...) XBT_INFO(__VA_ARGS__)
#define INFO6(...) XBT_INFO(__VA_ARGS__)
#define INFO7(...) XBT_INFO(__VA_ARGS__)
#define INFO8(...) XBT_INFO(__VA_ARGS__)
#define INFO9(...) XBT_INFO(__VA_ARGS__)
#define INFO10(...) XBT_INFO(__VA_ARGS__)

#define WARN0(...) XBT_WARN(__VA_ARGS__)
#define WARN1(...) XBT_WARN(__VA_ARGS__)
#define WARN2(...) XBT_WARN(__VA_ARGS__)
#define WARN3(...) XBT_WARN(__VA_ARGS__)
#define WARN4(...) XBT_WARN(__VA_ARGS__)
#define WARN5(...) XBT_WARN(__VA_ARGS__)
#define WARN6(...) XBT_WARN(__VA_ARGS__)
#define WARN7(...) XBT_WARN(__VA_ARGS__)
#define WARN8(...) XBT_WARN(__VA_ARGS__)
#define WARN9(...) XBT_WARN(__VA_ARGS__)
#define WARN10(...) XBT_WARN(__VA_ARGS__)

#define ERROR0(...) XBT_ERROR(__VA_ARGS__)
#define ERROR1(...) XBT_ERROR(__VA_ARGS__)
#define ERROR2(...) XBT_ERROR(__VA_ARGS__)
#define ERROR3(...) XBT_ERROR(__VA_ARGS__)
#define ERROR4(...) XBT_ERROR(__VA_ARGS__)
#define ERROR5(...) XBT_ERROR(__VA_ARGS__)
#define ERROR6(...) XBT_ERROR(__VA_ARGS__)
#define ERROR7(...) XBT_ERROR(__VA_ARGS__)
#define ERROR8(...) XBT_ERROR(__VA_ARGS__)
#define ERROR9(...) XBT_ERROR(__VA_ARGS__)
#define ERROR10(...) XBT_ERROR(__VA_ARGS__)

#define CRITICAL0(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL1(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL2(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL3(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL4(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL5(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL6(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL7(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL8(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL9(...) XBT_CRITICAL(__VA_ARGS__)
#define CRITICAL10(...) XBT_CRITICAL(__VA_ARGS__)

#define XBT_IN1(...) XBT_IN(__VA_ARGS__);
#define XBT_IN2(...) XBT_IN(__VA_ARGS__);
#define XBT_IN3(...) XBT_IN(__VA_ARGS__);
#define XBT_IN4(...) XBT_IN(__VA_ARGS__);
#define XBT_IN5(...) XBT_IN(__VA_ARGS__);
#define XBT_IN6(...) XBT_IN(__VA_ARGS__);

#endif

SG_END_DECL()
#endif                          /* ! _XBT_LOG_H_ */
