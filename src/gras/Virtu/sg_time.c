/* $Id$ */

/* time - time related syscal wrappers                                      */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "Virtu/virtu_sg.h"

/**
 * gras_time:
 * @Returns: The current time
 * 
 * The epoch since when the time is given is not specified. It is thus only usefull to compute intervals
 */
double gras_os_time() {
  return MSG_getClock();
}

/**
 * gras_sleep:
 * @sec: amount of sec to sleep
 * @usec: amount of micro second to sleep
 * 
 * Freeze the process for the specified amount of time
 */
void gras_os_sleep(unsigned long sec,unsigned long usec) {
  MSG_process_sleep((double)sec + ((double)usec)/1000000);
}
