/* $Id$ */

/* dict_cursor - iterators over dictionnaries                               */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 Martin Quinson.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"
#include "dict_private.h"

#include <stdlib.h> /* malloc() */
#include <string.h> /* strlen() */

GRAS_LOG_EXTERNAL_CATEGORY(dict);
GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(dict_cursor,dict,"To traverse dictionaries");


/*####[ Dict cursor functions ]#############################################*/
/* To traverse (simple) dicts                                               */
/* Don't add or remove entries to the dict while traversing !!!             */
/*###########################################################################*/
struct gras_dict_cursor_ {
  /* we store all level encountered until here, to backtrack on next() */
  gras_dynar_t *keys;
  gras_dynar_t *key_lens;
  int           pos;
  int           pos_len;
  gras_dictelm_t  *head;
};

static inline
gras_error_t
_cursor_push_keys(gras_dict_cursor_t *p_cursor,
                  gras_dictelm_t        *p_elm);

#undef gras_dict_CURSOR_DEBUG
//#define gras_dict_CURSOR_DEBUG 1

/**
 * gras_dict_cursor_new:
 *
 * @head: the head of the dict
 * @cursor: the curent position in the dict
 *
 * Structure creator
 */
gras_error_t
gras_dict_cursor_new(const gras_dict_t          *p_head,
                     /*OUT*/gras_dict_cursor_t **pp_cursor) {
  gras_error_t        errcode  = no_error;
  gras_dict_cursor_t *p_cursor = NULL;

  p_cursor = malloc(sizeof(gras_dict_cursor_t));
  if (!p_cursor)
    RAISE_MALLOC;

  TRY(gras_dynar_new(&p_cursor->keys,     sizeof(char **), NULL));
  TRY(gras_dynar_new(&p_cursor->key_lens, sizeof(int  *),  NULL));

  p_cursor->pos     = 0;
  p_cursor->pos_len = 0;
  p_cursor->head    = p_head ? p_head->head : NULL;

  TRY(gras_dict_cursor_rewind(p_cursor));

  *pp_cursor = p_cursor;

  return errcode;
}

/**
 * gras_dict_cursor_free:
 *
 * @cursor: poor victim
 *
 * Structure destructor
 */
void
gras_dict_cursor_free(gras_dict_cursor_t *p_cursor) {
  if (p_cursor) {
    gras_dynar_free(p_cursor->keys);
    gras_dynar_free(p_cursor->key_lens);
    memset(p_cursor, 0, sizeof(gras_dict_cursor_t));
    free(p_cursor);
  }
}

/**
 * __cursor_not_null:
 *
 * Sanity check to see if the head contains something
 */
static inline
gras_error_t
__cursor_not_null(gras_dict_cursor_t *p_cursor) {

  gras_assert0(p_cursor, "Null cursor");

  if (!p_cursor->head) {
    return mismatch_error;
  }

  return no_error;
}


static inline
gras_error_t
_cursor_push_keys(gras_dict_cursor_t *p_cursor,
                  gras_dictelm_t        *p_elm) {
  gras_error_t         errcode = no_error;
  gras_dictelm_t      *p_child = NULL;
  int                  i       = 0;
  static volatile int  count   = 0; /* ??? */

  CDEBUG1(dict_cursor, "Push childs of %p in the cursor", p_elm);

  if (p_elm->content) {
    TRY(gras_dynar_push(p_cursor->keys,     &p_elm->key    ));
    TRY(gras_dynar_push(p_cursor->key_lens, &p_elm->key_len));
    count++;
  }

  gras_dynar_foreach(p_elm->sub, i, p_child) {
    if (p_child)
      TRY(_cursor_push_keys(p_cursor, p_child));
  }

  CDEBUG1(dict_cursor, "Count = %d", count);

  return errcode;
}

/**
 * gras_dict_cursor_rewind:
 * @cursor: the cursor
 * @Returns: gras_error_t
 *
 * back to the first element
 */
