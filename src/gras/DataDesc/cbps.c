/* $Id$ */

/* cbps - persistant states for callbacks                                   */

/* Authors: Olivier Aumage, Martin Quinson                                  */
/* Copyright (C) 2003, 2004 da GRAS posse.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "DataDesc/datadesc_private.h"

typedef struct {
  gras_datadesc_type_t *type;
  void                 *data;
} gras_dd_cbps_elm_t;

struct s_gras_dd_cbps {
  gras_dict_t  *space;
  gras_dynar_t *stack;
  gras_dynar_t *globals;
};

gras_error_t
gras_dd_cbps_new(gras_dd_cbps_t **dst) {
  gras_error_t errcode;
  gras_dd_cbps_t *res;

  if (!(res=malloc(sizeof(gras_dd_cbps_t))))
    RAISE_MALLOC;

  TRY(gras_dict_new(&(res->space)));
  /* FIXME:leaking on content of dynars*/
  TRY(gras_dynar_new(&(res->stack), sizeof(gras_dynar_t*), NULL));
  TRY(gras_dynar_new(&(res->globals), sizeof(char*), NULL));

  *dst = res;
  return no_error;
}

void
gras_dd_cbps_free(gras_dd_cbps_t **state) {

  gras_dict_free ( &( (*state)->space   ) );
  gras_dynar_free(    (*state)->stack     );
  gras_dynar_free(    (*state)->globals   );

  free(*state);
  *state = NULL;
}

/**
 * gras_dd_cbps_push:
 *
 * Declare a new element in the PS, and give it a value. If an element of that
 * name already exists, it is masked by the one given here, and will be 
 * seeable again only after a pop to remove the value this push adds.
 */
void
gras_dd_cbps_push(gras_dd_cbps_t        *ps,
		  const char            *name,
		  void                  *data,
		  gras_datadesc_type_t  *ddt) {

  gras_dynar_t            *p_dynar        = NULL;
  gras_dd_cbps_elm_t      *p_var          = NULL;
 
  gras_dict_get(ps->space, name, (void **)&p_dynar);
 
  if (!p_dynar) {
    gras_dynar_new(&p_dynar, sizeof (gras_dd_cbps_elm_t *), NULL);
    gras_dict_set(ps->space, name, (void **)p_dynar, NULL);
    /* FIXME: leaking on dynar. Insert in dict with a way to free it */
  }
 
  p_var       = calloc(1, sizeof(gras_dd_cbps_elm_t));
  p_var->type = ddt;
  p_var->data = data;
  
  gras_dynar_push(p_dynar, &p_var);
  
  gras_dynar_pop(ps->stack, &p_dynar);
  gras_dynar_push(p_dynar, &name);
  gras_dynar_push(ps->stack, &p_dynar); 
}

/**
 * gras_dd_cbps_pop:
 *
 * Retrieve an element from the PS, and remove it from the PS. If it's not
 * present in the current block, it will fail (with abort) and not search
 * in upper blocks since this denotes a programmation error.
 */
void *
gras_dd_cbps_pop (gras_dd_cbps_t        *ps, 
		  const char            *name,
		  gras_datadesc_type_t **ddt) {
  gras_dynar_t            *p_dynar        = NULL;
  gras_dd_cbps_elm_t      *p_elm          = NULL;
  void                    *data           = NULL;

  /* FIXME: Error handling */
  gras_dict_get(ps->space, name, (void **)&p_dynar);
  gras_dynar_pop(p_dynar, &p_elm);
  
  if (!gras_dynar_length(p_dynar)) {
    gras_dict_remove(ps->space, name);
                gras_dynar_free_container(p_dynar);
  }
  
  if (ddt) {
    *ddt = p_elm->type;
  }
  
  data    = p_elm->data;
  
  free(p_elm);
  
  gras_dynar_pop(ps->stack, &p_dynar);
  {
    int l = gras_dynar_length(p_dynar);
    
    while (l--) {
      char *_name = NULL;
                                                                                
      gras_dynar_get(p_dynar, l, &_name);
      if (!strcmp(name, _name)) {
	gras_dynar_remove_at(p_dynar, l, &_name);
	free(_name);
	break;
      }
    }
  }
  gras_dynar_push(ps->stack, &p_dynar);
  
  
  return data;
}

/**
 * gras_dd_cbps_set:
 *
 * Change the value of an element in the PS.  
 * If it's not present in the current block, look in the upper ones.
 * If it's not present in any of them, look in the globals
 *   (FIXME: which no function of this API allows to set). 
 * If not present there neither, the code may segfault (Oli?).
 *
 * Once a reference to an element of that name is found somewhere in the PS,
 *   its value is changed.
 */
void
gras_dd_cbps_set (gras_dd_cbps_t        *ps,
		  const char            *name,
		  void                  *data,
		  gras_datadesc_type_t  *ddt) {

  gras_dynar_t            *p_dynar        = NULL;
  gras_dd_cbps_elm_t      *p_elm          = NULL;
  
  gras_dict_get(ps->space, name, (void **)&p_dynar);
  
  if (!p_dynar) {
    gras_dynar_new(&p_dynar, sizeof (gras_dd_cbps_elm_t *), NULL);
    gras_dict_set(ps->space, name, (void **)p_dynar, NULL);
    
    p_elm   = calloc(1, sizeof(gras_dd_cbps_elm_t));
    gras_dynar_push(ps->globals, &name);
  } else {
    gras_dynar_pop(p_dynar, &p_elm);
  }
  
  p_elm->type   = ddt;
  p_elm->data   = data;
 
  gras_dynar_push(p_dynar, &p_elm);

}

/**
 * gras_dd_cbps_get:
 *
 * Get the value of an element in the PS without modifying it. 
 * (note that you get the content of the data struct and not a copy to it)
 * If it's not present in the current block, look in the upper ones.
 * If it's not present in any of them, look in the globals
 *   (FIXME: which no function of this API allows to set). 
 * If not present there neither, the code may segfault (Oli?).
 */
void *
gras_dd_cbps_get (gras_dd_cbps_t        *ps, 
		  const char            *name,
		  gras_datadesc_type_t **ddt) {
  
  gras_dynar_t            *p_dynar        = NULL;
  gras_dd_cbps_elm_t      *p_elm          = NULL;
  
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
 * gras_dd_cbps_block_begin:
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
gras_dd_cbps_block_begin(gras_dd_cbps_t *ps) {

  gras_dynar_t            *p_dynar        = NULL;
  
  gras_dynar_new(&p_dynar, sizeof (char *), NULL);
  gras_dynar_push(ps->stack, &p_dynar);
}

/**
 * gras_dd_cbps_block_begin:
 *
 * End the current block, and go back to the upper one.
 */
void
gras_dd_cbps_block_end(gras_dd_cbps_t *ps) {

  gras_dynar_t            *p_dynar        = NULL;
  int                      cursor         =    0;
  char                    *name           = NULL;
  
  gras_dynar_pop(ps->stack, &p_dynar);
  
  gras_dynar_foreach(p_dynar, cursor, name) {

    gras_dynar_t            *p_dynar_elm    = NULL;
    gras_dd_cbps_elm_t      *p_elm          = NULL;
 
    gras_dict_get(ps->space, name, (void **)&p_dynar_elm);
    gras_dynar_pop(p_dynar_elm, &p_elm);
 
    if (!gras_dynar_length(p_dynar_elm)) {
      gras_dict_remove(ps->space, name);
      gras_dynar_free_container(p_dynar_elm);
    }
    
    free(p_elm);
    free(name);
  }
  
  gras_dynar_free_container(p_dynar);
}



