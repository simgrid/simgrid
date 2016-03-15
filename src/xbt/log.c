/* log - a generic logging facility in the spirit of log4j                  */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>              /* snprintf */
#include <stdlib.h>             /* snprintf */

#include "src/internal_config.h"

#include "src/xbt_modinter.h"

#include "xbt/misc.h"
#include "xbt/ex.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"
#include "src/xbt/log_private.h"
#include "xbt/dynar.h"
#include "xbt/xbt_os_thread.h"

int xbt_log_no_loc = 0;         /* if set to true (with --log=no_loc), file localization will be omitted (for tesh tests) */
static xbt_os_mutex_t log_cat_init_mutex = NULL;

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
of MSG while only being interested into the messages at level "error" or
higher about the XBT internals.

\subsection log_app 1.3 Message appenders

The message appenders are the elements in charge of actually displaying the
message to the user. For now, four appenders exist: 
- the default one prints stuff on stderr 
- file sends the data to a single file
- rollfile overwrites the file when the file grows too large
- splitfile creates new files with a specific maximum size

Other are planed (such as the one sending everything to a remote server) 
One day, for sure ;)

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

Here is an example of the most basic type of macro. This is a logging
request with priority <i>warning</i>.

<code>XBT_CLOG(MyCat, xbt_log_priority_warning, "Values are: %d and '%s'", 5,
"oops");</code>

A logging request is said to be enabled if its priority is higher than or
equal to the threshold priority of its category. Otherwise, the request is
said to be disabled. A category without an assigned priority will inherit
one from the hierarchy.

It is possible to use any non-negative integer as a priority. If, as in the
example, one of the standard priorities is used, then there is a convenience
macro that is typically used instead. For example, the above example is
equivalent to the shorter:

<code>XBT_CWARN(MyCat, "Values are: %d and '%s'", 5, "oops");</code>

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

<code>XBT_WARN("Values are: %s and '%d'", 5, "oops");</code>

Only one default category can be created per file, though multiple
non-defaults can be created and used.

\section log_API_easy 2.5 Putting all together: the easy interface

First of all, each module should register its own category into the categories
tree using \ref XBT_LOG_NEW_DEFAULT_SUBCATEGORY.

Then, logging should be done with the #XBT_DEBUG, #XBT_VERB, #XBT_INFO,
#XBT_WARN, #XBT_ERROR and #XBT_CRITICAL macros.

Under GCC, these macro check there arguments the same way than printf does. So,
if you compile with -Wall, the following code will issue a warning:
<code>XBT_DEBUG("Found %s (id %d)", some_string, a_double)</code>

