/* $Id$ */

/* cbps - persistant states for callbacks                                   */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 da GRAS posse.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/DataDesc/datadesc_private.h"
GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(ddt_cbps,datadesc,"callback persistant state");

typedef struct {
  gras_datadesc_type_t *type;
  void                 *data;
} gras_cbps_elm_t;

struct s_gras_cbps {
  gras_dynar_t *lints; /* simple stack of long integers (easy interface) */

  gras_dict_t  *space; /* varname x dynar of gras_cbps_elm_t */
  gras_dynar_t *frames; /* of dynar of names defined within this frame
			   (and to pop when we leave it) */
  gras_dynar_t *globals;
};

static void free_string(void *d);

static void free_string(void *d){
  gras_free(*(void**)d);
}

gras_cbps_t * gras_cbps_new(void) {
  gras_error_t errcode;
  gras_cbps_t *res;

  res=gras_new(gras_cbps_t,1);

  res->lints = gras_dynar_new(sizeof(int), NULL);
  res->space = gras_dict_new();
  /* no leak, the content is freed manually on block_end */
  res->frames = gras_dynar_new(sizeof(gras_dynar_t*), NULL);
  res->globals = gras_dynar_new(sizeof(char*), NULL);

  gras_cbps_block_begin(res);

  return res;
}

void gras_cbps_free(gras_cbps_t **state) {

  gras_dynar_free(    (*state)->lints   );

  gras_cbps_block_end(*state);
  gras_dict_free ( &( (*state)->space   ) );
  gras_dynar_free(    (*state)->frames    );
  gras_dynar_free(    (*state)->globals   );

  gras_free(*state);
  *state = NULL;
}

/**
 * gras_cbps_v_push:
 *
 * Declare a new element in the PS, and give it a value. If an element of that
 * name already exists, it is masked by the one given here, and will be 
 * seeable again only after a pop to remove the value this push adds.
 */
gras_error_t
gras_cbps_v_push(gras_cbps_t        *ps,
		 const char            *name,
		 void                  *data,
		 gras_datadesc_type_t  *ddt) {

  gras_dynar_t            *varstack,*frame;
  gras_cbps_elm_t      *p_var;
  gras_error_t errcode;
  char *varname = (char*)strdup(name);

  DEBUG2("push(%s,%p)",name,(void*)data);
  errcode = gras_dict_get(ps->space, name, (void **)&varstack);
 
  if (errcode == mismatch_error) {
    DEBUG1("Create a new variable stack for '%s' into the space",name);
    varstack = gras_dynar_new(sizeof (gras_cbps_elm_t *), NULL);
    gras_dict_set(ps->space, varname, (void **)varstack, NULL);
    /* leaking, you think? only if you do not close all the openned blocks ;)*/
  } else if (errcode != no_error) {
    return errcode;
  }
 
  p_var       = gras_new0(gras_cbps_elm_t,1);
  p_var->type = ddt;
  p_var->data = data;
  
  gras_dynar_push(varstack, &p_var);
  
  gras_dynar_pop(ps->frames, &frame);
  DEBUG4("Push %s (%p @%p) into frame %p",varname,(void*)varname,(void*)&varname,(void*)frame);
  gras_dynar_push(frame, &varname);
  gras_dynar_push(ps->frames, &frame); 
  return no_error;
}

/**
 * gras_cbps_v_pop:
 *
 * Retrieve an element from the PS, and remove it from the PS. If it's not
 * present in the current block, it will fail (with abort) and not search
 * in upper blocks since this denotes a programmation error.
 */
