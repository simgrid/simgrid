/* log - a generic logging facility in the spirit of log4j                  */

/* Copyright (c) 2004-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>              /* snprintf */
#include <stdlib.h>             /* snprintf */

#include "src/internal_config.h"

#include "src/xbt_modinter.h"

#include "src/xbt/log_private.h"
#include "xbt/asserts.h"
#include "xbt/dynar.h"
#include "xbt/ex.h"
#include "xbt/misc.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"
#include "xbt/xbt_os_thread.h"

int xbt_log_no_loc = 0; /* if set to true (with --log=no_loc), file localization will be omitted (for tesh tests) */
static xbt_os_mutex_t log_cat_init_mutex = NULL;

/** \addtogroup XBT_log
 *
 *  For more information, please refer to @ref outcomes_logs Section.
 */

xbt_log_appender_t xbt_log_default_appender = NULL;     /* set in log_init */
xbt_log_layout_t xbt_log_default_layout = NULL; /* set in log_init */

typedef struct {
  char *catname;
  char *fmt;
  e_xbt_log_priority_t thresh;
  int additivity;
  xbt_log_appender_t appender;
} s_xbt_log_setting_t;

typedef s_xbt_log_setting_t* xbt_log_setting_t;

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
  XBT_LOG_CONNECT(xbt_cfg);
  XBT_LOG_CONNECT(xbt_dict);
  XBT_LOG_CONNECT(xbt_dict_cursor);
  XBT_LOG_CONNECT(xbt_dict_elm);
  XBT_LOG_CONNECT(xbt_dyn);
  XBT_LOG_CONNECT(xbt_ex);
  XBT_LOG_CONNECT(xbt_automaton);
  XBT_LOG_CONNECT(xbt_backtrace);
  XBT_LOG_CONNECT(xbt_exception);
  XBT_LOG_CONNECT(xbt_graph);
  XBT_LOG_CONNECT(xbt_mallocator);
  XBT_LOG_CONNECT(xbt_memory_map);
  XBT_LOG_CONNECT(xbt_parmap);
  XBT_LOG_CONNECT(xbt_sync);
  XBT_LOG_CONNECT(xbt_sync_os);

#ifdef simgrid_EXPORTS
  /* The following categories are only defined in libsimgrid */

  /* bindings */
#if SIMGRID_HAVE_LUA
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
  XBT_LOG_CONNECT(instr_paje_containers);
  XBT_LOG_CONNECT(instr_paje_header);
  XBT_LOG_CONNECT(instr_paje_trace);
  XBT_LOG_CONNECT(instr_paje_types);
  XBT_LOG_CONNECT(instr_paje_values);
  XBT_LOG_CONNECT(instr_resource);
  XBT_LOG_CONNECT(instr_routing);
  XBT_LOG_CONNECT(instr_surf);

  /* jedule */
#if SIMGRID_HAVE_JEDULE
  XBT_LOG_CONNECT(jedule);
  XBT_LOG_CONNECT(jed_sd);
#endif

  /* mc */