gras_error_t
gras_dict_cursor_rewind(gras_dict_cursor_t *p_cursor) {
  gras_error_t errcode = no_error;

  CDEBUG0(dict_cursor, "gras_dict_cursor_rewind");
  gras_assert(p_cursor);

  gras_dynar_reset(p_cursor->keys);
  gras_dynar_reset(p_cursor->key_lens);

  if (!p_cursor->head)
    return no_error;

  TRY(_cursor_push_keys(p_cursor, p_cursor->head));

  gras_dynar_cursor_first(p_cursor->keys,     &p_cursor->pos    );
  gras_dynar_cursor_first(p_cursor->key_lens, &p_cursor->pos_len);

  return errcode;
}

/**
 * gras_dict_cursor_first:
 * @dict: on what to let the cursor iterate
 * @cursor: dest address
 *
 * Create the cursor if it does not exists. Rewind it in any case.
 */
void         gras_dict_cursor_first    (const gras_dict_t   *dict,
					gras_dict_cursor_t **cursor){
  gras_error_t errcode;

  if (!*cursor) {
    DEBUG0("Create the cursor on first use");
    errcode = gras_dict_cursor_new(dict,cursor);
    gras_assert1(errcode == no_error, "Unable to create the cursor, got error %s",
		 gras_error_name(errcode));
  }

  errcode = gras_dict_cursor_rewind(*cursor);
  gras_assert1(errcode == no_error, "Unable to rewind the cursor before use, got error %s",
	       gras_error_name(errcode));
}


/**
 * gras_dict_cursor_step:
 * @cursor: the cursor
 *
 * Move to the next element. 
 */
void
gras_dict_cursor_step(gras_dict_cursor_t *p_cursor) {
  gras_assert(p_cursor);

  gras_dynar_cursor_step(p_cursor->keys,     &p_cursor->pos);
  gras_dynar_cursor_step(p_cursor->key_lens, &p_cursor->pos_len);
}

/**
 * gras_dict_cursor_get_or_free:
 * @cursor: the cursor
 * @Returns: true if it's ok, false if there is no more data
 *
 * Get current data
 */
int
gras_dict_cursor_get_or_free(gras_dict_cursor_t **cursor,
			     char               **key,
			     void               **data) {
  gras_error_t  errcode = no_error;
  int           key_len = 0;
  
  if (!cursor || !(*cursor))
    return FALSE;

  if (gras_dynar_length((*cursor)->keys) <= (*cursor)->pos) {
    gras_dict_cursor_free(*cursor);
    *cursor=NULL;
    return FALSE;
  }
    
  gras_dynar_get((*cursor)->keys,     (*cursor)->pos,     key    );
  gras_dynar_get((*cursor)->key_lens, (*cursor)->pos_len, &key_len);

  errcode = gras_dictelm_get_ext((*cursor)->head, *key, key_len, data);
  if (errcode == mismatch_error) {
    gras_dict_cursor_free(*cursor);
    *cursor=NULL;
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
gras_dict_cursor_get_key(gras_dict_cursor_t  *p_cursor,
                         /*OUT*/char        **key) {
  gras_error_t errcode = no_error;

  TRY(__cursor_not_null(p_cursor));

  gras_dynar_get(p_cursor->keys, p_cursor->pos - 1, key);

  return errcode;
}

/**
 * gras_dict_cursor_get_data:
 * @cursor: the cursor
 *
 * Get current data
 */
gras_error_t
gras_dict_cursor_get_data(gras_dict_cursor_t  *p_cursor,
                          /*OUT*/void        **data) {
  gras_error_t  errcode = no_error;
  char         *key     = NULL;
  int           key_len = 0;

  TRY(__cursor_not_null(p_cursor));

  gras_dynar_get(p_cursor->keys,     p_cursor->pos-1,     &key    );
  gras_dynar_get(p_cursor->key_lens, p_cursor->pos_len-1, &key_len);

  TRY(gras_dictelm_get_ext(p_cursor->head, key, key_len, data));

  return errcode;
}


