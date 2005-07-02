/* $Id$ */

/* config - Dictionnary where the type of each variable is provided.            */

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

/* xbt_cfgelm_t: the typedef corresponding to a config variable. 

   Both data and DTD are mixed, but fixing it now would prevent me to ever
   defend my thesis. */

typedef struct {
  /* Allowed type of the variable */
  e_xbt_cfgelm_type_t type;
  int min,max;
  
  /* Callbacks */
  xbt_cfg_cb_t cb_set;
  xbt_cfg_cb_t cb_rm;

  /* actual content 
     (cannot be an union because type host uses both str and i) */
  xbt_dynar_t content;
} s_xbt_cfgelm_t,*xbt_cfgelm_t;

static const char *xbt_cfgelm_type_name[xbt_cfgelm_type_count]=
  {"int","double","string","host","any"};

/* Internal stuff used in cache to free a variable */
static void xbt_cfgelm_free(void *data);

/* Retrieve the variable we'll modify */
static xbt_error_t xbt_cfgelm_get(xbt_cfg_t cfg, const char *name,
				    e_xbt_cfgelm_type_t type,
				    /* OUT */ xbt_cfgelm_t *whereto);

static void xbt_cfg_str_free(void *d){
  free(*(void**)d);
}
static void xbt_cfg_host_free(void *d){
  xbt_host_t *h=(xbt_host_t*) *(void**)d; 
  if (h) {
    if (h->name) free(h->name);
    free(h);
  }
}

/*----[ Memory management ]-----------------------------------------------*/

/** @brief Constructor
 *
 * Initialise an config set
 */


xbt_cfg_t xbt_cfg_new(void) {
  return (xbt_cfg_t)xbt_dict_new();
}

/** \brief Copy an existing configuration set
 *
 * \arg whereto the config set to be created
 * \arg tocopy the source data
 *
 * This only copy the registrations, not the actual content
 */

void
xbt_cfg_cpy(xbt_cfg_t tocopy,xbt_cfg_t *whereto) {
  xbt_dict_cursor_t cursor=NULL; 
  xbt_cfgelm_t variable=NULL;
  char *name=NULL;
  
  *whereto=NULL;
  xbt_assert0(tocopy,"cannot copy NULL config");

  xbt_dict_foreach((xbt_dict_t)tocopy,cursor,name,variable) {
    xbt_cfg_register(*whereto, name, variable->type, variable->min, variable->max,
                     variable->cb_set, variable->cb_rm);
  }
}

/** @brief Destructor */
void xbt_cfg_free(xbt_cfg_t *cfg) {
  xbt_dict_free((xbt_dict_t*)cfg);
}

/** @brief Dump a config set for debuging purpose
 *
 * \arg name The name to give to this config set
 * \arg indent what to write at the begining of each line (right number of spaces)
 * \arg cfg the config set
 */