#if SIMGRID_HAVE_MC
  XBT_LOG_CONNECT(mc);
  XBT_LOG_CONNECT(mc_checkpoint);
  XBT_LOG_CONNECT(mc_comm_determinism);
  XBT_LOG_CONNECT(mc_compare);
  XBT_LOG_CONNECT(mc_dwarf);
  XBT_LOG_CONNECT(mc_hash);
  XBT_LOG_CONNECT(mc_liveness);
  XBT_LOG_CONNECT(mc_memory);
  XBT_LOG_CONNECT(mc_page_snapshot);
  XBT_LOG_CONNECT(mc_request);
  XBT_LOG_CONNECT(mc_safety);
  XBT_LOG_CONNECT(mc_VisitedState);
  XBT_LOG_CONNECT(mc_client);
  XBT_LOG_CONNECT(mc_client_api);
  XBT_LOG_CONNECT(mc_comm_pattern);
  XBT_LOG_CONNECT(mc_process);
  XBT_LOG_CONNECT(mc_protocol);
  XBT_LOG_CONNECT(mc_Channel);
  XBT_LOG_CONNECT(mc_ModelChecker);
  XBT_LOG_CONNECT(mc_RegionSnaphot);
  XBT_LOG_CONNECT(mc_Session);
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

  /* s4u */
  XBT_LOG_CONNECT(s4u);
  XBT_LOG_CONNECT(s4u_activity);
  XBT_LOG_CONNECT(s4u_actor);
  XBT_LOG_CONNECT(s4u_netzone);
  XBT_LOG_CONNECT(s4u_channel);
  XBT_LOG_CONNECT(s4u_comm);
  XBT_LOG_CONNECT(s4u_file);
  XBT_LOG_CONNECT(s4u_link);
  XBT_LOG_CONNECT(s4u_vm);

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
  XBT_LOG_CONNECT(simix_mailbox);
  XBT_LOG_CONNECT(simix_network);
  XBT_LOG_CONNECT(simix_process);
  XBT_LOG_CONNECT(simix_popping);
  XBT_LOG_CONNECT(simix_synchro);

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
#if SIMGRID_HAVE_NS3
  XBT_LOG_CONNECT(ns3);
