/* $Id$ */

/* set - data container consisting in dict+dynar                            */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the GRAS posse.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(set,GRAS);

/*####[ Type definition ]####################################################*/
struct gras_set_ {
  gras_dict_t  *dict;  /* data stored by name */
  gras_dynar_t *dynar; /* data stored by ID   */
};

/*####[ Memory  ]############################################################*/
/**
 * gras_set_new:
 * @dst: where to
 *
 * Creates a new set.
 */
gras_error_t gras_set_new (gras_set_t **dst) {
  gras_set_t *res=(gras_set_t*)malloc(sizeof(gras_set_t));
  gras_error_t errcode;

  if (!res)
    RAISE_MALLOC;

  TRY(gras_dict_new (&(res->dict)));
  TRY(gras_dynar_new(&(res->dynar), sizeof(void*),NULL));

  *dst=res;
  return no_error;
}

/**
 * gras_set_free:
 * @set:
 *
 * Frees a set.
 */
void         gras_set_free(gras_set_t **set) {
  if (*set) {
    gras_dict_free ( &( (*set)->dict  ) );
    gras_dynar_free(    (*set)->dynar  );
    free(*set);
    *set=NULL;
  }
}

/**
 * gras_set_add:
 * @set: set to populate
 * @elm: element to add. 
 * @free_ctn: How to add the data 
 *
 * Add an element to a set. 
 *
 * elm->name must be set;
 * elm->name_len is used as is unless it's <= 0 (in which case it's recomputed);
 * elm->ID is attributed automatically.
 */
gras_error_t gras_set_add    (gras_set_t     *set,
			      gras_set_elm_t *elm,
			      void_f_pvoid_t *free_func) {

  gras_error_t    errcode;
  gras_set_elm_t *found_in_dict;

  if (elm->name_len <= 0) {
    elm->name_len = strlen(elm->name);
  }

  errcode = gras_dict_retrieve_ext (set->dict, 
				    elm->name, elm->name_len,
				    (void**) &found_in_dict);
  if (errcode == no_error) {
    elm->ID=found_in_dict->ID;
    DEBUG2("Reinsertion of key %s (id %d)", elm->name, elm->ID);
    TRY(gras_dict_insert_ext(set->dict, elm->name, elm->name_len, elm, free_func));
    TRY(gras_dynar_set(set->dynar, elm->ID, &elm));
    return no_error;

  } else if (errcode != mismatch_error) {
    return errcode; /* I expected mismatch_error */
  }

  elm->ID = gras_dynar_length( set->dynar );
  TRY(gras_dict_insert_ext(set->dict, elm->name, elm->name_len, elm, free_func));
  TRY(gras_dynar_set(set->dynar, elm->ID, &elm));
  DEBUG2("Insertion of key '%s' (id %d)", elm->name, elm->ID);

  return no_error;
}

/**
 * gras_set_get_by_name:
 * @set:
 * @name: Name of the searched cell
 * @dst: where to put the found data into
 *
 * Retrieve a data stored in the cell by providing its name.
 */
gras_error_t gras_set_get_by_name    (gras_set_t     *set,
				      const char     *name,
				      /* OUT */gras_set_elm_t **dst) {

  return gras_dict_retrieve_ext(set->dict, name, strlen(name), (void**) dst);
}
/**
 * gras_set_get_by_name_ext:
 * @set:
 * @name: Name of the searched cell
 * @name_len: length of the name, when strlen cannot be trusted
 * @dst: where to put the found data into
 *
 * Retrieve a data stored in the cell by providing its name (and the length
 * of the name, when strlen cannot be trusted because you don't use a char*
 * as name, you weird guy).
 */
gras_error_t gras_set_get_by_name_ext(gras_set_t     *set,
				      const char     *name,
				      int             name_len,
				      /* OUT */gras_set_elm_t **dst) {

  return gras_dict_retrieve_ext (set->dict, name, name_len, (void**)dst);
}

/**
 * gras_set_get_by_code:
 * @set:
 * @name: Name of the searched cell
 * @name_len: length of the name, when strlen cannot be trusted
 * @dst: where to put the found data into
 *
 * Retrieve a data stored in the cell by providing its name (and the length
 * of the name, when strlen cannot be trusted because you don't use a char*
 * as name, you weird guy).
 */
gras_error_t gras_set_get_by_id      (gras_set_t     *set,
				      int             id,
				      /* OUT */gras_set_elm_t **dst) {
  DEBUG2("Lookup type of id %d (of %d)", 
	 id, gras_dynar_length(set->dynar));
  if (id < gras_dynar_length(set->dynar) &&
      id >= 0) {
    gras_dynar_get(set->dynar,id,dst);
    DEBUG3("Lookup type of id %d (of %d): %s", 
	   id, gras_dynar_length(set->dynar), (*dst)->name);
  
  } else {
    DEBUG1("Cannot get ID %d: out of bound", id);
    return mismatch_error;
  }
  return no_error;
}

/***
 *** Cursors
 ***/
struct gras_set_cursor_ {
  gras_set_t *set;
  int val;
};

/**
 * gras_set_cursor_first:
 * @set: on what to let the cursor iterate
 * @cursor: dest address
 *
 * Create the cursor if it does not exists. Rewind it in any case.
 */
void         gras_set_cursor_first       (gras_set_t   *set,
					  gras_set_cursor_t **cursor) {

  if (set != NULL) {
    if (!*cursor) {
      DEBUG0("Create the cursor on first use");
      *cursor = (gras_set_cursor_t*)malloc(sizeof(gras_set_cursor_t));
      gras_assert0(*cursor,
		   "Malloc error during the creation of the cursor");
    }
    (*cursor)->set = set;
    gras_dynar_cursor_first(set->dynar, &( (*cursor)->val) );
  } else {
    *cursor = NULL;
  }
}

/**
 * gras_set_cursor_step:
 * @cursor: the cursor
 *
 * Move to the next element. 
 */
void         gras_set_cursor_step        (gras_set_cursor_t  *cursor) {
  gras_dynar_cursor_step(cursor->set->dynar, &( cursor->val ) );
}

/**
 * gras_set_cursor_get_or_free:
 * @cursor: the cursor
 * @Returns: true if it's ok, false if there is no more data
 *
 * Get current data
 */
int          gras_set_cursor_get_or_free (gras_set_cursor_t **curs,
					  gras_set_elm_t    **elm) {
  gras_set_cursor_t *cursor;

  if (!curs || !(*curs))
    return FALSE;

  cursor=*curs;

  if (! gras_dynar_cursor_get( cursor->set->dynar,&(cursor->val),elm) ) {
    free(cursor);
    *curs=NULL;
    return FALSE;    
  } 
  return TRUE;
}