void xbt_cfg_dump(const char *name,const char *indent,xbt_cfg_t cfg) {
  xbt_dict_t dict = (xbt_dict_t) cfg;
  xbt_dict_cursor_t cursor=NULL; 
  xbt_cfgelm_t variable=NULL;
  char *key=NULL;
  int i; 
  int size;
  int ival;
  char *sval;
  double dval;
  xbt_host_t *hval;

  if (name)
    printf("%s>> Dumping of the config set '%s':\n",indent,name);
  xbt_dict_foreach(dict,cursor,key,variable) {

    printf("%s  %s:",indent,key);

    size = xbt_dynar_length(variable->content);
    printf("%d_to_%d_%s. Actual size=%d. prerm=%p,postset=%p, List of values:\n",
	   variable->min,variable->max,xbt_cfgelm_type_name[variable->type],
	   size,
           variable->cb_rm, variable->cb_set);

    switch (variable->type) {
       
    case xbt_cfgelm_int:
      for (i=0; i<size; i++) {
	ival = xbt_dynar_get_as(variable->content,i,int);
	printf ("%s    %d\n",indent,ival);
      }
      break;

    case xbt_cfgelm_double:
      for (i=0; i<size; i++) {
	dval = xbt_dynar_get_as(variable->content,i,double);
	printf ("%s    %f\n",indent,dval);
      }
      break;

    case xbt_cfgelm_string:
      for (i=0; i<size; i++) {
	sval = xbt_dynar_get_as(variable->content,i,char*);
	printf ("%s    %s\n",indent,sval);
      }
      break;

    case xbt_cfgelm_host:
      for (i=0; i<size; i++) {
	hval = xbt_dynar_get_as(variable->content,i,xbt_host_t*);
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

/*
 * free an config element
 */

void xbt_cfgelm_free(void *data) {
  xbt_cfgelm_t c=(xbt_cfgelm_t)data;

  if (!c) return;
  xbt_dynar_free(&(c->content));
  free(c);
}

/*----[ Registering stuff ]-----------------------------------------------*/

/** @brief Register an element within a config set
 *
 *  @arg cfg the config set
 *  @arg type the type of the config element
 *  @arg min the minimum
 *  @arg max the maximum
 */

void
xbt_cfg_register(xbt_cfg_t cfg,
		  const char *name, e_xbt_cfgelm_type_t type,
		  int min, int max,
		  xbt_cfg_cb_t cb_set,  xbt_cfg_cb_t cb_rm){
  xbt_cfgelm_t res;
  xbt_error_t errcode;

  xbt_assert4(type>=xbt_cfgelm_int && type<=xbt_cfgelm_host,
              "type of %s not valid (%d should be between %d and %d)",
              name,type,xbt_cfgelm_int, xbt_cfgelm_host);
  DEBUG5("Register cfg elm %s (%d to %d %s (=%d))",name,min,max,xbt_cfgelm_type_name[type],type);
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
  res->cb_set = cb_set;
  res->cb_rm  = cb_rm;

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

/** @brief Unregister an element from a config set. 
 * 
 *  @arg cfg the config set
 *  @arg name the name of the elem to be freed
 * 
 *  Note that it removes both the description and the actual content.
 */

xbt_error_t
xbt_cfg_unregister(xbt_cfg_t cfg,const char *name) {
  return xbt_dict_remove((xbt_dict_t)cfg,name);
}

/**
 * @brief Parse a string and register the stuff described.
 *
 * @arg cfg the config set
 * @arg entry a string describing the element to register
 *
 * The string may consist in several variable descriptions separated by a space. 
 * Each of them must use the following syntax: \<name\>:\<min nb\>_to_\<max nb\>_\<type\>
 * with type being one of  'string','int', 'host' or 'double'.
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
    free(entrycpy);
    xbt_abort();
  }
  *(tok++)='\0';

  min=strtol(tok, &tok, 10);
  if (!tok) {
    ERROR1("Invalid minimum in config element descriptor %s",entry);
    free(entrycpy);
    xbt_abort();
  }

  if (!strcmp(tok,"_to_")){
    ERROR3("%s%s%s",
	  "Invalid config element descriptor: ",entry,
	  "; Should be <name>:<min nb>_to_<max nb>_<type>");
    free(entrycpy);
    xbt_abort();
  }
  tok += strlen("_to_");

  max=strtol(tok, &tok, 10);
  if (!tok) {
    ERROR1("Invalid maximum in config element descriptor %s",entry);
    free(entrycpy);
    xbt_abort();
  }

  if (*(tok++)!='_') {
    ERROR3("%s%s%s",
	  "Invalid config element descriptor: ",entry,
	  "; Should be <name>:<min nb>_to_<max nb>_<type>");
    free(entrycpy);
    xbt_abort();
  }

  for (type=0; 
       type<xbt_cfgelm_type_count && strcmp(tok,xbt_cfgelm_type_name[type]); 
       type++);
  if (type == xbt_cfgelm_type_count) {
    ERROR3("%s%s%s",
	  "Invalid type in config element descriptor: ",entry,
	  "; Should be one of 'string', 'int', 'host' or 'double'.");
    free(entrycpy);
    xbt_abort();
  }

  xbt_cfg_register(cfg,entrycpy,type,min,max,NULL,NULL);

  free(entrycpy); /* strdup'ed by dict mechanism, but cannot be const */
  return no_error;
}

/** @brief Check that each variable have the right amount of values */

xbt_error_t
xbt_cfg_check(xbt_cfg_t cfg) {
  xbt_dict_cursor_t cursor; 
  xbt_cfgelm_t variable;
  char *name;
  int size;

  xbt_assert0(cfg,"NULL config set.");

  xbt_dict_foreach((xbt_dict_t)cfg,cursor,name,variable) {
    size = xbt_dynar_length(variable->content);
    if (variable->min > size) { 
      ERROR4("Config elem %s needs at least %d %s, but there is only %d values.",
	     name,
	     variable->min,
	     xbt_cfgelm_type_name[variable->type],
	     size); 
      xbt_dict_cursor_free(&cursor);
      return mismatch_error;
    }

    if (variable->max > 0 && variable->max < size) {
      ERROR4("Config elem %s accepts at most %d %s, but there is %d values.",
	     name,
	     variable->max,
	     xbt_cfgelm_type_name[variable->type],
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
    RAISE1(mismatch_error,
           "No registered variable '%s' in this config set",
	   name);
  }
  if (errcode != no_error)
     return errcode;

  xbt_assert3(type == xbt_cfgelm_any || (*whereto)->type == type,
	       "You tried to access to the config element %s as an %s, but its type is %s.",
	       name,
	       xbt_cfgelm_type_name[type],
	       xbt_cfgelm_type_name[(*whereto)->type]);

  return no_error;
}

/** @brief Get the type of this variable in that configuration set
 *
 * \arg cfg the config set
 * \arg name the name of the element 
 * \arg type the result
 *
 */

xbt_error_t
xbt_cfg_get_type(xbt_cfg_t cfg, const char *name, 
		      /* OUT */e_xbt_cfgelm_type_t *type) {

  xbt_cfgelm_t variable;
  xbt_error_t errcode;

  errcode=xbt_dict_get((xbt_dict_t)cfg,name,(void**)&variable);

  if (errcode == mismatch_error) {
    RAISE1(mismatch_error,"Can't get the type of '%s' since this variable does not exist",
	   name);
  } else if (errcode != no_error) {
    return errcode;
  }

  INFO1("type in variable = %d",variable->type);
  *type=variable->type;

  return no_error;
}

/*----[ Setting ]---------------------------------------------------------*/
/**  @brief va_args version of xbt_cfg_set
 *
 * \arg cfg config set to fill
 * \arg n   variable name
 * \arg pa  variable value
 *
 * Add some values to the config set.
 */
xbt_error_t
xbt_cfg_set_vargs(xbt_cfg_t cfg, const char *name, va_list pa) {
  char *str;
  int i;
  double d;
  e_xbt_cfgelm_type_t type;

  xbt_error_t errcode;
  
  errcode = xbt_cfg_get_type(cfg,name,&type);
  if (errcode != no_error) {
    ERROR1("Can't set the property '%s' since it's not registered",name);
    return mismatch_error;
  }

  switch (type) {
  case xbt_cfgelm_host:
    str = va_arg(pa, char *);
    i=va_arg(pa,int);
    TRYOLD(xbt_cfg_set_host(cfg,name,str,i));
    break;
      
  case xbt_cfgelm_string:
    str=va_arg(pa, char *);
    TRYOLD(xbt_cfg_set_string(cfg, name, str));
    break;

  case xbt_cfgelm_int:
    i=va_arg(pa,int);
    TRYOLD(xbt_cfg_set_int(cfg,name,i));
    break;

  case xbt_cfgelm_double:
    d=va_arg(pa,double);
    TRYOLD(xbt_cfg_set_double(cfg,name,d));
    break;

  default:
    xbt_assert2(0,"Config element variable %s not valid (type=%d)",name,type);
  }

  return no_error;
}

/** @brief Add a NULL-terminated list of pairs {(char*)key, value} to the set
 *
 * \arg cfg config set to fill
 * \arg name variable name
 * \arg varargs variable value
 *
 */
xbt_error_t xbt_cfg_set(xbt_cfg_t cfg, const char *name, ...) {
  va_list pa;
  xbt_error_t errcode;

  va_start(pa,name);
  errcode=xbt_cfg_set_vargs(cfg,name,pa);
  va_end(pa);
  return errcode;
}

/** @brief Add values parsed from a string into a config set
 *
 * \arg cfg config set to fill
 * \arg options a string containing the content to add to the config set. This
 * is a '\\t',' ' or '\\n' separated list of variables. Each individual variable is
 * like "[name]:[value]" where [name] is the name of an already registred 
 * variable, and [value] conforms to the data type under which this variable was
 * registred.
 *
 * @todo This is a crude manual parser, it should be a proper lexer.
 */

xbt_error_t
xbt_cfg_set_parse(xbt_cfg_t cfg, const char *options) {
  int i;
  double d;
  char *str;

  xbt_cfgelm_t variable;
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
      DEBUG1("Take %c.",*option);
      option++;
    }
    if (option-name == len) {
      DEBUG0("Boundary=EOL");
      option=NULL; /* don't do next iteration */

    } else {
      DEBUG3("Boundary on '%c'. len=%d;option-name=%d",
	     *option,len,option-name);

      /* Pass the following blank chars */
      *(option++)='\0';
      while (option-name<(len-1) && 
	     (*option == ' ' || *option == '\n' || *option == '\t')) {
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
      free(optionlist_cpy);
      xbt_assert1(FALSE,
	     "Malformated option: '%s'; Should be of the form 'name:value'",
		  name);
    }
    *(val++)='\0';

    DEBUG2("name='%s';val='%s'",name,val);

    errcode=xbt_dict_get((xbt_dict_t)cfg,name,(void**)&variable);
    switch (errcode) {
    case no_error:
      break;
    case mismatch_error:
      ERROR1("No registrated variable corresponding to '%s'.",name);
      free(optionlist_cpy);
      return mismatch_error;
      break;
    default:
      free(optionlist_cpy);
      return errcode;
    }

    switch (variable->type) {
    case xbt_cfgelm_string:
      errcode = xbt_cfg_set_string(cfg, name, val);
      if (errcode != no_error) {
	 free(optionlist_cpy);
         return errcode;
      }
      break;

    case xbt_cfgelm_int:
      i=strtol(val, &val, 0);
      if (val==NULL) {
	free(optionlist_cpy);	
	xbt_assert1(FALSE,
		     "Value of option %s not valid. Should be an integer",
		     name);
      }

      errcode = xbt_cfg_set_int(cfg,name,i);
      if (errcode != no_error) {
	 free(optionlist_cpy);
	 return errcode;
      }
      break;

    case xbt_cfgelm_double:
      d=strtod(val, &val);
      if (val==NULL) {
	free(optionlist_cpy);	
	xbt_assert1(FALSE,
	       "Value of option %s not valid. Should be a double",
	       name);
      }

      errcode = xbt_cfg_set_double(cfg,name,d);
      if (errcode != no_error) {
	 free(optionlist_cpy);
	 return errcode;
      }
      break;

    case xbt_cfgelm_host:
      str=val;
      val=strchr(val,':');
      if (!val) {
	free(optionlist_cpy);	
	xbt_assert1(FALSE,
	    "Value of option %s not valid. Should be an host (machine:port)",
	       name);
      }

      *(val++)='\0';
      i=strtol(val, &val, 0);
      if (val==NULL) {
	free(optionlist_cpy);	
	xbt_assert1(FALSE,
	    "Value of option %s not valid. Should be an host (machine:port)",
	       name);
      }

      errcode = xbt_cfg_set_host(cfg,name,str,i);
      if (errcode != no_error) {
	 free(optionlist_cpy);
	 return errcode;
      }
      break;      

    default: 
      free(optionlist_cpy);
      RAISE1(unknown_error,"Type of config element %s is not valid.",name);
    }
    
  }
  free(optionlist_cpy);
  return no_error;
}

/** @brief Set or add an integer value to \a name within \a cfg
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value of the variable
 */ 
xbt_error_t
xbt_cfg_set_int(xbt_cfg_t cfg,const char*name, int val) {
  xbt_cfgelm_t variable;
  xbt_error_t errcode;

  VERB2("Configuration setting: %s=%d",name,val);
  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_int,&variable));

  if (variable->max == 1) {
    if (variable->cb_rm && xbt_dynar_length(variable->content))
      (*variable->cb_rm)(name, 0);
          
    xbt_dynar_set(variable->content,0,&val);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == variable->max)
      RAISE3(mismatch_error,
             "Cannot add value %d to the config element %s since it's already full (size=%d)",
             val,name,variable->max); 
             
    xbt_dynar_push(variable->content,&val);
  }

  if (variable->cb_set)
    (*variable->cb_set)(name, xbt_dynar_length(variable->content) -1);
  return no_error;
}

/** @brief Set or add a double value to \a name within \a cfg
 * 
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the doule to set
 */ 

xbt_error_t
xbt_cfg_set_double(xbt_cfg_t cfg,const char*name, double val) {
  xbt_cfgelm_t variable;
  xbt_error_t errcode;

  VERB2("Configuration setting: %s=%f",name,val);
  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_double,&variable));

  if (variable->max == 1) {
    if (variable->cb_rm && xbt_dynar_length(variable->content))
      (*variable->cb_rm)(name, 0);
          
    xbt_dynar_set(variable->content,0,&val);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == variable->max)
      RAISE3(mismatch_error,
             "Cannot add value %f to the config element %s since it's already full (size=%d)",
             val,name,variable->max); 
             
    xbt_dynar_push(variable->content,&val);
  }

  if (variable->cb_set)
    (*variable->cb_set)(name, xbt_dynar_length(variable->content) -1);
  return no_error;
}

