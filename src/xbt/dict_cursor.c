/* $Id$ */

/* dict_cursor - iterators over dictionnaries                               */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "dict_private.h"

#include <string.h> /* strlen() */

XBT_LOG_EXTERNAL_CATEGORY(dict);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dict_cursor,dict,"To traverse dictionaries");


/*####[ Dict cursor functions ]#############################################*/
/* To traverse (simple) dicts                                               */
/* Don't add or remove entries to the dict while traversing !!!             */
/*###########################################################################*/
struct xbt_dict_cursor_ {
  /* we store all level encountered until here, to backtrack on next() */
  xbt_dynar_t   keys;
  xbt_dynar_t   key_lens;
  int            pos;
  int            pos_len;
  xbt_dictelm_t head;
};

static _XBT_INLINE void
_cursor_push_keys(xbt_dict_cursor_t p_cursor,
                  xbt_dictelm_t     p_elm);

#undef xbt_dict_CURSOR_DEBUG
/*#define xbt_dict_CURSOR_DEBUG 1*/

/**
 * xbt_dict_cursor_new:
 *
 * @head: the head of the dict
 * @cursor: the curent position in the dict
 *
 * Structure creator
 */
xbt_dict_cursor_t 
xbt_dict_cursor_new(const xbt_dict_t head) {
  xbt_dict_cursor_t res = NULL;

  res = xbt_new(s_xbt_dict_cursor_t,1);
  res->keys     = xbt_dynar_new(sizeof(char **), NULL);
  res->key_lens = xbt_dynar_new(sizeof(int  *),  NULL);
  res->pos      = 0;
  res->pos_len  = 0;
  res->head     = head ? head->head : NULL;

  xbt_dict_cursor_rewind(res);

  return res;
}

/**
 * xbt_dict_cursor_free:
 *
 * @cursor: poor victim
 *
 * Structure destructor
 */
void
xbt_dict_cursor_free(xbt_dict_cursor_t *cursor) {
  if (*cursor) {
    xbt_dynar_free(&((*cursor)->keys));
    xbt_dynar_free(&((*cursor)->key_lens));
    xbt_free(*cursor);
    *cursor = NULL;
  }
}

/**
 * __cursor_not_null:
 *
 * Sanity check to see if the head contains something
 */
static _XBT_INLINE
xbt_error_t
__cursor_not_null(xbt_dict_cursor_t cursor) {

  xbt_assert0(cursor, "Null cursor");

  if (!cursor->head) {
    return mismatch_error;
  }

  return no_error;
}


static _XBT_INLINE
void
_cursor_push_keys(xbt_dict_cursor_t cursor,
                  xbt_dictelm_t     elm) {
  xbt_dictelm_t       child = NULL;
  int                  i       = 0;
  static volatile int  count   = 0; /* ??? */

  CDEBUG1(dict_cursor, "Push childs of %p in the cursor", (void*)elm);

  if (elm->content) {
    xbt_dynar_push(cursor->keys,     &elm->key    );
    xbt_dynar_push(cursor->key_lens, &elm->key_len);
    count++;
  }

  xbt_dynar_foreach(elm->sub, i, child) {
    if (child)
      _cursor_push_keys(cursor, child);
  }

  CDEBUG1(dict_cursor, "Count = %d", count);
}

/**
 * xbt_dict_cursor_rewind:
 * @cursor: the cursor
 * @Returns: xbt_error_t
 *
 * back to the first element
 */
void
xbt_dict_cursor_rewind(xbt_dict_cursor_t cursor) {

  CDEBUG0(dict_cursor, "xbt_dict_cursor_rewind");
  xbt_assert(cursor);

  xbt_dynar_reset(cursor->keys);
  xbt_dynar_reset(cursor->key_lens);

  if (!cursor->head)
    return ;

  _cursor_push_keys(cursor, cursor->head);

  xbt_dynar_cursor_first(cursor->keys,     &cursor->pos    );
  xbt_dynar_cursor_first(cursor->key_lens, &cursor->pos_len);

}

/**
 * xbt_dict_cursor_first:
 * @dict: on what to let the cursor iterate
 * @cursor: dest address
 *
 * Create the cursor if it does not exists. Rewind it in any case.
 */
void xbt_dict_cursor_first (const xbt_dict_t   dict,
			     xbt_dict_cursor_t *cursor){

  if (!*cursor) {
    DEBUG0("Create the cursor on first use");
    *cursor=xbt_dict_cursor_new(dict);
  }

  xbt_dict_cursor_rewind(*cursor);
}


/**
 * xbt_dict_cursor_step:
 * @cursor: the cursor
 *
 * Move to the next element. 
 */
void
xbt_dict_cursor_step(xbt_dict_cursor_t cursor) {
  xbt_assert(cursor);

  xbt_dynar_cursor_step(cursor->keys,     &cursor->pos);
  xbt_dynar_cursor_step(cursor->key_lens, &cursor->pos_len);
}

/**
 * xbt_dict_cursor_get_or_free:
 * @cursor: the cursor
 * @Returns: true if it's ok, false if there is no more data
 *
 * Get current data
 */
int
xbt_dict_cursor_get_or_free(xbt_dict_cursor_t  *cursor,
			     char               **key,
			     void               **data) {
  xbt_error_t  errcode = no_error;
  int           key_len = 0;
  
  if (!cursor || !(*cursor))
    return FALSE;

  if (xbt_dynar_length((*cursor)->keys) <= (*cursor)->pos) {
    xbt_dict_cursor_free(cursor);
    return FALSE;
  }
    
  *key    = xbt_dynar_get_as((*cursor)->keys,     (*cursor)->pos,     char*);
  key_len = xbt_dynar_get_as((*cursor)->key_lens, (*cursor)->pos_len, int);

  errcode = xbt_dictelm_get_ext((*cursor)->head, *key, key_len, data);
  if (errcode == mismatch_error) {
    xbt_dict_cursor_free(cursor);
    return FALSE;
  }

  xbt_assert1(errcode == no_error,
	       "Unexpected problem while retrieving the content of cursor. Got %s",
	       xbt_error_name(errcode));

  return TRUE;
}

/**
 * xbt_dict_cursor_get_key:
 * @cursor: the cursor
 * @key: the current element
 * @Returns: xbt_error_t
 *
 * Get current key
 */
xbt_error_t
xbt_dict_cursor_get_key(xbt_dict_cursor_t   cursor,
                         /*OUT*/char        **key) {
  xbt_error_t errcode = no_error;

  TRY(__cursor_not_null(cursor));

  *key = xbt_dynar_get_as(cursor->keys, cursor->pos - 1, char*);

  return errcode;
}

/**
 * xbt_dict_cursor_get_data:
 * @cursor: the cursor
 *
 * Get current data
 */
xbt_error_t
xbt_dict_cursor_get_data(xbt_dict_cursor_t   cursor,
                          /*OUT*/void        **data) {
  xbt_error_t  errcode = no_error;
  char         *key     = NULL;
  int           key_len = 0;

  TRY(__cursor_not_null(cursor));

  key     = xbt_dynar_get_as(cursor->keys,     cursor->pos-1,  char *);
  key_len = xbt_dynar_get_as(cursor->key_lens, cursor->pos_len-1, int);

  TRY(xbt_dictelm_get_ext(cursor->head, key, key_len, data));

  return errcode;
}


