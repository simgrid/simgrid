/* $Id$ */

/* process - GRAS process handling (common code for RL and SG)              */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "gras/transport.h"
#include "gras/datadesc.h"
#include "gras/messages.h"
#include "gras_modinter.h"

#include "gras/Virtu/virtu_private.h"

XBT_LOG_NEW_SUBCATEGORY(virtu,gras,"Virtualization code");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(process,virtu,"Process manipulation code");


/* Functions to handle gras_procdata_t->libdata cells*/
typedef struct {
   char *name;
   pvoid_f_void_t *creator;
   void_f_pvoid_t *destructor;
} s_gras_procdata_fabric_t, *gras_procdata_fabric_t;

static xbt_dynar_t _gras_procdata_fabrics = NULL; /* content: s_gras_procdata_fabric_t */

static void gras_procdata_fabric_free(void *fab) {
   free( ((gras_procdata_fabric_t)fab)->name );
}

/** @brief declare the functions in charge of creating/destructing the procdata of a module
 *  
 *  This is intended to be called from the gras_<module>_register function.
 */
void gras_procdata_add(const char *name, pvoid_f_void_t creator,void_f_pvoid_t destructor) {
   
   gras_procdata_fabric_t fab;
   
   if (!_gras_procdata_fabrics) {
      /* create the dynar if needed */
      _gras_procdata_fabrics = xbt_dynar_new(sizeof(s_gras_procdata_fabric_t),
					     gras_procdata_fabric_free);
   }
   
   fab=xbt_dynar_push_ptr(_gras_procdata_fabrics);
   
   fab->name       = xbt_strdup(name);
   fab->creator    = creator;
   fab->destructor = destructor;
}

/* **************************************************************************
 * Process data
 * **************************************************************************/
void *gras_userdata_get(void) {
  gras_procdata_t *pd=gras_procdata_get();
  return pd->userdata;
}

void gras_userdata_set(void *ud) {
  gras_procdata_t *pd=gras_procdata_get();

  pd->userdata = ud;
}

void *gras_libdata_get(const char *name) {
  gras_procdata_t *pd=gras_procdata_get();
  void *res;
  xbt_error_t errcode;
   
  errcode = xbt_dict_get(pd->libdata, name, &res);
  xbt_assert2(errcode == no_error, 
	      "Cannot retrive the libdata associated to %s: %s",
	      name, xbt_error_name(errcode));
   
  return res;
}

void
gras_procdata_init() {
  gras_procdata_t *pd=gras_procdata_get();
  s_gras_procdata_fabric_t fab;
   
  int cursor;
   
  xbt_error_t errcode;
  void *data;

  pd->userdata  = NULL;
  pd->libdata   = xbt_dict_new();
   
  xbt_dynar_foreach(_gras_procdata_fabrics,cursor,fab){
     
     xbt_assert1(fab.name,"Name of fabric #%d is NULL!",cursor);
     DEBUG1("Create the procdata for %s",fab.name);
     /* Check for our own errors */
     errcode = xbt_dict_get(pd->libdata, fab.name, &data);
     xbt_assert1(errcode == mismatch_error,
		 "MayDay: two modules use '%s' as libdata name", fab.name);
     
     /* Add the data in place */
     xbt_dict_set(pd->libdata, fab.name, (fab.creator)(), fab.destructor);

  }
}

void
gras_procdata_exit() {
  gras_procdata_t *pd=gras_procdata_get();

  xbt_dict_free(&( pd->libdata ));
}
