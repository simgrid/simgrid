/* $Id$ */

/* config - Dictionnary where the type of each cell is provided.            */

/* This is useful to build named structs, like option or property sets.     */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2001,2002,2003,2004 the OURAGAN project.                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h" /* prototypes of this module */

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(config,gros,"configuration support");

/* gras_cfgelm_t: the typedef corresponding to a config cell. 

   Both data and DTD are mixed, but fixing it now would prevent me to ever
   defend my thesis. */

typedef struct {
  /* Allowed type of the cell */
  gras_cfgelm_type_t type;
  int min,max;

  /* actual content 
     (cannot be an union because type host uses both str and i) */
  gras_dynar_t *content;
} gras_cfgelm_t;

static const char *gras_cfgelm_type_name[gras_cfgelm_type_count]=
  {"int","double","string","host"};

/* Internal stuff used in cache to free a cell */
static void gras_cfgelm_free(void *data);

/* Retrieve the cell we'll modify */
static gras_error_t gras_cfgelm_get(gras_cfg_t *cfg, const char *name,
				    gras_cfgelm_type_t type,
				    /* OUT */ gras_cfgelm_t **whereto);

void gras_cfg_str_free(void *d);
void gras_cfg_host_free(void *d);

void gras_cfg_str_free(void *d){
  gras_free(*(void**)d);
}
void gras_cfg_host_free(void *d){
  gras_host_t *h=(gras_host_t*) *(void**)d; 
  if (h) {
    if (h->name) gras_free(h->name);
    gras_free(h);
  }
}

/*----[ Memory management ]-----------------------------------------------*/

/**
 * gras_cfg_new:
 *
 * @whereto: 
 *
 * Initialise an config set
 */


gras_error_t gras_cfg_new(gras_cfg_t **whereto) {
  return gras_dict_new((gras_dict_t**)whereto);
}

/**
 * gras_cfg_cpy:
 *
 * @whereto: the config set to be created
 * @tocopy: the source data
 *
 */

gras_error_t
gras_cfg_cpy(gras_cfg_t **whereto, gras_cfg_t *tocopy) {
  gras_dict_cursor_t *cursor=NULL; 
  gras_cfgelm_t *cell=NULL;
  char *name=NULL;
  int errcode=no_error;
  
  *whereto=NULL;
  gras_assert0(tocopy,"cannot copy NULL config");

  gras_dict_foreach((gras_dict_t*)tocopy,cursor,name,cell) {
    if ((errcode=gras_cfg_register(*whereto, name, cell->type, 
				   cell->min, cell->max))         != no_error) {
      if (cursor)   gras_dict_cursor_free(cursor);
      if (*whereto) gras_cfg_free(whereto);
      return errcode;
    }
  }

  return errcode;
}

void gras_cfg_free(gras_cfg_t **cfg) {
  gras_dict_free((gras_dict_t**)cfg);
}

/**
 * gras_cfg_dump:
 * @name: The name to give to this config set
 * @indent: what to write at the begining of each line (right number of spaces)
 * @cfg: the config set
 *
 * Dumps a config set for debuging purpose
 */
void
gras_cfg_dump(const char *name,const char *indent,gras_cfg_t *cfg) {
  gras_dict_t *dict = (gras_dict_t*) cfg;
  gras_dict_cursor_t *cursor=NULL; 
  gras_cfgelm_t *cell=NULL;
  char *key=NULL;
  int i; 
  gras_error_t errcode;
  int size;
  int ival;
  char *sval;
  double dval;
  gras_host_t hval;

  if (name)
    printf("%s>> Dumping of the config set '%s':\n",indent,name);
  gras_dict_foreach(dict,cursor,key,cell) {

    printf("%s  %s:",indent,key);

    size = gras_dynar_length(cell->content);
    printf("%d_to_%d_%s. Actual size=%d. List of values:\n",
	   cell->min,cell->max,gras_cfgelm_type_name[cell->type],
	   size);

    switch (cell->type) {
       
    case gras_cfgelm_int:
      for (i=0; i<size; i++) {
	gras_dynar_get(cell->content,i,&ival);
	printf ("%s    %d\n",indent,ival);
      }
      break;

    case gras_cfgelm_double:
      for (i=0; i<size; i++) {
	gras_dynar_get(cell->content,i,&dval);
	printf ("%s    %f\n",indent,dval);
      }
      break;

    case gras_cfgelm_string:
      for (i=0; i<size; i++) {
	gras_dynar_get(cell->content,i,&sval);
	printf ("%s    %s\n",indent,sval);
      }
      break;

    case gras_cfgelm_host:
      for (i=0; i<size; i++) {
	gras_dynar_get(cell->content,i,&hval);
	printf ("%s    %s:%d\n",indent,hval.name,hval.port);
      }
      break;

    default:
      printf("%s    Invalid type!!\n",indent);
    }

  }

  if (name) printf("%s<< End of the config set '%s'\n",indent,name);
  fflush(stdout);

  gras_dict_cursor_free(cursor);
  return;
}

