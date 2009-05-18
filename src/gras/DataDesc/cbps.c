/* $Id$ */

/* cbps - persistant states for callbacks                                   */

/* Copyright (c) 2003 Olivier Aumage.                                       */
/* Copyright (c) 2003, 2004 Martin Quinson.                                 */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/DataDesc/datadesc_private.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_ddt_cbps,gras_ddt,"callback persistant state");

typedef struct {
	gras_datadesc_type_t  type;
	void                 *data;
} s_gras_cbps_elm_t, *gras_cbps_elm_t;

typedef struct s_gras_cbps {
	xbt_dynar_t lints; /* simple stack of long integers (easy interface) */

	xbt_dict_t  space; /* varname x dynar of gras_cbps_elm_t */
	xbt_dynar_t frames; /* of dynar of names defined within this frame
			   (and to pop when we leave it) */
	xbt_dynar_t globals;
} s_gras_cbps_t;

gras_cbps_t gras_cbps_new(void) {
	gras_cbps_t  res;

	res=xbt_new(s_gras_cbps_t,1);

	res->lints = xbt_dynar_new(sizeof(int), NULL);
	res->space = xbt_dict_new();
	/* no leak, the content is freed manually on block_end */
	res->frames = xbt_dynar_new(sizeof(xbt_dynar_t), NULL);
	res->globals = xbt_dynar_new(sizeof(char*), NULL);

	gras_cbps_block_begin(res);

	return res;
}

void gras_cbps_free(gras_cbps_t *state) {

	xbt_dynar_free( &( (*state)->lints   ) );

	gras_cbps_block_end(*state);
	xbt_dict_free ( &( (*state)->space   ) );
	xbt_dynar_free( &( (*state)->frames  ) );
	xbt_dynar_free( &( (*state)->globals ) );

	free(*state);
	*state = NULL;
}

void gras_cbps_reset(gras_cbps_t state) {

	xbt_dynar_reset(state->lints);

	xbt_dict_reset (state->space);

	xbt_dynar_reset(state->frames);
	xbt_dynar_reset(state->globals);
}

/** \brief Declare a new element in the PS, and give it a value.
 *
 * If an element of that
 * name already exists, it is masked by the one given here, and will be
 * seeable again only after a pop to remove the value this push adds.
 */
void
gras_cbps_v_push(gras_cbps_t          ps,
		const char          *name,
		void                *data,
		gras_datadesc_type_t ddt) {

	xbt_dynar_t     varstack=NULL,frame;
	gras_cbps_elm_t var;
	char           *varname = (char*)xbt_strdup(name);
	xbt_ex_t        e;

	DEBUG2("push(%s,%p)",name,(void*)data);

	TRY {
		varstack = xbt_dict_get(ps->space, name);
	} CATCH(e) {
		if (e.category != mismatch_error)
			RETHROW;

		DEBUG1("Create a new variable stack for '%s' into the space",name);
		varstack = xbt_dynar_new(sizeof (gras_cbps_elm_t *), NULL);
		xbt_dict_set(ps->space, varname, (void **)varstack, NULL);
		xbt_ex_free(e);
		/* leaking, you think? only if you do not close all the openned blocks ;)*/
	}

	var       = xbt_new0(s_gras_cbps_elm_t,1);
	var->type = ddt;
	var->data = data;

	xbt_dynar_push(varstack, &var);

	xbt_dynar_pop(ps->frames, &frame);
	DEBUG4("Push %s (%p @%p) into frame %p",varname,(void*)varname,(void*)&varname,(void*)frame);
	xbt_dynar_push(frame, &varname);
	xbt_dynar_push(ps->frames, &frame);
}

/** \brief Retrieve an element from the PS, and remove it from the PS.
 *
 * If it's not present in the current block, it will fail (throwing not_found)
 * and not search in upper blocks since this denotes a programmation error.
 */
