/* $Id$ */

/* set - data container consisting in dict+dynar                            */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

#include "xbt/set.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(set,xbt,
	     "set: data container consisting in dict+dynar");

/*####[ Type definition ]####################################################*/
typedef struct xbt_set_ {
  xbt_dict_t  dict;  /* data stored by name */
  xbt_dynar_t dynar; /* data stored by ID   */
} s_xbt_set_t;

/*####[ Memory  ]############################################################*/
/** @brief Constructor */
xbt_set_t xbt_set_new (void) {
  xbt_set_t res=xbt_new(s_xbt_set_t,1);

  res->dict=xbt_dict_new ();
  res->dynar=xbt_dynar_new(sizeof(void*),NULL);

  return res;
}

/** @brief Destructor */
void  xbt_set_free(xbt_set_t *set) {
  if (*set) {
    xbt_dict_free ( &( (*set)->dict  ) );
    xbt_dynar_free( &( (*set)->dynar ) );
    free(*set);
    *set = NULL;
  }
}

/** @brief Add an element to a set. 
 *
 * \param set set to populate
 * \param elm element to add. 
 * \param free_func How to add the data 
 *
 * elm->name must be set;
 * if elm->name_len <= 0, it is recomputed. If >0, it's used as is;
 * elm->ID is attributed automatically.
 */
void xbt_set_add    (xbt_set_t      set,
		      xbt_set_elm_t  elm,
		      void_f_pvoid_t *free_func) {

  xbt_error_t   errcode;
  xbt_set_elm_t found_in_dict;

  if (elm->name_len <= 0) {
    elm->name_len = strlen(elm->name);
  }

  errcode = xbt_dict_get_ext (set->dict, 
				    elm->name, elm->name_len,
				    (void**)&found_in_dict);
  if (errcode == no_error) {
    if (elm == found_in_dict) {
      DEBUG2("Ignoring request to insert the same element twice (key %s ; id %d)",
	     elm->name, elm->ID);
      return;
    } else {
      elm->ID=found_in_dict->ID;
      DEBUG2("Reinsertion of key %s (id %d)", elm->name, elm->ID);
      xbt_dict_set_ext(set->dict, elm->name, elm->name_len, elm, free_func);
      xbt_dynar_set(set->dynar, elm->ID, &elm);
      return;
    }
  } else {
    xbt_assert_error(mismatch_error);
  }

  elm->ID = xbt_dynar_length( set->dynar );
  xbt_dict_set_ext(set->dict, elm->name, elm->name_len, elm, free_func);
  xbt_dynar_set(set->dynar, elm->ID, &elm);
  DEBUG2("Insertion of key '%s' (id %d)", elm->name, elm->ID);

}

/** @brief Retrive data by providing its name.
 * 
 * \param set
 * \param name Name of the searched cell
 * \param dst where to put the found data into
 */
xbt_error_t xbt_set_get_by_name    (xbt_set_t     set,
				      const char     *name,
				      /* OUT */xbt_set_elm_t *dst) {
  xbt_error_t errcode;
  errcode = xbt_dict_get_ext(set->dict, name, strlen(name), (void**) dst);
  DEBUG2("Lookup key %s: %s",name,xbt_error_name(errcode));
  return errcode;
}

/** @brief Retrive data by providing its name and the length of the name
 *
 * \param set
 * \param name Name of the searched cell
 * \param name_len length of the name, when strlen cannot be trusted
 * \param dst where to put the found data into
 *
 * This is useful when strlen cannot be trusted because you don't use a char*
 * as name, you weirdo.
 */
xbt_error_t xbt_set_get_by_name_ext(xbt_set_t      set,
				      const char     *name,
				      int             name_len,
				      /* OUT */xbt_set_elm_t *dst) {

  return xbt_dict_get_ext (set->dict, name, name_len, (void**)dst);
}

/** @brief Retrive data by providing its ID
 *
 * \param set
 * \param id what you're looking for
 * \param dst where to put the found data into
 *
 * @warning, if the ID does not exists, you're getting into trouble
 */
xbt_error_t xbt_set_get_by_id      (xbt_set_t      set,
				      int             id,
				      /* OUT */xbt_set_elm_t *dst) {

  /* Don't bother checking the bounds, the dynar does so */

  *dst = xbt_dynar_get_as(set->dynar,id,xbt_set_elm_t);
  DEBUG3("Lookup type of id %d (of %lu): %s", 
	 id, xbt_dynar_length(set->dynar), (*dst)->name);
  
  return no_error;
}

/***
 *** Cursors
 ***/
typedef struct xbt_set_cursor_ {
  xbt_set_t set;
  int val;
} s_xbt_set_cursor_t;

/** @brief Create the cursor if it does not exists, rewind it in any case. */
void         xbt_set_cursor_first       (xbt_set_t         set,
					  xbt_set_cursor_t *cursor) {

  if (set != NULL) {
    if (!*cursor) {
      DEBUG0("Create the cursor on first use");
      *cursor = xbt_new(s_xbt_set_cursor_t,1);
      xbt_assert0(*cursor,
		   "Malloc error during the creation of the cursor");
    }
    (*cursor)->set = set;
    xbt_dynar_cursor_first(set->dynar, &( (*cursor)->val) );
  } else {
    *cursor = NULL;
  }
}

/** @brief Move to the next element.  */
void         xbt_set_cursor_step        (xbt_set_cursor_t cursor) {
  xbt_dynar_cursor_step(cursor->set->dynar, &( cursor->val ) );
}

/** @brief Get current data
 * 
 * \return true if it's ok, false if there is no more data
 */
int          xbt_set_cursor_get_or_free (xbt_set_cursor_t *curs,
					  xbt_set_elm_t    *elm) {
  xbt_set_cursor_t cursor;

  if (!curs || !(*curs))
    return FALSE;

  cursor=*curs;

  if (! xbt_dynar_cursor_get( cursor->set->dynar,&(cursor->val),elm) ) {
    free(cursor);
    *curs=NULL;
    return FALSE;    
  } 
  return TRUE;
}
