/* $Id$ */

/* config - Dictionnary where the type of each cell is provided.            */

/* This is useful to build named structs, like option or property sets.     */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2001,2002,2003,2004 the OURAGAN project.                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _GRAS_CONFIG_H_
#define _GRAS_CONFIG_H_

typedef struct {
  char *name;
  int port;
} gras_host_t;

/* For now, a config is only a special dynar. But don't rely on it, */
/* it may change in the future. */
typedef gras_dynar_t gras_cfg_t;

/* type of a typed hash cell */
typedef enum {
  gras_cfgelm_int=0, gras_cfgelm_double, gras_cfgelm_string, gras_cfgelm_host,
  gras_cfgelm_type_count
} gras_cfgelm_type_t;

/*----[ Memory management ]-----------------------------------------------*/
void gras_cfg_new (gras_cfg_t **whereto); /* (whereto == NULL) is ok */
void gras_cfg_cpy(gras_cfg_t **whereto, gras_cfg_t *tocopy);
void gras_cfg_free(gras_cfg_t **cfg);
void gras_cfg_dump(const char *name,const char*indent,gras_cfg_t *cfg);

/*----[ Registering stuff ]-----------------------------------------------*/
/* Register a possible cell */
void gras_cfg_register(gras_cfg_t *cfg,
		       const char *name, gras_cfgelm_type_t type,
		       int min, int max);
/* Unregister a possible cell */
gras_error_t gras_cfg_unregister(gras_cfg_t *cfg, const char *name);

/* Parse the configuration descriptor and register it */
/* Should be of the form "<name>:<min nb>_to_<max nb>_<type>", */
/*  with type being one of 'string','int', 'host' or 'double'  */
gras_error_t gras_cfg_register_str(gras_cfg_t *cfg, const char *entry);

/* Check that each cell have the right amount of elements */
gras_error_t gras_cfg_check(gras_cfg_t *cfg);

/* Get the type of this option in that repository */
gras_error_t gras_cfg_get_type(gras_cfg_t *cfg, const char *name, 
			       /* OUT */ gras_cfgelm_type_t *type);

/*----[ Setting ]---------------------------------------------------------
 * gras_cfg_set_* functions.
 *
 * If the registered maximum is equal to 1, those functions remplace the 
 * current value with the provided one. If max>1, the provided value is 
 * appended to the list.
 *
 * string values are strdup'ed before use, so you have to free your copy  */

gras_error_t gras_cfg_set_vargs(gras_cfg_t *cfg, va_list pa);
gras_error_t gras_cfg_set(gras_cfg_t *cfg, ...);

/*
  Add the cells described in a string to a typed hash.
 */
gras_error_t gras_cfg_set_parse(gras_cfg_t *cfg, const char *options);


/*
  Set the value of the cell @name in @cfg with the provided value.
 */
gras_error_t gras_cfg_set_int   (gras_cfg_t *cfg, const char *name, 
				 int val);
gras_error_t gras_cfg_set_double(gras_cfg_t *cfg, const char *name, 
				 double val);
gras_error_t gras_cfg_set_string(gras_cfg_t *cfg, const char *name, 
				 const char *val);
gras_error_t gras_cfg_set_host  (gras_cfg_t *cfg, const char *name, 
				 const char *host,int port);

/*
 Remove the provided value from the cell @name in @cfg.
 */
gras_error_t gras_cfg_rm_int   (gras_cfg_t *cfg, const char *name, 
				int val);
gras_error_t gras_cfg_rm_double(gras_cfg_t *cfg, const char *name, 
				double val);
gras_error_t gras_cfg_rm_string(gras_cfg_t *cfg, const char *name, 
				const char *val);
gras_error_t gras_cfg_rm_host  (gras_cfg_t *cfg, const char *name, 
				const char *host,int port);

/* rm every values */
gras_error_t gras_cfg_empty(gras_cfg_t *cfg, const char *name);	

/*----[ Getting ]---------------------------------------------------------*/
/* Returns a pointer to the values actually stored in the cache. Do not   */
/* modify them unless you really know what you're doing.                  */
gras_error_t gras_cfg_get_int   (gras_cfg_t *cfg,
				 const char *name,
				 int        *val);
gras_error_t gras_cfg_get_double(gras_cfg_t *cfg,
				 const char *name,
				 double     *val);
gras_error_t gras_cfg_get_string(gras_cfg_t *cfg,
				 const char *name, 
				 char      **val);
gras_error_t gras_cfg_get_host  (gras_cfg_t *cfg, 
				 const char *name, 
				 char      **host,
				 int        *port);
gras_error_t gras_cfg_get_dynar (gras_cfg_t    *cfg,
				 const char    *name,
				 gras_dynar_t **dynar);


#endif /* _GRAS_CONFIG_H_ */
