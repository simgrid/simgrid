/* $Id$ */

/* config - Dictionnary where the type of each cell is provided.            */

/* This is useful to build named structs, like option or property sets.     */

/* Copyright (c) 2001,2002,2003,2004 Martin Quinson. All rights reserved.   */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

#include "xbt/config.h" /* prototypes of this module */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(config,xbt,"configuration support");

/* xbt_cfgelm_t: the typedef corresponding to a config cell. 

   Both data and DTD are mixed, but fixing it now would prevent me to ever
   defend my thesis. */

typedef struct {
  /* Allowed type of the cell */
  e_xbt_cfgelm_type_t type;
  int min,max;

  /* actual content 
     (cannot be an union because type host uses both str and i) */
  xbt_dynar_t content;
} s_xbt_cfgelm_t,*xbt_cfgelm_t;

static const char *xbt_cfgelm_type_name[xbt_cfgelm_type_count]=
  {"int","double","string","host"};

/* Internal stuff used in cache to free a cell */
static void xbt_cfgelm_free(void *data);

/* Retrieve the cell we'll modify */
static xbt_error_t xbt_cfgelm_get(xbt_cfg_t cfg, const char *name,
				    e_xbt_cfgelm_type_t type,
				    /* OUT */ xbt_cfgelm_t *whereto);

void xbt_cfg_str_free(void *d);
void xbt_cfg_host_free(void *d);

void xbt_cfg_str_free(void *d){
  xbt_free(*(void**)d);
}
void xbt_cfg_host_free(void *d){
  xbt_host_t *h=(xbt_host_t*) *(void**)d; 
  if (h) {
    if (h->name) xbt_free(h->name);
    xbt_free(h);
  }
}

/*----[ Memory management ]-----------------------------------------------*/

/**
 * xbt_cfg_new:
 *
 * @whereto: 
 *
 * Initialise an config set
 */


xbt_cfg_t xbt_cfg_new(void) {
  return (xbt_cfg_t)xbt_dict_new();
}

/**
 * xbt_cfg_cpy:
 *
 * @whereto: the config set to be created
 * @tocopy: the source data
 *
 */

void
xbt_cfg_cpy(xbt_cfg_t tocopy,xbt_cfg_t *whereto) {
  xbt_dict_cursor_t cursor=NULL; 
  xbt_cfgelm_t cell=NULL;
  char *name=NULL;
  
  *whereto=NULL;
  xbt_assert0(tocopy,"cannot copy NULL config");

  xbt_dict_foreach((xbt_dict_t)tocopy,cursor,name,cell) {
    xbt_cfg_register(*whereto, name, cell->type, cell->min, cell->max);
  }
}

void xbt_cfg_free(xbt_cfg_t *cfg) {
  xbt_dict_free((xbt_dict_t*)cfg);
}

/**
 * xbt_cfg_dump:
 * @name: The name to give to this config set
 * @indent: what to write at the begining of each line (right number of spaces)
 * @cfg: the config set
 *
 * Dumps a config set for debuging purpose
 */
void
xbt_cfg_dump(const char *name,const char *indent,xbt_cfg_t cfg) {
  xbt_dict_t dict = (xbt_dict_t) cfg;
  xbt_dict_cursor_t cursor=NULL; 
  xbt_cfgelm_t cell=NULL;
  char *key=NULL;
  int i; 
  int size;
  int ival;
  char *sval;
  double dval;
  xbt_host_t *hval;

  if (name)
    printf("%s>> Dumping of the config set '%s':\n",indent,name);
  xbt_dict_foreach(dict,cursor,key,cell) {

    printf("%s  %s:",indent,key);

    size = xbt_dynar_length(cell->content);
    printf("%d_to_%d_%s. Actual size=%d. List of values:\n",
	   cell->min,cell->max,xbt_cfgelm_type_name[cell->type],
	   size);

    switch (cell->type) {
       
    case xbt_cfgelm_int:
      for (i=0; i<size; i++) {
	ival = xbt_dynar_get_as(cell->content,i,int);
	printf ("%s    %d\n",indent,ival);
      }
      break;

    case xbt_cfgelm_double:
      for (i=0; i<size; i++) {
	dval = xbt_dynar_get_as(cell->content,i,double);
	printf ("%s    %f\n",indent,dval);
      }
      break;

    case xbt_cfgelm_string:
      for (i=0; i<size; i++) {
	sval = xbt_dynar_get_as(cell->content,i,char*);
	printf ("%s    %s\n",indent,sval);
      }
      break;

    case xbt_cfgelm_host:
      for (i=0; i<size; i++) {
	hval = xbt_dynar_get_as(cell->content,i,xbt_host_t*);
	printf ("%s    %s:%d\n",indent,hval->name,hval->port);
      }
      break;

    default:
      printf("%s    Invalid type!!\n",indent);
    }

  }

  if (name) printf("%s<< End of the config set '%s'\n",indent,name);
  fflush(stdout);

  xbt_dict_cursor_free(&cursor);
  return;
}