/**
 * gras_cfgelm_free:
 *
 * @data: the data to be freed (typed as void* to be usable as free funct in dict)
 *
 * free an config element
 */

void gras_cfgelm_free(void *data) {
  gras_cfgelm_t *c=(gras_cfgelm_t *)data;

  if (!c) return;
  gras_dynar_free(c->content);
  gras_free(c);
}

/*----[ Registering stuff ]-----------------------------------------------*/

/**
 * gras_cfg_register:
 * @cfg: the config set
 * @type: the type of the config element
 * @min: the minimum
 * @max: the maximum
 *
 * register an element within a config set
 */

gras_error_t
gras_cfg_register(gras_cfg_t *cfg,
		  const char *name, gras_cfgelm_type_t type,
		  int min, int max){
  gras_cfgelm_t *res;
  gras_error_t errcode;

  DEBUG4("Register cfg elm %s (%d to %d %s)",name,min,max,gras_cfgelm_type_name[type]);
  TRYCATCH(mismatch_error,gras_dict_get((gras_dict_t*)cfg,name,(void**)&res));

  if (errcode != mismatch_error) {
    WARN1("Config elem %s registered twice.",name);
    /* Will be removed by the insertion of the new one */
  } 

  res=gras_new(gras_cfgelm_t,1);
  if (!res)
    RAISE_MALLOC;

  res->type=type;
  res->min=min;
  res->max=max;

  switch (type) {
  case gras_cfgelm_int:
    TRY(gras_dynar_new(&(res->content), sizeof(int), NULL));
    break;

  case gras_cfgelm_double:
    TRY(gras_dynar_new(&(res->content), sizeof(double), NULL));
    break;

  case gras_cfgelm_string:
   TRY(gras_dynar_new(&(res->content),sizeof(char*),&gras_cfg_str_free));
   break;

  case gras_cfgelm_host:
   TRY(gras_dynar_new(&(res->content),sizeof(gras_host_t*),&gras_cfg_host_free));
   break;

  default:
    ERROR1("%d is an invalide type code",type);
  }
    
  return gras_dict_set((gras_dict_t*)cfg,name,res,&gras_cfgelm_free);
}

/**
 * gras_cfg_unregister:
 * 
 * @cfg: the config set
 * @name: the name of the elem to be freed
 * 
 * unregister an element from a config set. 
 * Note that it removes both the DTD and the actual content.
 */

gras_error_t
gras_cfg_unregister(gras_cfg_t *cfg,const char *name) {
  return gras_dict_remove((gras_dict_t*)cfg,name);
}

/**
 * gras_cfg_register_str:
 *
 * @cfg: the config set
 * @entry: a string describing the element to register
 *
 * Parse a string and register the stuff described.
 */

