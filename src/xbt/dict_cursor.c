/* $Id$ */

/* dict_cursor - iterators over dictionnaries                               */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/ex.h"
#include "dict_private.h"

#include <string.h>             /* strlen() */

XBT_LOG_EXTERNAL_CATEGORY(xbt_dict);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dict_cursor, xbt_dict,
                                "To traverse dictionaries");


/*####[ Dict cursor functions ]#############################################*/
/* To traverse (simple) dicts                                               */
/* Don't add or remove entries to the dict while traversing !!!             */
/*###########################################################################*/
struct xbt_dict_cursor_ {
  xbt_dictelm_t current;
  int line;
  xbt_dict_t dict;
};

#undef xbt_dict_CURSOR_DEBUG
/*#define xbt_dict_CURSOR_DEBUG 1*/

/** @brief Creator
 *  @param dict the dict
 */
xbt_dict_cursor_t xbt_dict_cursor_new(const xbt_dict_t dict)
{
  xbt_dict_cursor_t res = NULL;

  res = xbt_new(s_xbt_dict_cursor_t, 1);
  res->dict = dict;

  xbt_dict_cursor_rewind(res);

  return res;
}

/**
 * @brief Destructor
 * @param cursor poor victim
 */
void xbt_dict_cursor_free(xbt_dict_cursor_t * cursor)
{
  if (*cursor) {
    xbt_free(*cursor);
    *cursor = NULL;
  }
}

/*
 * Sanity check to see if the head contains something
 */
static XBT_INLINE void __cursor_not_null(xbt_dict_cursor_t cursor)
{
  xbt_assert0(cursor, "Null cursor");
}


/** @brief Reinitialize the cursor. Mandatory after removal or add in dict. */
void xbt_dict_cursor_rewind(xbt_dict_cursor_t cursor)
{
  CDEBUG0(xbt_dict_cursor, "xbt_dict_cursor_rewind");
  xbt_assert(cursor);

  cursor->line = 0;
  if (cursor->dict != NULL) {
    cursor->current = cursor->dict->table[0];
  } else {
    cursor->current = NULL;
  }
}

/**
 * @brief Create the cursor if it does not exists. Rewind it in any case.
 *
 * @param      dict   on what to let the cursor iterate
 * @param[out] cursor dest address
 */
void xbt_dict_cursor_first(const xbt_dict_t dict, xbt_dict_cursor_t * cursor)
{
  DEBUG0("xbt_dict_cursor_first");
  if (!*cursor) {
    DEBUG0("Create the cursor on first use");
    *cursor = xbt_dict_cursor_new(dict);
  } else {
    xbt_dict_cursor_rewind(*cursor);
  }
  if (dict != NULL && (*cursor)->current == NULL) {
    xbt_dict_cursor_step(*cursor);      /* find the first element */
  }
}


/**
 * \brief Move to the next element.
 */
void xbt_dict_cursor_step(xbt_dict_cursor_t cursor)
{


  xbt_dictelm_t current;
  int line;

  DEBUG0("xbt_dict_cursor_step");
  xbt_assert(cursor);

  current = cursor->current;
  line = cursor->line;

  if (cursor->dict != NULL) {

    if (current != NULL) {
      DEBUG0("current is not null, take the next element");
      current = current->next;
      DEBUG1("next element: %p", current);
    }

    while (current == NULL && ++line <= cursor->dict->table_size) {
      DEBUG0("current is NULL, take the next line");
      current = cursor->dict->table[line];
      DEBUG1("element in the next line: %p", current);
    }
    DEBUG2("search finished, current = %p, line = %d", current, line);

    cursor->current = current;
    cursor->line = line;
  }
}

/**
 * @brief Get current data, or free the cursor if there is no data left
 *
 * @returns true if it's ok, false if there is no more data
 */
int xbt_dict_cursor_get_or_free(xbt_dict_cursor_t * cursor,
                                char **key, void **data)
{

  xbt_dictelm_t current;

  DEBUG0("xbt_dict_get_or_free");


  if (!cursor || !(*cursor))
    return FALSE;

  current = (*cursor)->current;
  if (current == NULL) {        /* no data left */
    xbt_dict_cursor_free(cursor);
    return FALSE;
  }

  *key = current->key;
  *data = current->content;
  return TRUE;
}

/**
 * @brief Get current key
 * @param cursor: the cursor
 * @returns the current key
 */
char *xbt_dict_cursor_get_key(xbt_dict_cursor_t cursor)
{
  __cursor_not_null(cursor);

  return cursor->current->key;
}

/**
 * @brief Get current data
 * @param cursor the cursor
 * @returns the current data
 */
void *xbt_dict_cursor_get_data(xbt_dict_cursor_t cursor)
{
  __cursor_not_null(cursor);

  return cursor->current->content;
}
