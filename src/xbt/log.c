/* $Id$ */

/* log - a generic logging facility in the spirit of log4j                  */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include <stdarg.h>
#include <ctype.h>
#include <stdio.h> /* snprintf */
#include <stdlib.h> /* snprintf */
#include "gras_config.h" /* to get a working stdarg.h */
#include "portable.h" /* to get a working stdarg.h */

#include "xbt_modinter.h"

#include "xbt/misc.h"
#include "xbt/ex.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"

/** \addtogroup XBT_log
 *
 *  This section describes the API to the log functions used 
 *  everywhere in this project.

\section XBT_log_toc Table of contents
 
 - \ref log_overview
   - \ref log_cat
   - \ref log_pri
   - \ref log_app
   - \ref log_hist
 - \ref log_API
   - \ref log_API_cat
   - \ref log_API_pri
   - \ref log_API_subcat
   - \ref log_API_easy
   - \ref log_API_example
 - \ref log_user
   - \ref log_use_conf
   - \ref log_use_misc
 - \ref log_internals
   - \ref log_in_perf
   - \ref log_in_app
 - \ref XBT_log_cats
     
\section log_overview 1. Introduction

This module is in charge of handling the log messages of every SimGrid
program. The main design goal are:

  - <b>configurability</b>: the user can choose <i>at runtime</i> what messages to show and 
    what to hide, as well as how messages get displayed.
  - <b>ease of use</b>: both to the programmer (using preprocessor macros black magic)
    and to the user (with command line options)
  - <b>performances</b>: logging shouldn't slow down the program when turned off, for example
  - deal with <b>distributed settings</b>: SimGrid programs are [often] distributed ones, 
    and the logging mecanism allows to syndicate each and every log source into the same place.
    At least, its design would allow to, once we write the last missing pieces
     
There is three main concepts in SimGrid's logging mecanism: <i>category</i>,
<i>priority</i> and <i>appender</i>. These three concepts work together to
enable developers to log messages according to message type and priority, and
to control at runtime how these messages are formatted and where they are
reported. 

\subsection log_cat 1.1 Category hierarchy

The first and foremost advantage of any logging API over plain printf()
resides in its ability to disable certain log statements while allowing
others to print unhindered. This capability assumes that the logging space,
that is, the space of all possible logging statements, is categorized
according to some developer-chosen criteria. 
	  
This observation led to choosing category as the central concept of the
system. In a certain sense, they can be considered as logging topics or
channels.

\subsection log_pri 1.2 Logging priorities

The user can naturally declare interest into this or that logging category, but
he also can specify the desired level of details for each of them. This is
controled by the <i>priority</i> concept (which should maybe be renamed to
<i>severity</i>). 

Empirically, the user can specify that he wants to see every debuging message
of GRAS while only being interested into the messages at level "error" or
higher about the XBT internals.

\subsection log_app 1.3 Message appenders

The message appenders are the elements in charge of actually displaying the
message to the user. For now, there is only one appender: the one able to print
stuff on stderr. But everything is in place internally to write new ones, such
as the one able to send the strings to a central server in charge of
syndicating the logs of every distributed daemons on a well known location.

It should also be possible to pass configuration informations to the appenders,
specifying for example that the message location (file and line number) is only
relevant to debugging information, not to critical error messages.

One day, for sure ;)

\subsection log_hist 1.4 History of this module

Historically, this module is an adaptation of the log4c project, which is dead
upstream, and which I was given the permission to fork under the LGPL licence
by the log4c's authors. The log4c project itself was loosely based on the
Apache project's Log4J, which also inspired Log4CC, Log4py and so on. Our work
differs somehow from these projects anyway, because the C programming language
is not object oriented.

\section log_API 2. Programmer interface

\subsection log_API_cat 2.1 Constructing the category hierarchy

Every category is declared by providing a name and an optional
parent. If no parent is explicitly named, the root category, LOG_ROOT_CAT is
the category's parent. 
      
A category is created by a macro call at the top level of a file.  A
category can be created with any one of the following macros:

 - \ref XBT_LOG_NEW_CATEGORY(MyCat,desc); Create a new root
 - \ref XBT_LOG_NEW_SUBCATEGORY(MyCat, ParentCat,desc);
    Create a new category being child of the category ParentCat
 - \ref XBT_LOG_NEW_DEFAULT_CATEGORY(MyCat,desc);
    Like XBT_LOG_NEW_CATEGORY, but the new category is the default one
      in this file
 -  \ref XBT_LOG_NEW_DEFAULT_SUBCATEGORY(MyCat, ParentCat,desc);
    Like XBT_LOG_NEW_SUBCATEGORY, but the new category is the default one
      in this file
	    
The parent cat can be defined in the same file or in another file (in
which case you want to use the \ref XBT_LOG_EXTERNAL_CATEGORY macro to make
it visible in the current file), but each category may have only one
definition.
      
Typically, there will be a Category for each module and sub-module, so you
can independently control logging for each module.

For a list of all existing categories, please refer to the \ref XBT_log_cats
section. This file is generated automatically from the SimGrid source code, so
it should be complete and accurate.

\section log_API_pri 2.2 Declaring message priority

A category may be assigned a threshold priorty. The set of priorites are
defined by the \ref e_xbt_log_priority_t enum. All logging request under
this priority will be discarded.
	  
If a given category is not assigned a threshold priority, then it inherits
one from its closest ancestor with an assigned threshold. To ensure that all
categories can eventually inherit a threshold, the root category always has
an assigned threshold priority.

Logging requests are made by invoking a logging macro on a category.  All of
the macros have a printf-style format string followed by arguments. If you
compile with the -Wall option, gcc will warn you for unmatched arguments, ie
when you pass a pointer to a string where an integer was specified by the
format. This is usualy a good idea.

Because some C compilers do not support vararg macros, there is a version of
the macro for any number of arguments from 0 to 6. The macro name ends with
the total number of arguments.
	
Here is an example of the most basic type of macro. This is a logging
request with priority <i>warning</i>.

<code>CLOG5(MyCat, gras_log_priority_warning, "Values are: %d and '%s'", 5,
"oops");</code>

A logging request is said to be enabled if its priority is higher than or
equal to the threshold priority of its category. Otherwise, the request is
said to be disabled. A category without an assigned priority will inherit
one from the hierarchy. 
      
It is possible to use any non-negative integer as a priority. If, as in the
example, one of the standard priorites is used, then there is a convenience
macro that is typically used instead. For example, the above example is
equivalent to the shorter:

<code>CWARN4(MyCat, "Values are: %d and '%s'", 5, "oops");</code>

\section log_API_subcat 2.3 Using a default category (the easy interface)
  
If \ref XBT_LOG_NEW_DEFAULT_SUBCATEGORY(MyCat, Parent) or
\ref XBT_LOG_NEW_DEFAULT_CATEGORY(MyCat) is used to create the
category, then the even shorter form can be used:

<code>WARN3("Values are: %d and '%s'", 5, "oops");</code>

Only one default category can be created per file, though multiple
non-defaults can be created and used.

\section log_API_easy 2.4 Putting all together: the easy interface

First of all, each module should register its own category into the categories
tree using \ref XBT_LOG_NEW_DEFAULT_SUBCATEGORY.

Then, logging should be done with the DEBUG<n>, VERB<n>, INFO<n>, WARN<n>,
ERROR<n> or CRITICAL<n> macro families. For each group, there is 6 different
macros (like DEBUG0, DEBUG1, DEBUG2, DEBUG3, DEBUG4 and DEBUG5), only differing
in the number of arguments passed along the format. This is because we want
SimGrid itself to keep compilable on ancient compiler not supporting variable
number of arguments to macros. But we should provide a macro simpler to use for
the users not interested in SP3 machines (FIXME).

Under GCC, these macro check there arguments the same way than printf does. So,
if you compile with -Wall, the folliwing code will issue a warning:
<code>DEBUG2("Found %s (id %f)", some_string, a_double)</code>

\section log_API_example 2.5 Example of use

Here is a more complete example:

\verbatim
#include "xbt/log.h"

/ * create a category and a default subcategory * /
XBT_LOG_NEW_CATEGORY(VSS);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(SA, VSS);

int main() {
       / * Now set the parent's priority.  (the string would typcially be a runtime option) * /
       xbt_log_control_set("SA.thresh=3");

       / * This request is enabled, because WARNING &gt;= INFO. * /
       CWARN2(VSS, "Low fuel level.");

       / * This request is disabled, because DEBUG &lt; INFO. * /
       CDEBUG2(VSS, "Starting search for nearest gas station.");

       / * The default category SA inherits its priority from VSS. Thus,
          the following request is enabled because INFO &gt;= INFO.  * /
       INFO1("Located nearest gas station.");

       / * This request is disabled, because DEBUG &lt; INFO. * /
       DEBUG1("Exiting gas station search"); 
}
\endverbatim


\section log_user 3. User interface

\section log_use_conf 3.1 Configuration
Configuration is typically done during program initialization by invoking
the xbt_log_control_set() method. The control string passed to it typically
comes from the command line. Look at the documentation for that function for
the format of the control string.

Any SimGrid program can furthermore be configured at run time by passing a
--xbt-log argument on the command line (--gras-log, --msg-log and --surf-log
are synonyms provided by aestheticism). You can provide several of those
arguments to change the setting of several categories, they will be applied
from left to right. So, 
\verbatim --xbt-log="root.thres=debug root.thres=critical"\endverbatim 
should disable any logging.

Note that the quotes on above line are mandatory because there is a space in
the argument, so we are protecting ourselves from the shell, not from SimGrid.
We could also reach the same effect with this:
\verbatim --xbt-log=root.thres=debug --xbt-log=root.thres=critical\endverbatim 

\section log_use_misc 3.2 Misc and Caveats

  - Do not use any of the macros that start with '_'.
  - Log4J has a 'rolling file appender' which you can select with a run-time
    option and specify the max file size. This would be a nice default for
    non-kernel applications.
  - Careful, category names are global variables.

\section log_internals 4. Internal considerations

This module is a mess of macro black magic, and when it goes wrong, SimGrid
studently loose its ability to explain its problems. When messing around this
module, I often find useful to define XBT_LOG_MAYDAY (which turns it back to
good old printf) for the time of finding what's going wrong.

\section log_in_perf 4.1 Performance

Except for the first invocation of a given category, a disabled logging request
requires an a single comparison of a static variable to a constant.

There is also compile time constant, \ref XBT_LOG_STATIC_THRESHOLD, which
causes all logging requests with a lower priority to be optimized to 0 cost
by the compiler. By setting it to gras_log_priority_infinite, all logging
requests are statically disabled and cost nothing. Released executables
might be compiled with
\verbatim-DXBT_LOG_STATIC_THRESHOLD=gras_log_priority_infinite\endverbatim

Compiling with the \verbatim-DNLOG\endverbatim option disables all logging 
requests at compilation time while the \verbatim-DNDEBUG\endverbatim disables 
the requests of priority below INFO.

\todo Logging performance *may* be improved further by improving the message
propagation from appender to appender in the category tree.

\section log_in_app 4.2 Appenders

Each category has an optional appender. An appender is a pointer to a
structure which starts with a pointer to a doAppend() function. DoAppend()
prints a message to a log.

When a category is passed a message by one of the logging macros, the
category performs the following actions:

  - if the category has an appender, the message is passed to the
    appender's doAppend() function,
  - if 'willLogToParent' is true for the category, the message is passed
    to the category's parent.
    
By default, only the root category have an appender, and 'willLogToParent'
is true for any other category. This situation causes all messages to be
logged by the root category's appender.

The default appender function currently prints to stderr, and no other one
exist, even if more would be needed, like the one able to send the logs to a
remote dedicated server, or other ones offering different output formats.
This is on our TODO list for quite a while now, but your help would be
welcome here, too.


*/