gras_error_t
gras_cfg_register_str(gras_cfg_t *cfg,const char *entry) {
  char *entrycpy=strdup(entry);
  char *tok;

  int min,max;
  gras_cfgelm_type_t type;

  gras_error_t errcode;

  tok=strchr(entrycpy, ':');
  if (!tok) {
    ERROR3("%s%s%s",
	  "Invalid config element descriptor: ",entry,
	  "; Should be <name>:<min nb>_to_<max nb>_<type>");
    gras_free(entrycpy);
    gras_abort();
  }
  *(tok++)='\0';

  min=strtol(tok, &tok, 10);
  if (!tok) {
    ERROR1("Invalid minimum in config element descriptor %s",entry);
    gras_free(entrycpy);
    gras_abort();
  }

  if (!strcmp(tok,"_to_")){
    ERROR3("%s%s%s",
	  "Invalid config element descriptor: ",entry,
	  "; Should be <name>:<min nb>_to_<max nb>_<type>");
    gras_free(entrycpy);
    gras_abort();
  }
  tok += strlen("_to_");

  max=strtol(tok, &tok, 10);
  if (!tok) {
    ERROR1("Invalid maximum in config element descriptor %s",entry);
    gras_free(entrycpy);
    gras_abort();
  }

  if (*(tok++)!='_') {
    ERROR3("%s%s%s",
	  "Invalid config element descriptor: ",entry,
	  "; Should be <name>:<min nb>_to_<max nb>_<type>");
    gras_free(entrycpy);
    gras_abort();
  }

  for (type=0; 
       type<gras_cfgelm_type_count && strcmp(tok,gras_cfgelm_type_name[type]); 
       type++);
  if (type == gras_cfgelm_type_count) {
    ERROR3("%s%s%s",
	  "Invalid type in config element descriptor: ",entry,
	  "; Should be one of 'string', 'int', 'host' or 'double'.");
    gras_free(entrycpy);
    gras_abort();
  }

  TRYCLEAN(gras_cfg_register(cfg,entrycpy,type,min,max),
	   gras_free(entrycpy));

  gras_free(entrycpy); /* strdup'ed by dict mechanism, but cannot be const */
  return no_error;
}

/**
 * gras_cfg_check:
 *
 * @cfg: the config set
 * 
 * Check the config set
 */

gras_error_t
gras_cfg_check(gras_cfg_t *cfg) {
  gras_dict_cursor_t *cursor; 
  gras_cfgelm_t *cell;
  char *name;
  int size;

  gras_assert0(cfg,"NULL config set.");

  gras_dict_foreach((gras_dict_t*)cfg,cursor,name,cell) {
    size = gras_dynar_length(cell->content);
    if (cell->min > size) { 
      ERROR4("Config elem %s needs at least %d %s, but there is only %d values.",
	     name,
	     cell->min,
	     gras_cfgelm_type_name[cell->type],
	     size); 
      gras_dict_cursor_free(cursor);
      return mismatch_error;
    }

    if (cell->max < size) {
      ERROR4("Config elem %s accepts at most %d %s, but there is %d values.",
	     name,
	     cell->max,
	     gras_cfgelm_type_name[cell->type],
	     size);
      gras_dict_cursor_free(cursor);
      return mismatch_error;
    }

  }

  gras_dict_cursor_free(cursor);
  return no_error;
}

static gras_error_t gras_cfgelm_get(gras_cfg_t *cfg,
				    const char *name,
				    gras_cfgelm_type_t type,
				    /* OUT */ gras_cfgelm_t **whereto){
   
  gras_error_t errcode = gras_dict_get((gras_dict_t*)cfg,name,
				       (void**)whereto);

  if (errcode == mismatch_error) {
    ERROR1("No registered cell %s in this config set",
	   name);
    return mismatch_error;
  }
  if (errcode != no_error)
     return errcode;

  gras_assert3((*whereto)->type == type,
	       "You tried to access to the config element %s as an %s, but its type is %s.",
	       name,
	       gras_cfgelm_type_name[type],
	       gras_cfgelm_type_name[(*whereto)->type]);

  return no_error;
}

/**
 * gras_cfg_get_type:
 *
 * @cfg: the config set
 * @name: the name of the element 
 * @type: the result
 *
 * Give the type of the config element
 */

