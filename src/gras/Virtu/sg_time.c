/* $Id$ */

/* time - time related syscal wrappers                                      */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Virtu/virtu_sg.h"

/* 
 * Time elapsed since the begining of the simulation.
 */
double gras_os_time() {
  return MSG_getClock();
}

/*
 * Freeze the process for the specified amount of time
 */
void gras_os_sleep(double sec) {
  MSG_process_sleep(sec);
}