/**
 * xbt_cfgelm_free:
 *
 * @data: the data to be freed (typed as void* to be usable as free funct in dict)
 *
 * free an config element
 */

void xbt_cfgelm_free(void *data) {
  xbt_cfgelm_t c=(xbt_cfgelm_t)data;

  if (!c) return;
  xbt_dynar_free(&(c->content));
  xbt_free(c);
}

/*----[ Registering stuff ]-----------------------------------------------*/

/**
 * xbt_cfg_register:
 * @cfg: the config set
 * @type: the type of the config element
 * @min: the minimum
 * @max: the maximum
 *
 * register an element within a config set
 */

void
xbt_cfg_register(xbt_cfg_t cfg,
		  const char *name, e_xbt_cfgelm_type_t type,
		  int min, int max){
  xbt_cfgelm_t res;
  xbt_error_t errcode;

  DEBUG4("Register cfg elm %s (%d to %d %s)",name,min,max,xbt_cfgelm_type_name[type]);
  errcode = xbt_dict_get((xbt_dict_t)cfg,name,(void**)&res);

  if (errcode == no_error) {
    WARN1("Config elem %s registered twice.",name);
    /* Will be removed by the insertion of the new one */
  } 
  xbt_assert_error(mismatch_error);

  res=xbt_new(s_xbt_cfgelm_t,1);

  res->type=type;
  res->min=min;
  res->max=max;

  switch (type) {
  case xbt_cfgelm_int:
    res->content = xbt_dynar_new(sizeof(int), NULL);
    break;

  case xbt_cfgelm_double:
    res->content = xbt_dynar_new(sizeof(double), NULL);
    break;

  case xbt_cfgelm_string:
   res->content = xbt_dynar_new(sizeof(char*),&xbt_cfg_str_free);
   break;

  case xbt_cfgelm_host:
   res->content = xbt_dynar_new(sizeof(xbt_host_t*),&xbt_cfg_host_free);
   break;

  default:
    ERROR1("%d is an invalide type code",type);
  }
    
  xbt_dict_set((xbt_dict_t)cfg,name,res,&xbt_cfgelm_free);
}

/**
 * xbt_cfg_unregister:
 * 
 * @cfg: the config set
 * @name: the name of the elem to be freed
 * 
 * unregister an element from a config set. 
 * Note that it removes both the DTD and the actual content.
 */

xbt_error_t
xbt_cfg_unregister(xbt_cfg_t cfg,const char *name) {
  return xbt_dict_remove((xbt_dict_t)cfg,name);
}

/**
 * xbt_cfg_register_str:
 *
 * @cfg: the config set
 * @entry: a string describing the element to register
 *
 * Parse a string and register the stuff described.
 */

