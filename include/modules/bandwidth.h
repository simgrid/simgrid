/* $Id$ */

/* gras_bandwidth - GRAS mecanism to do Bandwidth tests between to hosts    */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_BANDWIDTH_H
#define GRAS_BANDWIDTH_H

#include <gras/modules/base.h>

/* ****************************************************************************
 * The messages themselves
 * ****************************************************************************/

#ifndef GRAS_BANDWIDTH_FIRST_MESSAGE
#define GRAS_BANDWIDTH_FIRST_MESSAGE 0
#endif

#define GRASMSG_BW_REQUEST    GRAS_BANDWIDTH_FIRST_MESSAGE
#define GRASMSG_BW_RESULT     GRAS_BANDWIDTH_FIRST_MESSAGE+1
#define GRASMSG_BW_HANDSHAKE  GRAS_BANDWIDTH_FIRST_MESSAGE+2
#define GRASMSG_BW_HANDSHAKED GRAS_BANDWIDTH_FIRST_MESSAGE+3

#define GRASMSG_SAT_START     GRAS_BANDWIDTH_FIRST_MESSAGE+4
#define GRASMSG_SAT_STARTED   GRAS_BANDWIDTH_FIRST_MESSAGE+5
#define GRASMSG_SAT_STOP      GRAS_BANDWIDTH_FIRST_MESSAGE+6
#define GRASMSG_SAT_STOPPED   GRAS_BANDWIDTH_FIRST_MESSAGE+7

#define GRASMSG_SAT_BEGIN     GRAS_BANDWIDTH_FIRST_MESSAGE+8
#define GRASMSG_SAT_BEGUN     GRAS_BANDWIDTH_FIRST_MESSAGE+9
#define GRASMSG_SAT_END       GRAS_BANDWIDTH_FIRST_MESSAGE+10
#define GRASMSG_SAT_ENDED     GRAS_BANDWIDTH_FIRST_MESSAGE+11

/* ****************************************************************************
 * The functions to triger those messages
 * ****************************************************************************/

/**
 * grasbw_register_messages:
 *
 * Register all messages and callbacks needed for the current process to be ready
 * to do BW tests
 */
gras_error_t grasbw_register_messages(void);

/* ***************************************************************************
 * Bandwidth tests
 * ***************************************************************************/
/**
 * grasbw_test:
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
 * Conduct a bandwidth test from the local process to the given peer.
 * This call is blocking until the end of the experiment.
 */
gras_error_t grasbw_test(const char*to_name,unsigned int to_port,
			unsigned int bufSize,unsigned int expSize,unsigned int msgSize,
			/*OUT*/ double *sec, double*bw);

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
gras_error_t grasbw_request(const char* from_name,unsigned int from_port,
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
gras_error_t grasbw_saturate_start(const char* from_name,unsigned int from_port,
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
gras_error_t grasbw_saturate_stop(const char* from_name,unsigned int from_port,
				 const char* to_name,unsigned int to_port);



#endif /* GRAS_BANDWIDTH_H */