/** @brief Set or add a string value to \a name within \a cfg
 * 
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value to be added
 *
 */ 

xbt_error_t
xbt_cfg_set_string(xbt_cfg_t cfg,const char*name, const char*val) { 
  xbt_cfgelm_t variable;
  xbt_error_t errcode;
  char *newval = xbt_strdup(val);

  VERB2("Configuration setting: %s=%s",name,val);
  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_string,&variable));

  if (variable->max == 1) {
    if (variable->cb_rm && xbt_dynar_length(variable->content))
      (*variable->cb_rm)(name, 0);
          
    xbt_dynar_set(variable->content,0,&newval);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == variable->max)
      RAISE3(mismatch_error,
             "Cannot add value %s to the config element %s since it's already full (size=%d)",
             name,val,variable->max); 
             
    xbt_dynar_push(variable->content,&newval);
  }

  if (variable->cb_set)
    (*variable->cb_set)(name, xbt_dynar_length(variable->content) -1);
  return no_error;
}

/** @brief Set or add an host value to \a name within \a cfg
 * 
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg host the host
 * \arg port the port number
 *
 * \e host values are composed of a string (hostname) and an integer (port)
 */ 

xbt_error_t 
xbt_cfg_set_host(xbt_cfg_t cfg,const char*name, 
		  const char *host,int port) {
  xbt_cfgelm_t variable;
  xbt_error_t errcode;
  xbt_host_t *val=xbt_new(xbt_host_t,1);

  VERB3("Configuration setting: %s=%s:%d",name,host,port);

  val->name = xbt_strdup(name);
  val->port = port;

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_host,&variable));

  if (variable->max == 1) {
    if (variable->cb_rm && xbt_dynar_length(variable->content))
      (*variable->cb_rm)(name, 0);
          
    xbt_dynar_set(variable->content,0,&val);
  } else {
    if (variable->max && xbt_dynar_length(variable->content) == variable->max)
      RAISE4(mismatch_error,
             "Cannot add value %s:%d to the config element %s since it's already full (size=%d)",
             host,port,name,variable->max); 
             
    xbt_dynar_push(variable->content,&val);
  }

  if (variable->cb_set)
    (*variable->cb_set)(name, xbt_dynar_length(variable->content) -1);
  return no_error;
}

