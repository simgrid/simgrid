/* $Id$ */

/* dict_cursor - iterators over dictionnaries                               */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 Martin Quinson.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "dict_private.h"

#include <string.h> /* strlen() */

GRAS_LOG_EXTERNAL_CATEGORY(dict);
GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(dict_cursor,dict,"To traverse dictionaries");


/*####[ Dict cursor functions ]#############################################*/
/* To traverse (simple) dicts                                               */
/* Don't add or remove entries to the dict while traversing !!!             */
/*###########################################################################*/
struct gras_dict_cursor_ {
  /* we store all level encountered until here, to backtrack on next() */
  gras_dynar_t   keys;
  gras_dynar_t   key_lens;
  int            pos;
  int            pos_len;
  gras_dictelm_t head;
};

static _GRAS_INLINE void
_cursor_push_keys(gras_dict_cursor_t p_cursor,
                  gras_dictelm_t     p_elm);

#undef gras_dict_CURSOR_DEBUG
/*#define gras_dict_CURSOR_DEBUG 1*/

/**
 * gras_dict_cursor_new:
 *
 * @head: the head of the dict
 * @cursor: the curent position in the dict
 *
 * Structure creator
 */
gras_dict_cursor_t 
gras_dict_cursor_new(const gras_dict_t head) {
  gras_error_t        errcode  = no_error;
  gras_dict_cursor_t res = NULL;

  res = gras_new(s_gras_dict_cursor_t,1);
  res->keys     = gras_dynar_new(sizeof(char **), NULL);
  res->key_lens = gras_dynar_new(sizeof(int  *),  NULL);
  res->pos      = 0;
  res->pos_len  = 0;
  res->head     = head ? head->head : NULL;

  gras_dict_cursor_rewind(res);

  return res;
}

/**
 * gras_dict_cursor_free:
 *
 * @cursor: poor victim
 *
 * Structure destructor
 */
void
gras_dict_cursor_free(gras_dict_cursor_t *cursor) {
  if (*cursor) {
    gras_dynar_free(&((*cursor)->keys));
    gras_dynar_free(&((*cursor)->key_lens));
    gras_free(*cursor);
    *cursor = NULL;
  }
}

/**
 * __cursor_not_null:
 *
 * Sanity check to see if the head contains something
 */
static _GRAS_INLINE
gras_error_t
__cursor_not_null(gras_dict_cursor_t cursor) {

  gras_assert0(cursor, "Null cursor");

  if (!cursor->head) {
    return mismatch_error;
  }

  return no_error;
}


static _GRAS_INLINE
void
_cursor_push_keys(gras_dict_cursor_t cursor,
                  gras_dictelm_t     elm) {
  gras_error_t         errcode = no_error;
  gras_dictelm_t       child = NULL;
  int                  i       = 0;
  static volatile int  count   = 0; /* ??? */

  CDEBUG1(dict_cursor, "Push childs of %p in the cursor", (void*)elm);

  if (elm->content) {
    gras_dynar_push(cursor->keys,     &elm->key    );
    gras_dynar_push(cursor->key_lens, &elm->key_len);
    count++;
  }

  gras_dynar_foreach(elm->sub, i, child) {
    if (child)
      _cursor_push_keys(cursor, child);
  }

  CDEBUG1(dict_cursor, "Count = %d", count);
}

/**
 * gras_dict_cursor_rewind:
 * @cursor: the cursor
 * @Returns: gras_error_t
 *
 * back to the first element
 */
void
gras_dict_cursor_rewind(gras_dict_cursor_t cursor) {
  gras_error_t errcode = no_error;

  CDEBUG0(dict_cursor, "gras_dict_cursor_rewind");
  gras_assert(cursor);

  gras_dynar_reset(cursor->keys);
  gras_dynar_reset(cursor->key_lens);

  if (!cursor->head)
    return ;

  _cursor_push_keys(cursor, cursor->head);

  gras_dynar_cursor_first(cursor->keys,     &cursor->pos    );
  gras_dynar_cursor_first(cursor->key_lens, &cursor->pos_len);

}

/**
 * gras_dict_cursor_first:
 * @dict: on what to let the cursor iterate
 * @cursor: dest address
 *
 * Create the cursor if it does not exists. Rewind it in any case.
 */
void gras_dict_cursor_first (const gras_dict_t   dict,
			     gras_dict_cursor_t *cursor){

  if (!*cursor) {
    DEBUG0("Create the cursor on first use");
    *cursor=gras_dict_cursor_new(dict);
  }

  gras_dict_cursor_rewind(*cursor);
}


/**
 * gras_dict_cursor_step:
 * @cursor: the cursor
 *
 * Move to the next element. 
 */
void
gras_dict_cursor_step(gras_dict_cursor_t cursor) {
  gras_assert(cursor);

  gras_dynar_cursor_step(cursor->keys,     &cursor->pos);
  gras_dynar_cursor_step(cursor->key_lens, &cursor->pos_len);
}

/**
 * gras_dict_cursor_get_or_free:
 * @cursor: the cursor
 * @Returns: true if it's ok, false if there is no more data
 *
 * Get current data
 */
int
gras_dict_cursor_get_or_free(gras_dict_cursor_t  *cursor,
			     char               **key,
			     void               **data) {
  gras_error_t  errcode = no_error;
  int           key_len = 0;
  
  if (!cursor || !(*cursor))
    return FALSE;

  if (gras_dynar_length((*cursor)->keys) <= (*cursor)->pos) {
    gras_dict_cursor_free(cursor);
    return FALSE;
  }
    
  *key    = gras_dynar_get_as((*cursor)->keys,     (*cursor)->pos,     char*);
  key_len = gras_dynar_get_as((*cursor)->key_lens, (*cursor)->pos_len, int);

  errcode = gras_dictelm_get_ext((*cursor)->head, *key, key_len, data);
  if (errcode == mismatch_error) {
    gras_dict_cursor_free(cursor);
    return FALSE;
  }

  gras_assert1(errcode == no_error,
	       "Unexpected problem while retrieving the content of cursor. Got %s",
	       gras_error_name(errcode));

  return TRUE;
}

/**
 * gras_dict_cursor_get_key:
 * @cursor: the cursor
 * @key: the current element
 * @Returns: gras_error_t
 *
 * Get current key
 */
gras_error_t
gras_dict_cursor_get_key(gras_dict_cursor_t   cursor,
                         /*OUT*/char        **key) {
  gras_error_t errcode = no_error;

  TRY(__cursor_not_null(cursor));

  *key = gras_dynar_get_as(cursor->keys, cursor->pos - 1, char*);

  return errcode;
}

/**
 * gras_dict_cursor_get_data:
 * @cursor: the cursor
 *
 * Get current data
 */
gras_error_t
gras_dict_cursor_get_data(gras_dict_cursor_t   cursor,
                          /*OUT*/void        **data) {
  gras_error_t  errcode = no_error;
  char         *key     = NULL;
  int           key_len = 0;

  TRY(__cursor_not_null(cursor));

  key     = gras_dynar_get_as(cursor->keys,     cursor->pos-1,  char *);
  key_len = gras_dynar_get_as(cursor->key_lens, cursor->pos_len-1, int);

  TRY(gras_dictelm_get_ext(cursor->head, key, key_len, data));

  return errcode;
}