xbt_error_t
xbt_cfg_register_str(xbt_cfg_t cfg,const char *entry) {
  char *entrycpy=xbt_strdup(entry);
  char *tok;

  int min,max;
  e_xbt_cfgelm_type_t type;

  tok=strchr(entrycpy, ':');
  if (!tok) {
    ERROR3("%s%s%s",
	  "Invalid config element descriptor: ",entry,
	  "; Should be <name>:<min nb>_to_<max nb>_<type>");
    xbt_free(entrycpy);
    xbt_abort();
  }
  *(tok++)='\0';

  min=strtol(tok, &tok, 10);
  if (!tok) {
    ERROR1("Invalid minimum in config element descriptor %s",entry);
    xbt_free(entrycpy);
    xbt_abort();
  }

  if (!strcmp(tok,"_to_")){
    ERROR3("%s%s%s",
	  "Invalid config element descriptor: ",entry,
	  "; Should be <name>:<min nb>_to_<max nb>_<type>");
    xbt_free(entrycpy);
    xbt_abort();
  }
  tok += strlen("_to_");

  max=strtol(tok, &tok, 10);
  if (!tok) {
    ERROR1("Invalid maximum in config element descriptor %s",entry);
    xbt_free(entrycpy);
    xbt_abort();
  }

  if (*(tok++)!='_') {
    ERROR3("%s%s%s",
	  "Invalid config element descriptor: ",entry,
	  "; Should be <name>:<min nb>_to_<max nb>_<type>");
    xbt_free(entrycpy);
    xbt_abort();
  }

  for (type=0; 
       type<xbt_cfgelm_type_count && strcmp(tok,xbt_cfgelm_type_name[type]); 
       type++);
  if (type == xbt_cfgelm_type_count) {
    ERROR3("%s%s%s",
	  "Invalid type in config element descriptor: ",entry,
	  "; Should be one of 'string', 'int', 'host' or 'double'.");
    xbt_free(entrycpy);
    xbt_abort();
  }

  xbt_cfg_register(cfg,entrycpy,type,min,max);

  xbt_free(entrycpy); /* strdup'ed by dict mechanism, but cannot be const */
  return no_error;
}

/**
 * xbt_cfg_check:
 *
 * @cfg: the config set
 * 
 * Check the config set
 */

xbt_error_t
xbt_cfg_check(xbt_cfg_t cfg) {
  xbt_dict_cursor_t cursor; 
  xbt_cfgelm_t cell;
  char *name;
  int size;

  xbt_assert0(cfg,"NULL config set.");

  xbt_dict_foreach((xbt_dict_t)cfg,cursor,name,cell) {
    size = xbt_dynar_length(cell->content);
    if (cell->min > size) { 
      ERROR4("Config elem %s needs at least %d %s, but there is only %d values.",
	     name,
	     cell->min,
	     xbt_cfgelm_type_name[cell->type],
	     size); 
      xbt_dict_cursor_free(&cursor);
      return mismatch_error;
    }

    if (cell->max < size) {
      ERROR4("Config elem %s accepts at most %d %s, but there is %d values.",
	     name,
	     cell->max,
	     xbt_cfgelm_type_name[cell->type],
	     size);
      xbt_dict_cursor_free(&cursor);
      return mismatch_error;
    }

  }

  xbt_dict_cursor_free(&cursor);
  return no_error;
}

static xbt_error_t xbt_cfgelm_get(xbt_cfg_t  cfg,
				    const char *name,
				    e_xbt_cfgelm_type_t type,
				    /* OUT */ xbt_cfgelm_t *whereto){
   
  xbt_error_t errcode = xbt_dict_get((xbt_dict_t)cfg,name,
				       (void**)whereto);

  if (errcode == mismatch_error) {
    ERROR1("No registered cell %s in this config set",
	   name);
    return mismatch_error;
  }
  if (errcode != no_error)
     return errcode;

  xbt_assert3((*whereto)->type == type,
	       "You tried to access to the config element %s as an %s, but its type is %s.",
	       name,
	       xbt_cfgelm_type_name[type],
	       xbt_cfgelm_type_name[(*whereto)->type]);

  return no_error;
}

/**
 * xbt_cfg_get_type:
 *
 * @cfg: the config set
 * @name: the name of the element 
 * @type: the result
 *
 * Give the type of the config element
 */

xbt_error_t
xbt_cfg_get_type(xbt_cfg_t cfg, const char *name, 
		      /* OUT */e_xbt_cfgelm_type_t *type) {

  xbt_cfgelm_t cell;
  xbt_error_t errcode;

  TRYCATCH(mismatch_error,xbt_dict_get((xbt_dict_t)cfg,name,(void**)&cell));

  if (errcode == mismatch_error) {
    ERROR1("Can't get the type of '%s' since this cell does not exist",
	   name);
    return mismatch_error;
  }

  *type=cell->type;

  return no_error;
}