If you want to specify the category to log onto (for example because you
have more than one category per file, add a C before the name of the log
producing macro (ie, use #XBT_CDEBUG, #XBT_CVERB, #XBT_CINFO, #XBT_CWARN,
#XBT_CERROR and #XBT_CCRITICAL and friends), and pass the category name as
first argument.

The TRACE priority is not used the same way than the other. You should use
the #XBT_IN, #XBT_OUT and #XBT_HERE macros instead.

\section log_API_example 2.6 Example of use

Here is a more complete example:

\verbatim
#include "xbt/log.h"

/ * create a category and a default subcategory * /
XBT_LOG_NEW_CATEGORY(VSS);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(SA, VSS);

int main() {
       / * Now set the parent's priority.  (the string would typically be a runtime option) * /
       xbt_log_control_set("SA.thresh:3");

       / * This request is enabled, because WARNING >= INFO. * /
       XBT_CWARN(VSS, "Low fuel level.");

       / * This request is disabled, because DEBUG < INFO. * /
       XBT_CDEBUG(VSS, "Starting search for nearest gas station.");

       / * The default category SA inherits its priority from VSS. Thus,
          the following request is enabled because INFO >= INFO.  * /
       XBT_INFO("Located nearest gas station.");

       / * This request is disabled, because DEBUG < INFO. * /
       XBT_DEBUG("Exiting gas station search");
}
\endverbatim

\section log_user 3. User interface

\section log_use_conf 3.1 Configuration

Although rarely done, it is possible to configure the logs during
program initialization by invoking the xbt_log_control_set() method
manually. A more conventional way is to use the --log command line
argument. xbt_init() (called by MSG_init() and friends)
checks and deals properly with such arguments.

\subsection log_use_conf_thres 3.1.1 Threshold configuration

The most common setting is to control which logging event will get
displayed by setting a threshold to each category through the
<tt>threshold</tt> keyword.

For example, \verbatim --log=root.threshold:debug\endverbatim will make
SimGrid <b>extremely</b> verbose while \verbatim
--log=root.thres:critical\endverbatim should shut it almost
completely off.

Note that the <tt>threshold</tt> keyword can be abbreviated here. For example,
all the following notations have the same result.
\verbatim
--log=root.threshold:debug
--log=root.threshol:debug
--log=root.thresho:debug
--log=root.thresh:debug
--log=root.thres:debug
--log=root.thre:debug
--log=root.thr:debug
--log=root.th:debug
--log=root.t:debug
--log=root.:debug     <--- That's obviously really ugly, but it actually works.
\endverbatim

The full list of recognized thresholds is the following:

 - trace: enter and return of some functions
 - debug: crufty output
 - verbose: verbose output for the user wanting more
 - info: output about the regular functionning
 - warning: minor issue encountered
 - error: issue encountered
 - critical: major issue encountered 

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
 - %%P: Process name (SimGrid extension -- note that with SMPI this is the integer value of the process rank)
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

For now, there is only two format modifiers: the precision and the
width fields. You can for example specify %.4r to get the application
age with 4 numbers after the radix, or %15p to get the process name
on 15 columns. Finally, you can specify %10.6r to get the time on at
most 10 columns, with 6 numbers after the radix. 

Note that when specifying the width, it is filled with spaces. That
is to say that for example %5r in your format is converted to "% 5f"
for printf (note the extra space); there is no way to fill the empty
columns with 0 (ie, pass "%05f" to printf). Another limitation is
that you cannot set specific layouts to the several priorities.

\subsection log_use_conf_app 3.1.4 Category appender

As with SimGrid 3.3, it is possible to control the appender of log
messages. This is done through the <tt>app</tt> keyword. For example,
\verbatim --log=root.app:file:mylogfile\endverbatim redirects the output
to the file mylogfile.

For splitfile appender, the format is 
\verbatim --log=root.app:splitfile:size:mylogfile_%.format\endverbatim

The size is in bytes, and the % wildcard will be replaced by the number of the
file. If no % is present, it will be appended at the end.

rollfile appender is also available, it can be used as
\verbatim --log=root.app:rollfile:size:mylogfile\endverbatim
When the file grows to be larger than the size, it will be emptied and new log 
events will be sent at its beginning 

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
  - When writing a log format, you often want to use spaces. If you don't
    protect these spaces, they are used as configuration elements separators.
    For example, if you want to remove the date from the logs, you want to pass the following 
    argument on the command line. The outer quotes are here to protect the string from the shell 
    interpretation while the inner ones are there to prevent simgrid from splitting the string 
    in several log parameters (that would be invalid).
    \verbatim --log="'root.fmt:%l: [%p/%c]: %m%n'"\endverbatim
    Another option is to use the SimGrid-specific format directive \%e for
    spaces, like in the following.
    \verbatim --log="root.fmt:%l:%e[%p/%c]:%e%m%n"\endverbatim

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
by the compiler. By setting it to xbt_log_priority_infinite, all logging
requests are statically disabled at compile time and cost nothing. Released executables
<i>might</i>  be compiled with (note that it will prevent users to debug their problems)
\verbatim-DXBT_LOG_STATIC_THRESHOLD=xbt_log_priority_infinite\endverbatim

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

The default appender function currently prints to stderr
*/

xbt_log_appender_t xbt_log_default_appender = NULL;     /* set in log_init */
xbt_log_layout_t xbt_log_default_layout = NULL; /* set in log_init */

typedef struct {
  char *catname;
  char *fmt;
  e_xbt_log_priority_t thresh;
  int additivity;
  xbt_log_appender_t appender;
} s_xbt_log_setting_t, *xbt_log_setting_t;

static xbt_dynar_t xbt_log_settings = NULL;

static void _free_setting(void *s)
{
  xbt_log_setting_t set = *(xbt_log_setting_t *) s;
  if (set) {
    free(set->catname);
    free(set->fmt);
    free(set);
  }
}

static void _xbt_log_cat_apply_set(xbt_log_category_t category, xbt_log_setting_t setting);

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
      "root", "The common ancestor for all categories",
      0 /*initialized */, xbt_log_priority_uninitialized /* threshold */ ,
      0 /* isThreshInherited */ ,
      NULL /* appender */ , NULL /* layout */ ,
      0                         /* additivity */
};

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(log, xbt, "Loggings from the logging mechanism itself");

/* create the default appender and install it in the root category,
   which were already created (damnit. Too slow little beetle) */
void xbt_log_preinit(void)
{
  xbt_log_default_appender = xbt_log_appender_file_new(NULL);
  xbt_log_default_layout = xbt_log_layout_simple_new(NULL);
  _XBT_LOGV(XBT_LOG_ROOT_CAT).appender = xbt_log_default_appender;
  _XBT_LOGV(XBT_LOG_ROOT_CAT).layout = xbt_log_default_layout;
  log_cat_init_mutex = xbt_os_mutex_init();
}

static void xbt_log_connect_categories(void)
{
  /* Connect our log channels: that must be done manually under windows */
  /* Also permit that they are correctly listed by xbt_log_help_categories() */

  /* xbt */
  XBT_LOG_CONNECT(xbt);
  XBT_LOG_CONNECT(log);
  XBT_LOG_CONNECT(module);
  XBT_LOG_CONNECT(replay);
  XBT_LOG_CONNECT(strbuff);
  XBT_LOG_CONNECT(xbt_cfg);
  XBT_LOG_CONNECT(xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_cursor);
  XBT_LOG_CONNECT(xbt_dict_elm);
  XBT_LOG_CONNECT(xbt_dyn);
  XBT_LOG_CONNECT(xbt_ex);
  XBT_LOG_CONNECT(xbt_fifo);
  XBT_LOG_CONNECT(xbt_graph);
  XBT_LOG_CONNECT(xbt_heap);
  XBT_LOG_CONNECT(xbt_lib);
  XBT_LOG_CONNECT(xbt_mallocator);
  XBT_LOG_CONNECT(xbt_matrix);
  XBT_LOG_CONNECT(xbt_parmap);
  XBT_LOG_CONNECT(xbt_sync);
  XBT_LOG_CONNECT(xbt_sync_os);

#ifdef simgrid_EXPORTS
  /* The following categories are only defined in libsimgrid */

  /* bindings */
#if HAVE_LUA
  XBT_LOG_CONNECT(lua);
  XBT_LOG_CONNECT(lua_host);
  XBT_LOG_CONNECT(lua_platf);
  XBT_LOG_CONNECT(lua_debug);
#endif

  /* instr */
  XBT_LOG_CONNECT(instr);
  XBT_LOG_CONNECT(instr_api);
  XBT_LOG_CONNECT(instr_config);
  XBT_LOG_CONNECT(instr_msg);
  XBT_LOG_CONNECT(instr_msg_process);
  XBT_LOG_CONNECT(instr_msg_vm);
  XBT_LOG_CONNECT(instr_paje_containers);
  XBT_LOG_CONNECT(instr_paje_header);
  XBT_LOG_CONNECT(instr_paje_trace);
  XBT_LOG_CONNECT(instr_paje_types);
  XBT_LOG_CONNECT(instr_paje_values);
  XBT_LOG_CONNECT(instr_resource);
  XBT_LOG_CONNECT(instr_routing);
  XBT_LOG_CONNECT(instr_surf);
  XBT_LOG_CONNECT(instr_trace);
  XBT_LOG_CONNECT(instr_TI_trace);

  /* jedule */
#if HAVE_JEDULE
  XBT_LOG_CONNECT(jedule);
  XBT_LOG_CONNECT(jed_out);
  XBT_LOG_CONNECT(jed_sd);
#endif

  /* mc */
#if HAVE_MC
  XBT_LOG_CONNECT(mc);
  XBT_LOG_CONNECT(mc_checkpoint);
  XBT_LOG_CONNECT(mc_comm_determinism);
  XBT_LOG_CONNECT(mc_compare);
  XBT_LOG_CONNECT(mc_diff);
  XBT_LOG_CONNECT(mc_dwarf);
  XBT_LOG_CONNECT(mc_hash);
  XBT_LOG_CONNECT(mc_liveness);
  XBT_LOG_CONNECT(mc_memory);
  XBT_LOG_CONNECT(mc_page_snapshot);
  XBT_LOG_CONNECT(mc_request);
  XBT_LOG_CONNECT(mc_safety);
  XBT_LOG_CONNECT(mc_visited);
  XBT_LOG_CONNECT(mc_client);
  XBT_LOG_CONNECT(mc_client_api);
  XBT_LOG_CONNECT(mc_comm_pattern);
  XBT_LOG_CONNECT(mc_process);
  XBT_LOG_CONNECT(mc_protocol);
  XBT_LOG_CONNECT(mc_RegionSnaphot);
  XBT_LOG_CONNECT(mc_ModelChecker);
  XBT_LOG_CONNECT(mc_state);
#endif
  XBT_LOG_CONNECT(mc_global);
  XBT_LOG_CONNECT(mc_config);
  XBT_LOG_CONNECT(mc_record);

  /* msg */
  XBT_LOG_CONNECT(msg);
  XBT_LOG_CONNECT(msg_action);
  XBT_LOG_CONNECT(msg_gos);
  XBT_LOG_CONNECT(msg_io);
  XBT_LOG_CONNECT(msg_kernel);
  XBT_LOG_CONNECT(msg_mailbox);
  XBT_LOG_CONNECT(msg_process);
  XBT_LOG_CONNECT(msg_synchro);
  XBT_LOG_CONNECT(msg_task);
  XBT_LOG_CONNECT(msg_vm);

  /* sg */
  XBT_LOG_CONNECT(sg_host);

  /* simdag */
  XBT_LOG_CONNECT(sd);
  XBT_LOG_CONNECT(sd_daxparse);
#if HAVE_GRAPHVIZ
  XBT_LOG_CONNECT(sd_dotparse);
#endif
  XBT_LOG_CONNECT(sd_kernel);
  XBT_LOG_CONNECT(sd_task);

  /* simix */
  XBT_LOG_CONNECT(simix);
  XBT_LOG_CONNECT(simix_context);
  XBT_LOG_CONNECT(simix_deployment);
  XBT_LOG_CONNECT(simix_environment);
  XBT_LOG_CONNECT(simix_host);
  XBT_LOG_CONNECT(simix_io);
  XBT_LOG_CONNECT(simix_kernel);
  XBT_LOG_CONNECT(simix_network);
  XBT_LOG_CONNECT(simix_process);
  XBT_LOG_CONNECT(simix_popping);
  XBT_LOG_CONNECT(simix_synchro);
  XBT_LOG_CONNECT(simix_vm);

  /* smpi */
  /* SMPI categories are connected in smpi_global.c */

  /* surf */
  XBT_LOG_CONNECT(surf);
  XBT_LOG_CONNECT(surf_config);
  XBT_LOG_CONNECT(surf_cpu);
  XBT_LOG_CONNECT(surf_cpu_cas);
  XBT_LOG_CONNECT(surf_cpu_ti);
  XBT_LOG_CONNECT(surf_energy);
  XBT_LOG_CONNECT(surf_kernel);
  XBT_LOG_CONNECT(surf_lagrange);
  XBT_LOG_CONNECT(surf_lagrange_dichotomy);
  XBT_LOG_CONNECT(surf_maxmin);
  XBT_LOG_CONNECT(surf_network);
#if HAVE_NS3
  XBT_LOG_CONNECT(ns3);
#endif
  XBT_LOG_CONNECT(surf_parse);
  XBT_LOG_CONNECT(surf_route);
  XBT_LOG_CONNECT(surf_routing_generic);
  XBT_LOG_CONNECT(surf_route_cluster);
  XBT_LOG_CONNECT(surf_route_cluster_torus);
  XBT_LOG_CONNECT(surf_route_dijkstra);
  XBT_LOG_CONNECT(surf_route_fat_tree);
  XBT_LOG_CONNECT(surf_route_floyd);
  XBT_LOG_CONNECT(surf_route_full);
  XBT_LOG_CONNECT(surf_route_none);
  XBT_LOG_CONNECT(surf_route_vivaldi);
  XBT_LOG_CONNECT(surf_storage);
  XBT_LOG_CONNECT(surf_trace);
  XBT_LOG_CONNECT(surf_vm);
  XBT_LOG_CONNECT(surf_host);

#endif /* simgrid_EXPORTS */
}

static void xbt_log_help(void);
static void xbt_log_help_categories(void);

/** @brief Get all logging settings from the command line
 *
 * xbt_log_control_set() is called on each string we got from cmd line
 */
void xbt_log_init(int *argc, char **argv)
{
  unsigned help_requested = 0;  /* 1: logs; 2: categories */
  int i, j;
  char *opt;

  //    _XBT_LOGV(log).threshold = xbt_log_priority_debug; /* uncomment to set the LOG category to debug directly */

  xbt_log_connect_categories();

  /* Set logs and init log submodule */
  for (j = i = 1; i < *argc; i++) {
    if (!strncmp(argv[i], "--log=", strlen("--log="))) {
      opt = strchr(argv[i], '=');
      opt++;
      xbt_log_control_set(opt);
      XBT_DEBUG("Did apply '%s' as log setting", opt);
    } else if (!strcmp(argv[i], "--help-logs")) {
      help_requested |= 1;
    } else if (!strcmp(argv[i], "--help-log-categories")) {
      help_requested |= 2;
    } else {
      argv[j++] = argv[i];
    }
  }
  if (j < *argc) {
    argv[j] = NULL;
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
  XBT_VERB("Exiting log");
  xbt_os_mutex_destroy(log_cat_init_mutex);
  xbt_dynar_free(&xbt_log_settings);
  log_cat_exit(&_XBT_LOGV(XBT_LOG_ROOT_CAT));
}

 /* Size of the static string in which we  build the log string */
#define XBT_LOG_STATIC_BUFFER_SIZE 2048
/* Minimum size of the dynamic string in which we build the log string
   (should be greater than XBT_LOG_STATIC_BUFFER_SIZE) */
#define XBT_LOG_DYNAMIC_BUFFER_SIZE 4096

void _xbt_log_event_log(xbt_log_event_t ev, const char *fmt, ...)
{
  xbt_log_category_t cat = ev->cat;

  xbt_assert(ev->priority >= 0, "Negative logging priority naturally forbidden");
  xbt_assert(ev->priority < sizeof(xbt_log_priority_names), "Priority %d is greater than the biggest allowed value",
             ev->priority);

  do {
    xbt_log_appender_t appender = cat->appender;

    if (!appender)
      continue;                 /* No appender, try next */

    xbt_assert(cat->layout, "No valid layout for the appender of category %s", cat->name);

    /* First, try with a static buffer */
    if (XBT_LOG_STATIC_BUFFER_SIZE) {
      char buff[XBT_LOG_STATIC_BUFFER_SIZE];
      int done;
      ev->buffer = buff;
      ev->buffer_size = sizeof buff;
      va_start(ev->ap, fmt);
      done = cat->layout->do_layout(cat->layout, ev, fmt);
      va_end(ev->ap);
      if (done) {
        appender->do_append(appender, buff);
        continue;               /* Ok, that worked: go next */
      }
    }

    /* The static buffer was too small, use a dynamically expanded one */
    ev->buffer_size = XBT_LOG_DYNAMIC_BUFFER_SIZE;
    ev->buffer = xbt_malloc(ev->buffer_size);
    while (1) {
      int done;
      va_start(ev->ap, fmt);
      done = cat->layout->do_layout(cat->layout, ev, fmt);
      va_end(ev->ap);
      if (done)
        break;                  /* Got it */
      ev->buffer_size *= 2;
      ev->buffer = xbt_realloc(ev->buffer, ev->buffer_size);
    }
    appender->do_append(appender, ev->buffer);
    xbt_free(ev->buffer);

  } while (cat->additivity && (cat = cat->parent, 1));
}

#undef XBT_LOG_DYNAMIC_BUFFER_SIZE
#undef XBT_LOG_STATIC_BUFFER_SIZE

/* NOTE:
 *
 * The standard logging macros use _XBT_LOG_ISENABLED, which calls _xbt_log_cat_init().  Thus, if we want to avoid an
 * infinite recursion, we can not use the standard logging macros in _xbt_log_cat_init(), and in all functions called
 * from it.
 *
 * To circumvent the problem, we define the macro_xbt_log_init() as (0) for the length of the affected functions, and
 * we do not forget to undefine it at the end!
 */

static void _xbt_log_cat_apply_set(xbt_log_category_t category, xbt_log_setting_t setting)
{
#define _xbt_log_cat_init(a, b) (0)

  if (setting->thresh != xbt_log_priority_uninitialized) {
    xbt_log_threshold_set(category, setting->thresh);

    XBT_DEBUG("Apply settings for category '%s': set threshold to %s (=%d)",
           category->name, xbt_log_priority_names[category->threshold], category->threshold);
  }

  if (setting->fmt) {
    xbt_log_layout_set(category, xbt_log_layout_format_new(setting->fmt));

    XBT_DEBUG("Apply settings for category '%s': set format to %s", category->name, setting->fmt);
  }

  if (setting->additivity != -1) {
    xbt_log_additivity_set(category, setting->additivity);

    XBT_DEBUG("Apply settings for category '%s': set additivity to %s",
           category->name, (setting->additivity ? "on" : "off"));
  }
  if (setting->appender) {
    xbt_log_appender_set(category, setting->appender);
    if (!category->layout)
      xbt_log_layout_set(category, xbt_log_layout_simple_new(NULL));
    category->additivity = 0;
    XBT_DEBUG("Set %p as appender of category '%s'", setting->appender, category->name);
  }
#undef _xbt_log_cat_init
}

/*
 * This gets called the first time a category is referenced and performs the initialization.
 * Also resets threshold to inherited!
 */
int _xbt_log_cat_init(xbt_log_category_t category, e_xbt_log_priority_t priority)
{
#define _xbt_log_cat_init(a, b) (0)

  if (log_cat_init_mutex != NULL)
    xbt_os_mutex_acquire(log_cat_init_mutex);

  if (category->initialized) {
    if (log_cat_init_mutex != NULL)
      xbt_os_mutex_release(log_cat_init_mutex);
    return priority >= category->threshold;
  }

  unsigned int cursor;
  xbt_log_setting_t setting = NULL;
  int found = 0;

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

      XBT_DEBUG("Children of %s: %s; nextSibling: %s", category->parent->name, res,
             (category->parent->nextSibling ? category->parent->nextSibling->name : "none"));

      free(res);
    }
  }

  /* Apply the control */
  if (xbt_log_settings) {
    xbt_assert(category, "NULL category");
    xbt_assert(category->name);

    xbt_dynar_foreach(xbt_log_settings, cursor, setting) {
      xbt_assert(setting, "Damnit, NULL cat in the list");
      xbt_assert(setting->catname, "NULL setting(=%p)->catname", (void *) setting);

      if (!strcmp(setting->catname, category->name)) {
        found = 1;
        _xbt_log_cat_apply_set(category, setting);
        xbt_dynar_cursor_rm(xbt_log_settings, &cursor);
      }
    }

    if (!found)
      XBT_DEBUG("Category '%s': inherited threshold = %s (=%d)",
                category->name, xbt_log_priority_names[category->threshold], category->threshold);
  }

  category->initialized = 1;
  if (log_cat_init_mutex != NULL)
    xbt_os_mutex_release(log_cat_init_mutex);
  return priority >= category->threshold;

#undef _xbt_log_cat_init
}

