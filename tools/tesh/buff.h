/* $Id$ */

/* buff -- buffers as needed by tesh                                        */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef TESH_BUFF_H
#define TESH_BUFF_H

#include "portable.h"
#include "xbt/sysdep.h"
#include "xbt/function_types.h"
#include "xbt/log.h"
#include "xbt/str.h"

/**
 ** Buffer code
 **/
typedef struct {
  char *data;
  int used,size;
} buff_t;


void buff_empty(buff_t *b);
buff_t *buff_new(void);
void buff_free(buff_t *b);
void buff_append(buff_t *b, char *toadd);
void buff_chomp(buff_t *b);
void buff_trim(buff_t* b);

#endif