/*----[ Setting ]---------------------------------------------------------*/
/** 
 * xbt_cfg_set_vargs(): 
 * @cfg: config set to fill
 * @varargs: NULL-terminated list of pairs {(const char*)key, value}
 *
 * Add some values to the config set.
 * @warning: if the list isn't NULL terminated, it will segfault. 
 */
xbt_error_t
xbt_cfg_set_vargs(xbt_cfg_t cfg, va_list pa) {
  char *str,*name;
  int i;
  double d;
  e_xbt_cfgelm_type_t type;

  xbt_error_t errcode;
  
  while ((name=va_arg(pa,char *))) {

    if (!xbt_cfg_get_type(cfg,name,&type)) {
      ERROR1("Can't set the property '%s' since it's not registered",name);
      return mismatch_error;
    }

    switch (type) {
    case xbt_cfgelm_host:
      str = va_arg(pa, char *);
      i=va_arg(pa,int);
      TRY(xbt_cfg_set_host(cfg,name,str,i));
      break;
      
    case xbt_cfgelm_string:
      str=va_arg(pa, char *);
      TRY(xbt_cfg_set_string(cfg, name, str));
      break;

    case xbt_cfgelm_int:
      i=va_arg(pa,int);
      TRY(xbt_cfg_set_int(cfg,name,i));
      break;

    case xbt_cfgelm_double:
      d=va_arg(pa,double);
      TRY(xbt_cfg_set_double(cfg,name,d));
      break;

    default: 
      RAISE1(unknown_error,"Config element cell %s not valid.",name);
    }
  }
  return no_error;
}

/** 
 * xbt_cfg_set():
 * @cfg: config set to fill
 * @varargs: NULL-terminated list of pairs {(const char*)key, value}
 *
 * Add some values to the config set.
 * @warning: if the list isn't NULL terminated, it will segfault. 
 */
xbt_error_t xbt_cfg_set(xbt_cfg_t cfg, ...) {
  va_list pa;
  xbt_error_t errcode;

  va_start(pa,cfg);
  errcode=xbt_cfg_set_vargs(cfg,pa);
  va_end(pa);
  return errcode;
}

/**
 * xbt_cfg_set_parse():
 * @cfg: config set to fill
 * @options: a string containing the content to add to the config set. This
 * is a '\t',' ' or '\n' separated list of cells. Each individual cell is
 * like "[name]:[value]" where [name] is the name of an already registred 
 * cell, and [value] conforms to the data type under which this cell was
 * registred.
 *
 * Add the cells described in a string to a config set.
 */

