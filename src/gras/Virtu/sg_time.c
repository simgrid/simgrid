/* $Id$ */

/* time - time related syscal wrappers                                      */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "Virtu/virtu_sg.h"

double gras_time() {
  return MSG_getClock();
}
 
void gras_sleep(unsigned long sec,unsigned long usec) {
  MSG_process_sleep((double)sec + ((double)usec)/1000000);
}
