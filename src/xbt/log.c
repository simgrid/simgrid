/* $Id$ */

/* log - a generic logging facility in the spirit of log4j                  */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003, 2004 Martin Quinson.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdarg.h>
#include <ctype.h>

#include "xbt_modinter.h"

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"

typedef struct {
  char *catname;
  e_xbt_log_priority_t thresh;
} s_xbt_log_setting_t,*xbt_log_setting_t;

static xbt_dynar_t xbt_log_settings=NULL;
static void _free_setting(void *s) {
  xbt_log_setting_t set=(xbt_log_setting_t)s;
  if (set) {
    xbt_free(set->catname);
/*    xbt_free(set); FIXME: uncommenting this leads to segfault when more than one chunk is passed as gras-log */
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

XBT_LOG_NEW_SUBCATEGORY(gras,XBT_LOG_ROOT_CAT,"All GRAS categories");
XBT_LOG_NEW_SUBCATEGORY(xbt,XBT_LOG_ROOT_CAT,"All XBT categories (gras toolbox)");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(log,xbt,"Loggings from the logging mecanism itself");


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

      if (cat->threshold <= xbt_log_priority_verbose) {
	s_xbt_log_event_t _log_ev = 
	  {cat,xbt_log_priority_verbose,__FILE__,_XBT_GNUC_FUNCTION,__LINE__};
	_xbt_log_event_log(&_log_ev,
	         "Apply settings for category '%s': set threshold to %s (=%d)",
		 cat->name, 
	         xbt_log_priority_names[cat->threshold], cat->threshold);
      }
    }
  }
  if (!found && cat->threshold <= xbt_log_priority_verbose) {
    s_xbt_log_event_t _log_ev = 
      {cat,xbt_log_priority_verbose,__FILE__,_XBT_GNUC_FUNCTION,__LINE__};
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

  if (!strncmp(dot + 1, "thresh", min(eq - dot - 1,strlen("thresh")))) {
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
      set->thresh=i;
    } else {
      xbt_assert1(FALSE,"Unknown priority name: %s",eq+1);
    }
    xbt_free(neweq);
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

static xbt_error_t _xbt_log_cat_searchsub(xbt_log_category_t cat,char *name,
					    /*OUT*/xbt_log_category_t*whereto) {
  xbt_error_t errcode;
  xbt_log_category_t child;
  
  if (!strcmp(cat->name,name)) {
    *whereto=cat;
    return no_error;
  }
  for(child=cat->firstChild ; child != NULL; child = child->nextSibling) {
    errcode=_xbt_log_cat_searchsub(child,name,whereto);
    if (errcode==no_error)
      return no_error;
  }
  return mismatch_error;
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
 * xbt_log_control_set:
 * @cs: What to parse
 *
 * Typically passed a command-line argument. The string has the syntax:
 *
 *      ( [category] "." [keyword] "=" value (" ")... )...
 *
 * where [category] is one the category names and keyword is one of the
 * following:
 *
 *      thresh  	value is an integer priority level. Sets the category's
 *                        threshold priority.
 *
 * @warning
 * This routine may only be called once and that must be before any other
 * logging command! Typically, this is done from main().
 */
void xbt_log_control_set(const char* control_string) {
  xbt_error_t errcode;
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
    xbt_log_category_t cat;
    
    p=strrchr(cs,' ');
    if (p) {
      *p='\0';
      *p++;
    } else {
      p=cs;
      done = 1;
    }
    _xbt_log_parse_setting(p,set);
    
    errcode = _xbt_log_cat_searchsub(&_XBT_LOGV(root),set->catname,&cat);
    if (errcode == mismatch_error) {
      DEBUG0("Store for further application");
      DEBUG1("push %p to the settings",(void*)set);
      xbt_dynar_push(xbt_log_settings,&set);
      /* malloc in advance the next slot */
      set = xbt_new(s_xbt_log_setting_t,1);
    } else {
      DEBUG0("Apply directly");
      xbt_free(set->catname);
      xbt_log_threshold_set(cat,set->thresh);
    }
  }
  xbt_free(set);
  xbt_free(cs);
} 

void xbt_log_appender_set(xbt_log_category_t cat, xbt_log_appender_t app) {
  cat->appender = app;
}

void xbt_log_exit(void) {
  VERB0("Exiting log");
  xbt_dynar_free(&xbt_log_settings);
  VERB0("Exited log");
}