/* ---- [ Removing ] ---- */

/** @brief Remove the provided \e val integer value from a variable
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value to be removed
 */
xbt_error_t xbt_cfg_rm_int(xbt_cfg_t cfg,const char*name, int val) {

  xbt_cfgelm_t variable;
  int cpt,seen;
  xbt_error_t errcode;

  if (xbt_dynar_length(variable->content) == variable->min)
    RAISE3(mismatch_error,
           "Cannot remove value %d from the config element %s since it's already at its minimal size (=%d)",
           val,name,variable->min); 

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_int,&variable));
  
  xbt_dynar_foreach(variable->content,cpt,seen) {
    if (seen == val) {
      if (variable->cb_rm) (*variable->cb_rm)(name, cpt);
      xbt_dynar_cursor_rm(variable->content,&cpt);
      return no_error;
    }
  }

  ERROR2("Can't remove the value %d of config element %s: value not found.",
	 val,name);
  return mismatch_error;
}

/** @brief Remove the provided \e val double value from a variable
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value to be removed
 */

xbt_error_t xbt_cfg_rm_double(xbt_cfg_t cfg,const char*name, double val) {
  xbt_cfgelm_t variable;
  int cpt;
  double seen;
  xbt_error_t errcode;

  if (xbt_dynar_length(variable->content) == variable->min)
    RAISE3(mismatch_error,
           "Cannot remove value %f from the config element %s since it's already at its minimal size (=%d)",
           val,name,variable->min); 

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_double,&variable));
  
  xbt_dynar_foreach(variable->content,cpt,seen) {
    if (seen == val) {
      xbt_dynar_cursor_rm(variable->content,&cpt);
      if (variable->cb_rm) (*variable->cb_rm)(name, cpt);
      return no_error;
    }
  }

  ERROR2("Can't remove the value %f of config element %s: value not found.",
	 val,name);
  return mismatch_error;
}

