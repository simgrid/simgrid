/**** MSG_LICENCE DO NOT REMOVE ****/

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdio.h>
#include <stdarg.h>
#include "msg/datatypes.h"
#include "xbt/error.h"

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
    fprintf(stderr, "[%Lg] P%d | ", MSG_getClock(),
	    MSG_process_get_PID(self));
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
    fprintf(stderr, "DEBUG [%Lg] P%d | ", MSG_getClock(),
	    MSG_process_get_PID(self));
  vfprintf(stderr, fmt, ap);
  va_end(ap);
#endif
  return;
}

#endif
