/* $Id$ */

/* amok_bandwidth - Bandwidth test facilities                               */

/* Copyright (c) 2003-2005 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef AMOK_BANDWIDTH_H
#define AMOK_BANDWIDTH_H

#include "amok/base.h"

/* module handling */

   
/** \addtogroup AMOK_bw
 *  \brief Test the bandwidth between two nodes
 *
 *  This module allows you to retrieve the bandwidth between to arbitrary hosts, 
 *  provided that they run some GRAS process which initialized this module. 
 * 
 *  Retrieving the bandwidth is usually done by active measurment: one send
 *  a packet of known size, time how long it needs to go back and forth,
 *  and you get the bandwidth in Kb/s available on the wire.
 * 
 *  This is not as easy as it first seems to do so in GRAS. The first issue
 *  is that GRAS messages can get buffered, or the receiver cannot be
 *  waiting for the message when it arrives. This results in extra delays
 *  impacting the measurement quality. You thus have to setup a rendez-vous
 *  protocol. The second issue is that GRAS message do have an header, so
 *  figuring out their size is not trivial. Moreover, they get converted
 *  when the sender and receiver processor architecture are different,
 *  inducing extra delays. For this, GRAS provide the so-called measurement
 *  sockets. On them, you can send raw data which is not converted (see
 *  \ref GRAS_sock_meas). 
 *
 *  Solving all these problems is quite error prone and anoying, so we
 *  implemented this in the current module so that you don't have to do it
 *  yourself. The API is very simple. Use amok_bw_test() to get the BW
 *  between the local host and the specified peer, or amok_bw_request() to
 *  get the BW between two remote hosts. The elapsed time, as long as the
 *  achieved bandwidth is returned in the last arguments of the functions.
 * 
 *  All sizes are in kilo bytes. The \a buf_size is the size of the buffer
 *   (this is a socket parameter set automatically). The \a exp_size is the
 *   amount of data to send during an experiment. \a msg_size is the size
 *   of each message sent. These values allow you to study phenomenon such
 *   as TCP slow start (which are not correctly modelized by \ref SURF_API,
 *   yet). They are mimicked from the NWS API, and default values could be
 *   buf_size=32k, msg_size=16k and exp_size=64k. That means that the
 *   socket will be prepared to accept 32k in its buffer and then four
 *   messages of 16k will be sent (so that the total amount of data equals
 *   64k). Of course, you can use other values if you want to.
 * 
 *  \todo Cleanup and implement the link saturation stuff.
 *
 *  @{
 */

void amok_bw_init(void);
void amok_bw_exit(void);

void amok_bw_test(gras_socket_t peer,
		  unsigned long int buf_size,unsigned long int exp_size,unsigned long int msg_size,
	  /*OUT*/ double *sec, double *bw);

void amok_bw_request(const char* from_name,unsigned int from_port,
		     const char* to_name,unsigned int to_port,
		     unsigned long int buf_size,unsigned long int exp_size,unsigned long int msg_size,
	     /*OUT*/ double *sec, double*bw);

/** @} */
#if 0   

/* ***************************************************************************
 * Link saturation
 * ***************************************************************************/

/**
 * grasbw_saturate_start:
 * @from_name: Name of the host we are asking to do a experiment with (to_name:to_port)
 * @from_port: port on which the process we are asking for an experiment is listening
 * (for message, do not give a raw socket here. The needed raw socket will be negociated 
 * between the peers)
 * @to_name: Name of the host with which we should conduct the experiment
 * @to_port: port on which the peer process is listening for message
 * @msgSize: Size of each message sent.
 * @timeout: How long in maximum should be the saturation.
 *
 * Ask the process 'from_name:from_port' to start to saturate the link between itself
 * and to_name:to_name.
 */
void grasbw_saturate_start(const char* from_name,unsigned int from_port,
				  const char* to_name,unsigned int to_port,
				  unsigned int msgSize, unsigned int timeout);

/**
 * grasbw_saturate_stop:
 * @from_name: Name of the host we are asking to do a experiment with (to_name:to_port)
 * @from_port: port on which the process we are asking for an experiment is listening
 * (for message, do not give a raw socket here. The needed raw socket will be negociated 
 * between the peers)
 * @to_name: Name of the host with which we should conduct the experiment
 * @to_port: port on which the peer process is listening for message
 *
 * Ask the process 'from_name:from_port' to stop saturating the link between itself
 * and to_name:to_name.
 */
void grasbw_saturate_stop(const char* from_name,unsigned int from_port,
				 const char* to_name,unsigned int to_port);


#endif /* if 0 */
#endif /* AMOK_BANDWIDTH_H */