/** @brief Remove the provided \e val string value from a variable
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the value of the string which will be removed
 */
xbt_error_t
xbt_cfg_rm_string(xbt_cfg_t cfg,const char*name, const char *val) {
  xbt_cfgelm_t variable;
  int cpt;
  char *seen;
  xbt_error_t errcode;

  if (xbt_dynar_length(variable->content) == variable->min)
    RAISE3(mismatch_error,
           "Cannot remove value %s from the config element %s since it's already at its minimal size (=%d)",
           name,val,variable->min); 
           
  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_string,&variable));
  
  xbt_dynar_foreach(variable->content,cpt,seen) {
    if (!strcpy(seen,val)) {
      if (variable->cb_rm) (*variable->cb_rm)(name, cpt);
      xbt_dynar_cursor_rm(variable->content,&cpt);
      return no_error;
    }
  }

  ERROR2("Can't remove the value %s of config element %s: value not found.",
	 val,name);
  return mismatch_error;
}

/** @brief Remove the provided \e val host value from a variable
 * 
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg host the hostname
 * \arg port the port number
 */

xbt_error_t
xbt_cfg_rm_host(xbt_cfg_t cfg,const char*name, const char *host,int port) {
  xbt_cfgelm_t variable;
  int cpt;
  xbt_host_t *seen;
  xbt_error_t errcode;

  if (xbt_dynar_length(variable->content) == variable->min)
    RAISE4(mismatch_error,
           "Cannot remove value %s:%d from the config element %s since it's already at its minimal size (=%d)",
           host,port,name,variable->min); 
           
  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_host,&variable));
  
  xbt_dynar_foreach(variable->content,cpt,seen) {
    if (!strcpy(seen->name,host) && seen->port == port) {
      if (variable->cb_rm) (*variable->cb_rm)(name, cpt);
      xbt_dynar_cursor_rm(variable->content,&cpt);
      return no_error;
    }
  }

  ERROR3("Can't remove the value %s:%d of config element %s: value not found.",
	 host,port,name);
  return mismatch_error;
}