gras_error_t
gras_cfg_get_type(gras_cfg_t *cfg, const char *name, 
		      /* OUT */gras_cfgelm_type_t *type) {

  gras_cfgelm_t *cell;
  gras_error_t errcode;

  TRYCATCH(mismatch_error,gras_dict_get((gras_dict_t*)cfg,name,(void**)&cell));

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
 * gras_cfg_set_vargs(): 
 * @cfg: config set to fill
 * @varargs: NULL-terminated list of pairs {(const char*)key, value}
 *
 * Add some values to the config set.
 * @warning: if the list isn't NULL terminated, it will segfault. 
 */
gras_error_t
gras_cfg_set_vargs(gras_cfg_t *cfg, va_list pa) {
  char *str,*name;
  int i;
  double d;
  gras_cfgelm_type_t type;

  gras_error_t errcode;
  
  while ((name=va_arg(pa,char *))) {

    if (!gras_cfg_get_type(cfg,name,&type)) {
      ERROR1("Can't set the property '%s' since it's not registered",name);
      return mismatch_error;
    }

    switch (type) {
    case gras_cfgelm_host:
      str = va_arg(pa, char *);
      i=va_arg(pa,int);
      TRY(gras_cfg_set_host(cfg,name,str,i));
      break;
      
    case gras_cfgelm_string:
      str=va_arg(pa, char *);
      TRY(gras_cfg_set_string(cfg, name, str));
      break;

    case gras_cfgelm_int:
      i=va_arg(pa,int);
      TRY(gras_cfg_set_int(cfg,name,i));
      break;

    case gras_cfgelm_double:
      d=va_arg(pa,double);
      TRY(gras_cfg_set_double(cfg,name,d));
      break;

    default: 
      RAISE1(unknown_error,"Config element cell %s not valid.",name);
    }
  }
  return no_error;
}

/** 
 * gras_cfg_set():
 * @cfg: config set to fill
 * @varargs: NULL-terminated list of pairs {(const char*)key, value}
 *
 * Add some values to the config set.
 * @warning: if the list isn't NULL terminated, it will segfault. 
 */
gras_error_t gras_cfg_set(gras_cfg_t *cfg, ...) {
  va_list pa;
  gras_error_t errcode;

  va_start(pa,cfg);
  errcode=gras_cfg_set_vargs(cfg,pa);
  va_end(pa);
  return errcode;
}

/**
 * gras_cfg_set_parse():
 * @cfg: config set to fill
 * @options: a string containing the content to add to the config set. This
 * is a '\t',' ' or '\n' separated list of cells. Each individual cell is
 * like "[name]:[value]" where [name] is the name of an already registred 
 * cell, and [value] conforms to the data type under which this cell was
 * registred.
 *
 * Add the cells described in a string to a config set.
 */

gras_error_t
gras_cfg_set_parse(gras_cfg_t *cfg, const char *options) {
  int i;
  double d;
  char *str;

  gras_cfgelm_t *cell;
  char *optionlist_cpy;
  char *option,  *name,*val;

  int len;
  gras_error_t errcode;

  GRAS_IN;
  if (!options || !strlen(options)) { /* nothing to do */
    return no_error;
  }
  optionlist_cpy=strdup(options);

  DEBUG1("List to parse and set:'%s'",options);
  option=optionlist_cpy;
  while (1) { /* breaks in the code */

    if (!option) 
      break;
    name=option;
    len=strlen(name);
    DEBUG3("Still to parse and set: '%s'. len=%d; option-name=%d",name,len,option-name);

    /* Pass the value */
    while (option-name<=(len-1) && *option != ' ' && *option != '\n' && *option != '\t') {
      //fprintf(stderr,"Take %c.\n",*option);
      option++;
    }
    if (option-name == len) {
      //fprintf(stderr,"Boundary=EOL\n");
      option=NULL; /* don't do next iteration */

    } else {
      //fprintf(stderr,"Boundary on '%c'. len=%d;option-name=%d\n",*option,len,option-name);

      /* Pass the following blank chars */
      *(option++)='\0';
      while (option-name<(len-1) && (*option == ' ' || *option == '\n' || *option == '\t')) {
	//      fprintf(stderr,"Ignore a blank char.\n");
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
      gras_free(optionlist_cpy);
      gras_assert1(FALSE,
		   "Malformated option: '%s'; Should be of the form 'name:value'",
		   name);
    }
    *(val++)='\0';

    DEBUG2("name='%s';val='%s'",name,val);

    errcode=gras_dict_get((gras_dict_t*)cfg,name,(void**)&cell);
    switch (errcode) {
    case no_error:
      break;
    case mismatch_error:
      ERROR1("No registrated cell corresponding to '%s'.",name);
      gras_free(optionlist_cpy);
      return mismatch_error;
      break;
    default:
      gras_free(optionlist_cpy);
      return errcode;
    }

    switch (cell->type) {
    case gras_cfgelm_string:
      TRYCLEAN(gras_cfg_set_string(cfg, name, val),
	       gras_free(optionlist_cpy));
      break;

    case gras_cfgelm_int:
      i=strtol(val, &val, 0);
      if (val==NULL) {
	gras_free(optionlist_cpy);	
	gras_assert1(FALSE,
		     "Value of option %s not valid. Should be an integer",
		     name);
      }

      TRYCLEAN(gras_cfg_set_int(cfg,name,i),
	       gras_free(optionlist_cpy));
      break;

    case gras_cfgelm_double:
      d=strtod(val, &val);
      if (val==NULL) {
	gras_free(optionlist_cpy);	
	gras_assert1(FALSE,
	       "Value of option %s not valid. Should be a double",
	       name);
      }

      TRYCLEAN(gras_cfg_set_double(cfg,name,d),
	       gras_free(optionlist_cpy));
      break;

    case gras_cfgelm_host:
      str=val;
      val=strchr(val,':');
      if (!val) {
	gras_free(optionlist_cpy);	
	gras_assert1(FALSE,
	       "Value of option %s not valid. Should be an host (machine:port)",
	       name);
      }

      *(val++)='\0';
      i=strtol(val, &val, 0);
      if (val==NULL) {
	gras_free(optionlist_cpy);	
	gras_assert1(FALSE,
	       "Value of option %s not valid. Should be an host (machine:port)",
	       name);
      }

      TRYCLEAN(gras_cfg_set_host(cfg,name,str,i),
	       gras_free(optionlist_cpy));
      break;      

    default: 
      gras_free(optionlist_cpy);
      RAISE1(unknown_error,"Type of config element %s is not valid.",name);
    }
    
  }
  gras_free(optionlist_cpy);
  return no_error;
}

/**
 * gras_cfg_set_int:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value of the cell
 *
 * Set the value of the cell @name in @cfg with the provided value.
 */ 
gras_error_t
gras_cfg_set_int(gras_cfg_t *cfg,const char*name, int val) {
  gras_cfgelm_t *cell;
  gras_error_t errcode;

  VERB2("Configuration setting: %s=%d",name,val);
  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_int,&cell));

  if (cell->max > 1) {
    return gras_dynar_push(cell->content,&val);
  } else {
    return gras_dynar_set(cell->content,0,&val);
  }
}

