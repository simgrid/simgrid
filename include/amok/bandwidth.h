/* $Id$ */

/* amok_bandwidth - Bandwidth test facilities                               */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef AMOK_BANDWIDTH_H
#define AMOK_BANDWIDTH_H

#include "amok/base.h"

/* module handling */

void amok_bw_init(void);
void amok_bw_exit(void);
   


/* ***************************************************************************
 * Bandwidth tests
 * ***************************************************************************/
/**
 * amok_bw_test:
 * @peer: A (regular) socket at which the the host with which we should conduct the experiment can be contacted
 * @buf_size: Size of the socket buffer
 * @exp_size: Total size of data sent across the network
 * @msg_size: Size of each message sent. Ie, (@expSize % @msgSize) messages will be sent.
 * @sec: where the result (in seconds) should be stored.
 * @bw: observed Bandwidth (in Mb/s) 
 *
 * Conduct a bandwidth test from the local process to the given peer.
 * This call is blocking until the end of the experiment.
 */
xbt_error_t amok_bw_test(gras_socket_t peer,
			  unsigned int buf_size,unsigned int exp_size,unsigned int msg_size,
			  /*OUT*/ double *sec, double *bw);

#if 0   
/**
 * grasbw_request:
 * @from_name: Name of the host we are asking to do a experiment with (to_name:to_port)
 * @from_port: port on which the process we are asking for an experiment is listening for message
 * @to_name: Name of the host with which we should conduct the experiment
 * @to_port: port on which the peer process is listening (for message, do not 
 * give a raw socket here. The needed raw socket will be negociated between 
 * the peers)
 * @bufSize: Size of the socket buffer
 * @expSize: Total size of data sent across the network
 * @msgSize: Size of each message sent. Ie, (@expSize % @msgSize) messages will be sent.
 * @sec: where the result (in seconds) should be stored.
 * @bw: observed Bandwidth (in Mb/s)
 *
 * Conduct a bandwidth test from the process from_host:from_port to to_host:to_port.
 * This call is blocking until the end of the experiment.
 */
xbt_error_t grasbw_request(const char* from_name,unsigned int from_port,
			   const char* to_name,unsigned int to_port,
			   unsigned int bufSize,unsigned int expSize,unsigned int msgSize,
			   /*OUT*/ double *sec, double*bw);


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
xbt_error_t grasbw_saturate_start(const char* from_name,unsigned int from_port,
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
xbt_error_t grasbw_saturate_stop(const char* from_name,unsigned int from_port,
				 const char* to_name,unsigned int to_port);


#endif /* if 0 */
#endif /* AMOK_BANDWIDTH_H */