/** @brief Remove the \e pos th value from the provided variable */

xbt_error_t xbt_cfg_rm_at   (xbt_cfg_t cfg, const char *name, int pos) {

  xbt_cfgelm_t variable;
  int cpt;
  xbt_host_t *seen;
  xbt_error_t errcode;

  if (xbt_dynar_length(variable->content) == variable->min)
    RAISE3(mismatch_error,
           "Cannot remove %dth value from the config element %s since it's already at its minimal size (=%d)",
           pos,name,variable->min); 
           
  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_any,&variable));
  
  if (variable->cb_rm) (*variable->cb_rm)(name, pos);	  
  xbt_dynar_remove_at(variable->content, pos, NULL);

  return mismatch_error;
}

/** @brief Remove all the values from a variable
 * 
 * \arg cfg the config set
 * \arg name the name of the variable
 */

xbt_error_t 
xbt_cfg_empty(xbt_cfg_t cfg,const char*name) {
  xbt_cfgelm_t variable;

  xbt_error_t errcode;

  TRYCATCH(mismatch_error,
	   xbt_dict_get((xbt_dict_t)cfg,name,(void**)&variable));
  if (errcode == mismatch_error) {
    ERROR1("Can't empty  '%s' since this config element does not exist",
	   name);
    return mismatch_error;
  }

  if (variable) {
    if (variable->cb_rm) {
      int cpt;
      void *ignored;
      xbt_dynar_foreach(variable->content,cpt,ignored) {
        (*variable->cb_rm)(name, cpt);
      }
    }
    xbt_dynar_reset(variable->content);
  }
  return no_error;
}

