/* timer - Delayed and repetitive actions                                   */

/* Copyright (c) 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/Msg/msg_private.h"
#include "gras/timer.h"
#include "gras/Virtu/virtu_interface.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_timer, gras,
                                "Delayed and repetitive actions");

/** @brief Request \a action to be called once in \a delay seconds */
void gras_timer_delay(double delay, void_f_void_t action)
{
  gras_msg_procdata_t pd =
    (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);

  gras_timer_t timer = xbt_dynar_push_ptr(pd->timers);

  VERB1("Register delayed action %p", action);
  timer->period = delay;
  timer->expiry = delay + gras_os_time();
  timer->action = action;
  timer->repeat = FALSE;
}

/** @brief Request \a action to be called every \a interval seconds */
void gras_timer_repeat(double interval, void_f_void_t action)
{
  gras_msg_procdata_t pd =
    (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);

  gras_timer_t timer = xbt_dynar_push_ptr(pd->timers);

  VERB1("Register repetitive action %p", action);
  timer->period = interval;
  timer->expiry = interval + gras_os_time();
  timer->action = action;
  timer->repeat = TRUE;
}

/** @brief Cancel a delayed task */
void gras_timer_cancel_delay(double interval, void_f_void_t action)
{
  gras_msg_procdata_t pd =
    (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);
  unsigned int cursor;
  int found;
  s_gras_timer_t timer;

  found = FALSE;
  xbt_dynar_foreach(pd->timers, cursor, timer) {
    if (timer.repeat == FALSE &&
        timer.period == interval && timer.action == action) {

      found = TRUE;
      xbt_dynar_cursor_rm(pd->timers, &cursor);
    }
  }

  if (!found)
    THROW2(mismatch_error, 0,
           "Cannot remove the action %p delayed of %f second: not found",
           action, interval);

}

/** @brief Cancel a repetitive task */
void gras_timer_cancel_repeat(double interval, void_f_void_t action)
{
  gras_msg_procdata_t pd =
    (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);
  unsigned int cursor;
  int found;
  s_gras_timer_t timer;

  found = FALSE;
  xbt_dynar_foreach(pd->timers, cursor, timer) {
    if (timer.repeat == TRUE &&
        timer.period == interval && timer.action == action) {

      found = TRUE;
      xbt_dynar_cursor_rm(pd->timers, &cursor);
    }
  }

  if (!found)
    THROW2(mismatch_error, 0,
           "Cannot remove the action %p delayed of %f second: not found",
           action, interval);
}

/** @brief Cancel all delayed tasks */
void gras_timer_cancel_delay_all(void)
{
  gras_msg_procdata_t pd =
    (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);
  unsigned int cursor;
  int found;
  s_gras_timer_t timer;

  found = FALSE;
  xbt_dynar_foreach(pd->timers, cursor, timer) {
    if (timer.repeat == FALSE) {

      found = TRUE;
      xbt_dynar_cursor_rm(pd->timers, &cursor);
    }
  }

  if (!found)
    THROW0(mismatch_error, 0, "No delayed action to remove");

}

/** @brief Cancel all repetitive tasks */
void gras_timer_cancel_repeat_all(void)
{
  gras_msg_procdata_t pd =
    (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);
  unsigned int cursor;
  int found;
  s_gras_timer_t timer;

  found = FALSE;
  xbt_dynar_foreach(pd->timers, cursor, timer) {
    if (timer.repeat == FALSE) {

      found = TRUE;
      xbt_dynar_cursor_rm(pd->timers, &cursor);
    }
  }

  if (!found)
    THROW0(mismatch_error, 0, "No repetitive action to remove");
}

/** @brief Cancel all delayed and repetitive tasks */
void gras_timer_cancel_all(void)
{
  gras_msg_procdata_t pd =
    (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);
  xbt_dynar_reset(pd->timers);
}


/* returns 0 if it handled a timer, or the delay until next timer, or -1 if no armed timer */
double gras_msg_timer_handle(void)
{
  gras_msg_procdata_t pd =
    (gras_msg_procdata_t) gras_libdata_by_id(gras_msg_libdata_id);
  unsigned int cursor;
  gras_timer_t timer;
  double now = gras_os_time();
  double untilnext = -1.0;

  for (cursor = 0; cursor < xbt_dynar_length(pd->timers); cursor++) {
    double untilthis;

    timer = xbt_dynar_get_ptr(pd->timers, cursor);
    untilthis = timer->expiry - now;

    DEBUG2("Action %p expires in %f", timer->action, untilthis);

    if (untilthis <= 0.0) {
      void_f_void_t action = timer->action;

      DEBUG5("[%.0f] Serve %s action %p (%f<%f)", gras_os_time(),
             timer->repeat ? "repetitive" : "delayed", timer->action,
             timer->expiry, now);

      if (timer->repeat) {
        timer->expiry = now + timer->period;
        DEBUG4("[%.0f] Re-arm repetitive action %p for %f (period=%f)",
               gras_os_time(), timer->action, timer->expiry, timer->period);
      } else {
        DEBUG2("[%.0f] Remove %p now that it's done", gras_os_time(),
               timer->action);
        xbt_dynar_cursor_rm(pd->timers, &cursor);
      }
      (*action) ();
      return 0.0;
    } else if (untilthis < untilnext || untilnext == -1) {
      untilnext = untilthis;
    }
  }
  return untilnext;
}
