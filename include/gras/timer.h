/* $Id$ */

/* timer - delayed and repetitive tasks                                     */
/* module's public interface exported to end user.                          */

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TIMER_H
#define GRAS_TIMER_H

#include "xbt/misc.h"

SG_BEGIN_DECL()

/** @addtogroup GRAS_timer
 *  @brief Delayed and repetitive tasks
 *
 *  This is how to have a specific function called only once after the
 *  specified amount of time or a function executed every 5 mn until it gets 
 *  removed. In the UNIX world, this is comparable to <tt>at</tt> and 
 *  <tt>cron</tt>.
 *
 *  Note that this is very soft timers: the execution of the processes won't
 *  get interrupted at all. This is on purpose: the GRAS programming model
 *  is distributed sequential, so that users don't have to deal with mutexes
 *  and such within a specific process.
 *
 *  Timers are served by the gras_handle() function: if there is an elapsed 
 *  timer, the associated code gets executed before any incomming connexion 
 *  are checked. 
 *
 *  The section \ref GRAS_ex_timer constitutes a perfect example of these features.
 * 
 *  @{
 */
XBT_PUBLIC(void) gras_timer_delay(double delay, void_f_void_t action);
XBT_PUBLIC(void) gras_timer_repeat(double interval, void_f_void_t action);

XBT_PUBLIC(void) gras_timer_cancel_delay(double interval,
                                         void_f_void_t action);
XBT_PUBLIC(void) gras_timer_cancel_repeat(double interval,
                                          void_f_void_t action);

XBT_PUBLIC(void) gras_timer_cancel_delay_all(void);
XBT_PUBLIC(void) gras_timer_cancel_repeat_all(void);

XBT_PUBLIC(void) gras_timer_cancel_all(void);

/** @} */

SG_END_DECL()
#endif /* GRAS_TIMER_H */