/*----[ Getting ]---------------------------------------------------------*/

/** @brief Retrieve an integer value of a variable (get a warning if not uniq)
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using 
 * xbt_cfg_get_dynar() instead.
 *
 * \warning the returned value is the actual content of the config set
 */
xbt_error_t
xbt_cfg_get_int   (xbt_cfg_t  cfg,
		    const char *name,
		    int        *val) {
  xbt_cfgelm_t variable;
  xbt_error_t errcode;

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_int,&variable));

  if (xbt_dynar_length(variable->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %lu values",
	     name, xbt_dynar_length(variable->content));
  }

  *val = xbt_dynar_get_as(variable->content, 0, int);
  return no_error;
}

/** @brief Retrieve a double value of a variable (get a warning if not uniq)
 * 
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg val the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using 
 * xbt_cfg_get_dynar() instead.
 *
 * \warning the returned value is the actual content of the config set
 */

xbt_error_t
xbt_cfg_get_double(xbt_cfg_t  cfg,
		    const char *name,
		    double     *val) {
  xbt_cfgelm_t variable;
  xbt_error_t  errcode;

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_double,&variable));

  if (xbt_dynar_length(variable->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %lu values\n",
	     name, xbt_dynar_length(variable->content));
  }

  *val = xbt_dynar_get_as(variable->content, 0, double);
  return no_error;
}

/** @brief Retrieve a string value of a variable (get a warning if not uniq)
 *
 * \arg th the config set
 * \arg name the name of the variable
 * \arg val the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using 
 * xbt_cfg_get_dynar() instead.
 *
 * \warning the returned value is the actual content of the config set
 */

