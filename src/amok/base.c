/* $Id$ */

/* gras_addons - several addons to do specific stuff not in GRAS itself     */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>

#include <gras.h>

void
grasRepportError (gras_sock_t *sock, gras_msgid_t id, int SeqCount,
		  const char *severeError,
		  gras_error_t errcode, const char* format,...) {
  msgError_t *error;
  va_list ap;

  if (!(error=(msgError_t*)malloc(sizeof(msgError_t)))) {
    fprintf(stderr,severeError);
    return;
  }

  error->errcode=errcode;
  va_start(ap,format);
  vsprintf(error->errmsg,format,ap);
  va_end(ap);

  /* FIXME: If message id have more than 17 sequences, the following won't work */
  if (gras_msg_new_and_send(sock,id,SeqCount,   error,1,NULL,0,NULL,0,NULL,0,NULL,0,NULL,0,NULL,0,
		     NULL,0,NULL,0,NULL,0,NULL,0,NULL,0,NULL,0,NULL,0,NULL,0,NULL,0,NULL,0)) {
    fprintf(stderr,severeError);
  }
}