#endif
  XBT_LOG_CONNECT(surf_parse);
  XBT_LOG_CONNECT(surf_plugin_load);
  XBT_LOG_CONNECT(surf_route);
  XBT_LOG_CONNECT(surf_routing_generic);
  XBT_LOG_CONNECT(surf_route_cluster);
  XBT_LOG_CONNECT(surf_route_cluster_torus);
  XBT_LOG_CONNECT(surf_route_cluster_dragonfly);
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
  int j                   = 1;

  /* Set logs and init log submodule */
  for (int i = 1; i < *argc; i++) {
    if (!strncmp(argv[i], "--log=", strlen("--log="))) {
      char* opt = strchr(argv[i], '=');
      opt++;
      xbt_log_control_set(opt);
      XBT_DEBUG("Did apply '%s' as log setting", opt);
    } else if (!strcmp(argv[i], "--help-logs")) {
      help_requested |= 1U;
    } else if (!strcmp(argv[i], "--help-log-categories")) {
      help_requested |= 2U;
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

/* Size of the static string in which we build the log string */
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

  while (1) {
    xbt_log_appender_t appender = cat->appender;

    if (appender != NULL) {
      xbt_assert(cat->layout, "No valid layout for the appender of category %s", cat->name);

      /* First, try with a static buffer */
      int done = 0;
      char buff[XBT_LOG_STATIC_BUFFER_SIZE];
      ev->buffer      = buff;
      ev->buffer_size = sizeof buff;
      va_start(ev->ap, fmt);
      done = cat->layout->do_layout(cat->layout, ev, fmt);
      va_end(ev->ap);
      if (done) {
        appender->do_append(appender, buff);
      } else {

        /* The static buffer was too small, use a dynamically expanded one */
        ev->buffer_size = XBT_LOG_DYNAMIC_BUFFER_SIZE;
        ev->buffer      = xbt_malloc(ev->buffer_size);
        while (1) {
          va_start(ev->ap, fmt);
          done = cat->layout->do_layout(cat->layout, ev, fmt);
          va_end(ev->ap);
          if (done)
            break; /* Got it */
          ev->buffer_size *= 2;
          ev->buffer = xbt_realloc(ev->buffer, ev->buffer_size);
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
static int fake_xbt_log_cat_init(xbt_log_category_t XBT_ATTRIB_UNUSED category,
                                 e_xbt_log_priority_t XBT_ATTRIB_UNUSED priority)
{
  return 0;
}
#define DISABLE_XBT_LOG_CAT_INIT()                                                                                     \
  int (*_xbt_log_cat_init)(xbt_log_category_t, e_xbt_log_priority_t) XBT_ATTRIB_UNUSED = fake_xbt_log_cat_init;

static void _xbt_log_cat_apply_set(xbt_log_category_t category, xbt_log_setting_t setting)
{
  DISABLE_XBT_LOG_CAT_INIT();
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
}

/*
 * This gets called the first time a category is referenced and performs the initialization.
 * Also resets threshold to inherited!
 */
int _xbt_log_cat_init(xbt_log_category_t category, e_xbt_log_priority_t priority)
{
  DISABLE_XBT_LOG_CAT_INIT();
  if (log_cat_init_mutex != NULL)
    xbt_os_mutex_acquire(log_cat_init_mutex);

  if (category->initialized) {
    if (log_cat_init_mutex != NULL)
      xbt_os_mutex_release(log_cat_init_mutex);
    return priority >= category->threshold;
  }

  unsigned int cursor;
  xbt_log_setting_t setting = NULL;

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
      char *buf;
      char *res = NULL;
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
    int found = 0;

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

  set->catname = NULL;
  set->thresh = xbt_log_priority_uninitialized;
  set->fmt = NULL;
  set->additivity = -1;
  set->appender = NULL;

  if (!*control_string)
    return set;
  XBT_DEBUG("Parse log setting '%s'", control_string);

  control_string += strspn(control_string, " ");
  const char *name = control_string;
  control_string += strcspn(control_string, ".:= ");
  const char *dot = control_string;
  control_string += strcspn(control_string, ":= ");
  const char *eq = control_string;

  xbt_assert(*dot == '.' || (*eq != '=' && *eq != ':'), "Invalid control string '%s'", orig_control_string);

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
    xbt_die("Unknown setting of the log category: '%s'", buff);
  }
  set->catname = (char *) xbt_malloc(dot - name + 1);

  memcpy(set->catname, name, dot - name);
  set->catname[dot - name] = '\0';      /* Just in case */
  XBT_DEBUG("This is for cat '%s'", set->catname);

  return set;
}

static xbt_log_category_t _xbt_log_cat_searchsub(xbt_log_category_t cat, char *name)
{
  xbt_log_category_t child;
  xbt_log_category_t res;

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

  /* Parse each entry and either use it right now (if the category was already created), or store it for further use */
  xbt_dynar_foreach(set_strings, cpt, str) {
    set = _xbt_log_parse_setting(str);
    xbt_log_category_t cat = _xbt_log_cat_searchsub(&_XBT_LOGV(XBT_LOG_ROOT_CAT), set->catname);

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
  DISABLE_XBT_LOG_CAT_INIT();
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
}

void xbt_log_additivity_set(xbt_log_category_t cat, int additivity)
{
  cat->additivity = additivity;
}

static void xbt_log_help(void)
{
  printf("Description of the logging output:\n"
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
         "         -> %%P: Process name (SimGrid extension)\n"
         "         -> %%t: Thread \"name\" (LOG4J compatible -- actually the address of the thread in memory)\n"
         "         -> %%i: Process PID (SimGrid extension -- this is a 'i' as in 'i'dea)\n"
         "\n"
         "         -> %%F: file name where the log event was raised (LOG4J compatible)\n"
         "         -> %%l: location where the log event was raised (LOG4J compatible, like '%%F:%%L' -- this is a l as "
         "in 'l'etter)\n"
         "         -> %%L: line number where the log event was raised (LOG4J compatible)\n"
         "         -> %%M: function name (LOG4J compatible -- called method name here of course).\n"
         "                 Defined only when using gcc because there is no __FUNCTION__ elsewhere.\n"
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
         "   Miscellaneous:\n"
         "      --help-log-categories    Display the current hierarchy of log categories.\n"
         "      --log=no_loc             Don't print file names in messages (for tesh tests).\n"
         "\n");
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

  xbt_dynar_t dynar = xbt_dynar_new(sizeof(xbt_log_category_t), NULL);
  for (cat = category ; cat != NULL; cat = cat->nextSibling)
    xbt_dynar_push_as(dynar, xbt_log_category_t, cat);

  xbt_dynar_sort(dynar, xbt_log_cat_cmp);

  xbt_dynar_foreach(dynar, i, cat){
    if (i == xbt_dynar_length(dynar) - 1 && category->parent)
      *strrchr(child_prefix, '|') = ' ';
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
