/* log - a generic logging facility in the spirit of log4j                  */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>              /* snprintf */
#include <stdlib.h>             /* snprintf */

#include "portable.h"           /* to get a working stdarg.h */

#include "xbt_modinter.h"

#include "xbt/misc.h"
#include "xbt/ex.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"
#include "xbt/log_private.h"
#include "xbt/dynar.h"

XBT_PUBLIC_DATA(int) (*xbt_pid) ();
int xbt_log_no_loc = 0;         /* if set to true (with --log=no_loc), file localization will be omitted (for tesh tests) */

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
   - \ref log_API_isenabled
   - \ref log_API_subcat
   - \ref log_API_easy
   - \ref log_API_example
 - \ref log_user
   - \ref log_use_conf
     - \ref log_use_conf_thres
     - \ref log_use_conf_multi
     - \ref log_use_conf_fmt
     - \ref log_use_conf_app
     - \ref log_use_conf_add
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
    and the logging mechanism allows to syndicate each and every log source into the same place.
    At least, its design would allow to, once we write the last missing pieces

There is three main concepts in SimGrid's logging mechanism: <i>category</i>,
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
controlled by the <i>priority</i> concept (which should maybe be renamed to
<i>severity</i>).

Empirically, the user can specify that he wants to see every debugging message
of GRAS while only being interested into the messages at level "error" or
higher about the XBT internals.

\subsection log_app 1.3 Message appenders

The message appenders are the elements in charge of actually displaying the
message to the user. For now, only two appenders exist: the default one prints
stuff on stderr while it is possible to create appenders printing to a specific
file.

Other are planed (such as the one sending everything to a remote server,
or the one using only a fixed amount of lines in a file, and rotating content on
need). One day, for sure ;)

\subsection log_lay 1.4 Message layouts

The message layouts are the elements in charge of choosing how each message
will look like. Their result is a string which is then passed to the appender
attached to the category to be displayed.

For now, there is two layouts: The simple one, which is good for most cases,
and another one allowing users to specify the format they want.
\ref log_use_conf provides more info on this.

\subsection log_hist 1.5 History of this module

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
definition. Likewise, you can use a category defined in another file as 
default one using \ref XBT_LOG_EXTERNAL_DEFAULT_CATEGORY

Typically, there will be a Category for each module and sub-module, so you
can independently control logging for each module.

For a list of all existing categories, please refer to the \ref XBT_log_cats
section. This file is generated automatically from the SimGrid source code, so
it should be complete and accurate.

\section log_API_pri 2.2 Declaring message priority

A category may be assigned a threshold priority. The set of priorities are
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
format. This is usually a good idea.

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
example, one of the standard priorities is used, then there is a convenience
macro that is typically used instead. For example, the above example is
equivalent to the shorter:

<code>CWARN4(MyCat, "Values are: %d and '%s'", 5, "oops");</code>

\section log_API_isenabled 2.3 Checking if a particular category/priority is enabled

It is sometimes useful to check whether a particular category is
enabled at a particular priority. One example is when you want to do
some extra computation to prepare a nice debugging message. There is
no use of doing so if the message won't be used afterward because
debugging is turned off.

Doing so is extremely easy, thanks to the XBT_LOG_ISENABLED(category, priority).

\section log_API_subcat 2.4 Using a default category (the easy interface)

If \ref XBT_LOG_NEW_DEFAULT_SUBCATEGORY(MyCat, Parent) or
\ref XBT_LOG_NEW_DEFAULT_CATEGORY(MyCat) is used to create the
category, then the even shorter form can be used:

<code>WARN3("Values are: %s and '%d'", 5, "oops");</code>

Only one default category can be created per file, though multiple
non-defaults can be created and used.

\section log_API_easy 2.5 Putting all together: the easy interface

First of all, each module should register its own category into the categories
tree using \ref XBT_LOG_NEW_DEFAULT_SUBCATEGORY.

