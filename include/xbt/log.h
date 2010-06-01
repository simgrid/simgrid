/* log - a generic logging facility in the spirit of log4j                  */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
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
/*#define XBT_LOG_MAYDAY*/

#ifndef _XBT_LOG_H_
#define _XBT_LOG_H_

#include "xbt/misc.h"
#include "gras_config.h"

#include <stdarg.h>
SG_BEGIN_DECL()
/**\brief Log priorities
 * \ingroup XBT_log
 *
 * The different existing priorities.
*/
     typedef enum {
       xbt_log_priority_none = 0,       /* used internally (don't poke with) */
       xbt_log_priority_trace = 1,     /**< enter and return of some functions */
       xbt_log_priority_debug = 2,     /**< crufty output  */
       xbt_log_priority_verbose = 3,   /**< verbose output for the user wanting more */
       xbt_log_priority_info = 4,      /**< output about the regular functionning */
       xbt_log_priority_warning = 5,   /**< minor issue encountered */
       xbt_log_priority_error = 6,     /**< issue encountered */
       xbt_log_priority_critical = 7,  /**< major issue encountered */

       xbt_log_priority_infinite = 8,  /**< value for XBT_LOG_STATIC_THRESHOLD to not log */

       xbt_log_priority_uninitialized = -1      /* used internally (don't poke with) */
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
#  else /* !NLOG && !NDEBUG */

#    ifndef XBT_LOG_STATIC_THRESHOLD
#      define XBT_LOG_STATIC_THRESHOLD xbt_log_priority_none
#    endif /* !XBT_LOG_STATIC_THRESHOLD */
#  endif /* NDEBUG */
#endif /* !defined(NLOG) */

/* Transforms a category name to a global variable name. */
#define _XBT_LOGV(cat)   _XBT_LOG_CONCAT(_simgrid_log_category__, cat)
#define _XBT_LOG_CONCAT(x,y) x ## y

/* The root of the category hierarchy. */
#define XBT_LOG_ROOT_CAT   root

/* In strict ansi C, we are not allowed to initialize a variable with
 * a non-constant value. But the whole tree of categories is
 * connected by setting the address of the parent category as a field
 * of the child one.
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
#if defined(__STRICT_ANSI__) || defined(_MSC_VER)
# define _XBT_LOG_PARENT_INITIALIZER(parent) NULL
# define XBT_LOG_CONNECT(parent_cat,child)       _XBT_LOGV(child).parent = &_XBT_LOGV(parent_cat)
#else
# define _XBT_LOG_PARENT_INITIALIZER(parent) &_XBT_LOGV(parent)
# define XBT_LOG_CONNECT(parent_cat,child)      /*  xbt_assert(_XBT_LOGV(child).parent == &_XBT_LOGV(parent_cat)) */
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

#if defined(XBT_LOG_MAYDAY) || defined(SUPERNOVAE_MODE)     /*|| defined (NLOG) * turning logging off */
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
#ifdef WIN32
#define XBT_LOG_BUFF_SIZE  16384        /* Size of the static string in which we build the log string */
#else
#define XBT_LOG_BUFF_SIZE 2048  /* Size of the static string in which we build the log string */
#endif
     struct xbt_log_category_s {
       xbt_log_category_t parent;
       xbt_log_category_t firstChild;
       xbt_log_category_t nextSibling;
       const char *name;
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
       va_list ap_copy;         /* need a copy to launch dynamic layouts when the static ones overflowed */
#ifdef WIN32
       char *buffer;
#else
       char buffer[XBT_LOG_BUFF_SIZE];
#endif
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
 * Call to _log_initCat only happens once.
 */
#define _XBT_LOG_ISENABLEDV(catv, priority)                  \
       (priority >= XBT_LOG_STATIC_THRESHOLD                 \
        && priority >= catv.threshold                         \
        && (catv.threshold != xbt_log_priority_uninitialized \
            || _xbt_log_cat_init(&catv, priority)) )

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
#ifdef WIN32
#define _XBT_LOG_PRE(catv, priority) do {			 \
     if (_XBT_LOG_ISENABLEDV(catv, priority)) {                  \
         s_xbt_log_event_t _log_ev =                             \
             {NULL,priority,__FILE__,_XBT_FUNCTION,__LINE__}; \
                _log_ev.cat = &(catv);                           \
				_log_ev.buffer = (char*) calloc(XBT_LOG_BUFF_SIZE + 1, sizeof(char)); \
              _xbt_log_event_log(&_log_ev
#else
#define _XBT_LOG_PRE(catv, priority) do {			 \
     if (_XBT_LOG_ISENABLEDV(catv, priority)) {                  \
         s_xbt_log_event_t _log_ev =                             \
             {NULL,priority,__FILE__,_XBT_FUNCTION,__LINE__}; \
                _log_ev.cat = &(catv);                           \
              _xbt_log_event_log(&_log_ev 			 \

#endif

#define _XBT_LOG_POST                          \
                        );                      \
     } } while(0)


/* Logging Macros */

#ifdef XBT_LOG_MAYDAY
# define CLOG0(c, p, f)                   fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__)
# define CLOG1(c, p, f,a1)                fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1)
# define CLOG2(c, p, f,a1,a2)             fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2)
# define CLOG3(c, p, f,a1,a2,a3)          fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3)
# define CLOG4(c, p, f,a1,a2,a3,a4)       fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4)
# define CLOG5(c, p, f,a1,a2,a3,a4,a5)    fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5)
# define CLOG6(c, p, f,a1,a2,a3,a4,a5,a6) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6)
# define CLOG7(c, p, f,a1,a2,a3,a4,a5,a6,a7) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6,a7)
# define CLOG8(c, p, f,a1,a2,a3,a4,a5,a6,a7,a8) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6,a7,a8)
# define CLOG9(c, p, f,a1,a2,a3,a4,a5,a6,a7,a8,a9) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6,a7,a8,a9)
# define CLOG10(c, p, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)
#else
# define CLOG0(c, p, f)                   _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f _XBT_LOG_POST
# define CLOG1(c, p, f,a1)                _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1 _XBT_LOG_POST
# define CLOG2(c, p, f,a1,a2)             _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1,a2 _XBT_LOG_POST
# define CLOG3(c, p, f,a1,a2,a3)          _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1,a2,a3 _XBT_LOG_POST
# define CLOG4(c, p, f,a1,a2,a3,a4)       _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1,a2,a3,a4 _XBT_LOG_POST
# define CLOG5(c, p, f,a1,a2,a3,a4,a5)    _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1,a2,a3,a4,a5 _XBT_LOG_POST
# define CLOG6(c, p, f,a1,a2,a3,a4,a5,a6) _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1,a2,a3,a4,a5,a6 _XBT_LOG_POST
# define CLOG7(c, p, f,a1,a2,a3,a4,a5,a6,a7) _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1,a2,a3,a4,a5,a6,a7 _XBT_LOG_POST
# define CLOG8(c, p, f,a1,a2,a3,a4,a5,a6,a7,a8) _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1,a2,a3,a4,a5,a6,a7,a8 _XBT_LOG_POST
# define CLOG9(c, p, f,a1,a2,a3,a4,a5,a6,a7,a8,a9) _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1,a2,a3,a4,a5,a6,a7,a8,a9 _XBT_LOG_POST
# define CLOG10(c, p, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) _XBT_LOG_PRE(_XBT_LOGV(c),p) ,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10 _XBT_LOG_POST
#endif
#define CDEBUG0(c, f)                   CLOG0(c, xbt_log_priority_debug, f)
#define CDEBUG1(c, f,a1)                CLOG1(c, xbt_log_priority_debug, f,a1)
#define CDEBUG2(c, f,a1,a2)             CLOG2(c, xbt_log_priority_debug, f,a1,a2)
#define CDEBUG3(c, f,a1,a2,a3)          CLOG3(c, xbt_log_priority_debug, f,a1,a2,a3)
#define CDEBUG4(c, f,a1,a2,a3,a4)       CLOG4(c, xbt_log_priority_debug, f,a1,a2,a3,a4)
#define CDEBUG5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, xbt_log_priority_debug, f,a1,a2,a3,a4,a5)
#define CDEBUG6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6)
#define CDEBUG7(c, f,a1,a2,a3,a4,a5,a6,a7) CLOG7(c, xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6,a7)
#define CDEBUG8(c, f,a1,a2,a3,a4,a5,a6,a7,a8) CLOG8(c, xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define CDEBUG9(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9) CLOG9(c, xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 * \param c the category on which to log
 * \param f the format string
 * \param a1 first argument of the format
 * \param a2 second argument of the format
 * \param a3 third argument of the format
 * \param a4 fourth argument of the format
 * \param a5 fifth argument of the format
 * \param a6 sixth argument of the format
 * \param a7 seventh argument of the format
 * \param a8 eighth argument of the format
 * \param a9 ninth argument of the format
 * \param a10 tenth argument of the format
 *  @brief Log an event at the DEBUG priority on the specified category with these args (CDEBUGn exists for any n<10).
 */
#define CDEBUG10(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) CLOG10(c, xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#define CVERB0(c, f)                   CLOG0(c, xbt_log_priority_verbose, f)
#define CVERB1(c, f,a1)                CLOG1(c, xbt_log_priority_verbose, f,a1)
#define CVERB2(c, f,a1,a2)             CLOG2(c, xbt_log_priority_verbose, f,a1,a2)
#define CVERB3(c, f,a1,a2,a3)          CLOG3(c, xbt_log_priority_verbose, f,a1,a2,a3)
#define CVERB4(c, f,a1,a2,a3,a4)       CLOG4(c, xbt_log_priority_verbose, f,a1,a2,a3,a4)
#define CVERB5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, xbt_log_priority_verbose, f,a1,a2,a3,a4,a5)
#define CVERB6(c, f,a1,a2,a3,a4,a5,a6)    CLOG6(c, xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6)
#define CVERB7(c, f,a1,a2,a3,a4,a5,a6,a7)    CLOG7(c, xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6,a7)
#define CVERB8(c, f,a1,a2,a3,a4,a5,a6,a7,a8)    CLOG8(c, xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define CVERB9(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)    CLOG9(c, xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the VERB priority on the specified category with these args (CVERBn exists for any n<10).
 */
#define CVERB10(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)    CLOG10(c, xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#define CINFO0(c, f)                   CLOG0(c, xbt_log_priority_info, f)
#define CINFO1(c, f,a1)                CLOG1(c, xbt_log_priority_info, f,a1)
#define CINFO2(c, f,a1,a2)             CLOG2(c, xbt_log_priority_info, f,a1,a2)
#define CINFO3(c, f,a1,a2,a3)          CLOG3(c, xbt_log_priority_info, f,a1,a2,a3)
#define CINFO4(c, f,a1,a2,a3,a4)       CLOG4(c, xbt_log_priority_info, f,a1,a2,a3,a4)
#define CINFO5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, xbt_log_priority_info, f,a1,a2,a3,a4,a5)
#define CINFO6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6)
#define CINFO7(c, f,a1,a2,a3,a4,a5,a6,a7) CLOG7(c, xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6,a7)
#define CINFO8(c, f,a1,a2,a3,a4,a5,a6,a7,a8) CLOG8(c, xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define CINFO9(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9) CLOG9(c, xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the INFO priority on the specified category with these args (CINFOn exists for any n<10).
 */
#define CINFO10(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) CLOG10(c, xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#define CWARN0(c, f)                   CLOG0(c, xbt_log_priority_warning, f)
#define CWARN1(c, f,a1)                CLOG1(c, xbt_log_priority_warning, f,a1)
#define CWARN2(c, f,a1,a2)             CLOG2(c, xbt_log_priority_warning, f,a1,a2)
#define CWARN3(c, f,a1,a2,a3)          CLOG3(c, xbt_log_priority_warning, f,a1,a2,a3)
#define CWARN4(c, f,a1,a2,a3,a4)       CLOG4(c, xbt_log_priority_warning, f,a1,a2,a3,a4)
#define CWARN5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, xbt_log_priority_warning, f,a1,a2,a3,a4,a5)
#define CWARN6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6)
#define CWARN7(c, f,a1,a2,a3,a4,a5,a6,a7) CLOG7(c, xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6,a7)
#define CWARN8(c, f,a1,a2,a3,a4,a5,a6,a7,a8) CLOG8(c, xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define CWARN9(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9) CLOG9(c, xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the WARN priority on the specified category with these args (CWARNn exists for any n<10).
 */
#define CWARN10(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) CLOG10(c, xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,10)

#define CERROR0(c, f)                   CLOG0(c, xbt_log_priority_error, f)
#define CERROR1(c, f,a1)                CLOG1(c, xbt_log_priority_error, f,a1)
#define CERROR2(c, f,a1,a2)             CLOG2(c, xbt_log_priority_error, f,a1,a2)
#define CERROR3(c, f,a1,a2,a3)          CLOG3(c, xbt_log_priority_error, f,a1,a2,a3)
#define CERROR4(c, f,a1,a2,a3,a4)       CLOG4(c, xbt_log_priority_error, f,a1,a2,a3,a4)
#define CERROR5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, xbt_log_priority_error, f,a1,a2,a3,a4,a5)
#define CERROR6(c, f,a1,a2,a3,a4,a5,a6) CLOG6(c, xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6)
#define CERROR7(c, f,a1,a2,a3,a4,a5,a6,a7) CLOG7(c, xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6,a7)
#define CERROR8(c, f,a1,a2,a3,a4,a5,a6,a7,a8) CLOG8(c, xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define CERROR9(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9) CLOG9(c, xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the ERROR priority on the specified category with these args (CERRORn exists for any n<10).
 */
#define CERROR10(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) CLOG10(c, xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#define CCRITICAL0(c, f)                   CLOG0(c, xbt_log_priority_critical, f)
#define CCRITICAL1(c, f,a1)                CLOG1(c, xbt_log_priority_critical, f,a1)
#define CCRITICAL2(c, f,a1,a2)             CLOG2(c, xbt_log_priority_critical, f,a1,a2)
#define CCRITICAL3(c, f,a1,a2,a3)          CLOG3(c, xbt_log_priority_critical, f,a1,a2,a3)
#define CCRITICAL4(c, f,a1,a2,a3,a4)       CLOG4(c, xbt_log_priority_critical, f,a1,a2,a3,a4)
#define CCRITICAL5(c, f,a1,a2,a3,a4,a5)    CLOG5(c, xbt_log_priority_critical, f,a1,a2,a3,a4,a5)
#define CCRITICAL6(c, f,a1,a2,a3,a4,a5,a6)    CLOG6(c, xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6)
#define CCRITICAL7(c, f,a1,a2,a3,a4,a5,a6,a7)    CLOG7(c, xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6,a7)
#define CCRITICAL8(c, f,a1,a2,a3,a4,a5,a6,a7,a8)    CLOG8(c, xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define CCRITICAL9(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)    CLOG9(c, xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the CRITICAL priority on the specified category with these args (CCRITICALn exists for any n<10).
 */
#define CCRITICAL10(c, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)    CLOG10(c, xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#ifdef XBT_LOG_MAYDAY
# define LOG0(p, f)                   fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__)
# define LOG1(p, f,a1)                fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1)
# define LOG2(p, f,a1,a2)             fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2)
# define LOG3(p, f,a1,a2,a3)          fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3)
# define LOG4(p, f,a1,a2,a3,a4)       fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4)
# define LOG5(p, f,a1,a2,a3,a4,a5)    fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5)
# define LOG6(p, f,a1,a2,a3,a4,a5,a6) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6)
# define LOG7(p, f,a1,a2,a3,a4,a5,a6,a7) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6,a7)
# define LOG8(p, f,a1,a2,a3,a4,a5,a6,a7,a8) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6,a7,a8)
# define LOG9(p, f,a1,a2,a3,a4,a5,a6,a7,a8,a9) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6,a7,a8,a9)
# define LOG10(p, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) fprintf(stderr,"%s:%d:" f "\n",__FILE__,__LINE__,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)
#else
# define LOG0(p, f)                   _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f _XBT_LOG_POST
# define LOG1(p, f,a1)                _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1 _XBT_LOG_POST
# define LOG2(p, f,a1,a2)             _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1,a2 _XBT_LOG_POST
# define LOG3(p, f,a1,a2,a3)          _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1,a2,a3 _XBT_LOG_POST
# define LOG4(p, f,a1,a2,a3,a4)       _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1,a2,a3,a4 _XBT_LOG_POST
# define LOG5(p, f,a1,a2,a3,a4,a5)    _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1,a2,a3,a4,a5 _XBT_LOG_POST
# define LOG6(p, f,a1,a2,a3,a4,a5,a6) _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1,a2,a3,a4,a5,a6 _XBT_LOG_POST
# define LOG7(p, f,a1,a2,a3,a4,a5,a6,a7) _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1,a2,a3,a4,a5,a6,a7 _XBT_LOG_POST
# define LOG8(p, f,a1,a2,a3,a4,a5,a6,a7,a8) _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1,a2,a3,a4,a5,a6,a7,a8 _XBT_LOG_POST
# define LOG9(p, f,a1,a2,a3,a4,a5,a6,a7,a8,a9) _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1,a2,a3,a4,a5,a6,a7,a8,a9 _XBT_LOG_POST
# define LOG10(p,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) _XBT_LOG_PRE((*_XBT_LOGV(default)),p) ,f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10 _XBT_LOG_POST
#endif

#define DEBUG0(f)                   LOG0(xbt_log_priority_debug, f)
#define DEBUG1(f,a1)                LOG1(xbt_log_priority_debug, f,a1)
#define DEBUG2(f,a1,a2)             LOG2(xbt_log_priority_debug, f,a1,a2)
#define DEBUG3(f,a1,a2,a3)          LOG3(xbt_log_priority_debug, f,a1,a2,a3)
#define DEBUG4(f,a1,a2,a3,a4)       LOG4(xbt_log_priority_debug, f,a1,a2,a3,a4)
#define DEBUG5(f,a1,a2,a3,a4,a5)    LOG5(xbt_log_priority_debug, f,a1,a2,a3,a4,a5)
#define DEBUG6(f,a1,a2,a3,a4,a5,a6) LOG6(xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6)
#define DEBUG7(f,a1,a2,a3,a4,a5,a6,a7) LOG7(xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6,a7)
#define DEBUG8(f,a1,a2,a3,a4,a5,a6,a7,a8) LOG8(xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define DEBUG9(f,a1,a2,a3,a4,a5,a6,a7,a8,a9) LOG9(xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 * \param f the format string
 * \param a1 first argument of the format
 * \param a2 second argument of the format
 * \param a3 third argument of the format
 * \param a4 fourth argument of the format
 * \param a5 fifth argument of the format
 * \param a6 sixth argument of the format
 * \param a7 seventh argument of the format
 * \param a8 eighth argument of the format
 * \param a9 ninth argument of the format
 * \param a10 tenth argument of the format
 *  @brief Log an event at the DEBUG priority on the default category with these args (DEBUGn exists for any n<10)
 */
#define DEBUG10(f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) LOG10(xbt_log_priority_debug, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#define VERB0(f)                   LOG0(xbt_log_priority_verbose, f)
#define VERB1(f,a1)                LOG1(xbt_log_priority_verbose, f,a1)
#define VERB2(f,a1,a2)             LOG2(xbt_log_priority_verbose, f,a1,a2)
#define VERB3(f,a1,a2,a3)          LOG3(xbt_log_priority_verbose, f,a1,a2,a3)
#define VERB4(f,a1,a2,a3,a4)       LOG4(xbt_log_priority_verbose, f,a1,a2,a3,a4)
#define VERB5(f,a1,a2,a3,a4,a5)    LOG5(xbt_log_priority_verbose, f,a1,a2,a3,a4,a5)
#define VERB6(f,a1,a2,a3,a4,a5,a6) LOG6(xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6)
#define VERB7(f,a1,a2,a3,a4,a5,a6,a7) LOG7(xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6,a7)
#define VERB8(f,a1,a2,a3,a4,a5,a6,a7,a8) LOG8(xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define VERB9(f,a1,a2,a3,a4,a5,a6,a7,a8,a9) LOG9(xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the VERB priority on the default category with these args (VERBn exists for any n<10).
 */
#define VERB10(f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) LOG10(xbt_log_priority_verbose, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#define INFO0(f)                   LOG0(xbt_log_priority_info, f)
#define INFO1(f,a1)                LOG1(xbt_log_priority_info, f,a1)
#define INFO2(f,a1,a2)             LOG2(xbt_log_priority_info, f,a1,a2)
#define INFO3(f,a1,a2,a3)          LOG3(xbt_log_priority_info, f,a1,a2,a3)
#define INFO4(f,a1,a2,a3,a4)       LOG4(xbt_log_priority_info, f,a1,a2,a3,a4)
#define INFO5(f,a1,a2,a3,a4,a5)    LOG5(xbt_log_priority_info, f,a1,a2,a3,a4,a5)
#define INFO6(f,a1,a2,a3,a4,a5,a6) LOG6(xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6)
#define INFO7(f,a1,a2,a3,a4,a5,a6,a7) LOG7(xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6,a7)
#define INFO8(f,a1,a2,a3,a4,a5,a6,a7,a8) LOG8(xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define INFO9(f,a1,a2,a3,a4,a5,a6,a7,a8,a9) LOG9(xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the INFO priority on the default category with these args (INFOn exists for any n<10).
 */
#define INFO10(f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) LOG10(xbt_log_priority_info, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#define WARN0(f)                   LOG0(xbt_log_priority_warning, f)
#define WARN1(f,a1)                LOG1(xbt_log_priority_warning, f,a1)
#define WARN2(f,a1,a2)             LOG2(xbt_log_priority_warning, f,a1,a2)
#define WARN3(f,a1,a2,a3)          LOG3(xbt_log_priority_warning, f,a1,a2,a3)
#define WARN4(f,a1,a2,a3,a4)       LOG4(xbt_log_priority_warning, f,a1,a2,a3,a4)
#define WARN5(f,a1,a2,a3,a4,a5)    LOG5(xbt_log_priority_warning, f,a1,a2,a3,a4,a5)
#define WARN6(f,a1,a2,a3,a4,a5,a6) LOG6(xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6)
#define WARN7(f,a1,a2,a3,a4,a5,a6,a7) LOG7(xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6,a7)
#define WARN8(f,a1,a2,a3,a4,a5,a6,a7,a8) LOG8(xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define WARN9(f,a1,a2,a3,a4,a5,a6,a7,a8,a9) LOG9(xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the WARN priority on the default category with these args (WARNn exists for any n<10).
 */
#define WARN10(f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) LOG10(xbt_log_priority_warning, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)


#define ERROR0(f)                   LOG0(xbt_log_priority_error, f)
#define ERROR1(f,a1)                LOG1(xbt_log_priority_error, f,a1)
#define ERROR2(f,a1,a2)             LOG2(xbt_log_priority_error, f,a1,a2)
#define ERROR3(f,a1,a2,a3)          LOG3(xbt_log_priority_error, f,a1,a2,a3)
#define ERROR4(f,a1,a2,a3,a4)       LOG4(xbt_log_priority_error, f,a1,a2,a3,a4)
#define ERROR5(f,a1,a2,a3,a4,a5)    LOG5(xbt_log_priority_error, f,a1,a2,a3,a4,a5)
#define ERROR6(f,a1,a2,a3,a4,a5,a6) LOG6(xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6)
#define ERROR7(f,a1,a2,a3,a4,a5,a6,a7) LOG7(xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6,a7)
#define ERROR8(f,a1,a2,a3,a4,a5,a6,a7,a8) LOG8(xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define ERROR9(f,a1,a2,a3,a4,a5,a6,a7,a8,a9) LOG9(xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the ERROR priority on the default category with these args (ERRORn exists for any n<10).
 */
#define ERROR10(f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) LOG10(xbt_log_priority_error, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#define CRITICAL0(f)                   LOG0(xbt_log_priority_critical, f)
#define CRITICAL1(f,a1)                LOG1(xbt_log_priority_critical, f,a1)
#define CRITICAL2(f,a1,a2)             LOG2(xbt_log_priority_critical, f,a1,a2)
#define CRITICAL3(f,a1,a2,a3)          LOG3(xbt_log_priority_critical, f,a1,a2,a3)
#define CRITICAL4(f,a1,a2,a3,a4)       LOG4(xbt_log_priority_critical, f,a1,a2,a3,a4)
#define CRITICAL5(f,a1,a2,a3,a4,a5)    LOG5(xbt_log_priority_critical, f,a1,a2,a3,a4,a5)
#define CRITICAL6(f,a1,a2,a3,a4,a5,a6) LOG6(xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6)
#define CRITICAL7(f,a1,a2,a3,a4,a5,a6,a7) LOG7(xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6,a7)
#define CRITICAL8(f,a1,a2,a3,a4,a5,a6,a7,a8) LOG8(xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6,a7,a8)
#define CRITICAL9(f,a1,a2,a3,a4,a5,a6,a7,a8,a9) LOG9(xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6,a7,a8,a9)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log an event at the CRITICAL priority on the default category with these args (CRITICALn exists for any n<10).
 */
#define CRITICAL10(f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) LOG10(xbt_log_priority_critical, f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority that we entered in current function.
 */
#define XBT_IN               LOG1(xbt_log_priority_trace, ">> begin of %s",     _XBT_FUNCTION)
#define XBT_IN1(fmt,a)       LOG2(xbt_log_priority_trace, ">> begin of %s" fmt, _XBT_FUNCTION, a)
#define XBT_IN2(fmt,a,b)     LOG3(xbt_log_priority_trace, ">> begin of %s" fmt, _XBT_FUNCTION, a,b)
#define XBT_IN3(fmt,a,b,c)   LOG4(xbt_log_priority_trace, ">> begin of %s" fmt, _XBT_FUNCTION, a,b,c)
#define XBT_IN4(fmt,a,b,c,d) LOG5(xbt_log_priority_trace, ">> begin of %s" fmt, _XBT_FUNCTION, a,b,c,d)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority that we entered in current function, appending a user specified format taking 5 args (XBT_INn exists for all n in [1,6])
 */
#define XBT_IN5(fmt,a,b,c,d,e) LOG6(xbt_log_priority_trace, ">> begin of %s" fmt, _XBT_FUNCTION, a,b,c,d,e)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority that we exited the current function.
 */
#define XBT_OUT              LOG1(xbt_log_priority_trace, "<< end of %s",       _XBT_FUNCTION)
/** @ingroup XBT_log
 *  @hideinitializer
 *  @brief Log at TRACE priority a message indicating that we reached that point.
 */
#define XBT_HERE             LOG0(xbt_log_priority_trace, "-- was here")


SG_END_DECL()
#endif /* ! _XBT_LOG_H_ */
