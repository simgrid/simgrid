/* $Id$ */

/* process - GRAS process handling (common code for RL and SG)              */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "gras/transport.h"
#include "gras/datadesc.h"
#include "gras/messages.h"
#include "gras_modinter.h"

#include "gras/Virtu/virtu_private.h"

XBT_LOG_NEW_SUBCATEGORY(gras_virtu,gras,"Virtualization code");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_virtu_process,gras_virtu,"Process manipulation code");


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
 *  This returns the module ID you can use for gras_libdata_by_id()
 */
int gras_procdata_add(const char *name, pvoid_f_void_t creator,void_f_pvoid_t destructor) {
   
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
   return xbt_dynar_length(_gras_procdata_fabrics)-1;
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

void *gras_libdata_by_name(const char *name) {
  gras_procdata_t *pd=gras_procdata_get();
  void *res=NULL;
  xbt_ex_t e;
   
  TRY {
    res = xbt_set_get_by_name(pd->libdata, name);
  } CATCH(e) {
    RETHROW1("Cannot retrieve the libdata associated to %s: %s",name);
  }   
  return res;
}

void *gras_libdata_by_id(int id) {
  gras_procdata_t *pd=gras_procdata_get();
  return xbt_set_get_by_id(pd->libdata, id);
}

void
gras_procdata_init() {
  gras_procdata_t *pd=gras_procdata_get();
  s_gras_procdata_fabric_t fab;
   
  int cursor;
   
  xbt_ex_t e;
  void *data;

  pd->userdata  = NULL;
  pd->libdata   = xbt_set_new();
   
  xbt_dynar_foreach(_gras_procdata_fabrics,cursor,fab){
    volatile int found = 0;
     
    xbt_assert1(fab.name,"Name of fabric #%d is NULL!",cursor);
    DEBUG1("Create the procdata for %s",fab.name);
    /* Check for our own errors */
    TRY {
      data = xbt_set_get_by_name(pd->libdata, fab.name);
      found = 1;
    } CATCH(e) {
      xbt_ex_free(e);
      found = 0;
    }
    if (found)
      THROW1(unknown_error,0,"MayDay: two modules use '%s' as libdata name", fab.name);
    
    /* Add the data in place */
    xbt_set_add(pd->libdata, (fab.creator)(), fab.destructor);
  }
}

void
gras_procdata_exit() {
  int len;
  gras_procdata_t *pd=gras_procdata_get();

  xbt_set_free(&( pd->libdata ));
  
  /* Remove procdata in reverse order wrt creation */
  while ((len=xbt_dynar_length(_gras_procdata_fabrics))) {
    xbt_dynar_remove_at(_gras_procdata_fabrics,len-1,NULL);
  }
  xbt_dynar_free( & _gras_procdata_fabrics );
}