Then, logging should be done with the DEBUG<n>, VERB<n>, INFO<n>, WARN<n>,
ERROR<n> or CRITICAL<n> macro families (such as #DEBUG10, #VERB10,
#INFO10, #WARN10, #ERROR10 and #CRITICAL10). For each group, there is at
least 11 different macros (like DEBUG0, DEBUG1, DEBUG2, DEBUG3, DEBUG4 and
DEBUG5, DEBUG6, DEBUG7, DEBUG8, DEBUG9, DEBUG10), only differing in the number of arguments passed along the format.
This is because we want SimGrid itself to keep compilable on ancient
compiler not supporting variable number of arguments to macros. But we
should provide a macro simpler to use for the users not interested in SP3
machines (FIXME).

Under GCC, these macro check there arguments the same way than printf does. So,
if you compile with -Wall, the following code will issue a warning:
<code>DEBUG2("Found %s (id %f)", some_string, a_double)</code>

If you want to specify the category to log onto (for example because you
have more than one category per file, add a C before the name of the log
producing macro (ie, use #CDEBUG10, #CVERB10, #CINFO10, #CWARN10, #CERROR10 and
#CCRITICAL10 and friends), and pass the category name as first argument.

The TRACE priority is not used the same way than the other. You should use
the #XBT_IN, XBT_IN<n> (up to #XBT_IN5), #XBT_OUT and #XBT_HERE macros
instead.

\section log_API_example 2.6 Example of use

Here is a more complete example:

\verbatim
#include "xbt/log.h"

/ * create a category and a default subcategory * /
XBT_LOG_NEW_CATEGORY(VSS);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(SA, VSS);

int main() {
       / * Now set the parent's priority.  (the string would typcially be a runtime option) * /
       xbt_log_control_set("SA.thresh:3");

       / * This request is enabled, because WARNING >= INFO. * /
       CWARN0(VSS, "Low fuel level.");

       / * This request is disabled, because DEBUG < INFO. * /
       CDEBUG0(VSS, "Starting search for nearest gas station.");

       / * The default category SA inherits its priority from VSS. Thus,
          the following request is enabled because INFO >= INFO.  * /
       INFO0("Located nearest gas station.");

       / * This request is disabled, because DEBUG < INFO. * /
       DEBUG0("Exiting gas station search");
}
\endverbatim

Another example can be found in the relevant part of the GRAS tutorial:
\ref GRAS_tut_tour_logs.

\section log_user 3. User interface

\section log_use_conf 3.1 Configuration

Although rarely done, it is possible to configure the logs during
program initialization by invoking the xbt_log_control_set() method
manually. A more conventional way is to use the --log command line
argument. xbt_init() (called by MSG_init(), gras_init() and friends)
checks and deals properly with such arguments.

The following command line arguments exist, but are deprecated and
may disappear in the future: --xbt-log, --gras-log, --msg-log and
--surf-log.

\subsection log_use_conf_thres 3.1.1 Threshold configuration

The most common setting is to control which logging event will get
displayed by setting a threshold to each category through the
<tt>thres</tt> keyword.

For example, \verbatim --log=root.thres:debug\endverbatim will make
SimGrid <b>extremely</b> verbose while \verbatim
--log=root.thres:critical\endverbatim should shut it almost
completely off.

\subsection log_use_conf_multi 3.1.2 Passing several settings

You can provide several of those arguments to change the setting of several
categories, they will be applied from left to right. So,
\verbatim --log="root.thres:debug root.thres:critical"\endverbatim should
disable almost any logging.

Note that the quotes on above line are mandatory because there is a space in
the argument, so we are protecting ourselves from the shell, not from SimGrid.
We could also reach the same effect with this:
\verbatim --log=root.thres:debug --log=root.thres:critical\endverbatim

\subsection log_use_conf_fmt 3.1.3 Format configuration

As with SimGrid 3.3, it is possible to control the format of log
messages. This is done through the <tt>fmt</tt> keyword. For example,
\verbatim --log=root.fmt:%m\endverbatim reduces the output to the
user-message only, removing any decoration such as the date, or the
process ID, everything.

Here are the existing format directives:

 - %%: the % char
 - %%n: platform-dependent line separator (LOG4J compatible)
 - %%e: plain old space (SimGrid extension)

 - %%m: user-provided message

 - %%c: Category name (LOG4J compatible)
 - %%p: Priority name (LOG4J compatible)

 - %%h: Hostname (SimGrid extension)
 - %%P: Process name (SimGrid extension)
 - %%t: Thread "name" (LOG4J compatible -- actually the address of the thread in memory)
 - %%i: Process PID (SimGrid extension -- this is a 'i' as in 'i'dea)

 - %%F: file name where the log event was raised (LOG4J compatible)
 - %%l: location where the log event was raised (LOG4J compatible, like '%%F:%%L' -- this is a l as in 'l'etter)
 - %%L: line number where the log event was raised (LOG4J compatible)
 - %%M: function name (LOG4J compatible -- called method name here of course).
   Defined only when using gcc because there is no __FUNCTION__ elsewhere.

 - %%b: full backtrace (Called %%throwable in LOG4J).
   Defined only under windows or when using the GNU libc because backtrace() is not defined
   elsewhere, and we only have a fallback for windows boxes, not mac ones for example.
 - %%B: short backtrace (only the first line of the %%b).
   Called %%throwable{short} in LOG4J; defined where %%b is.

 - %%d: date (UNIX-like epoch)
 - %%r: application age (time elapsed since the beginning of the application)


If you want to mimic the simple layout with the format one, you would use this
format: '[%%h:%%i:(%%i) %%r] %%l: [%%c/%%p] %%m%%n'. This is not completely correct
because the simple layout do not display the message location for messages at
priority INFO (thus, the fmt is '[%%h:%%i:(%%i) %%r] [%%c/%%p] %%m%%n' in this
case). Moreover, if there is no process name (ie, messages coming from the
library itself, or test programs doing strange things) do not display the
process identity (thus, fmt is '[%%r] %%l: [%%c/%%p] %%m%%n' in that case, and '[%%r]
[%%c/%%p] %%m%%n' if they are at priority INFO).

For now, there is only one format modifier: the precision field. You
can for example specify %.4r to get the application age with 4
numbers after the radix. Another limitation is that you cannot set
specific layouts to the several priorities.

\subsection log_use_conf_app 3.1.4 Category appender

As with SimGrid 3.3, it is possible to control the appender of log
messages. This is done through the <tt>app</tt> keyword. For example,
\verbatim --log=root.app:file:mylogfile\endverbatim redirects the output
to the file mylogfile.

Any appender setup this way have its own layout format (simple one by default),
so you may have to change it too afterward. Moreover, the additivity of the log category
is also set to false to prevent log event displayed by this appender to "leak" to any other
appender higher in the hierarchy. If it is not what you wanted, you can naturally change it
manually.

\subsection log_use_conf_add 3.1.5 Category additivity

The <tt>add</tt> keyword allows to specify the additivity of a
category (see \ref log_in_app). '0', '1', 'no', 'yes', 'on'
and 'off' are all valid values, with 'yes' as default.

The following example resets the additivity of the xbt category to true (which is its default value).
\verbatim --log=xbt.add:yes\endverbatim

\section log_use_misc 3.2 Misc and Caveats

  - Do not use any of the macros that start with '_'.
  - Log4J has a 'rolling file appender' which you can select with a run-time
    option and specify the max file size. This would be a nice default for
    non-kernel applications.
  - Careful, category names are global variables.

\section log_internals 4. Internal considerations

This module is a mess of macro black magic, and when it goes wrong,
SimGrid studently loose its ability to explain its problems. When
messing around this module, I often find useful to define
XBT_LOG_MAYDAY (which turns it back to good old printf) for the time
of finding what's going wrong. But things are quite verbose when
everything is enabled...

\section log_in_perf 4.1 Performance

Except for the first invocation of a given category, a disabled logging request
requires an a single comparison of a static variable to a constant.

There is also compile time constant, \ref XBT_LOG_STATIC_THRESHOLD, which
causes all logging requests with a lower priority to be optimized to 0 cost
by the compiler. By setting it to gras_log_priority_infinite, all logging
requests are statically disabled at compile time and cost nothing. Released executables
<i>might</i>  be compiled with (note that it will prevent users to debug their problems)
\verbatim-DXBT_LOG_STATIC_THRESHOLD=gras_log_priority_infinite\endverbatim

Compiling with the \verbatim-DNLOG\endverbatim option disables all logging
requests at compilation time while the \verbatim-DNDEBUG\endverbatim disables
the requests of priority below INFO.

\todo Logging performance *may* be improved further by improving the message
propagation from appender to appender in the category tree.

\section log_in_app 4.2 Appenders

Each category has an optional appender. An appender is a pointer to a
structure which starts with a pointer to a do_append() function. do_append()
prints a message to a log.

When a category is passed a message by one of the logging macros, the
category performs the following actions:

  - if the category has an appender, the message is passed to the
    appender's do_append() function,
  - if additivity is true for the category, the message is passed to
    the category's parent. Additivity is true by default, and can be
    controlled by xbt_log_additivity_set() or something like --log=root.add:1 (see \ref log_use_conf_add).
    Also, when you add an appender to a category, its additivity is automatically turned to off.
    Turn it back on afterward if it is not what you wanted.

By default, only the root category have an appender, and any other category has
its additivity set to true. This causes all messages to be logged by the root
category's appender.

The default appender function currently prints to stderr, and the only other
existing one writes to the specified file. More would be needed, like the one
able to send the logs to a remote dedicated server.
This is on our TODO list for quite a while now, but your help would be
welcome here, too.


                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              *//*' */


xbt_log_appender_t xbt_log_default_appender = NULL;     /* set in log_init */
xbt_log_layout_t xbt_log_default_layout = NULL; /* set in log_init */

typedef struct {
  char *catname;
  e_xbt_log_priority_t thresh;
  char *fmt;
  int additivity;
  xbt_log_appender_t appender;
} s_xbt_log_setting_t, *xbt_log_setting_t;

static xbt_dynar_t xbt_log_settings = NULL;

static void _free_setting(void *s)
{
  xbt_log_setting_t set = *(xbt_log_setting_t *) s;
  if (set) {
    free(set->catname);
    if (set->fmt)
      free(set->fmt);
    free(set);
  }
}

static void _xbt_log_cat_apply_set(xbt_log_category_t category,
                                   xbt_log_setting_t setting);

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
  NULL /*parent */ , NULL /* firstChild */ , NULL /* nextSibling */ ,
      "root", xbt_log_priority_uninitialized /* threshold */ ,
      0 /* isThreshInherited */ ,
      NULL /* appender */ , NULL /* layout */ ,
      0                         /* additivity */
};

XBT_LOG_NEW_CATEGORY(xbt, "All XBT categories (simgrid toolbox)");
XBT_LOG_NEW_CATEGORY(surf, "All SURF categories");
XBT_LOG_NEW_CATEGORY(msg, "All MSG categories");
XBT_LOG_NEW_CATEGORY(simix, "All SIMIX categories");
XBT_LOG_NEW_CATEGORY(mc, "All MC categories");
XBT_LOG_NEW_CATEGORY(bindings, "All bindings categories");

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(log, xbt,
                                "Loggings from the logging mechanism itself");

/* create the default appender and install it in the root category,
   which were already created (damnit. Too slow little beetle) */
void xbt_log_preinit(void)
{
  xbt_log_default_appender = xbt_log_appender_file_new(NULL);
  xbt_log_default_layout = xbt_log_layout_simple_new(NULL);
  _XBT_LOGV(XBT_LOG_ROOT_CAT).appender = xbt_log_default_appender;
  _XBT_LOGV(XBT_LOG_ROOT_CAT).layout = xbt_log_default_layout;
}

/** @brief Get all logging settings from the command line
 *
 * xbt_log_control_set() is called on each string we got from cmd line
 */
void xbt_log_init(int *argc, char **argv)
{
  int i, j;
  char *opt;

  //    _XBT_LOGV(log).threshold = xbt_log_priority_debug; /* uncomment to set the LOG category to debug directly */

  /* Set logs and init log submodule */
  for (i = 1; i < *argc; i++) {
    if (!strncmp(argv[i], "--log=", strlen("--log=")) ||
        !strncmp(argv[i], "--gras-log=", strlen("--gras-log=")) ||
        !strncmp(argv[i], "--surf-log=", strlen("--surf-log=")) ||
        !strncmp(argv[i], "--msg-log=", strlen("--msg-log=")) ||
        !strncmp(argv[i], "--simix-log=", strlen("--simix-log=")) ||
        !strncmp(argv[i], "--xbt-log=", strlen("--xbt-log="))) {

      if (strncmp(argv[i], "--log=", strlen("--log=")))
        WARN2
            ("Option %.*s is deprecated and will disapear in the future. Use --log instead.",
             (int) (strchr(argv[i], '=') - argv[i]), argv[i]);

      opt = strchr(argv[i], '=');
      opt++;
      xbt_log_control_set(opt);
      DEBUG1("Did apply '%s' as log setting", opt);
      /*remove this from argv */

      for (j = i + 1; j < *argc; j++) {
        argv[j - 1] = argv[j];
      }

      argv[j - 1] = NULL;
      (*argc)--;
      i--;                      /* compensate effect of next loop incrementation */
    }
  }
}

static void log_cat_exit(xbt_log_category_t cat)
{
  xbt_log_category_t child;

  if (cat->appender) {
    if (cat->appender->free_)
      cat->appender->free_(cat->appender);
    free(cat->appender);
  }
  if (cat->layout) {
    if (cat->layout->free_)
      cat->layout->free_(cat->layout);
    free(cat->layout);
  }

  for (child = cat->firstChild; child != NULL; child = child->nextSibling)
    log_cat_exit(child);
}

void xbt_log_postexit(void)
{
  VERB0("Exiting log");
  xbt_dynar_free(&xbt_log_settings);
  log_cat_exit(&_XBT_LOGV(XBT_LOG_ROOT_CAT));
}

void _xbt_log_event_log(xbt_log_event_t ev, const char *fmt, ...)
{

  xbt_log_category_t cat = ev->cat;

  va_start(ev->ap, fmt);
  va_start(ev->ap_copy, fmt);
  while (1) {
    xbt_log_appender_t appender = cat->appender;
    if (appender != NULL) {
      xbt_assert1(cat->layout,
                  "No valid layout for the appender of category %s",
                  cat->name);
      cat->layout->do_layout(cat->layout, ev, fmt, appender);
    }
    if (!cat->additivity)
      break;

    cat = cat->parent;
  }
  va_end(ev->ap);
  va_end(ev->ap_copy);

#ifdef _XBT_WIN32
  free(ev->buffer);
#endif
}

/* NOTE:
 *
 * The standard logging macros use _XBT_LOG_ISENABLED, which calls
 * _xbt_log_cat_init().  Thus, if we want to avoid an infinite
 * recursion, we can not use the standard logging macros in
 * _xbt_log_cat_init(), and in all functions called from it.
 *
 * To circumvent the problem, we define the macro_xbt_log_init() as
 * (0) for the length of the affected functions, and we do not forget
 * to undefine it at the end!
 */

static void _xbt_log_cat_apply_set(xbt_log_category_t category,
                                   xbt_log_setting_t setting)
{
#define _xbt_log_cat_init(a, b) (0)

  if (setting->thresh != xbt_log_priority_uninitialized) {
    xbt_log_threshold_set(category, setting->thresh);

    DEBUG3("Apply settings for category '%s': set threshold to %s (=%d)",
           category->name, xbt_log_priority_names[category->threshold],
           category->threshold);
  }

  if (setting->fmt) {
    xbt_log_layout_set(category, xbt_log_layout_format_new(setting->fmt));

    DEBUG2("Apply settings for category '%s': set format to %s",
           category->name, setting->fmt);
  }

  if (setting->additivity != -1) {
    xbt_log_additivity_set(category, setting->additivity);

    DEBUG2("Apply settings for category '%s': set additivity to %s",
           category->name, (setting->additivity ? "on" : "off"));
  }
  if (setting->appender) {
    xbt_log_appender_set(category, setting->appender);
    if (!category->layout)
      xbt_log_layout_set(category, xbt_log_layout_simple_new(NULL));
    category->additivity = 0;
    DEBUG2("Set %p as appender of category '%s'",
           setting->appender, category->name);
  }
#undef _xbt_log_cat_init
}

/*
 * This gets called the first time a category is referenced and performs the
 * initialization.
 * Also resets threshold to inherited!
 */
int _xbt_log_cat_init(xbt_log_category_t category,
                      e_xbt_log_priority_t priority)
{
#define _xbt_log_cat_init(a, b) (0)

  unsigned int cursor;
  xbt_log_setting_t setting = NULL;
  int found = 0;

  DEBUG3("Initializing category '%s' (firstChild=%s, nextSibling=%s)",
         category->name,
         (category->firstChild ? category->firstChild->name : "none"),
         (category->nextSibling ? category->nextSibling->name : "none"));

  if (category == &_XBT_LOGV(XBT_LOG_ROOT_CAT)) {
    category->threshold = xbt_log_priority_info;
    /* xbt_log_priority_debug */ ;
    category->appender = xbt_log_default_appender;
    category->layout = xbt_log_default_layout;
  } else {

    if (!category->parent)
      category->parent = &_XBT_LOGV(XBT_LOG_ROOT_CAT);

    DEBUG3("Set %s (%s) as father of %s ",
           category->parent->name,
           (category->parent->threshold == xbt_log_priority_uninitialized ?
            "uninited" : xbt_log_priority_names[category->
                                                parent->threshold]),
           category->name);
    xbt_log_parent_set(category, category->parent);

    if (XBT_LOG_ISENABLED(log, xbt_log_priority_debug)) {
      char *buf, *res = NULL;
      xbt_log_category_t cpp = category->parent->firstChild;
      while (cpp) {
        if (res) {
          buf = bprintf("%s %s", res, cpp->name);
          free(res);
          res = buf;
        } else {
          res = xbt_strdup(cpp->name);
        }
        cpp = cpp->nextSibling;
      }

      DEBUG3("Childs of %s: %s; nextSibling: %s",
             category->parent->name, res,
             (category->parent->nextSibling ?
              category->parent->nextSibling->name : "none"));

      free(res);
    }

  }

  /* Apply the control */
  if (!xbt_log_settings)
    return priority >= category->threshold;

  xbt_assert0(category, "NULL category");
  xbt_assert(category->name);

  xbt_dynar_foreach(xbt_log_settings, cursor, setting) {
    xbt_assert0(setting, "Damnit, NULL cat in the list");
    xbt_assert1(setting->catname, "NULL setting(=%p)->catname",
                (void *) setting);

    if (!strcmp(setting->catname, category->name)) {

      found = 1;

      _xbt_log_cat_apply_set(category, setting);

      xbt_dynar_cursor_rm(xbt_log_settings, &cursor);
    }
  }

  if (!found)
    DEBUG3("Category '%s': inherited threshold = %s (=%d)",
           category->name, xbt_log_priority_names[category->threshold],
           category->threshold);

  return priority >= category->threshold;

#undef _xbt_log_cat_init
}

void xbt_log_parent_set(xbt_log_category_t cat, xbt_log_category_t parent)
{

  xbt_assert0(cat, "NULL category to be given a parent");
  xbt_assert1(parent, "The parent category of %s is NULL", cat->name);

  /*
   * if the threshold is initialized
   * unlink from current parent
   */
  if (cat->threshold != xbt_log_priority_uninitialized) {

    xbt_log_category_t *cpp = &parent->firstChild;

    while (*cpp != cat && *cpp != NULL) {
      cpp = &(*cpp)->nextSibling;
    }

    xbt_assert(*cpp == cat);
    *cpp = cat->nextSibling;
  }

  cat->parent = parent;
  cat->nextSibling = parent->firstChild;

  parent->firstChild = cat;

  if (parent->threshold == xbt_log_priority_uninitialized) {

    _xbt_log_cat_init(parent,
                      xbt_log_priority_uninitialized /* ignored */ );
  }

  cat->threshold = parent->threshold;

  cat->isThreshInherited = 1;

}

static void _set_inherited_thresholds(xbt_log_category_t cat)
{

  xbt_log_category_t child = cat->firstChild;

  for (; child != NULL; child = child->nextSibling) {
    if (child->isThreshInherited) {
      if (cat != &_XBT_LOGV(log))
        VERB3("Set category threshold of %s to %s (=%d)",
              child->name, xbt_log_priority_names[cat->threshold],
              cat->threshold);
      child->threshold = cat->threshold;
      _set_inherited_thresholds(child);
    }
  }


}

void xbt_log_threshold_set(xbt_log_category_t cat,
                           e_xbt_log_priority_t threshold)
{
  cat->threshold = threshold;
  cat->isThreshInherited = 0;

  _set_inherited_thresholds(cat);

}

static xbt_log_setting_t _xbt_log_parse_setting(const char *control_string)
{

  xbt_log_setting_t set = xbt_new(s_xbt_log_setting_t, 1);
  const char *name, *dot, *eq;

  set->catname = NULL;
  set->thresh = xbt_log_priority_uninitialized;
  set->fmt = NULL;
  set->additivity = -1;
  set->appender = NULL;

  if (!*control_string)
    return set;
  DEBUG1("Parse log setting '%s'", control_string);

  control_string += strspn(control_string, " ");
  name = control_string;
  control_string += strcspn(control_string, ".= ");
  dot = control_string;
  control_string += strcspn(control_string, ":= ");
  eq = control_string;
  control_string += strcspn(control_string, " ");

  xbt_assert1(*dot == '.' && (*eq == '=' || *eq == ':'),
              "Invalid control string '%s'", control_string);

  if (!strncmp(dot + 1, "thresh", (size_t) (eq - dot - 1))) {
    int i;
    char *neweq = xbt_strdup(eq + 1);
    char *p = neweq - 1;

    while (*(++p) != '\0') {
      if (*p >= 'a' && *p <= 'z') {
        *p -= 'a' - 'A';
      }
    }

    DEBUG1("New priority name = %s", neweq);
    for (i = 0; i < xbt_log_priority_infinite; i++) {
      if (!strncmp(xbt_log_priority_names[i], neweq, p - eq)) {
        DEBUG1("This is priority %d", i);
        break;
      }
    }
    if (i < xbt_log_priority_infinite) {
      set->thresh = (e_xbt_log_priority_t) i;
    } else {
      THROW1(arg_error, 0,
             "Unknown priority name: %s (must be one of: trace,debug,verbose,info,warning,error,critical)",
             eq + 1);
    }
    free(neweq);
  } else if (!strncmp(dot + 1, "add", (size_t) (eq - dot - 1)) ||
             !strncmp(dot + 1, "additivity", (size_t) (eq - dot - 1))) {

    char *neweq = xbt_strdup(eq + 1);
    char *p = neweq - 1;

    while (*(++p) != '\0') {
      if (*p >= 'a' && *p <= 'z') {
        *p -= 'a' - 'A';
      }
    }
    if (!strcmp(neweq, "ON") || !strcmp(neweq, "YES")
        || !strcmp(neweq, "1")) {
      set->additivity = 1;
    } else {
      set->additivity = 0;
    }
    free(neweq);
  } else if (!strncmp(dot + 1, "app", (size_t) (eq - dot - 1)) ||
             !strncmp(dot + 1, "appender", (size_t) (eq - dot - 1))) {

    char *neweq = xbt_strdup(eq + 1);

    if (!strncmp(neweq, "file:", 5)) {
      set->appender = xbt_log_appender_file_new(neweq + 5);
    } else {
      THROW1(arg_error, 0, "Unknown appender log type: '%s'", neweq);
    }
    free(neweq);
  } else if (!strncmp(dot + 1, "fmt", (size_t) (eq - dot - 1))) {
    set->fmt = xbt_strdup(eq + 1);
  } else {
    char buff[512];
    snprintf(buff, min(512, eq - dot), "%s", dot + 1);
    THROW1(arg_error, 0, "Unknown setting of the log category: '%s'",
           buff);
  }
  set->catname = (char *) xbt_malloc(dot - name + 1);

  memcpy(set->catname, name, dot - name);
  set->catname[dot - name] = '\0';      /* Just in case */
  DEBUG1("This is for cat '%s'", set->catname);

  return set;
}

static xbt_log_category_t _xbt_log_cat_searchsub(xbt_log_category_t cat,
                                                 char *name)
{
  xbt_log_category_t child, res;

  DEBUG4("Search '%s' into '%s' (firstChild='%s'; nextSibling='%s')", name,
         cat->name, (cat->firstChild ? cat->firstChild->name : "none"),
         (cat->nextSibling ? cat->nextSibling->name : "none"));
  if (!strcmp(cat->name, name))
    return cat;

  for (child = cat->firstChild; child != NULL; child = child->nextSibling) {
    DEBUG1("Dig into %s", child->name);
    res = _xbt_log_cat_searchsub(child, name);
    if (res)
      return res;
  }

  return NULL;
}

/**
 * \ingroup XBT_log
 * \param control_string What to parse
 *
 * Typically passed a command-line argument. The string has the syntax:
 *
 *      ( [category] "." [keyword] ":" value (" ")... )...
 *
 * where [category] is one the category names (see \ref XBT_log_cats for
 * a complete list of the ones defined in the SimGrid library)
 * and keyword is one of the following:
 *
 *    - thres: category's threshold priority. Possible values:
 *             TRACE,DEBUG,VERBOSE,INFO,WARNING,ERROR,CRITICAL
 *    - add or additivity: whether the logging actions must be passed to
 *      the parent category.
 *      Possible values: 0, 1, no, yes, on, off.
 *      Default value: yes.
 *    - fmt: the format to use. See \ref log_use_conf_fmt for more information.
 *    - app or appender: the appender to use. See \ref log_use_conf_app for more
 *      information.
 *
 */
void xbt_log_control_set(const char *control_string)
{
  xbt_log_setting_t set;

  /* To split the string in commands, and the cursors */
  xbt_dynar_t set_strings;
  char *str;
  unsigned int cpt;

  if (!control_string)
    return;
  DEBUG1("Parse log settings '%s'", control_string);

  /* Special handling of no_loc request, which asks for any file localization to be omitted (for tesh runs) */
  if (!strcmp(control_string, "no_loc")) {
    xbt_log_no_loc = 1;
    return;
  }
  /* some initialization if this is the first time that this get called */
  if (xbt_log_settings == NULL)
    xbt_log_settings = xbt_dynar_new(sizeof(xbt_log_setting_t),
                                     _free_setting);

  /* split the string, and remove empty entries */
  set_strings = xbt_str_split_quoted(control_string);

  if (xbt_dynar_length(set_strings) == 0) {     /* vicious user! */
    xbt_dynar_free(&set_strings);
    return;
  }

  /* Parse each entry and either use it right now (if the category was already
     created), or store it for further use */
  xbt_dynar_foreach(set_strings, cpt, str) {
    xbt_log_category_t cat = NULL;

    set = _xbt_log_parse_setting(str);
    cat =
        _xbt_log_cat_searchsub(&_XBT_LOGV(XBT_LOG_ROOT_CAT), set->catname);

    if (cat) {
      DEBUG0("Apply directly");
      _xbt_log_cat_apply_set(cat, set);
      _free_setting((void *) &set);
    } else {

      DEBUG0("Store for further application");
      DEBUG1("push %p to the settings", (void *) set);
      xbt_dynar_push(xbt_log_settings, &set);
    }
  }
  xbt_dynar_free(&set_strings);
}

void xbt_log_appender_set(xbt_log_category_t cat, xbt_log_appender_t app)
{
  if (cat->appender) {
    if (cat->appender->free_)
      cat->appender->free_(cat->appender);
    free(cat->appender);
  }
  cat->appender = app;
}

void xbt_log_layout_set(xbt_log_category_t cat, xbt_log_layout_t lay)
{
  if (!cat->appender) {
    VERB1
        ("No appender to category %s. Setting the file appender as default",
         cat->name);
    xbt_log_appender_set(cat, xbt_log_appender_file_new(NULL));
  }
  if (cat->layout && cat != &_XBT_LOGV(root)) {
    /* better leak the default layout than check every categories to
       change it */
    if (cat->layout->free_) {
      cat->layout->free_(cat->layout);
      free(cat->layout);
    }
  }
  cat->layout = lay;
  xbt_log_additivity_set(cat, 0);
}

void xbt_log_additivity_set(xbt_log_category_t cat, int additivity)
{
  cat->additivity = additivity;
}