typedef struct {
  char *catname;
  e_xbt_log_priority_t thresh;
} s_xbt_log_setting_t,*xbt_log_setting_t;

static xbt_dynar_t xbt_log_settings=NULL;
static void _free_setting(void *s) {
  xbt_log_setting_t set=(xbt_log_setting_t)s;
  if (set) {
    free(set->catname);
/*    free(set); FIXME: uncommenting this leads to segfault when more than one chunk is passed as gras-log */
  }
}

const char *xbt_log_priority_names[8] = {
  "NONE",
  "TRACE",
  "DEBUG",
  "VERBOSE",
  "INFO",
  "WARNING",
  "ERROR",
  "CRITICAL"
};

s_xbt_log_category_t _XBT_LOGV(XBT_LOG_ROOT_CAT) = {
  0, 0, 0,
  "root", xbt_log_priority_uninitialized, 0,
  NULL, 0
};

XBT_LOG_NEW_CATEGORY(xbt,"All XBT categories (simgrid toolbox)");
XBT_LOG_NEW_CATEGORY(surf,"All SURF categories");
XBT_LOG_NEW_CATEGORY(msg,"All MSG categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(log,xbt,"Loggings from the logging mechanism itself");

void xbt_log_init(int *argc,char **argv) {
  int i,j;
  char *opt;

  /* Set logs and init log submodule */
  for (i=1; i<*argc; i++) {
    if (!strncmp(argv[i],"--gras-log=",strlen("--gras-log=")) ||
	!strncmp(argv[i],"--surf-log=",strlen("--surf-log=")) ||
	!strncmp(argv[i],"--msg-log=",strlen("--msg-log=")) ||
	!strncmp(argv[i],"--xbt-log=",strlen("--xbt-log="))) {
      opt=strchr(argv[i],'=');
      opt++;
      xbt_log_control_set(opt);
      DEBUG1("Did apply '%s' as log setting",opt);
      /*remove this from argv*/
      for (j=i+1; j<*argc; j++) {
	argv[j-1] = argv[j];
      } 
      argv[j-1] = NULL;
      (*argc)--;
      i--; /* compensate effect of next loop incrementation */
    }
  }
}

void xbt_log_exit(void) {
  VERB0("Exiting log");
  xbt_dynar_free(&xbt_log_settings);
  VERB0("Exited log");
}

static void _apply_control(xbt_log_category_t cat) {
  int cursor;
  xbt_log_setting_t setting=NULL;
  int found = 0;

  if (!xbt_log_settings)
    return;

  xbt_assert0(cat,"NULL category");
  xbt_assert(cat->name);

  xbt_dynar_foreach(xbt_log_settings,cursor,setting) {
    xbt_assert0(setting,"Damnit, NULL cat in the list");
    xbt_assert1(setting->catname,"NULL setting(=%p)->catname",(void*)setting);

    if (!strcmp(setting->catname,cat->name)) {
      found = 1;

      xbt_log_threshold_set(cat, setting->thresh);
      xbt_dynar_cursor_rm(xbt_log_settings,&cursor);

      if (cat->threshold <= xbt_log_priority_debug) {
	s_xbt_log_event_t _log_ev = 
	  {cat,xbt_log_priority_debug,__FILE__,_XBT_FUNCTION,__LINE__};
	_xbt_log_event_log(&_log_ev,
	         "Apply settings for category '%s': set threshold to %s (=%d)",
		 cat->name, 
	         xbt_log_priority_names[cat->threshold], cat->threshold);
      }
    }
  }
  if (!found && cat->threshold <= xbt_log_priority_verbose) {
    s_xbt_log_event_t _log_ev = 
      {cat,xbt_log_priority_verbose,__FILE__,_XBT_FUNCTION,__LINE__};
    _xbt_log_event_log(&_log_ev,
			"Category '%s': inherited threshold = %s (=%d)",
			cat->name,
			xbt_log_priority_names[cat->threshold], cat->threshold);
  }

}

void _xbt_log_event_log( xbt_log_event_t ev, const char *fmt, ...) {
  xbt_log_category_t cat = ev->cat;
  va_start(ev->ap, fmt);
  while(1) {
    xbt_log_appender_t appender = cat->appender;
    if (appender != NULL) {
      appender->do_append(appender, ev, fmt);
    }
    if (!cat->willLogToParent)
      break;

    cat = cat->parent;
  } 
  va_end(ev->ap);
}

static void _cat_init(xbt_log_category_t category) {
  if (category == &_XBT_LOGV(XBT_LOG_ROOT_CAT)) {
    category->threshold = xbt_log_priority_info;
    category->appender = xbt_log_default_appender;
  } else {
    xbt_log_parent_set(category, category->parent);
  }
  _apply_control(category);
}

/*
 * This gets called the first time a category is referenced and performs the
 * initialization. 
 * Also resets threshold to inherited!
 */
int _xbt_log_cat_init(e_xbt_log_priority_t priority,
		       xbt_log_category_t   category) {
    
  _cat_init(category);
        
  return priority >= category->threshold;
}

void xbt_log_parent_set(xbt_log_category_t cat,
			 xbt_log_category_t parent) {

  xbt_assert0(cat,"NULL category to be given a parent");
  xbt_assert1(parent,"The parent category of %s is NULL",cat->name);

  /* unlink from current parent */
  if (cat->threshold != xbt_log_priority_uninitialized) {
    xbt_log_category_t* cpp = &parent->firstChild;
    while(*cpp != cat && *cpp != NULL) {
      cpp = &(*cpp)->nextSibling;
    }
    xbt_assert(*cpp == cat);
    *cpp = cat->nextSibling;
  }

  /* Set new parent */
  cat->parent = parent;
  cat->nextSibling = parent->firstChild;
  parent->firstChild = cat;

  /* Make sure parent is initialized */
  if (parent->threshold == xbt_log_priority_uninitialized) {
    _cat_init(parent);
  }
    
  /* Reset priority */
  cat->threshold = parent->threshold;
  cat->isThreshInherited = 1;
} /* log_setParent */

static void _set_inherited_thresholds(xbt_log_category_t cat) {
  xbt_log_category_t child = cat->firstChild;
  for( ; child != NULL; child = child->nextSibling) {
    if (child->isThreshInherited) {
      if (cat != &_XBT_LOGV(log))
	VERB3("Set category threshold of %s to %s (=%d)",
	      child->name,xbt_log_priority_names[cat->threshold],cat->threshold);
      child->threshold = cat->threshold;
      _set_inherited_thresholds(child);
    }
  }
}

void xbt_log_threshold_set(xbt_log_category_t   cat,
			    e_xbt_log_priority_t threshold) {
  cat->threshold = threshold;
  cat->isThreshInherited = 0;
  _set_inherited_thresholds(cat);
}

static void _xbt_log_parse_setting(const char*        control_string,
				    xbt_log_setting_t set) {
  const char *name, *dot, *eq;
  
  set->catname=NULL;
  if (!*control_string) 
    return;
  DEBUG1("Parse log setting '%s'",control_string);

  control_string += strspn(control_string, " ");
  name = control_string;
  control_string += strcspn(control_string, ".= ");
  dot = control_string;
  control_string += strcspn(control_string, "= ");
  eq = control_string;
  control_string += strcspn(control_string, " ");

  xbt_assert1(*dot == '.' && *eq == '=',
	       "Invalid control string '%s'",control_string);

  if (!strncmp(dot + 1, "thresh", min((size_t)(eq - dot - 1),strlen("thresh")))) {
    int i;
    char *neweq=xbt_strdup(eq+1);
    char *p=neweq-1;
    
    while (*(++p) != '\0') {
      if (*p >= 'a' && *p <= 'z') {
	*p-='a'-'A';
      }
    }
    
    DEBUG1("New priority name = %s",neweq);
    for (i=0; i<xbt_log_priority_infinite-1; i++) {
      if (!strncmp(xbt_log_priority_names[i],neweq,p-eq)) {
	DEBUG1("This is priority %d",i);
	break;
      }
    }
    if (i<xbt_log_priority_infinite-1) {
      set->thresh= (e_xbt_log_priority_t) i;
    } else {
      xbt_assert1(FALSE,"Unknown priority name: %s",eq+1);
    }
    free(neweq);
  } else {
    char buff[512];
    snprintf(buff,min(512,eq - dot - 1),"%s",dot+1);
    xbt_assert1(FALSE,"Unknown setting of the log category: %s",buff);
  }
  set->catname=(char*)xbt_malloc(dot - name+1);
    
  strncpy(set->catname,name,dot-name);
  set->catname[dot-name]='\0'; /* Just in case */
  DEBUG1("This is for cat '%s'", set->catname);
}

static xbt_log_category_t _xbt_log_cat_searchsub(xbt_log_category_t cat,char *name) {
  xbt_log_category_t child;
  
  if (!strcmp(cat->name,name)) {
    return cat;
  }
  for(child=cat->firstChild ; child != NULL; child = child->nextSibling) {
    return _xbt_log_cat_searchsub(child,name);
  }
  THROW0(not_found_error,0,"No such category");
}

static void _cleanup_double_spaces(char *s) {
  char *p = s;
  int   e = 0;
  
  while (1) {
    if (!*p)
      goto end;
    
    if (!isspace(*p))
      break;
    
    p++;
  }
  
  e = 1;
  
  do {
    if (e)
      *s++ = *p;
    
    if (!*++p)
      goto end;
    
    if (e ^ !isspace(*p))
      if ((e = !e))
	*s++ = ' ';
  } while (1);

 end:
  *s = '\0';
}

/**
 * \ingroup XBT_log  
 * \param control_string What to parse
 *
 * Typically passed a command-line argument. The string has the syntax:
 *
 *      ( [category] "." [keyword] "=" value (" ")... )...
 *
 * where [category] is one the category names (see \ref XBT_log_cats for a complete list) 
 * and keyword is one of the following:
 *
 *    - thres: category's threshold priority. Possible values:
 *             TRACE,DEBUG,VERBOSE,INFO,WARNING,ERROR,CRITICAL
 *             
 *
 * \warning
 * This routine may only be called once and that must be before any other
 * logging command! Typically, this is done from main().
 * \todo the previous warning seems a bit old and need double checking
 */
void xbt_log_control_set(const char* control_string) {
  xbt_log_setting_t set;
  char *cs;
  char *p;
  int done = 0;
  
  DEBUG1("Parse log settings '%s'",control_string);
  if (control_string == NULL)
    return;
  if (xbt_log_settings == NULL)
    xbt_log_settings = xbt_dynar_new(sizeof(xbt_log_setting_t),
				       _free_setting);

  set = xbt_new(s_xbt_log_setting_t,1);
  cs=xbt_strdup(control_string);

  _cleanup_double_spaces(cs);

  while (!done) {
    xbt_log_category_t cat=NULL;
    int found=0;
    xbt_ex_t e;
    
    p=strrchr(cs,' ');
    if (p) {
      *p='\0';
      *p++;
    } else {
      p=cs;
      done = 1;
    }
    _xbt_log_parse_setting(p,set);

    TRY {
      cat = _xbt_log_cat_searchsub(&_XBT_LOGV(root),set->catname);
      found = 1;
    } CATCH(e) {
      if (e.category != not_found_error)
	RETHROW;
      xbt_ex_free(&e);
      found = 0;

      DEBUG0("Store for further application");
      DEBUG1("push %p to the settings",(void*)set);
      xbt_dynar_push(xbt_log_settings,&set);
      /* malloc in advance the next slot */
      set = xbt_new(s_xbt_log_setting_t,1);
    } 

    if (found) {
      DEBUG0("Apply directly");
      free(set->catname);
      xbt_log_threshold_set(cat,set->thresh);
    }
  }
  free(set);
  free(cs);
} 

void xbt_log_appender_set(xbt_log_category_t cat, xbt_log_appender_t app) {
  cat->appender = app;
}

