/* $Id$ */

/* log - a generic logging facility in the spirit of log4j                  */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003, 2004 Martin Quinson.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

/* GRAS_LOG_MAYDAY: define this to replace the logging facilities with basic
   printf function. Useful to debug the logging facilities themselves */
#undef GRAS_LOG_MAYDAY
//#define GRAS_LOG_MAYDAY


#ifndef _GRAS_LOG_H_
#define _GRAS_LOG_H_

#include <stdarg.h>

/**
 * gras_log_priority_t:
 * @gras_log_priority_none:          used internally (don't poke with)
 * @gras_log_priority_debug:         crufty output 
 * @gras_log_priority_verbose:       verbose output for the user wanting more
 * @gras_log_priority_info:          output about the regular functionning 
 * @gras_log_priority_warning:       minor issue encountered 
 * @gras_log_priority_error:         issue encountered 
 * @gras_log_priority_critical:      major issue encountered 
 * @gras_log_priority_infinite:      value for GRAS_LOG_STATIC_THRESHOLD to not log
 * @gras_log_priority_uninitialized: used internally (don't poke with)
 *
 * The different existing priorities.
 */
typedef enum {
  gras_log_priority_none          = 0, 
  gras_log_priority_debug         = 1, 
  gras_log_priority_verbose       = 2, 
  gras_log_priority_info          = 3, 
  gras_log_priority_warning       = 4, 
  gras_log_priority_error         = 5, 
  gras_log_priority_critical      = 6, 

  gras_log_priority_infinite      = 7, 

  gras_log_priority_uninitialized = -1 
} gras_log_priority_t;
	      

/**
 * GRAS_LOG_STATIC_THRESHOLD:
 *
 * All logging with priority < GRAS_LOG_STATIC_THRESHOLD is disabled at
 * compile time, i.e., compiled out.
 */
#ifndef GRAS_LOG_STATIC_THRESHOLD
#  define GRAS_LOG_STATIC_THRESHOLD gras_log_priority_none
#endif


/* Transforms a category name to a global variable name. */
#define _GRAS_LOGV(cat)   _GRAS_LOG_CONCAT(_gras_this_log_category_does_not_exist__, cat)
#define _GRAS_LOG_CONCAT(x,y) x ## y

/* The root of the category hierarchy. */
#define GRAS_LOG_ROOT_CAT   root

/**
 * GRAS_LOG_NEW_SUBCATEGORY:
 * @catName: name of new category
 * @parent: father of the new category in the tree
 *
 * Defines a new subcategory of the parent. 
 */
#define GRAS_LOG_NEW_SUBCATEGORY(catName, parent)     \
    extern gras_log_category_t _GRAS_LOGV(parent);    \
    gras_log_category_t _GRAS_LOGV(catName) = {       \
        &_GRAS_LOGV(parent), 0, 0,                    \
        #catName, gras_log_priority_uninitialized, 1, \
        0, 1                                          \
    };

/**
 * GRAS_LOG_NEW_CATEGORY:
 * @catName: name of new category
 *
 * Creates a new subcategory of the root category.
 */
#define GRAS_LOG_NEW_CATEGORY(catName)  GRAS_LOG_NEW_SUBCATEGORY(catName, GRAS_LOG_ROOT_CAT)

/**
 * GRAS_LOG_DEFAULT_CATEGORY:
 * @cname: name of the cat
 *
 * Indicates which category is the default one.
 */

#ifdef GRAS_LOG_MAYDAY /* debuging the logs themselves */
# define GRAS_LOG_DEFAULT_CATEGORY(cname)
#else
# define GRAS_LOG_DEFAULT_CATEGORY(cname) \
	 static gras_log_category_t* _GRAS_LOGV(default) = &_GRAS_LOGV(cname)
#endif

/**
 * GRAS_LOG_NEW_DEFAULT_CATEGORY:
 * @cname: name of the cat
 *
 * Creates a new subcategory of the root category and makes it the default
 * (used by macros that don't explicitly specify a category).
 */
#define GRAS_LOG_NEW_DEFAULT_CATEGORY(cname)        \
    GRAS_LOG_NEW_CATEGORY(cname)                    \
    GRAS_LOG_DEFAULT_CATEGORY(cname)

/**
 * GRAS_LOG_NEW_DEFAULT_SUBCATEGORY:
 * @cname: name of the cat
 * @parent: name of the parent
 *
 * Creates a new subcategory of the parent category and makes it the default
 * (used by macros that don't explicitly specify a category).
 */
