/* $Id$ */

/* gras_addons - several addons to do specific stuff not in GRAS itself     */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_ADDONS_H
#define GRAS_ADDONS_H

#include <gras.h>

#define HOSTNAME_LEN 256
#define ERRMSG_LEN  50

/* ****************************************************************************
 * The common types used as payload in the messages and their definitions
 * ****************************************************************************/

/**
 * msgHost_t:
 * 
 * Description of an host
 */

typedef struct {
  char host[HOSTNAME_LEN];
  unsigned int port;
} msgHost_t;

static const DataDescriptor msgHostDesc[] = 
 { SIMPLE_MEMBER(CHAR_TYPE,HOSTNAME_LEN,offsetof(msgHost_t,host)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE,1,   offsetof(msgHost_t,port))};
#define msgHostLen 2

/**
 * msgError_t:
 *
 * how to indicate an eventual error
 */

typedef struct {
  char errmsg[ERRMSG_LEN];
  unsigned int errcode;
} msgError_t;

static const DataDescriptor msgErrorDesc[] = 
 { SIMPLE_MEMBER(CHAR_TYPE,         ERRMSG_LEN,offsetof(msgError_t,errmsg)),
   SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1,         offsetof(msgError_t,errcode))};
#define msgErrorLen 2

/**
 * msgResult_t:
 *
 * how to report the result of an experiment
 */

typedef struct {
  unsigned int timestamp;
  double value;
} msgResult_t;

static const DataDescriptor msgResultDesc[] = 
 { SIMPLE_MEMBER(UNSIGNED_INT_TYPE, 1, offsetof(msgResult_t,timestamp)),
   SIMPLE_MEMBER(DOUBLE_TYPE,       1, offsetof(msgResult_t,value))};
#define msgResultLen 2

/**
 * grasRepportError:
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
grasRepportError (gras_socket_t *sock, int id, int SeqCount,
		  const char *severeError,
		  xbt_error_t errcode, const char* format,...);

#endif /* GRAS_ADDONS_H */