void
gras_cbps_v_pop (gras_cbps_t            ps,
		const char            *name,
		gras_datadesc_type_t  *ddt,
		void                 **res) {
	xbt_dynar_t          varstack,frame;
	gras_cbps_elm_t       var            = NULL;
	void                 *data           = NULL;
	xbt_ex_t e;

	DEBUG1("pop(%s)",name);
	TRY {
		varstack = xbt_dict_get(ps->space, name);
	} CATCH(e) {
		if (e.category != mismatch_error)
			RETHROW;

		xbt_ex_free(e);
		THROW1(not_found_error,1,"Asked to pop the non-existant %s", name);
	}
	xbt_dynar_pop(varstack, &var);

	if (!xbt_dynar_length(varstack)) {
		DEBUG1("Last incarnation of %s poped. Kill it",name);
		xbt_dict_remove(ps->space, name);
		xbt_dynar_free(&varstack);
	}

	if (ddt)
		*ddt = var->type;
	data = var->data;

	free(var);

	xbt_dynar_pop(ps->frames, &frame);
	{
		int l = xbt_dynar_length(frame);

		while (l--) {
			char *_name = NULL;

			_name = xbt_dynar_get_as(frame, l, char*);
			if (!strcmp(name, _name)) {
				xbt_dynar_remove_at(frame, l, &_name);
				free(_name);
				break;
			}
		}
	}
	xbt_dynar_push(ps->frames, &frame);

	*res = data;
}

/** \brief Change the value of an element in the PS.
 *
 * If it's not present in the current block, look in the upper ones.
 * If it's not present in any of them, modify in the globals
 * If not present there neither, the code may segfault (Oli?).
 *
 * Once a reference to an element of that name is found somewhere in the PS,
 *   its value is changed.
 */
void
gras_cbps_v_set (gras_cbps_t          ps,
		const char          *name,
		void                *data,
		gras_datadesc_type_t ddt) {

	xbt_dynar_t dynar = NULL;
	gras_cbps_elm_t elm = NULL;

	DEBUG1("set(%s)",name);
	dynar = xbt_dict_get_or_null(ps->space, name);

	if (dynar == NULL) {
		dynar = xbt_dynar_new(sizeof (gras_cbps_elm_t), NULL);
		xbt_dict_set(ps->space, name, (void **)dynar, NULL);

		elm   = xbt_new0(s_gras_cbps_elm_t,1);
		xbt_dynar_push(ps->globals, &name);
	} else {
		xbt_dynar_pop(dynar, &elm);
	}

	elm->type   = ddt;
	elm->data   = data;

	xbt_dynar_push(dynar, &elm);

}

/** \brief Get the value of an element in the PS without modifying it.
 *
 * (note that you get the content of the data struct and not a copy to it)
 * If it's not present in the current block, look in the upper ones.
 * If it's not present in any of them, look in the globals
 * If not present there neither, the code may segfault (Oli?).
 */
void *
gras_cbps_v_get (gras_cbps_t           ps,
		const char           *name,
		/* OUT */gras_datadesc_type_t *ddt) {

	xbt_dynar_t    dynar = NULL;
	gras_cbps_elm_t elm   = NULL;

	DEBUG1("get(%s)",name);
	dynar = xbt_dict_get(ps->space, name);
	xbt_dynar_pop(dynar, &elm);
	xbt_dynar_push(dynar, &elm);

	if (ddt) {
		*ddt = elm->type;
	}

	return elm->data;

}

/** \brief Begins a new block.
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
gras_cbps_block_begin(gras_cbps_t ps) {

	xbt_dynar_t dynar = NULL;

	DEBUG0(">>> Block begin");
	dynar = xbt_dynar_new(sizeof (char *), NULL);
	xbt_dynar_push(ps->frames, &dynar);
}

/** \brief End the current block, and go back to the upper one. */
void
gras_cbps_block_end(gras_cbps_t ps) {

	xbt_dynar_t  frame        = NULL;
	unsigned int cursor       =    0;
	char         *name         = NULL;

	xbt_assert0(xbt_dynar_length(ps->frames),
			"More block_end than block_begin");
	xbt_dynar_pop(ps->frames, &frame);

	xbt_dynar_foreach(frame, cursor, name) {

		xbt_dynar_t    varstack    = NULL;
		gras_cbps_elm_t var         = NULL;

		DEBUG2("Get ride of %s (%p)",name,(void*)name);
		varstack = xbt_dict_get(ps->space, name);
		xbt_dynar_pop(varstack, &var);

		if (!xbt_dynar_length(varstack)) {
			xbt_dict_remove(ps->space, name);
			xbt_dynar_free_container(&varstack); /*already empty, save a test ;) */
		}

		if (var->data) free(var->data);
		free(var);
		free(name);
	}
	xbt_dynar_free_container(&frame);/* we just emptied it */
	DEBUG0("<<< Block end");
}