#define GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(cname, parent)     \
    GRAS_LOG_NEW_SUBCATEGORY(cname, parent)                 \
    GRAS_LOG_DEFAULT_CATEGORY(cname)

/**
 * GRAS_LOG_EXTERNAL_CATEGORY:
 * @cname: name of the cat
 *
 * Indicates that a category you'll use in this file (to get subcategories of it, 
 * for example) really lives in another file.
 */

#define GRAS_LOG_EXTERNAL_CATEGORY(cname) \
   extern gras_log_category_t _GRAS_LOGV(cname)

// Functions you may call

extern gras_error_t gras_log_control_set(const char* cs);

// Forward declarations
typedef struct gras_log_appender_s gras_log_appender_t;
typedef struct gras_log_event_s    gras_log_event_t;
typedef struct gras_log_category_s gras_log_category_t;

/**
 * Do NOT access any members of this structure directly.
 */
struct gras_log_category_s {
  gras_log_category_t *parent;
  gras_log_category_t *firstChild, *nextSibling;
  const char *name;
  int threshold;
  int isThreshInherited;
  gras_log_appender_t *appender;
  int willLogToParent;
  // TODO: Formats?
};

struct gras_log_appender_s {
  void (*do_append) (gras_log_appender_t* thisLogAppender,
		    gras_log_event_t* event);
  void *appender_data;
};

struct gras_log_event_s {
  gras_log_category_t* cat;
  gras_log_priority_t priority;
  const char* fileName;
  const char* functionName;
  int lineNum;
  const char* fmt;
  va_list ap;
};

/**
 * gras_log_threshold_set:
 * @cat: the category (not only its name, but the variable)
 * @thresholdPriority: the priority
 *
 * Programatically alters a category's threshold priority (don't use).
 */
extern void gras_log_threshold_set(gras_log_category_t* cat,
				   gras_log_priority_t thresholdPriority);

/**
 * gras_log_parent_set:
 * @cat: the category (not only its name, but the variable)
 * @parent: the parent cat
 *
 * Programatically alter a category's parent (don't use).
 */
extern void gras_log_parent_set(gras_log_category_t* cat, 
				gras_log_category_t* parent);

/**
 * gras_log_appender_set:
 * @cat: the category (not only its name, but the variable)
 * @app: the appender
 *
 * Programatically sets the category's appender (don't use).
 */
extern void gras_log_appender_set(gras_log_category_t* cat,
				  gras_log_appender_t* app);

// Functions that you shouldn't call. 
extern void _gras_log_event_log(gras_log_event_t*ev,
				...);
extern int _gras_log_cat_init(gras_log_priority_t priority, 
			      gras_log_category_t* category);


extern gras_log_category_t _GRAS_LOGV(GRAS_LOG_ROOT_CAT);
GRAS_LOG_EXTERNAL_CATEGORY(GRAS);
extern gras_log_appender_t *gras_log_default_appender;

/**
 * GRAS_LOG_ISENABLED:
 * @catName: name of the category
 * @priority: minimal priority to be enabled to return true
 *
 * Returns true if the given priority is enabled for the category.
 * If you have expensive expressions that are computed outside of the log
 * command and used only within it, you should make its evaluation conditional
 * using this macro.
 */
#define GRAS_LOG_ISENABLED(catName, priority) \
            _GRAS_LOG_ISENABLEDV(_GRAS_LOGV(catName), priority)

/*
 * Helper function that implements GRAS_LOG_ISENABLED.
 *
 * NOTES
 * First part is a compile-time constant.
 * Call to _log_initCat only happens once.
 */
#define _GRAS_LOG_ISENABLEDV(catv, priority)                  \
       (priority >= GRAS_LOG_STATIC_THRESHOLD                 \
        && priority >= catv.threshold                         \
        && (catv.threshold != gras_log_priority_uninitialized \
            || _gras_log_cat_init(priority, &catv)) )

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

#define _GRAS_LOG_PRE(catv, priority, fmt) do {                         \
     if (_GRAS_LOG_ISENABLEDV(catv, priority)) {                        \
         gras_log_event_t _log_ev =                                     \
             {&(catv),priority,__FILE__,__FUNCTION__,__LINE__,fmt};         \
         _gras_log_event_log(&_log_ev
#define _GRAS_LOG_POST                          \
                        );                      \
     } } while(0)