xbt_error_t
xbt_cfg_set_parse(xbt_cfg_t cfg, const char *options) {
  int i;
  double d;
  char *str;

  xbt_cfgelm_t cell;
  char *optionlist_cpy;
  char *option,  *name,*val;

  int len;
  xbt_error_t errcode;

  XBT_IN;
  if (!options || !strlen(options)) { /* nothing to do */
    return no_error;
  }
  optionlist_cpy=xbt_strdup(options);

  DEBUG1("List to parse and set:'%s'",options);
  option=optionlist_cpy;
  while (1) { /* breaks in the code */

    if (!option) 
      break;
    name=option;
    len=strlen(name);
    DEBUG3("Still to parse and set: '%s'. len=%d; option-name=%ld",
	   name,len,(long)(option-name));

    /* Pass the value */
    while (option-name<=(len-1) && *option != ' ' && *option != '\n' && *option != '\t') {
      /*fprintf(stderr,"Take %c.\n",*option);*/
      option++;
    }
    if (option-name == len) {
      /*fprintf(stderr,"Boundary=EOL\n");*/
      option=NULL; /* don't do next iteration */

    } else {
      /*fprintf(stderr,"Boundary on '%c'. len=%d;option-name=%d\n",*option,len,option-name);*/

      /* Pass the following blank chars */
      *(option++)='\0';
      while (option-name<(len-1) && (*option == ' ' || *option == '\n' || *option == '\t')) {
	/*      fprintf(stderr,"Ignore a blank char.\n");*/
	option++;
      }
      if (option-name == len-1)
	option=NULL; /* don't do next iteration */
    }
    DEBUG2("parse now:'%s'; parse later:'%s'",name,option);

    if (name[0] == ' ' || name[0] == '\n' || name[0] == '\t')
      continue;
    if (!strlen(name))
      break;
    
    val=strchr(name,':');
    if (!val) {
      xbt_free(optionlist_cpy);
      xbt_assert1(FALSE,
		   "Malformated option: '%s'; Should be of the form 'name:value'",
		   name);
    }
    *(val++)='\0';

    DEBUG2("name='%s';val='%s'",name,val);

    errcode=xbt_dict_get((xbt_dict_t)cfg,name,(void**)&cell);
    switch (errcode) {
    case no_error:
      break;
    case mismatch_error:
      ERROR1("No registrated cell corresponding to '%s'.",name);
      xbt_free(optionlist_cpy);
      return mismatch_error;
      break;
    default:
      xbt_free(optionlist_cpy);
      return errcode;
    }

    switch (cell->type) {
    case xbt_cfgelm_string:
      TRYCLEAN(xbt_cfg_set_string(cfg, name, val),
	       xbt_free(optionlist_cpy));
      break;

    case xbt_cfgelm_int:
      i=strtol(val, &val, 0);
      if (val==NULL) {
	xbt_free(optionlist_cpy);	
	xbt_assert1(FALSE,
		     "Value of option %s not valid. Should be an integer",
		     name);
      }

      TRYCLEAN(xbt_cfg_set_int(cfg,name,i),
	       xbt_free(optionlist_cpy));
      break;

    case xbt_cfgelm_double:
      d=strtod(val, &val);
      if (val==NULL) {
	xbt_free(optionlist_cpy);	
	xbt_assert1(FALSE,
	       "Value of option %s not valid. Should be a double",
	       name);
      }

      TRYCLEAN(xbt_cfg_set_double(cfg,name,d),
	       xbt_free(optionlist_cpy));
      break;

    case xbt_cfgelm_host:
      str=val;
      val=strchr(val,':');
      if (!val) {
	xbt_free(optionlist_cpy);	
	xbt_assert1(FALSE,
	       "Value of option %s not valid. Should be an host (machine:port)",
	       name);
      }

      *(val++)='\0';
      i=strtol(val, &val, 0);
      if (val==NULL) {
	xbt_free(optionlist_cpy);	
	xbt_assert1(FALSE,
	       "Value of option %s not valid. Should be an host (machine:port)",
	       name);
      }

      TRYCLEAN(xbt_cfg_set_host(cfg,name,str,i),
	       xbt_free(optionlist_cpy));
      break;      

    default: 
      xbt_free(optionlist_cpy);
      RAISE1(unknown_error,"Type of config element %s is not valid.",name);
    }
    
  }
  xbt_free(optionlist_cpy);
  return no_error;
}

/**
 * xbt_cfg_set_int:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value of the cell
 *
 * Set the value of the cell @name in @cfg with the provided value.
 */ 
xbt_error_t
xbt_cfg_set_int(xbt_cfg_t cfg,const char*name, int val) {
  xbt_cfgelm_t cell;
  xbt_error_t errcode;

  VERB2("Configuration setting: %s=%d",name,val);
  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_int,&cell));

  if (cell->max > 1) {
    xbt_dynar_push(cell->content,&val);
  } else {
    xbt_dynar_set(cell->content,0,&val);
  }
  return no_error;
}

/**
 * xbt_cfg_set_double:
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the doule to set
 * 
 * Set the value of the cell @name in @cfg with the provided value.
 */ 

xbt_error_t
xbt_cfg_set_double(xbt_cfg_t cfg,const char*name, double val) {
  xbt_cfgelm_t cell;
  xbt_error_t errcode;

  VERB2("Configuration setting: %s=%f",name,val);
  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_double,&cell));

  if (cell->max > 1) {
    xbt_dynar_push(cell->content,&val);
  } else {
    xbt_dynar_set(cell->content,0,&val);
  }
  return no_error;
}

/**
 * xbt_cfg_set_string:
 * 
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value to be added
 *
 * Set the value of the cell @name in @cfg with the provided value.
 */ 

xbt_error_t
xbt_cfg_set_string(xbt_cfg_t cfg,const char*name, const char*val) { 
  xbt_cfgelm_t cell;
  xbt_error_t errcode;
  char *newval = xbt_strdup(val);

  VERB2("Configuration setting: %s=%s",name,val);
  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_string,&cell));

  if (cell->max > 1) {
    xbt_dynar_push(cell->content,&newval);
  } else {
    xbt_dynar_set(cell->content,0,&newval);
  }
  return no_error;
}