gras_error_t
gras_cbps_v_pop (gras_cbps_t        *ps, 
		    const char            *name,
		    gras_datadesc_type_t **ddt,
		    void                 **res) {
  gras_dynar_t            *varstack,*frame;
  gras_cbps_elm_t      *var          = NULL;
  void                    *data           = NULL;
  gras_error_t errcode;

  DEBUG1("pop(%s)",name);
  /* FIXME: Error handling */
  errcode = gras_dict_get(ps->space, name, (void **)&varstack);
  if (errcode == mismatch_error) {
    RAISE1(mismatch_error,"Asked to pop the non-existant %s",
	   name);
  }
  gras_dynar_pop(varstack, &var);
  
  if (!gras_dynar_length(varstack)) {
    DEBUG1("Last incarnation of %s poped. Kill it",name);
    gras_dict_remove(ps->space, name);
    gras_dynar_free(varstack);
  }
  
  if (ddt)
    *ddt = var->type;  
  data    = var->data;
  
  gras_free(var);
  
  gras_dynar_pop(ps->frames, &frame);
  {
    int l = gras_dynar_length(frame);
    
    while (l--) {
      char *_name = NULL;
                                                                                
      _name = gras_dynar_get_as(frame, l, char*);
      if (!strcmp(name, _name)) {
	gras_dynar_remove_at(frame, l, &_name);
	gras_free(_name);
	break;
      }
    }
  }
  gras_dynar_push(ps->frames, &frame);
  
  *res = data;
  return no_error;
}

/**
 * gras_cbps_v_set:
 *
 * Change the value of an element in the PS.  
 * If it's not present in the current block, look in the upper ones.
 * If it's not present in any of them, modify in the globals
 * If not present there neither, the code may segfault (Oli?).
 *
 * Once a reference to an element of that name is found somewhere in the PS,
 *   its value is changed.
 */
void
gras_cbps_v_set (gras_cbps_t        *ps,
		    const char            *name,
		    void                  *data,
		    gras_datadesc_type_t  *ddt) {

  gras_dynar_t            *p_dynar        = NULL;
  gras_cbps_elm_t      *p_elm          = NULL;
  gras_error_t errcode;
  
  DEBUG1("set(%s)",name);
  errcode = gras_dict_get(ps->space, name, (void **)&p_dynar);
  
  if (errcode == mismatch_error) {
    p_dynar = gras_dynar_new(sizeof (gras_cbps_elm_t *), NULL);
    gras_dict_set(ps->space, name, (void **)p_dynar, NULL);
    
    p_elm   = gras_new0(gras_cbps_elm_t,1);
    gras_dynar_push(ps->globals, &name);
  } else {
    gras_dynar_pop(p_dynar, &p_elm);
  }
  
  p_elm->type   = ddt;
  p_elm->data   = data;
 
  gras_dynar_push(p_dynar, &p_elm);

}

/**
 * gras_cbps_v_get:
 *
 * Get the value of an element in the PS without modifying it. 
 * (note that you get the content of the data struct and not a copy to it)
 * If it's not present in the current block, look in the upper ones.
 * If it's not present in any of them, look in the globals
 * If not present there neither, the code may segfault (Oli?).
 */
void *
gras_cbps_v_get (gras_cbps_t        *ps, 
		    const char            *name,
		    gras_datadesc_type_t **ddt) {
  
  gras_dynar_t            *p_dynar        = NULL;
  gras_cbps_elm_t      *p_elm          = NULL;
  
  DEBUG1("get(%s)",name);
  /* FIXME: Error handling */
  gras_dict_get(ps->space, name, (void **)&p_dynar);
  gras_dynar_pop(p_dynar, &p_elm);
  gras_dynar_push(p_dynar, &p_elm);
  
  if (ddt) {
    *ddt = p_elm->type;
  }
  
  return p_elm->data;

}

/**
 * gras_cbps_block_begin:
 *
 * Begins a new block. 
 *
 * Blocks are usefull to remove a whole set of declarations you don't even know
 *
 * E.g., they constitute an elegent solution to recursive data structures. 
 *
 * push/pop may be used in some cases for that, but if your recursive data 
 * struct contains other structs needing themselves callbacks, you have to
 * use block_{begin,end} to do the trick.
 */