/* Logging Macros */

#ifdef GRAS_LOG_MAYDAY
# define CLOG0(c, p, f)                   fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__)
# define CLOG1(c, p, f,a1)                fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1)               
# define CLOG2(c, p, f,a1,a2)             fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2)            
# define CLOG3(c, p, f,a1,a2,a3)          fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3)         
# define CLOG4(c, p, f,a1,a2,a3,a4)       fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4)      
# define CLOG5(c, p, f,a1,a2,a3,a4,a5)    fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5)   
# define CLOG6(c, p, f,a1,a2,a3,a4,a5,a6) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6)
#else
# define CLOG0(c, p, f)                   _GRAS_LOG_PRE(_GRAS_LOGV(c),p,f) _GRAS_LOG_POST		   
# define CLOG1(c, p, f,a1)                _GRAS_LOG_PRE(_GRAS_LOGV(c),p,f) ,a1 _GRAS_LOG_POST		   
# define CLOG2(c, p, f,a1,a2)             _GRAS_LOG_PRE(_GRAS_LOGV(c),p,f) ,a1,a2 _GRAS_LOG_POST
# define CLOG3(c, p, f,a1,a2,a3)          _GRAS_LOG_PRE(_GRAS_LOGV(c),p,f) ,a1,a2,a3 _GRAS_LOG_POST
# define CLOG4(c, p, f,a1,a2,a3,a4)       _GRAS_LOG_PRE(_GRAS_LOGV(c),p,f) ,a1,a2,a3,a4 _GRAS_LOG_POST
# define CLOG5(c, p, f,a1,a2,a3,a4,a5)    _GRAS_LOG_PRE(_GRAS_LOGV(c),p,f) ,a1,a2,a3,a4,a5 _GRAS_LOG_POST
# define CLOG6(c, p, f,a1,a2,a3,a4,a5,a6) _GRAS_LOG_PRE(_GRAS_LOGV(c),p,f) ,a1,a2,a3,a4,a5,a6 _GRAS_LOG_POST
#endif

#define CDEBUG0(c, f)                   CLOG0(c, gras_log_priority_debug, f)
#define CDEBUG1(c, f,a1)                CLOG1(c, gras_log_priority_debug, f,a1)
#define CDEBUG2(c, f,a1,a2)             CLOG2(c, gras_log_priority_debug, f,a1,a2)
#define CDEBUG3(c, f,a1,a2,a3)          CLOG3(c, gras_log_priority_debug, f,a1,a2,a3)
#define CDEBUG4(c, f,a1,a2,a3,a4)       CLOG4(c, gras_log_priority_debug, f,a1,a2,a3,a4)
#define CDEBUG5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, gras_log_priority_debug, f,a1,a2,a3,a4,a5)
#define CDEBUG6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, gras_log_priority_debug, f,a1,a2,a3,a4,a5,a6)

#define CVERB0(c, f)                   CLOG0(c, gras_log_priority_verbose, f)
#define CVERB1(c, f,a1)                CLOG1(c, gras_log_priority_verbose, f,a1)
#define CVERB2(c, f,a1,a2)             CLOG2(c, gras_log_priority_verbose, f,a1,a2)
#define CVERB3(c, f,a1,a2,a3)          CLOG3(c, gras_log_priority_verbose, f,a1,a2,a3)
#define CVERB4(c, f,a1,a2,a3,a4)       CLOG4(c, gras_log_priority_verbose, f,a1,a2,a3,a4)
#define CVERB5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, gras_log_priority_verbose, f,a1,a2,a3,a4,a5)
#define CVERB6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, gras_log_priority_verbose, f,a1,a2,a3,a4,a5,a6)

#define CINFO0(c, f)                   CLOG0(c, gras_log_priority_info, f)
#define CINFO1(c, f,a1)                CLOG1(c, gras_log_priority_info, f,a1)
#define CINFO2(c, f,a1,a2)             CLOG2(c, gras_log_priority_info, f,a1,a2)
#define CINFO3(c, f,a1,a2,a3)          CLOG3(c, gras_log_priority_info, f,a1,a2,a3)
#define CINFO4(c, f,a1,a2,a3,a4)       CLOG4(c, gras_log_priority_info, f,a1,a2,a3,a4)
#define CINFO5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, gras_log_priority_info, f,a1,a2,a3,a4,a5)
#define CINFO6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, gras_log_priority_info, f,a1,a2,a3,a4,a5,a6)

