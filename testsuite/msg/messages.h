/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdio.h>
#include <stdarg.h>
#include "msg/datatypes.h"
#include "xbt/error.h"

static void ASSERT(int value, const char *fmt, ...)
{
  m_process_t self = MSG_process_self();
  va_list ap;

  if(!value) {
    va_start(ap, fmt);
    if (self)
      fprintf(stderr, "[%Lg] P%d | ", MSG_getClock(),
	      MSG_process_get_PID(self));
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    
    xbt_abort();
  }
  return;
}

static void DIE(const char *fmt, ...)
{
  m_process_t self = MSG_process_self();
  va_list ap;

  va_start(ap, fmt);
  if (self)
    fprintf(stderr, "[%Lg] P%d | ", MSG_getClock(),
	    MSG_process_get_PID(self));
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  xbt_abort();
  return;
}

static void PRINT_MESSAGE(const char *fmt, ...)
{
#ifdef VERBOSE
  m_process_t self = MSG_process_self();
  va_list ap;

  va_start(ap, fmt);
  if (self)
    fprintf(stderr, "[%Lg] P%d | (%s:%s) ", MSG_getClock(),
	    MSG_process_get_PID(self), MSG_host_self()->name, self->name);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
#endif
  return;
}

static void PRINT_DEBUG_MESSAGE(const char *fmt, ...)
{
#ifdef VERBOSE
  m_process_t self = MSG_process_self();
  va_list ap;

  va_start(ap, fmt);
  if (self)
    fprintf(stderr, "DEBUG [%Lg] P%d | (%s) ", MSG_getClock(),
	    MSG_process_get_PID(self), MSG_host_self()->name);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
#endif
  return;
}

#endif