/**
 * xbt_cfg_set_host:
 * 
 * @cfg: the config set
 * @name: the name of the cell
 * @host: the host
 * @port: the port number
 *
 * Set the value of the cell @name in @cfg with the provided value 
 * on the given @host to the given @port
 */ 

xbt_error_t 
xbt_cfg_set_host(xbt_cfg_t cfg,const char*name, 
		  const char *host,int port) {
  xbt_cfgelm_t cell;
  xbt_error_t errcode;
  xbt_host_t *val=xbt_new(xbt_host_t,1);

  VERB3("Configuration setting: %s=%s:%d",name,host,port);

  val->name = xbt_strdup(name);
  val->port = port;

  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_host,&cell));

  if (cell->max > 1) {
    xbt_dynar_push(cell->content,&val);
  } else {
    xbt_dynar_set(cell->content,0,&val);
  }
  return no_error;
}

/* ---- [ Removing ] ---- */

/**
 * xbt_cfg_rm_int:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value to be removed
 *
 * Remove the provided @val from the cell @name in @cfg.
 */
xbt_error_t xbt_cfg_rm_int(xbt_cfg_t cfg,const char*name, int val) {

  xbt_cfgelm_t cell;
  int cpt,seen;
  xbt_error_t errcode;

  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_int,&cell));
  
  xbt_dynar_foreach(cell->content,cpt,seen) {
    if (seen == val) {
      xbt_dynar_cursor_rm(cell->content,&cpt);
      return no_error;
    }
  }

  ERROR2("Can't remove the value %d of config element %s: value not found.",
	 val,name);
  return mismatch_error;
}

/**
 * xbt_cfg_rm_double:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value to be removed
 *
 * Remove the provided @val from the cell @name in @cfg.
 */

xbt_error_t xbt_cfg_rm_double(xbt_cfg_t cfg,const char*name, double val) {
  xbt_cfgelm_t cell;
  int cpt;
  double seen;
  xbt_error_t errcode;

  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_double,&cell));
  
  xbt_dynar_foreach(cell->content,cpt,seen) {
    if (seen == val) {
      xbt_dynar_cursor_rm(cell->content,&cpt);
      return no_error;
    }
  }

  ERROR2("Can't remove the value %f of config element %s: value not found.",
	 val,name);
  return mismatch_error;
}

/**
 * xbt_cfg_rm_string:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value of the string which will be removed
 *
 * Remove the provided @val from the cell @name in @cfg.
 */
xbt_error_t
xbt_cfg_rm_string(xbt_cfg_t cfg,const char*name, const char *val) {
  xbt_cfgelm_t cell;
  int cpt;
  char *seen;
  xbt_error_t errcode;

  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_string,&cell));
  
  xbt_dynar_foreach(cell->content,cpt,seen) {
    if (!strcpy(seen,val)) {
      xbt_dynar_cursor_rm(cell->content,&cpt);
      return no_error;
    }
  }

  ERROR2("Can't remove the value %s of config element %s: value not found.",
	 val,name);
  return mismatch_error;
}

/**
 * xbt_cfg_rm_host:
 * 
 * @cfg: the config set
 * @name: the name of the cell
 * @host: the hostname
 * @port: the port number
 *
 * Remove the provided @host:@port from the cell @name in @cfg.
 */

xbt_error_t
xbt_cfg_rm_host(xbt_cfg_t cfg,const char*name, const char *host,int port) {
  xbt_cfgelm_t cell;
  int cpt;
  xbt_host_t *seen;
  xbt_error_t errcode;

  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_host,&cell));
  
  xbt_dynar_foreach(cell->content,cpt,seen) {
    if (!strcpy(seen->name,host) && seen->port == port) {
      xbt_dynar_cursor_rm(cell->content,&cpt);
      return no_error;
    }
  }

  ERROR3("Can't remove the value %s:%d of config element %s: value not found.",
	 host,port,name);
  return mismatch_error;
}

/* rm everything */

/**
 * xbt_cfg_empty:
 * 
 * @cfg: the config set
 * @name: the name of the cell
 *
 * rm evenything
 */

