/* $Id$ */

/* log - a generic logging facility in the spirit of log4j                  */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003, 2004 Martin Quinson.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

typedef struct {
  char *catname;
  gras_log_priority_t thresh;
} gras_log_setting_t;

static gras_dynar_t *gras_log_settings=NULL;
static void _free_setting(void *s) {
  gras_log_setting_t *set=(gras_log_setting_t*)s;
  if (set) {
    free(set->catname);
    free(set);
  }
}

const char *gras_log_priority_names[8] = {
  "NONE",
  "DEBUG",
  "VERBOSE",
  "INFO",
  "WARNING",
  "ERROR",
  "CRITICAL"
};

gras_log_category_t _GRAS_LOGV(GRAS_LOG_ROOT_CAT) = {
  0, 0, 0,
  "root", gras_log_priority_uninitialized, 0,
  NULL, 0
};
GRAS_LOG_NEW_SUBCATEGORY(GRAS,GRAS_LOG_ROOT_CAT);
GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(log,GRAS);


static void _apply_control(gras_log_category_t* cat) {
  int cursor;
  gras_log_setting_t *setting=NULL;
  int found = 0;

  if (!gras_log_settings)
    return;

  gras_assert0(cat,"NULL category");
  gras_assert(cat->name);

  gras_dynar_foreach(gras_log_settings,cursor,setting) {
    gras_assert0(setting,"Damnit, NULL cat in the list");
    gras_assert1(setting->catname,"NULL setting(=%p)->catname",setting);

    if (!strcmp(setting->catname,cat->name)) {
      found = 1;

      gras_log_threshold_set(cat, setting->thresh);
      gras_dynar_cursor_rm(gras_log_settings,&cursor);

      if (cat->threshold <= gras_log_priority_verbose) {
	gras_log_event_t _log_ev = 
	  {cat,gras_log_priority_verbose,__FILE__,__FUNCTION__,__LINE__,
	   "Apply settings for category '%s': set threshold to %s (=%d)",};
	_gras_log_event_log(&_log_ev, cat->name,
			    gras_log_priority_names[cat->threshold], cat->threshold);
      }
    }
  }
  if (!found && cat->threshold <= gras_log_priority_verbose) {
    gras_log_event_t _log_ev = 
      {cat,gras_log_priority_verbose,__FILE__,__FUNCTION__,__LINE__,
       "Category '%s': inherited threshold = %s (=%d)",};
    _gras_log_event_log(&_log_ev, cat->name,
			gras_log_priority_names[cat->threshold], cat->threshold);
  }

}

void _gras_log_event_log( gras_log_event_t* ev, ...) {
  gras_log_category_t* cat = ev->cat;
  va_start(ev->ap, ev);
  while(1) {
    gras_log_appender_t* appender = cat->appender;
    if (appender != NULL) {
      appender->do_append(appender, ev);
    }
    if (!cat->willLogToParent)
      break;

    cat = cat->parent;
  } 
  va_end(ev->ap);
}

static void _cat_init(gras_log_category_t* category) {
  if (category == &_GRAS_LOGV(GRAS_LOG_ROOT_CAT)) {
    category->threshold = gras_log_priority_info;
    category->appender = gras_log_default_appender;
  } else {
    gras_log_parent_set(category, category->parent);
  }
  _apply_control(category);
}

/*
 * This gets called the first time a category is referenced and performs the
 * initialization. 
 * Also resets threshold to inherited!
 */
int _gras_log_cat_init(gras_log_priority_t priority,
		       gras_log_category_t* category) {
    
  _cat_init(category);
        
  return priority >= category->threshold;
}

void gras_log_parent_set(gras_log_category_t* cat,
			 gras_log_category_t* parent) {

  gras_assert0(cat,"NULL category to be given a parent");
  gras_assert1(parent,"The parent category of %s is NULL",cat->name);

  // unlink from current parent
  if (cat->threshold != gras_log_priority_uninitialized) {
    gras_log_category_t** cpp = &parent->firstChild;
    while(*cpp != cat && *cpp != NULL) {
      cpp = &(*cpp)->nextSibling;
    }
    assert(*cpp == cat);
    *cpp = cat->nextSibling;
  }

  // Set new parent
  cat->parent = parent;
  cat->nextSibling = parent->firstChild;
  parent->firstChild = cat;

  // Make sure parent is initialized
  if (parent->threshold == gras_log_priority_uninitialized) {
    _cat_init(parent);
  }
    
  // Reset priority
  cat->threshold = parent->threshold;
  cat->isThreshInherited = 1;
} // log_setParent

static void _set_inherited_thresholds(gras_log_category_t* cat) {
  gras_log_category_t* child = cat->firstChild;
  for( ; child != NULL; child = child->nextSibling) {
    if (child->isThreshInherited) {
      if (cat != &_GRAS_LOGV(log))
	VERB3("Set category threshold of %s to %s (=%d)",
	      child->name,gras_log_priority_names[cat->threshold],cat->threshold);
      child->threshold = cat->threshold;
      _set_inherited_thresholds(child);
    }
  }
}