#define CWARNING0(c, f)                   CLOG0(c, gras_log_priority_warning, f)
#define CWARNING1(c, f,a1)                CLOG1(c, gras_log_priority_warning, f,a1)
#define CWARNING2(c, f,a1,a2)             CLOG2(c, gras_log_priority_warning, f,a1,a2)
#define CWARNING3(c, f,a1,a2,a3)          CLOG3(c, gras_log_priority_warning, f,a1,a2,a3)
#define CWARNING4(c, f,a1,a2,a3,a4)       CLOG4(c, gras_log_priority_warning, f,a1,a2,a3,a4)
#define CWARNING5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, gras_log_priority_warning, f,a1,a2,a3,a4,a5)
#define CWARNING6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, gras_log_priority_warning, f,a1,a2,a3,a4,a5,a6)

#define CERROR0(c, f)                   CLOG0(c, gras_log_priority_error, f)
#define CERROR1(c, f,a1)                CLOG1(c, gras_log_priority_error, f,a1)
#define CERROR2(c, f,a1,a2)             CLOG2(c, gras_log_priority_error, f,a1,a2)
#define CERROR3(c, f,a1,a2,a3)          CLOG3(c, gras_log_priority_error, f,a1,a2,a3)
#define CERROR4(c, f,a1,a2,a3,a4)       CLOG4(c, gras_log_priority_error, f,a1,a2,a3,a4)
#define CERROR5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, gras_log_priority_error, f,a1,a2,a3,a4,a5)
#define CERROR6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, gras_log_priority_error, f,a1,a2,a3,a4,a5,a6)

/**
 * CCRITICAL6:
 * @c: the category to log into
 * @f: the format string
 * @a1: first argument of the format
 * @a2: second argument of the format
 * @a3: third argument of the format
 * @a4: fourth argument of the format
 * @a5: fifth argument of the format
 * @a6: sixth argument of the format
 *
 * Log something to the current default category under the warning priority.
 *
 * The macros CCRITICAL0 ... CCRITICAL5 naturally also exist, but are not listed here 
 * for sake of clarity. They just differ in the number of arguments passed
 * along with the format string.
 */

#define CCRITICAL0(c, f)                   CLOG0(c, gras_log_priority_critical, f)
#define CCRITICAL1(c, f,a1)                CLOG1(c, gras_log_priority_critical, f,a1)
#define CCRITICAL2(c, f,a1,a2)             CLOG2(c, gras_log_priority_critical, f,a1,a2)
#define CCRITICAL3(c, f,a1,a2,a3)          CLOG3(c, gras_log_priority_critical, f,a1,a2,a3)
#define CCRITICAL4(c, f,a1,a2,a3,a4)       CLOG4(c, gras_log_priority_critical, f,a1,a2,a3,a4)
#define CCRITICAL5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, gras_log_priority_critical, f,a1,a2,a3,a4,a5)
#define CCRITICAL6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, gras_log_priority_critical, f,a1,a2,a3,a4,a5,a6)

#ifdef GRAS_LOG_MAYDAY
# define LOG0(p, f)                   fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__)
# define LOG1(p, f,a1)                fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1)               
# define LOG2(p, f,a1,a2)             fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2)            
# define LOG3(p, f,a1,a2,a3)          fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3)         
# define LOG4(p, f,a1,a2,a3,a4)       fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4)      
# define LOG5(p, f,a1,a2,a3,a4,a5)    fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5)   
# define LOG6(p, f,a1,a2,a3,a4,a5,a6) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6)
#else
# define LOG0(p, f)                   _GRAS_LOG_PRE((*_GRAS_LOGV(default)),p,f) _GRAS_LOG_POST
# define LOG1(p, f,a1)                _GRAS_LOG_PRE((*_GRAS_LOGV(default)),p,f) ,a1 _GRAS_LOG_POST
# define LOG2(p, f,a1,a2)             _GRAS_LOG_PRE((*_GRAS_LOGV(default)),p,f) ,a1,a2 _GRAS_LOG_POST
# define LOG3(p, f,a1,a2,a3)          _GRAS_LOG_PRE((*_GRAS_LOGV(default)),p,f) ,a1,a2,a3 _GRAS_LOG_POST
# define LOG4(p, f,a1,a2,a3,a4)       _GRAS_LOG_PRE((*_GRAS_LOGV(default)),p,f) ,a1,a2,a3,a4 _GRAS_LOG_POST
# define LOG5(p, f,a1,a2,a3,a4,a5)    _GRAS_LOG_PRE((*_GRAS_LOGV(default)),p,f) ,a1,a2,a3,a4,a5 _GRAS_LOG_POST
# define LOG6(p, f,a1,a2,a3,a4,a5,a6) _GRAS_LOG_PRE((*_GRAS_LOGV(default)),p,f) ,a1,a2,a3,a4,a5,a6 _GRAS_LOG_POST
#endif