xbt_error_t 
xbt_cfg_empty(xbt_cfg_t cfg,const char*name) {
  xbt_cfgelm_t cell;

  xbt_error_t errcode;

  TRYCATCH(mismatch_error,
	   xbt_dict_get((xbt_dict_t)cfg,name,(void**)&cell));
  if (errcode == mismatch_error) {
    ERROR1("Can't empty  '%s' since this config element does not exist",
	   name);
    return mismatch_error;
  }

  if (cell) {
    xbt_dynar_reset(cell->content);
  }
  return no_error;
}

/*----[ Getting ]---------------------------------------------------------*/

/**
 * xbt_cfg_get_int:
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using xbt_cfg_get_dynar() 
 * instead.
 *
 * @warning the returned value is the actual content of the config set
 */
xbt_error_t
xbt_cfg_get_int   (xbt_cfg_t  cfg,
		    const char *name,
		    int        *val) {
  xbt_cfgelm_t cell;
  xbt_error_t errcode;

  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_int,&cell));

  if (xbt_dynar_length(cell->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %lu values",
	     name, xbt_dynar_length(cell->content));
  }

  *val = xbt_dynar_get_as(cell->content, 0, int);
  return no_error;
}

/**
 * xbt_cfg_get_double:
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using xbt_cfg_get_dynar() 
 * instead.
 *
 * @warning the returned value is the actual content of the config set
 */

xbt_error_t
xbt_cfg_get_double(xbt_cfg_t  cfg,
		    const char *name,
		    double     *val) {
  xbt_cfgelm_t cell;
  xbt_error_t  errcode;

  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_double,&cell));

  if (xbt_dynar_length(cell->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %lu values\n",
	     name, xbt_dynar_length(cell->content));
  }

  *val = xbt_dynar_get_as(cell->content, 0, double);
  return no_error;
}

/**
 * xbt_cfg_get_string:
 *
 * @th: the config set
 * @name: the name of the cell
 * @val: the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using xbt_cfg_get_dynar() 
 * instead.
 *
 * @warning the returned value is the actual content of the config set
 */

xbt_error_t xbt_cfg_get_string(xbt_cfg_t  cfg,
				 const char *name,
				 char      **val) {
  xbt_cfgelm_t  cell;
  xbt_error_t errcode;

  *val=NULL;

  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_string,&cell));

  if (xbt_dynar_length(cell->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %lu values\n",
	     name, xbt_dynar_length(cell->content));
  }

  *val = xbt_dynar_get_as(cell->content, 0, char *);
  return no_error;
}

/**
 * xbt_cfg_get_host:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @host: the host
 * @port: the port number
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using xbt_cfg_get_dynar() 
 * instead.
 *
 * @warning the returned value is the actual content of the config set
 */

xbt_error_t xbt_cfg_get_host  (xbt_cfg_t  cfg,
				 const char *name,
				 char      **host,
				 int        *port) {
  xbt_cfgelm_t cell;
  xbt_error_t  errcode;
  xbt_host_t  *val;

  TRY (xbt_cfgelm_get(cfg,name,xbt_cfgelm_host,&cell));

  if (xbt_dynar_length(cell->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %lu values\n",
	     name, xbt_dynar_length(cell->content));
  }

  val = xbt_dynar_get_as(cell->content, 0, xbt_host_t*);
  *host=val->name;
  *port=val->port;
  
  return no_error;
}

/**
 * xbt_cfg_get_dynar:
 * @cfg: where to search in
 * @name: what to search for
 * @dynar: result
 *
 * Get the data stored in the config bag. 
 *
 * @warning the returned value is the actual content of the config set
 */
xbt_error_t xbt_cfg_get_dynar (xbt_cfg_t    cfg,
				 const char   *name,
				 xbt_dynar_t *dynar) {
  xbt_cfgelm_t cell;
  xbt_error_t  errcode = xbt_dict_get((xbt_dict_t)cfg,name,
					(void**)&cell);

  if (errcode == mismatch_error) {
    ERROR1("No registered cell %s in this config set",
	   name);
    return mismatch_error;
  }
  if (errcode != no_error)
     return errcode;

  *dynar = cell->content;
  return no_error;
}