void xbt_log_parent_set(xbt_log_category_t cat, xbt_log_category_t parent)
{
  xbt_assert(cat, "NULL category to be given a parent");
  xbt_assert(parent, "The parent category of %s is NULL", cat->name);

  /* if the category is initialized, unlink from current parent */
  if (cat->initialized) {
    xbt_log_category_t *cpp = &cat->parent->firstChild;

    while (*cpp != cat && *cpp != NULL) {
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

static void _set_inherited_thresholds(xbt_log_category_t cat)
{
  xbt_log_category_t child = cat->firstChild;

  for (; child != NULL; child = child->nextSibling) {
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
  xbt_log_setting_t set = xbt_new(s_xbt_log_setting_t, 1);
  const char *name, *dot, *eq;

  set->catname = NULL;
  set->thresh = xbt_log_priority_uninitialized;
  set->fmt = NULL;
  set->additivity = -1;
  set->appender = NULL;

  if (!*control_string)
    return set;
  XBT_DEBUG("Parse log setting '%s'", control_string);

  control_string += strspn(control_string, " ");
  name = control_string;
  control_string += strcspn(control_string, ".= ");
  dot = control_string;
  control_string += strcspn(control_string, ":= ");
  eq = control_string;

  if(*dot != '.' && (*eq == '=' || *eq == ':'))
    xbt_die ("Invalid control string '%s'", orig_control_string);

  if (!strncmp(dot + 1, "threshold", (size_t) (eq - dot - 1))) {
    int i;
    char *neweq = xbt_strdup(eq + 1);
    char *p = neweq - 1;

    while (*(++p) != '\0') {
      if (*p >= 'a' && *p <= 'z') {
        *p -= 'a' - 'A';
      }
    }

    XBT_DEBUG("New priority name = %s", neweq);
    for (i = 0; i < xbt_log_priority_infinite; i++) {
      if (!strncmp(xbt_log_priority_names[i], neweq, p - eq)) {
        XBT_DEBUG("This is priority %d", i);
        break;
      }
    }

    if(i<XBT_LOG_STATIC_THRESHOLD){
     fprintf(stderr,
         "Priority '%s' (in setting '%s') is above allowed priority '%s'.\n\n"
         "Compiling SimGrid with -DNDEBUG forbids the levels 'trace' and 'debug'\n"
         "while -DNLOG forbids any logging, at any level.",
             eq + 1, name, xbt_log_priority_names[XBT_LOG_STATIC_THRESHOLD]);
     exit(1);
    }else if (i < xbt_log_priority_infinite) {
      set->thresh = (e_xbt_log_priority_t) i;
    } else {
      THROWF(arg_error, 0,
             "Unknown priority name: %s (must be one of: trace,debug,verbose,info,warning,error,critical)", eq + 1);
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
    if (!strcmp(neweq, "ON") || !strcmp(neweq, "YES") || !strcmp(neweq, "1")) {
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
    }else if (!strncmp(neweq, "rollfile:", 9)) {
    set->appender = xbt_log_appender2_file_new(neweq + 9,1);
    }else if (!strncmp(neweq, "splitfile:", 10)) {
    set->appender = xbt_log_appender2_file_new(neweq + 10,0);
    } else {
      THROWF(arg_error, 0, "Unknown appender log type: '%s'", neweq);
    }
    free(neweq);
  } else if (!strncmp(dot + 1, "fmt", (size_t) (eq - dot - 1))) {
    set->fmt = xbt_strdup(eq + 1);
  } else {
    char buff[512];
    snprintf(buff, MIN(512, eq - dot), "%s", dot + 1);
    THROWF(arg_error, 0, "Unknown setting of the log category: '%s'", buff);
  }
  set->catname = (char *) xbt_malloc(dot - name + 1);

  memcpy(set->catname, name, dot - name);
  set->catname[dot - name] = '\0';      /* Just in case */
  XBT_DEBUG("This is for cat '%s'", set->catname);

  return set;
}

static xbt_log_category_t _xbt_log_cat_searchsub(xbt_log_category_t cat, char *name)
{
  xbt_log_category_t child, res;

  XBT_DEBUG("Search '%s' into '%s' (firstChild='%s'; nextSibling='%s')", name,
         cat->name, (cat->firstChild ? cat->firstChild->name : "none"),
         (cat->nextSibling ? cat->nextSibling->name : "none"));
  if (!strcmp(cat->name, name))
    return cat;

  for (child = cat->firstChild; child != NULL; child = child->nextSibling) {
    XBT_DEBUG("Dig into %s", child->name);
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
 * where [category] is one the category names (see \ref XBT_log_cats for a complete list of the ones defined in the
 * SimGrid library) and keyword is one of the following:
 *
 *    - thres: category's threshold priority. Possible values:
 *             TRACE,DEBUG,VERBOSE,INFO,WARNING,ERROR,CRITICAL
 *    - add or additivity: whether the logging actions must be passed to the parent category.
 *      Possible values: 0, 1, no, yes, on, off.
 *      Default value: yes.
 *    - fmt: the format to use. See \ref log_use_conf_fmt for more information.
 *    - app or appender: the appender to use. See \ref log_use_conf_app for more information.
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
  XBT_DEBUG("Parse log settings '%s'", control_string);

  /* Special handling of no_loc request, which asks for any file localization to be omitted (for tesh runs) */
  if (!strcmp(control_string, "no_loc")) {
    xbt_log_no_loc = 1;
    return;
  }
  /* some initialization if this is the first time that this get called */
  if (xbt_log_settings == NULL)
    xbt_log_settings = xbt_dynar_new(sizeof(xbt_log_setting_t), _free_setting);

  /* split the string, and remove empty entries */
  set_strings = xbt_str_split_quoted(control_string);

  if (xbt_dynar_is_empty(set_strings)) {     /* vicious user! */
    xbt_dynar_free(&set_strings);
    return;
  }

  /* Parse each entry and either use it right now (if the category was already
     created), or store it for further use */
  xbt_dynar_foreach(set_strings, cpt, str) {
    xbt_log_category_t cat = NULL;

    set = _xbt_log_parse_setting(str);
    cat = _xbt_log_cat_searchsub(&_XBT_LOGV(XBT_LOG_ROOT_CAT), set->catname);

    if (cat) {
      XBT_DEBUG("Apply directly");
      _xbt_log_cat_apply_set(cat, set);
      _free_setting((void *) &set);
    } else {
      XBT_DEBUG("Store for further application");
      XBT_DEBUG("push %p to the settings", (void *) set);
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
#define _xbt_log_cat_init(a, b) (0)
  if (!cat->appender) {
    XBT_VERB ("No appender to category %s. Setting the file appender as default", cat->name);
    xbt_log_appender_set(cat, xbt_log_appender_file_new(NULL));
  }
  if (cat->layout) {
    if (cat->layout->free_) {
      cat->layout->free_(cat->layout);
    }
    free(cat->layout);
  }
  cat->layout = lay;
  xbt_log_additivity_set(cat, 0);
#undef _xbt_log_cat_init
}

void xbt_log_additivity_set(xbt_log_category_t cat, int additivity)
{
  cat->additivity = additivity;
}

static void xbt_log_help(void)
{
  printf(
"Description of the logging output:\n"
"\n"
"   Threshold configuration: --log=CATEGORY_NAME.thres:PRIORITY_LEVEL\n"
"      CATEGORY_NAME: defined in code with function 'XBT_LOG_NEW_CATEGORY'\n"
"      PRIORITY_LEVEL: the level to print (trace,debug,verbose,info,warning,error,critical)\n"
"         -> trace: enter and return of some functions\n"
"         -> debug: crufty output\n"
"         -> verbose: verbose output for the user wanting more\n"
"         -> info: output about the regular functionning\n"
"         -> warning: minor issue encountered\n"
"         -> error: issue encountered\n"
"         -> critical: major issue encountered\n"
"\n"
"   Format configuration: --log=CATEGORY_NAME.fmt:OPTIONS\n"
"      OPTIONS may be:\n"
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
"         -> %%P: Process name (SimGrid extension)\n"
"         -> %%t: Thread \"name\" (LOG4J compatible -- actually the address of the thread in memory)\n"
"         -> %%i: Process PID (SimGrid extension -- this is a 'i' as in 'i'dea)\n"
"\n"
"         -> %%F: file name where the log event was raised (LOG4J compatible)\n"
"         -> %%l: location where the log event was raised (LOG4J compatible, like '%%F:%%L' -- this is a l as in 'l'etter)\n"
"         -> %%L: line number where the log event was raised (LOG4J compatible)\n"
"         -> %%M: function name (LOG4J compatible -- called method name here of course).\n"
"                 Defined only when using gcc because there is no __FUNCTION__ elsewhere.\n"
"\n"
"         -> %%b: full backtrace (Called %%throwable in LOG4J). Defined only under windows or when using the GNU libc because\n"
"                 backtrace() is not defined elsewhere, and we only have a fallback for windows boxes, not mac ones for example.\n"
"         -> %%B: short backtrace (only the first line of the %%b). Called %%throwable{short} in LOG4J; defined where %%b is.\n"
"\n"
"         -> %%d: date (UNIX-like epoch)\n"
"         -> %%r: application age (time elapsed since the beginning of the application)\n"
"\n"
"   Miscellaneous:\n"
"      --help-log-categories    Display the current hierarchy of log categories.\n"
"      --log=no_loc             Don't print file names in messages (for tesh tests).\n"
"\n"
    );
}

static int xbt_log_cat_cmp(const void *pa, const void *pb)
{
  xbt_log_category_t a = *(xbt_log_category_t *)pa;
  xbt_log_category_t b = *(xbt_log_category_t *)pb;
  return strcmp(a->name, b->name);
}

static void xbt_log_help_categories_rec(xbt_log_category_t category, const char *prefix)
{
  char *this_prefix;
  char *child_prefix;
  xbt_dynar_t dynar;
  unsigned i;
  xbt_log_category_t cat;

  if (!category)
    return;

  if (category->parent) {
    this_prefix = bprintf("%s \\_ ", prefix);
    child_prefix = bprintf("%s |  ", prefix);
  } else {
    this_prefix = xbt_strdup(prefix);
    child_prefix = xbt_strdup(prefix);
  }

  dynar = xbt_dynar_new(sizeof(xbt_log_category_t), NULL);
  for (cat = category ; cat != NULL; cat = cat->nextSibling)
    xbt_dynar_push_as(dynar, xbt_log_category_t, cat);

  xbt_dynar_sort(dynar, xbt_log_cat_cmp);

  for (i = 0; i < xbt_dynar_length(dynar); i++) {
    if (i == xbt_dynar_length(dynar) - 1 && category->parent)
      *strrchr(child_prefix, '|') = ' ';
    cat = xbt_dynar_get_as(dynar, i, xbt_log_category_t);
    printf("%s%s: %s\n", this_prefix, cat->name, cat->description);
    xbt_log_help_categories_rec(cat->firstChild, child_prefix);
  }

  xbt_dynar_free(&dynar);
  xbt_free(this_prefix);
  xbt_free(child_prefix);
}

static void xbt_log_help_categories(void)
{
  printf("Current log category hierarchy:\n");
  xbt_log_help_categories_rec(&_XBT_LOGV(XBT_LOG_ROOT_CAT), "   ");
  printf("\n");
}