void gras_log_threshold_set(gras_log_category_t* cat, 
			    gras_log_priority_t threshold) {
  cat->threshold = threshold;
  cat->isThreshInherited = 0;
  _set_inherited_thresholds(cat);
}

static gras_error_t _gras_log_parse_setting(const char* control_string,
					    gras_log_setting_t *set) {
  const char *name, *dot, *eq;
  
  set->catname=NULL;
  if (!*control_string) 
    return no_error;
  DEBUG1("Parse log setting '%s'",control_string);

  control_string += strspn(control_string, " ");
  name = control_string;
  control_string += strcspn(control_string, ".= ");
  dot = control_string;
  control_string += strcspn(control_string, "= ");
  eq = control_string;
  control_string += strcspn(control_string, " ");

  gras_assert1(*dot == '.' && *eq == '=',
	       "Invalid control string '%s'",control_string);

  if (!strncmp(dot + 1, "thresh", MIN(eq - dot - 1,strlen("thresh")))) {
    int i;
    char *neweq=strdup(eq+1);
    char *p=neweq-1;
    
    while (*(++p) != '\0') {
      if (*p >= 'a' && *p <= 'z') {
	*p-='a'-'A';
      }
    }
    
    DEBUG1("New priority name = %s",neweq);
    for (i=0; i<6; i++) {
      if (!strncmp(gras_log_priority_names[i],neweq,p-eq)) {
	DEBUG1("This is priority %d",i);
	break;
      }
    }
    if (i<6) {
      set->thresh=i;
    } else {
      gras_assert1(FALSE,"Unknown priority name: %s",eq+1);
    }
    free(neweq);
  } else {
    char buff[512];
    snprintf(buff,MIN(512,eq - dot - 1),"%s",dot+1);
    gras_assert1(FALSE,"Unknown setting of the log category: %s",buff);
  }
  if (!(set->catname=malloc(dot - name+1)))
    RAISE_MALLOC;
    
  strncat(set->catname,name,dot-name);
  DEBUG1("This is for cat '%s'", set->catname);
  return no_error;
}

static gras_error_t _gras_log_cat_searchsub(gras_log_category_t *cat,char *name,gras_log_category_t**whereto) {
  gras_error_t errcode;
  gras_log_category_t *child;
  
  if (!strcmp(cat->name,name)) {
    *whereto=cat;
    return no_error;
  }
  for(child=cat->firstChild ; child != NULL; child = child->nextSibling) {
    errcode=_gras_log_cat_searchsub(child,name,whereto);
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
 * gras_log_control_set:
 * @cs: What to parse
 * @Returns: malloc_error or no_error
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
gras_error_t gras_log_control_set(const char* control_string) {
  gras_error_t errcode;
  gras_log_setting_t *set;
  char *cs;
  char *p;
  int done = 0;
  
  DEBUG1("Parse log settings '%s'",control_string);
  if (control_string == NULL)
    return no_error;
  if (gras_log_settings == NULL)
    TRY(gras_dynar_new(&gras_log_settings,sizeof(gras_log_setting_t*),_free_setting));

  if (!(set = malloc(sizeof(gras_log_setting_t))))
    RAISE_MALLOC;

  if (!(cs=strdup(control_string)))
    RAISE_MALLOC;
  _cleanup_double_spaces(cs);

  while (!done) {
    gras_log_category_t *cat;
    
    p=strrchr(cs,' ');
    if (p) {
      *p='\0';
      *p++;
    } else {
      p=cs;
      done = 1;
    }
    errcode = _gras_log_parse_setting(p,set);
    if (errcode != no_error) {
      free(set);
      free(cs);
    }
    
    TRYCATCH(_gras_log_cat_searchsub(&_GRAS_LOGV(root),set->catname,&cat),mismatch_error);
    if (errcode == mismatch_error) {
      DEBUG0("Store for further application");
      DEBUG1("push %p to the settings",set);
      TRY(gras_dynar_push(gras_log_settings,&set));
      /* malloc in advance the next slot */
      if (!(set = malloc(sizeof(gras_log_setting_t)))) { 
	free(cs);
	RAISE_MALLOC;
      }
    } else {
      DEBUG0("Apply directly");
      free(set->catname);
      gras_log_threshold_set(cat,set->thresh);
    }
  }
  free(set);
  free(cs);
  return no_error;
} 

void gras_log_appender_set(gras_log_category_t* cat, gras_log_appender_t* app) {
  cat->appender = app;
}

void gras_log_finalize(void);
void gras_log_finalize(void) {
  gras_dynar_free(gras_log_settings);
}