/**
 * gras_cfg_set_double:
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the doule to set
 * 
 * Set the value of the cell @name in @cfg with the provided value.
 */ 

gras_error_t
gras_cfg_set_double(gras_cfg_t *cfg,const char*name, double val) {
  gras_cfgelm_t *cell;
  gras_error_t errcode;

  VERB2("Configuration setting: %s=%f",name,val);
  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_double,&cell));

  if (cell->max > 1) {
    return gras_dynar_push(cell->content,&val);
  } else {
    return gras_dynar_set(cell->content,0,&val);
  }
}

/**
 * gras_cfg_set_string:
 * 
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value to be added
 *
 * Set the value of the cell @name in @cfg with the provided value.
 */ 

gras_error_t
gras_cfg_set_string(gras_cfg_t *cfg,const char*name, const char*val) { 
  gras_cfgelm_t *cell;
  gras_error_t errcode;
   char *newval = strdup(val);

  VERB2("Configuration setting: %s=%s",name,val);
  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_string,&cell));

  if (cell->max > 1) {
    return gras_dynar_push(cell->content,&newval);
  } else {
    return gras_dynar_set(cell->content,0,&newval);
  }
}

/**
 * gras_cfg_set_host:
 * 
 * @cfg: the config set
 * @name: the name of the cell
 * @host: the host
 * @port: the port number
 *
 * Set the value of the cell @name in @cfg with the provided value 
 * on the given @host to the given @port
 */ 

