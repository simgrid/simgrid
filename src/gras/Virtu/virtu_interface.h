/* $Id$ */

/* virtu[alization] - speciafic parts for each OS and for SG                */

/* module's public interface  exported within GRAS, but not to end user.    */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_VIRTU_INTERFACE_H
#define GRAS_VIRTU_INTERFACE_H

/**
 * gras_process_data_t:
 *
 * Data for each process 
 */
typedef struct {
  /* queue of messages which where received but not wanted in msgWait, and therefore
     temporarly queued until the next msgHandle */
  gras_dynar_t *msg_queue; /* elm type: gras_msg_t */

  /* registered callbacks for each message */
  gras_dynar_t *cbl_list; /* elm type: gras_cblist_t */
   

  /* The channel we are listening to in SG for formated messages */
  int chan;
  /* The channel we are listening to in SG for raw send/recv */
  int rawChan; 

  /* globals of the process */
  void *userdata;               
} gras_process_data_t;


#endif  /* GRAS_VIRTU_INTERFACE_H */
