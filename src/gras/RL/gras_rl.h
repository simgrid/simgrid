/* $Id$ */

/* gras_rl.h - private interface for GRAS when on real life                 */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_RL_H
#define GRAS_RL_H

#ifdef GRAS_SG_H
#error Impossible to load gras_sg.h and gras_rl.h at the same time
#endif

#include "gras_private.h"
//#include "diagnostic.h"

BEGIN_DECL

/****************************************************************************/
/****************************************************************************/
/* Openning/Maintaining/Closing connexions                                  */
/****************************************************************************/
/****************************************************************************/

/* Declaration of the socket type */
#define PEER_NAME_LEN 200
struct gras_sock_s {
  int sock;
  int port;         /* port on this side */
  char peer_name[PEER_NAME_LEN];  /* buffer to use PeerName_r */
  char *peer_addr;
};

struct gras_rawsock_s {
   int sock;
   int port;
};

/****************************************************************************/
/****************************************************************************/
/* Format convertion stuff (copied verbatim from formatutil.h because I need*/
/*            to put datadescriptor in gras.h and the functions only in RL) */
/****************************************************************************/
/****************************************************************************/

void ConvertData(void *destination,
		 const void *source,
		 const DataDescriptor *description,
		 size_t length,
		 FormatTypes sourceFormat);


int DifferentFormat(DataTypes whatType);
int DifferentOrder(void);
int DifferentSize(DataTypes whatType);
void HomogenousConvertData(void *destination,
			   const void *source,
			   DataTypes whatType,
			   size_t repetitions,
			   FormatTypes sourceFormat);
size_t HomogenousDataSize(DataTypes whatType,
			  size_t repetitions,
			  FormatTypes format);
void ReverseData(void *destination,
		 const void *source,
		 DataTypes whatType,
		 int repetitions,
		 FormatTypes format);


/****************************************************************************/
/****************************************************************************/
/* Messaging stuff (reimplementation of message.h)                          */
/****************************************************************************/
/****************************************************************************/
/**
 * gras_msg_discard:
 * @sd: Socket on which this message arrives
 * @size: Size of the message to discard.
 *
 * Discard data on the socket because of failure on our side (out of mem or msg
 * type unknown). 
 */
void
gras_msg_discard(gras_sock_t *sd, size_t size);

/**
 * grasRecvData:
 * @sd: Socket to listen on
 * @data: Where to store the data (allocated by this function)
 * @description: What data to expect.
 * @Returns: the number of bytes read for that.
 *
 * Receive data from the network on a buffer allocated by this function for that
 */
int
grasDataRecv( gras_sock_t *sd,
	      void **data,
	      const DataDescriptor *description,
	      size_t description_length,
	      unsigned int repetition);

/**
 * grasRecvData:
 * @sd: Socket to write on
 * @data: What to send
 * @description: Format of the data
 * @Returns: the number of bytes written
 *
 * Send a sequence of data across the network
 */
xbt_error_t
grasDataSend(gras_sock_t *sd,
	     const void *data,
	     const DataDescriptor *description,
	     size_t description_length,
	     int repetition);

END_DECL

#endif /* GRAS_SG_H */