gras_error_t 
gras_cfg_set_host(gras_cfg_t *cfg,const char*name, 
		  const char *host,int port) {
  gras_cfgelm_t *cell;
  gras_error_t errcode;
  gras_host_t *val=gras_new(gras_host_t,1);

  VERB3("Configuration setting: %s=%s:%d",name,host,port);
  if (!val)
    RAISE_MALLOC;
  val->name = strdup(name);
  val->port = port;

  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_host,&cell));

  if (cell->max > 1) {
    return gras_dynar_push(cell->content,&val);
  } else {
    return gras_dynar_set(cell->content,0,&val);
  }
}

/* ---- [ Removing ] ---- */

/**
 * gras_cfg_rm_int:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value to be removed
 *
 * Remove the provided @val from the cell @name in @cfg.
 */
gras_error_t gras_cfg_rm_int   (gras_cfg_t *cfg,const char*name, int val) {

  gras_cfgelm_t *cell;
  int cpt,seen;
  gras_error_t errcode;

  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_int,&cell));
  
  gras_dynar_foreach(cell->content,cpt,seen) {
    if (seen == val) {
      gras_dynar_cursor_rm(cell->content,&cpt);
      return no_error;
    }
  }

  ERROR2("Can't remove the value %d of config element %s: value not found.",
	 val,name);
  return mismatch_error;
}

/**
 * gras_cfg_rm_double:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value to be removed
 *
 * Remove the provided @val from the cell @name in @cfg.
 */

gras_error_t gras_cfg_rm_double(gras_cfg_t *cfg,const char*name, double val) {
  gras_cfgelm_t *cell;
  int cpt;
  double seen;
  gras_error_t errcode;

  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_double,&cell));
  
  gras_dynar_foreach(cell->content,cpt,seen) {
    if (seen == val) {
      gras_dynar_cursor_rm(cell->content,&cpt);
      return no_error;
    }
  }

  ERROR2("Can't remove the value %f of config element %s: value not found.",
	 val,name);
  return mismatch_error;
}

/**
 * gras_cfg_rm_string:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the value of the string which will be removed
 *
 * Remove the provided @val from the cell @name in @cfg.
 */
gras_error_t
gras_cfg_rm_string(gras_cfg_t *cfg,const char*name, const char *val) {
  gras_cfgelm_t *cell;
  int cpt;
  char *seen;
  gras_error_t errcode;

  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_string,&cell));
  
  gras_dynar_foreach(cell->content,cpt,seen) {
    if (!strcpy(seen,val)) {
      gras_dynar_cursor_rm(cell->content,&cpt);
      return no_error;
    }
  }

  ERROR2("Can't remove the value %s of config element %s: value not found.",
	 val,name);
  return mismatch_error;
}

/**
 * gras_cfg_rm_host:
 * 
 * @cfg: the config set
 * @name: the name of the cell
 * @host: the hostname
 * @port: the port number
 *
 * Remove the provided @host:@port from the cell @name in @cfg.
 */

gras_error_t
gras_cfg_rm_host  (gras_cfg_t *cfg,const char*name, const char *host,int port) {
  gras_cfgelm_t *cell;
  int cpt;
  gras_host_t *seen;
  gras_error_t errcode;

  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_host,&cell));
  
  gras_dynar_foreach(cell->content,cpt,seen) {
    if (!strcpy(seen->name,host) && seen->port == port) {
      gras_dynar_cursor_rm(cell->content,&cpt);
      return no_error;
    }
  }

  ERROR3("Can't remove the value %s:%d of config element %s: value not found.",
	 host,port,name);
  return mismatch_error;
}

/* rm everything */

/**
 * gras_cfg_empty:
 * 
 * @cfg: the config set
 * @name: the name of the cell
 *
 * rm evenything
 */

