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
 *  The API is very simple. Use amok_bw_test() to get the BW between the local host
 *  and the specified peer, or amok_bw_request() to get the BW between two remote 
 *  hosts. The elapsed time, as long as the achieved bandwidth is returned in the 
 *  last arguments of the functions.
 *
 *  All sizes are in kilo bytes.
 *
 *  \todo Cleanup and implement the link saturation stuff.
 *
 *  @{
 */

void amok_bw_init(void);
void amok_bw_exit(void);

xbt_error_t amok_bw_test(gras_socket_t peer,
			  unsigned long int buf_size,unsigned long int exp_size,unsigned long int msg_size,
			  /*OUT*/ double *sec, double *bw);

xbt_error_t amok_bw_request(const char* from_name,unsigned int from_port,
			   const char* to_name,unsigned int to_port,
			   unsigned long int bufSize,unsigned long int expSize,unsigned long int msgSize,
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