/**
 * DEBUG6:
 * @f: the format string
 * @a1: first argument of the format
 * @a2: second argument of the format
 * @a3: third argument of the format
 * @a4: fourth argument of the format
 * @a5: fifth argument of the format
 * @a6: sixth argument of the format
 *
 * Log something to the current default category under the debug priority.
 *
 * The macros DEBUG0 ... DEBUG5 naturally also exist, but are not listed here 
 * for sake of clarity. They just differ in the number of arguments passed
 * along with the format string.
 */

#define DEBUG0(f)                   LOG0(gras_log_priority_debug, f)
#define DEBUG1(f,a1)                LOG1(gras_log_priority_debug, f,a1)
#define DEBUG2(f,a1,a2)             LOG2(gras_log_priority_debug, f,a1,a2)
#define DEBUG3(f,a1,a2,a3)          LOG3(gras_log_priority_debug, f,a1,a2,a3)
#define DEBUG4(f,a1,a2,a3,a4)       LOG4(gras_log_priority_debug, f,a1,a2,a3,a4)
#define DEBUG5(f,a1,a2,a3,a4,a5)    LOG5(gras_log_priority_debug, f,a1,a2,a3,a4,a5)
#define DEBUG6(f,a1,a2,a3,a4,a5,a6) LOG6(gras_log_priority_debug, f,a1,a2,a3,a4,a5,a6)

/**
 * VERB6:
 * @f: the format string
 * @a1: first argument of the format
 * @a2: second argument of the format
 * @a3: third argument of the format
 * @a4: fourth argument of the format
 * @a5: fifth argument of the format
 * @a6: sixth argument of the format
 *
 * Log something to the current default category under the verbose priority.
 *
 * The macros VERB0 ... VERB5 naturally also exist, but are not listed here 
 * for sake of clarity. They just differ in the number of arguments passed
 * along with the format string.
 */

#define VERB0(f)                   LOG0(gras_log_priority_verbose, f)
#define VERB1(f,a1)                LOG1(gras_log_priority_verbose, f,a1)
#define VERB2(f,a1,a2)             LOG2(gras_log_priority_verbose, f,a1,a2)
#define VERB3(f,a1,a2,a3)          LOG3(gras_log_priority_verbose, f,a1,a2,a3)
#define VERB4(f,a1,a2,a3,a4)       LOG4(gras_log_priority_verbose, f,a1,a2,a3,a4)
#define VERB5(f,a1,a2,a3,a4,a5)    LOG5(gras_log_priority_verbose, f,a1,a2,a3,a4,a5)
#define VERB6(f,a1,a2,a3,a4,a5,a6) LOG6(gras_log_priority_verbose, f,a1,a2,a3,a4,a5,a6)

/**
 * INFO6:
 * @f: the format string
 * @a1: first argument of the format
 * @a2: second argument of the format
 * @a3: third argument of the format
 * @a4: fourth argument of the format
 * @a5: fifth argument of the format
 * @a6: sixth argument of the format
 *
 * Log something to the current default category under the info priority.
 *
 * The macros INFO0 ... INFO5 naturally also exist, but are not listed here 
 * for sake of clarity. They just differ in the number of arguments passed
 * along with the format string.
 */

#define INFO0(f)                   LOG0(gras_log_priority_info, f)
#define INFO1(f,a1)                LOG1(gras_log_priority_info, f,a1)
#define INFO2(f,a1,a2)             LOG2(gras_log_priority_info, f,a1,a2)
#define INFO3(f,a1,a2,a3)          LOG3(gras_log_priority_info, f,a1,a2,a3)
#define INFO4(f,a1,a2,a3,a4)       LOG4(gras_log_priority_info, f,a1,a2,a3,a4)
#define INFO5(f,a1,a2,a3,a4,a5)    LOG5(gras_log_priority_info, f,a1,a2,a3,a4,a5)
#define INFO6(f,a1,a2,a3,a4,a5,a6) LOG6(gras_log_priority_info, f,a1,a2,a3,a4,a5,a6)

