/* $Id$ */

/* config - Dictionnary where the type of each cell is provided.            */

/* This is useful to build named structs, like option or property sets.     */

/* Copyright (c) 2001,2002,2003,2004 Martin Quinson. All rights reserved.   */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONFIG_H_
#define _XBT_CONFIG_H_

#include "xbt/dynar.h"

BEGIN_DECL
  
/* For now, a config is only a special dynar. But don't rely on it, */
/* it may change in the future. */
typedef xbt_dynar_t xbt_cfg_t;

/* type of a typed hash cell */
typedef enum {
  xbt_cfgelm_int=0, xbt_cfgelm_double, xbt_cfgelm_string, xbt_cfgelm_host,
  xbt_cfgelm_type_count
} e_xbt_cfgelm_type_t;

/*----[ Memory management ]-----------------------------------------------*/
xbt_cfg_t xbt_cfg_new (void);
void xbt_cfg_cpy(xbt_cfg_t tocopy,
        /* OUT */ xbt_cfg_t *whereto);
void xbt_cfg_free(xbt_cfg_t *cfg);
void xbt_cfg_dump(const char *name,const char*indent,xbt_cfg_t cfg);

/*----[ Registering stuff ]-----------------------------------------------*/
/* Register a possible cell */
void xbt_cfg_register(xbt_cfg_t cfg,
		       const char *name, e_xbt_cfgelm_type_t type,
		       int min, int max);
/* Unregister a possible cell */
xbt_error_t xbt_cfg_unregister(xbt_cfg_t cfg, const char *name);

/* Parse the configuration descriptor and register it */
/* Should be of the form "<name>:<min nb>_to_<max nb>_<type>", */
/*  with type being one of 'string','int', 'host' or 'double'  */
xbt_error_t xbt_cfg_register_str(xbt_cfg_t cfg, const char *entry);

/* Check that each cell have the right amount of elements */
xbt_error_t xbt_cfg_check(xbt_cfg_t cfg);

/* Get the type of this option in that repository */
xbt_error_t xbt_cfg_get_type(xbt_cfg_t cfg, const char *name, 
			       /* OUT */ e_xbt_cfgelm_type_t *type);

/*----[ Setting ]---------------------------------------------------------
 * xbt_cfg_set_* functions.
 *
 * If the registered maximum is equal to 1, those functions remplace the 
 * current value with the provided one. If max>1, the provided value is 
 * appended to the list.
 *
 * string values are strdup'ed before use, so you have to free your copy  */

xbt_error_t xbt_cfg_set_vargs(xbt_cfg_t cfg, va_list pa);
xbt_error_t xbt_cfg_set(xbt_cfg_t cfg, ...);

/*
  Add the cells described in a string to a typed hash.
 */
xbt_error_t xbt_cfg_set_parse(xbt_cfg_t cfg, const char *options);


/*
  Set the value of the cell @name in @cfg with the provided value.
 */
xbt_error_t xbt_cfg_set_int   (xbt_cfg_t cfg, const char *name, 
				 int val);
xbt_error_t xbt_cfg_set_double(xbt_cfg_t cfg, const char *name, 
				 double val);
xbt_error_t xbt_cfg_set_string(xbt_cfg_t cfg, const char *name, 
				 const char *val);
xbt_error_t xbt_cfg_set_host  (xbt_cfg_t cfg, const char *name, 
				 const char *host,int port);

/*
 Remove the provided value from the cell @name in @cfg.
 */
xbt_error_t xbt_cfg_rm_int   (xbt_cfg_t cfg, const char *name, 
				int val);
xbt_error_t xbt_cfg_rm_double(xbt_cfg_t cfg, const char *name, 
				double val);
xbt_error_t xbt_cfg_rm_string(xbt_cfg_t cfg, const char *name, 
				const char *val);
xbt_error_t xbt_cfg_rm_host  (xbt_cfg_t cfg, const char *name, 
				const char *host,int port);

/* rm every values */
xbt_error_t xbt_cfg_empty(xbt_cfg_t cfg, const char *name);	

/*----[ Getting ]---------------------------------------------------------*/
/* Returns a pointer to the values actually stored in the cache. Do not   */
/* modify them unless you really know what you're doing.                  */
xbt_error_t xbt_cfg_get_int   (xbt_cfg_t  cfg,
				 const char *name,
				 int        *val);
xbt_error_t xbt_cfg_get_double(xbt_cfg_t  cfg,
				 const char *name,
				 double     *val);
xbt_error_t xbt_cfg_get_string(xbt_cfg_t  cfg,
				 const char *name, 
				 char      **val);
xbt_error_t xbt_cfg_get_host  (xbt_cfg_t  cfg, 
				 const char *name, 
				 char      **host,
				 int        *port);
xbt_error_t xbt_cfg_get_dynar (xbt_cfg_t    cfg,
				 const char   *name,
		       /* OUT */ xbt_dynar_t *dynar);

END_DECL
  
#endif /* _XBT_CONFIG_H_ */
