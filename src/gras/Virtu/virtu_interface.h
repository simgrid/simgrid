/* $Id$ */

/* virtu[alization] - speciafic parts for each OS and for SG                */

/* module's public interface  exported within GRAS, but not to end user.    */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_VIRTU_INTERFACE_H
#define GRAS_VIRTU_INTERFACE_H

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "gras/virtu.h"
#include "gras/process.h"

/**
 * gras_process_data_t:
 *
 * Data for each process 
 */
typedef struct {
  /*queue of msgs storing the ones got while msg_wait'ing for something else */
  xbt_dynar_t msg_queue; /* elm type: gras_msg_t */

  /* registered callbacks for each message */
  xbt_dynar_t cbl_list; /* elm type: gras_cblist_t */
   
  /* SG only elements. In RL, they are part of the OS ;) */
  int chan;    /* Formated messages channel */
  int rawChan; /* Unformated echange channel */
  xbt_dynar_t sockets; /* all sockets known to this process */

  /* globals of the process */
  void *userdata;               
} gras_procdata_t;

/* Access */
xbt_dynar_t gras_socketset_get(void);

/* FIXME: mv to _private? */
gras_procdata_t *gras_procdata_get(void);
void gras_procdata_init(void);
void gras_procdata_exit(void);
#endif  /* GRAS_VIRTU_INTERFACE_H */
