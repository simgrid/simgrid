/* $Id$ */

/* amok_base - things needed in amok, but too small to constitute a module  */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003, 2004 the OURAGAN project.                            */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef AMOK_BASE_H
#define AMOK_BASE_H

#include "gras/messages.h"

/* ****************************************************************************
 * The common types used as payload in the messages and their definitions
 * ****************************************************************************/

/**
 * amok_remoterr_t:
 *
 * how to indicate an eventual error
 */

typedef struct {
  char *msg;
  unsigned int code;
} s_amok_remoterr_t,*amok_remoterr_t;

amok_remoterr_t amok_remoterr_new(gras_error_t errcode, 
				  const char* format, ...);
amok_remoterr_t amok_remoterr_new_va(gras_error_t param_errcode, 
				     const char* format,va_list ap);
void amok_remoterr_free(amok_remoterr_t *err);


/**
 * amok_result_t:
 *
 * how to report the result of an experiment
 */

typedef struct {
  unsigned int timestamp;
  double value;
} amok_result_t;

/**
 * amok_repport_error:
 *
 * Repports an error to the process listening on socket sock. 
 *
 * The information will be embeeded in a message of type id, which must take a msgError_t as first
 * sequence (and SeqCount sequences in total). Other sequences beside the error one will be of
 * length 0.
 *
 * The message will be builded as sprintf would, using the given format and extra args.
 *
 * If the message cannot be builded and sent to recipient, the string severeError will be printed
 * on localhost's stderr.
 */
void
amok_repport_error (gras_socket_t sock, gras_msgtype_t msgtype,
		    gras_error_t errcode, const char* format,...);


void amok_base_init(void);
void amok_base_exit(void);


#endif /* AMOK_BASE_H */