xbt_error_t xbt_cfg_get_string(xbt_cfg_t  cfg,
				 const char *name,
				 char      **val) {
  xbt_cfgelm_t  variable;
  xbt_error_t errcode;

  *val=NULL;

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_string,&variable));

  if (xbt_dynar_length(variable->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %lu values\n",
	     name, xbt_dynar_length(variable->content));
  }

  *val = xbt_dynar_get_as(variable->content, 0, char *);
  return no_error;
}

/** @brief Retrieve an host value of a variable (get a warning if not uniq)
 *
 * \arg cfg the config set
 * \arg name the name of the variable
 * \arg host the host
 * \arg port the port number
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using
 * xbt_cfg_get_dynar() instead.
 *
 * \warning the returned value is the actual content of the config set
 */

xbt_error_t xbt_cfg_get_host  (xbt_cfg_t  cfg,
				 const char *name,
				 char      **host,
				 int        *port) {
  xbt_cfgelm_t variable;
  xbt_error_t  errcode;
  xbt_host_t  *val;

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_host,&variable));

  if (xbt_dynar_length(variable->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %lu values\n",
	     name, xbt_dynar_length(variable->content));
  }

  val = xbt_dynar_get_as(variable->content, 0, xbt_host_t*);
  *host=val->name;
  *port=val->port;
  
  return no_error;
}

/** @brief Retrieve the dynar of all the values stored in a variable
 * 
 * \arg cfg where to search in
 * \arg name what to search for
 * \arg dynar result
 *
 * Get the data stored in the config set. 
 *
 * \warning the returned value is the actual content of the config set
 */
xbt_error_t xbt_cfg_get_dynar (xbt_cfg_t    cfg,
				 const char   *name,
				 xbt_dynar_t *dynar) {
  xbt_cfgelm_t variable;
  xbt_error_t  errcode = xbt_dict_get((xbt_dict_t)cfg,name,
					(void**)&variable);

  if (errcode == mismatch_error) {
    ERROR1("No registered variable %s in this config set",
	   name);
    return mismatch_error;
  }
  if (errcode != no_error)
     return errcode;

  *dynar = variable->content;
  return no_error;
}


/** @brief Retrieve one of the integer value of a variable */
xbt_error_t
xbt_cfg_get_int_at(xbt_cfg_t   cfg,
                   const char *name,
                   int         pos,
                   int        *val) {
                  
  xbt_cfgelm_t variable;
  xbt_error_t errcode;

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_int,&variable));
  *val = xbt_dynar_get_as(variable->content, pos, int);
  return no_error; 
}

/** @brief Retrieve one of the double value of a variable */
xbt_error_t
xbt_cfg_get_double_at(xbt_cfg_t   cfg,
                      const char *name,
                      int         pos,
                      double     *val) {
                  
  xbt_cfgelm_t variable;
  xbt_error_t errcode;

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_double,&variable));
  *val = xbt_dynar_get_as(variable->content, pos, double);
  return no_error; 
}


/** @brief Retrieve one of the string value of a variable */
xbt_error_t
xbt_cfg_get_string_at(xbt_cfg_t   cfg,
                      const char *name,
                      int         pos,
                      char      **val) {
                  
  xbt_cfgelm_t variable;
  xbt_error_t errcode;

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_string,&variable));
  *val = xbt_dynar_get_as(variable->content, pos, char*);
  return no_error; 
}

/** @brief Retrieve one of the host value of a variable */
xbt_error_t
xbt_cfg_get_host_at(xbt_cfg_t   cfg,
                    const char *name,
                    int         pos,
                    char      **host,
                    int        *port) {
                  
  xbt_cfgelm_t variable;
  xbt_error_t errcode;
  xbt_host_t *val;

  TRYOLD(xbt_cfgelm_get(cfg,name,xbt_cfgelm_int,&variable));
  val = xbt_dynar_get_ptr(variable->content, pos);
  *port = val->port;
  *host = val->name;
  return no_error; 
}
