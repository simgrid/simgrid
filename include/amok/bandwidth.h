/* amok_bandwidth - Bandwidth test facilities                               */

/* Copyright (c) 2003-2005 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef AMOK_BANDWIDTH_H
#define AMOK_BANDWIDTH_H

/** \addtogroup AMOK_bw
 *  \brief Test the bandwidth between two nodes
 *
 *  This module allows you to retrieve the bandwidth between to arbitrary hosts
 *  and saturating the links leading to them, provided that they run some GRAS 
 *  process which initialized this module.
 * 
 * \htmlonly <h3>Bandwidth measurement</h3>\endhtmlonly
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
 *  All sizes are in bytes. The \a buf_size is the size of the buffer
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
 *  \htmlonly
 * <center><img align=center src="amok_bw_test.png" alt="amok bandwidth measurement protocol"><br>
 * Fig 1: AMOK bandwidth measurement protocol.</center>
 * <h3>Link saturation</h3>
 * \endhtmlonly
 * 
 *  You sometimes want to try saturating some link during the network
 *  related experiments (at least, we did ;). This also can turn quite
 *  untrivial to do, unless you use this great module. You can either ask
 *  for the saturation between the current host and a distant one with
 *  amok_bw_saturate_begin() or between two distant hosts with
 *  amok_bw_saturate_start(). In any case, remember that gras actors
 *  (processes) are not interruptible. It means that an actor you
 *  instructed to participate to a link saturation experiment will not do
 *  anything else until it is to its end (either because the asked duration
 *  was done or because someone used amok_bw_saturate_stop() on the emitter
 *  end of the experiment).
 * 
 *  The following figure depicts the used protocol. Note that any
 *  handshaking messages internal messages are omitted for sake of
 *  simplicity. In this example, the experiment ends before the planned
 *  experiment duration is over because one host use the
 *  amok_bw_saturate_stop() function, but things are not really different
 *  if the experiment stops alone. Also, it is not mandatory that the host
 *  calling amok_bw_saturate_stop() is the same than the one which called
 *  amok_bw_saturate_start(), despite what is depicted here.
 * 
 *  \htmlonly
 * <center><img align=center src="amok_bw_sat.png" alt="amok bandwidth saturation protocol"><br>
 * Fig 2: AMOK link saturation protocol.</center>
 * \endhtmlonly
 *
 *  @{
 */

/* module handling */

XBT_PUBLIC(void) amok_bw_init(void);
XBT_PUBLIC(void) amok_bw_exit(void);

XBT_PUBLIC(void) amok_bw_test(gras_socket_t peer,
                              unsigned long int buf_size,
                              unsigned long int msg_size,
                              unsigned long int msg_amount,
                              double min_duration,
                              /*OUT*/ double *sec, double *bw);

XBT_PUBLIC(void) amok_bw_request(const char *from_name,
                                 unsigned int from_port, const char *to_name,
                                 unsigned int to_port,
                                 unsigned long int buf_size,
                                 unsigned long int msg_size,
                                 unsigned long int msg_amount,
                                 double min_duration, /*OUT*/ double *sec,
                                 double *bw);

XBT_PUBLIC(double *) amok_bw_matrix(xbt_dynar_t hosts,  /* dynar of xbt_host_t */
                                    int buf_size_bw, int msg_size_bw,
                                    int msg_amount_bw, double min_duration);

/* ***************************************************************************
 * Link saturation
 * ***************************************************************************/


XBT_PUBLIC(void) amok_bw_saturate_start(const char *from_name,
                                        unsigned int from_port,
                                        const char *to_name,
                                        unsigned int to_port,
                                        unsigned int msg_size,
                                        double duration);

XBT_PUBLIC(void) amok_bw_saturate_begin(const char *to_name,
                                        unsigned int to_port,
                                        unsigned int msg_size,
                                        double duration,
                                        /*out */ double *elapsed, double *bw);

XBT_PUBLIC(void) amok_bw_saturate_stop(const char *from_name,
                                       unsigned int from_port,
                                       /*out */ double *time, double *bw);

/** @} */

#endif /* AMOK_BANDWIDTH_H */