void
gras_cbps_block_begin(gras_cbps_t *ps) {

  gras_dynar_t            *p_dynar        = NULL;

  DEBUG0(">>> Block begin");
  p_dynar = gras_dynar_new(sizeof (char *), NULL);
  gras_dynar_push(ps->frames, &p_dynar);
}

/**
 * gras_cbps_block_begin:
 *
 * End the current block, and go back to the upper one.
 */
void
gras_cbps_block_end(gras_cbps_t *ps) {

  gras_dynar_t            *frame        = NULL;
  int                      cursor         =    0;
  char                    *name           = NULL;

  gras_assert0(gras_dynar_length(ps->frames),
	       "More block_end than block_begin");
  gras_dynar_pop(ps->frames, &frame);
  
  gras_dynar_foreach(frame, cursor, name) {

    gras_dynar_t            *varstack    = NULL;
    gras_cbps_elm_t      *var         = NULL;
 
    DEBUG2("Get ride of %s (%p)",name,(void*)name);
    gras_dict_get(ps->space, name, (void **)&varstack);
    gras_dynar_pop(varstack, &var);
 
    if (!gras_dynar_length(varstack)) {
      gras_dict_remove(ps->space, name);
      gras_dynar_free_container(varstack); /*already empty, save a test ;) */
    }
    
    if (var->data) gras_free(var->data);
    gras_free(var);
    gras_free(name);
  }
  gras_dynar_free_container(frame);/* we just emptied it */
  DEBUG0("<<< Block end");
}


/**
 * gras_cbps_i_push:
 *
 * Push a new long integer value into the cbps.
 */
void
gras_cbps_i_push(gras_cbps_t        *ps, 
		 int val) {
  gras_error_t errcode;
  DEBUG1("push %d as a size",val);
  gras_dynar_push(ps->lints,&val);
}
/**
 * gras_cbps_i_pop:
 *
 * Pop the lastly pushed long integer value from the cbps.
 */
int
gras_cbps_i_pop(gras_cbps_t        *ps) {
  int ret;

  gras_assert0(gras_dynar_length(ps->lints) > 0,
	       "gras_cbps_i_pop: no value to pop");
  gras_dynar_pop(ps->lints, &ret);
  DEBUG1("pop %d as a size",ret);
  return ret;
}

/**
 * gras_datadesc_cb_pop:
 *
 * Generic cb returning the lastly pushed value
 */
int gras_datadesc_cb_pop(gras_cbps_t *vars, void *data) {
  return gras_cbps_i_pop(vars);
}

/**
 * gras_datadesc_cb_push_int:
 * 
 * Cb to push an integer. Must be attached to the field you want to push
 */
void gras_datadesc_cb_push_int(gras_cbps_t *vars, void *data) {
   int *i = (int*)data;
   gras_cbps_i_push(vars, (int) *i);
}

/**
 * gras_datadesc_cb_push_uint:
 * 
 * Cb to push an unsigned integer. Must be attached to the field you want to push
 */
void gras_datadesc_cb_push_uint(gras_cbps_t *vars, void *data) {
   unsigned int *i = (unsigned int*)data;
   gras_cbps_i_push(vars, (int) *i);
}

/**
 * gras_datadesc_cb_push_lint:
 * 
 * Cb to push an long integer. Must be attached to the field you want to push
 */
void gras_datadesc_cb_push_lint(gras_cbps_t *vars, void *data) {
   long int *i = (long int*)data;
   gras_cbps_i_push(vars, (int) *i);
}
/**
 * gras_datadesc_cb_push_ulint:
 * 
 * Cb to push an long integer. Must be attached to the field you want to push
 */
void gras_datadesc_cb_push_ulint(gras_cbps_t *vars, void *data) {
   unsigned long int *i = (unsigned long int*)data;
   gras_cbps_i_push(vars, (int) *i);
}