/**
 * WARNING6:
 * @f: the format string
 * @a1: first argument of the format
 * @a2: second argument of the format
 * @a3: third argument of the format
 * @a4: fourth argument of the format
 * @a5: fifth argument of the format
 * @a6: sixth argument of the format
 *
 * Log something to the current default category under the warning priority.
 *
 * The macros WARNING0 ... WARNING5 naturally also exist, but are not listed here 
 * for sake of clarity. They just differ in the number of arguments passed
 * along with the format string.
 */

#define WARNING0(f)                   LOG0(gras_log_priority_warning, f)
#define WARNING1(f,a1)                LOG1(gras_log_priority_warning, f,a1)
#define WARNING2(f,a1,a2)             LOG2(gras_log_priority_warning, f,a1,a2)
#define WARNING3(f,a1,a2,a3)          LOG3(gras_log_priority_warning, f,a1,a2,a3)
#define WARNING4(f,a1,a2,a3,a4)       LOG4(gras_log_priority_warning, f,a1,a2,a3,a4)
#define WARNING5(f,a1,a2,a3,a4,a5)    LOG5(gras_log_priority_warning, f,a1,a2,a3,a4,a5)
#define WARNING6(f,a1,a2,a3,a4,a5,a6) LOG6(gras_log_priority_warning, f,a1,a2,a3,a4,a5,a6)

/**
 * ERROR6:
 * @f: the format string
 * @a1: first argument of the format
 * @a2: second argument of the format
 * @a3: third argument of the format
 * @a4: fourth argument of the format
 * @a5: fifth argument of the format
 * @a6: sixth argument of the format
 *
 * Log something to the current default category under the error priority.
 *
 * The macros ERROR0 ... ERROR5 naturally also exist, but are not listed here 
 * for sake of clarity. They just differ in the number of arguments passed
 * along with the format string.
 */

#define ERROR0(f)                   LOG0(gras_log_priority_error, f)
#define ERROR1(f,a1)                LOG1(gras_log_priority_error, f,a1)
#define ERROR2(f,a1,a2)             LOG2(gras_log_priority_error, f,a1,a2)
#define ERROR3(f,a1,a2,a3)          LOG3(gras_log_priority_error, f,a1,a2,a3)
#define ERROR4(f,a1,a2,a3,a4)       LOG4(gras_log_priority_error, f,a1,a2,a3,a4)
#define ERROR5(f,a1,a2,a3,a4,a5)    LOG5(gras_log_priority_error, f,a1,a2,a3,a4,a5)
#define ERROR6(f,a1,a2,a3,a4,a5,a6) LOG6(gras_log_priority_error, f,a1,a2,a3,a4,a5,a6)

/**
 * CRITICAL6:
 * @f: the format string
 * @a1: first argument of the format
 * @a2: second argument of the format
 * @a3: third argument of the format
 * @a4: fourth argument of the format
 * @a5: fifth argument of the format
 * @a6: sixth argument of the format
 *
 * Log something to the current default category under the critical priority.
 *
 * The macros CRITICAL0 ... CRITICAL5 naturally also exist, but are not listed here 
 * for sake of clarity. They just differ in the number of arguments passed
 * along with the format string.
 */
#define CRITICAL0(f)                   LOG0(gras_log_priority_critical, f)
#define CRITICAL1(f,a1)                LOG1(gras_log_priority_critical, f,a1)
#define CRITICAL2(f,a1,a2)             LOG2(gras_log_priority_critical, f,a1,a2)
#define CRITICAL3(f,a1,a2,a3)          LOG3(gras_log_priority_critical, f,a1,a2,a3)
#define CRITICAL4(f,a1,a2,a3,a4)       LOG4(gras_log_priority_critical, f,a1,a2,a3,a4)
#define CRITICAL5(f,a1,a2,a3,a4,a5)    LOG5(gras_log_priority_critical, f,a1,a2,a3,a4,a5)
#define CRITICAL6(f,a1,a2,a3,a4,a5,a6) LOG6(gras_log_priority_critical, f,a1,a2,a3,a4,a5,a6)

#define GRAS_IN  DEBUG0(">> begin of function")
#define GRAS_OUT DEBUG0("<< end of function")

#endif /* ! _GRAS_LOG_H_ */
