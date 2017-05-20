/* strbuff -- string buffers                                                */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/strbuff.h"
#include <stdarg.h>

#define minimal_increment 512

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(strbuff, xbt, "String buffers");

/** @brief Remove any content from the buffer */
inline void xbt_strbuff_clear(xbt_strbuff_t b)
{
  b->used = 0;
  b->data[0] = '\0';
}

/** @brief Constructor */
xbt_strbuff_t xbt_strbuff_new(void)
{
  xbt_strbuff_t res = xbt_malloc(sizeof(s_xbt_strbuff_t));
  res->data = xbt_malloc(512);
  res->size = 512;
  xbt_strbuff_clear(res);
  return res;
}

/** @brief creates a new string buffer containing the provided string
 *
 * Beware, the ctn is copied, you want to free it afterward, anyhow
 */
inline xbt_strbuff_t xbt_strbuff_new_from(const char *ctn)
{
  xbt_strbuff_t res = xbt_malloc(sizeof(s_xbt_strbuff_t));
  res->data = xbt_strdup(ctn);
  res->size = strlen(ctn);
  res->used = res->size;
  return res;
}

/** @brief frees only the container without touching to the contained string */
inline void xbt_strbuff_free_container(xbt_strbuff_t b)
{
  free(b);
}

/** @brief frees the buffer and its content */
inline void xbt_strbuff_free(xbt_strbuff_t b)
{
  if (b) {
    free(b->data);
    free(b);
  }
}

/** @brief Adds some content at the end of the buffer */
void xbt_strbuff_append(xbt_strbuff_t b, const char *toadd)
{
  int addlen;
  int needed_space;

  xbt_assert(b, "Asked to append stuff to NULL buffer");

  addlen = strlen(toadd);
  needed_space = b->used + addlen + 1;

  if (needed_space > b->size) {
    b->size = MAX(minimal_increment + b->used, needed_space);
    b->data = xbt_realloc(b->data, b->size);
  }
  strncpy(b->data + b->used, toadd, b->size-b->used);
  b->used += addlen;
}

/** @brief format some content and push it at the end of the buffer */
void xbt_strbuff_printf(xbt_strbuff_t b, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *data = bvprintf(fmt, ap);
  xbt_strbuff_append(b, data);
  xbt_free(data);
  va_end(ap);
}
