/* $Id: buff.c 3483 2007-05-07 11:18:56Z mquinson $ */

/* strbuff -- string buffers                                                */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* specific to Borland Compiler */
#ifdef __BORLANDDC__
#pragma hdrstop
#endif

#include "xbt/strbuff.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(strbuff,xbt,"String buffers");

/**
 ** Buffer code
 **/

void xbt_strbuff_empty(xbt_strbuff_t b) {
  b->used=0;
  b->data[0]='\n';
  b->data[1]='\0';
}
xbt_strbuff_t xbt_strbuff_new(void) {
  xbt_strbuff_t res=malloc(sizeof(s_xbt_strbuff_t));
  res->data=malloc(512);
  res->size=512;
  xbt_strbuff_empty(res);
  return res;
}
void xbt_strbuff_free(xbt_strbuff_t b) {
  if (b) {
    if (b->data)
      free(b->data);
    free(b);
  }
}
void xbt_strbuff_append(xbt_strbuff_t b, const char *toadd) {
  int addlen;
  int needed_space;

  if (!b)
    THROW0(arg_error,0,"Asked to append stuff to NULL buffer");

  addlen = strlen(toadd);
  needed_space=b->used+addlen+1;

  if (needed_space > b->size) {
    b->data = realloc(b->data, needed_space);
    b->size = needed_space;
  }
  strcpy(b->data+b->used, toadd);
  b->used += addlen;  
}
void xbt_strbuff_chomp(xbt_strbuff_t b) {
  while (b->data[b->used] == '\n') {
    b->data[b->used] = '\0';
    if (b->used)
      b->used--;
  }
}

void xbt_strbuff_trim(xbt_strbuff_t b) {
  xbt_str_trim(b->data," ");
  b->used = strlen(b->data);
}