gras_error_t 
gras_cfg_empty(gras_cfg_t *cfg,const char*name) {
  gras_cfgelm_t *cell;

  gras_error_t errcode;

  TRYCATCH(mismatch_error,
	   gras_dict_get((gras_dict_t*)cfg,name,(void**)&cell));
  if (errcode == mismatch_error) {
    ERROR1("Can't empty  '%s' since this config element does not exist",
	   name);
    return mismatch_error;
  }

  if (cell) {
    gras_dynar_reset(cell->content);
  }
  return no_error;
}

/*----[ Getting ]---------------------------------------------------------*/

/**
 * gras_cfg_get_int:
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using gras_cfg_get_dynar() 
 * instead.
 *
 * @warning the returned value is the actual content of the config set
 */
gras_error_t
gras_cfg_get_int   (gras_cfg_t  *cfg, 
		    const char *name,
		    int        *val) {
  gras_cfgelm_t *cell;
  gras_error_t errcode;

  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_int,&cell));

  if (gras_dynar_length(cell->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %d values",
	     name, gras_dynar_length(cell->content));
  }

  gras_dynar_get(cell->content, 0, (void*)val);
  return no_error;
}

/**
 * gras_cfg_get_double:
 * @cfg: the config set
 * @name: the name of the cell
 * @val: the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using gras_cfg_get_dynar() 
 * instead.
 *
 * @warning the returned value is the actual content of the config set
 */

gras_error_t
gras_cfg_get_double(gras_cfg_t *cfg,
		    const char *name,
		    double     *val) {
  gras_cfgelm_t *cell;
  gras_error_t errcode;

  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_double,&cell));

  if (gras_dynar_length(cell->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %d values\n",
	     name, gras_dynar_length(cell->content));
  }

  gras_dynar_get(cell->content, 0, (void*)val);
  return no_error;
}

/**
 * gras_cfg_get_string:
 *
 * @th: the config set
 * @name: the name of the cell
 * @val: the wanted value
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using gras_cfg_get_dynar() 
 * instead.
 *
 * @warning the returned value is the actual content of the config set
 */

gras_error_t gras_cfg_get_string(gras_cfg_t *cfg,
				 const char *name,
				 char      **val) {
  gras_cfgelm_t *cell;
  gras_error_t errcode;

  *val=NULL;

  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_string,&cell));

  if (gras_dynar_length(cell->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %d values\n",
	     name, gras_dynar_length(cell->content));
  }

  gras_dynar_get(cell->content, 0, (void*)val);
  return no_error;
}

/**
 * gras_cfg_get_host:
 *
 * @cfg: the config set
 * @name: the name of the cell
 * @host: the host
 * @port: the port number
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning. Consider using gras_cfg_get_dynar() 
 * instead.
 *
 * @warning the returned value is the actual content of the config set
 */

gras_error_t gras_cfg_get_host  (gras_cfg_t *cfg,
				 const char *name,
				 char      **host,
				 int        *port) {
  gras_cfgelm_t *cell;
  gras_error_t errcode;
  gras_host_t *val;

  TRY (gras_cfgelm_get(cfg,name,gras_cfgelm_host,&cell));

  if (gras_dynar_length(cell->content) > 1) {
    WARN2("You asked for the first value of the config element '%s', but there is %d values\n",
	     name, gras_dynar_length(cell->content));
  }

  gras_dynar_get(cell->content, 0, (void*)val);
  *host=val->name;
  *port=val->port;
  
  return no_error;
}

/**
 * gras_cfg_get_dynar:
 * @cfg: where to search in
 * @name: what to search for
 * @dynar: result
 *
 * Get the data stored in the config bag. 
 *
 * @warning the returned value is the actual content of the config set
 */
gras_error_t gras_cfg_get_dynar (gras_cfg_t    *cfg,
				 const char    *name,
				 gras_dynar_t **dynar) {
  gras_cfgelm_t *cell;
  gras_error_t errcode = gras_dict_get((gras_dict_t*)cfg,name,
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

