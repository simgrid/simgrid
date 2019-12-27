/* dict_cursor - iterators over dictionaries                               */

/* Copyright (c) 2004-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/ex.h"
#include "dict_private.h"

#include <string.h>             /* strlen() */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_dict_cursor, xbt_dict, "To traverse dictionaries");

/*####[ Dict cursor functions ]#############################################*/
/* To traverse (simple) dicts                                               */
/* Don't add or remove entries to the dict while traversing !!!             */
/*###########################################################################*/

/** @brief Creator
 *  @param dict the dict
 */
inline xbt_dict_cursor_t xbt_dict_cursor_new(const xbt_dict_t dict)
{
  xbt_dict_cursor_t res = xbt_new(struct s_xbt_dict_cursor, 1);
  res->dict = dict;

  xbt_dict_cursor_rewind(res);

  return res;
}

/**
 * @brief Destructor
 * @param cursor poor victim
 */
inline void xbt_dict_cursor_free(xbt_dict_cursor_t * cursor)
{
  xbt_free(*cursor);
  *cursor = NULL;
}

/*
 * Sanity check to see if the head contains something
 */
static inline void __cursor_not_null(const_xbt_dict_cursor_t cursor)
{
  xbt_assert(cursor, "Null cursor");
}

/** @brief Reinitialize the cursor. Mandatory after removal or add in dict. */
inline void xbt_dict_cursor_rewind(xbt_dict_cursor_t cursor)
{
  XBT_CDEBUG(xbt_dict_cursor, "xbt_dict_cursor_rewind");
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
inline void xbt_dict_cursor_first(const xbt_dict_t dict, xbt_dict_cursor_t * cursor)
{
  XBT_CDEBUG(xbt_dict_cursor, "xbt_dict_cursor_first");
  if (!*cursor) {
    XBT_CDEBUG(xbt_dict_cursor, "Create the cursor on first use");
    *cursor = xbt_dict_cursor_new(dict);
  } else {
    xbt_dict_cursor_rewind(*cursor);
  }
  if (dict != NULL && (*cursor)->current == NULL) {
    xbt_dict_cursor_step(*cursor);      /* find the first element */
  }
}

/** @brief Move to the next element. */
inline void xbt_dict_cursor_step(xbt_dict_cursor_t cursor)
{
  xbt_dictelm_t current;
  int line;

  XBT_CDEBUG(xbt_dict_cursor, "xbt_dict_cursor_step");
  xbt_assert(cursor);

  current = cursor->current;
  line = cursor->line;

  if (cursor->dict != NULL) {
    if (current != NULL) {
      XBT_CDEBUG(xbt_dict_cursor, "current is not null, take the next element");
      current = current->next;
      XBT_CDEBUG(xbt_dict_cursor, "next element: %p", current);
    }

    while (current == NULL && (line + 1) <= cursor->dict->table_size) {
      line++;
      XBT_CDEBUG(xbt_dict_cursor, "current is NULL, take the next line");
      current = cursor->dict->table[line];
      XBT_CDEBUG(xbt_dict_cursor, "element in the next line: %p", current);
    }
    XBT_CDEBUG(xbt_dict_cursor, "search finished, current = %p, line = %d", current, line);

    cursor->current = current;
    cursor->line = line;
  }
}

/**
 * @brief Get current data, or free the cursor if there is no data left
 *
 * @returns true if it's ok, false if there is no more data
 */
inline int xbt_dict_cursor_get_or_free(xbt_dict_cursor_t * cursor, char **key, void **data)
{
  const struct s_xbt_dictelm* current;

  XBT_CDEBUG(xbt_dict_cursor, "xbt_dict_get_or_free");

  if (!cursor || !(*cursor))
    return 0;

  current = (*cursor)->current;
  if (current == NULL) {        /* no data left */
    xbt_dict_cursor_free(cursor);
    return 0;
  }

  *key = current->key;
  *data = current->content;
  return 1;
}

/**
 * @brief Get current key
 * @param cursor: the cursor
 * @returns the current key
 */
inline char *xbt_dict_cursor_get_key(xbt_dict_cursor_t cursor)
{
  __cursor_not_null(cursor);

  return cursor->current->key;
}

/**
 * @brief Get current data
 * @param cursor the cursor
 * @returns the current data
 */
inline void *xbt_dict_cursor_get_data(xbt_dict_cursor_t cursor)
{
  __cursor_not_null(cursor);

  return cursor->current->content;
}