/** \brief Push a new integer value into the cbps. */
void
gras_cbps_i_push(gras_cbps_t ps,
		int val) {
	DEBUG1("push %d as a size",val);
	xbt_dynar_push_as(ps->lints,int,val);
}
/** \brief Pop the lastly pushed integer value from the cbps. */
int
gras_cbps_i_pop(gras_cbps_t ps) {
	int ret;

	xbt_assert0(xbt_dynar_length(ps->lints) > 0,
			"gras_cbps_i_pop: no value to pop");
	ret = xbt_dynar_pop_as(ps->lints,int);
	DEBUG1("pop %d as a size",ret);
	return ret;
}

/** \brief Generic cb returning the lastly pushed value
 *
 * Used by \ref gras_datadesc_ref_pop_arr
 */
int gras_datadesc_cb_pop(gras_datadesc_type_t ignored, gras_cbps_t vars, void *data) {
	int res =  gras_cbps_i_pop(vars);
	DEBUG1("Pop %d as a size",res);
	return res;
}

/* ************************* */
/* **** PUSHy callbacks **** */
/* ************************* */

/** \brief Cb to push an integer. Must be attached to the field you want to push */
void gras_datadesc_cb_push_int(gras_datadesc_type_t ignored, gras_cbps_t vars, void *data) {
	int *i = (int*)data;
	gras_cbps_i_push(vars, (int) *i);
}

/** \brief Cb to push an unsigned integer. Must be attached to the field you want to push */
void gras_datadesc_cb_push_uint(gras_datadesc_type_t ignored, gras_cbps_t vars, void *data) {
	unsigned int *i = (unsigned int*)data;
	gras_cbps_i_push(vars, (int) *i);
}

/** \brief Cb to push an long integer. Must be attached to the field you want to push
 */
void gras_datadesc_cb_push_lint(gras_datadesc_type_t ignored, gras_cbps_t vars, void *data) {
	long int *i = (long int*)data;
	gras_cbps_i_push(vars, (int) *i);
}
/** \brief Cb to push an unsigned long integer. Must be attached to the field you want to push
 */
void gras_datadesc_cb_push_ulint(gras_datadesc_type_t ignored, gras_cbps_t vars, void *data) {
	unsigned long int *i = (unsigned long int*)data;
	gras_cbps_i_push(vars, (int) *i);
}

/* ************************************ */
/* **** PUSHy multiplier callbacks **** */
/* ************************************ */
/** \brief Cb to push an integer as multiplier. Must be attached to the field you want to push */
void gras_datadesc_cb_push_int_mult(gras_datadesc_type_t ignored, gras_cbps_t vars, void *data) {
	int old = *(int*)data;
	int new = gras_cbps_i_pop(vars);
	DEBUG2("push %d x %d as a size",old,new);
	gras_cbps_i_push(vars, old*new);
}

/** \brief Cb to push an unsigned integer as multiplier. Must be attached to the field you want to push */
void gras_datadesc_cb_push_uint_mult(gras_datadesc_type_t ignored, gras_cbps_t vars, void *data) {
	unsigned int old = *(unsigned int*)data;
	unsigned int new = gras_cbps_i_pop(vars);

	DEBUG2("push %d x %d as a size",old,new);
	gras_cbps_i_push(vars, (int) (old*new));
}

/** \brief Cb to push an long integer as multiplier. Must be attached to the field you want to push
 */
void gras_datadesc_cb_push_lint_mult(gras_datadesc_type_t ignored, gras_cbps_t vars, void *data) {
	long int i = *(long int*)data;
	i *= gras_cbps_i_pop(vars);
	gras_cbps_i_push(vars, (int) i);
}
/** \brief Cb to push an unsigned long integer as multiplier. Must be attached to the field you want to push
 */
void gras_datadesc_cb_push_ulint_mult(gras_datadesc_type_t ignored, gras_cbps_t vars, void *data) {
	unsigned long int old = *(unsigned long int*)data;
	unsigned long int new = gras_cbps_i_pop(vars);

	DEBUG2("push %ld x %ld as a size",old,new);
	gras_cbps_i_push(vars, (int) (old * new) );
}
