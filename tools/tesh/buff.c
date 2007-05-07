/* $Id$ */

/* buff -- buffers as needed by tesh                                        */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* specific to Borland Compiler */
#ifdef __BORLANDDC__
#pragma hdrstop
#endif

#include "buff.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

/**
 ** Buffer code
 **/

void buff_empty(buff_t b) {
  b->used=0;
  b->data[0]='\n';
  b->data[1]='\0';
}
buff_t buff_new(void) {
  buff_t res=malloc(sizeof(s_buff_t));
  res->data=malloc(512);
  res->size=512;
  buff_empty(res);
  return res;
}
void buff_free(buff_t b) {
  if (b) {
    if (b->data)
      free(b->data);
    free(b);
  }
}
void buff_append(buff_t b, const char *toadd) {
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
void buff_chomp(buff_t b) {
  while (b->data[b->used] == '\n') {
    b->data[b->used] = '\0';
    if (b->used)
      b->used--;
  }
}

void buff_trim(buff_t b) {
  xbt_str_trim(b->data," ");
  b->used = strlen(b->data);
}
