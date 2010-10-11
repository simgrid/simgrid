/* timer: repetitive and delayed actions                                    */

/* Copyright (c) 2005, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

int client(int argc, char *argv[]);     /* Placed here to not bother doxygen inclusion */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this test");

#define REPEAT_INTERVAL 1.0
#define DELAY_INTERVAL 2.0
#define LOOP_COUNT 5

typedef struct {
  int still_to_do;
} my_globals;

static void repetitive_action(void)
{
  my_globals *globals = (my_globals *) gras_userdata_get();

  /* Stop if nothing to do yet */
  if (globals->still_to_do <= 0) {
    INFO0("Repetitive_action has nothing to do yet");
    return;
  }

  if (globals->still_to_do == 1) {
    /* Unregister myself if I'm done */
    gras_timer_cancel_repeat(REPEAT_INTERVAL, repetitive_action);
  }

  INFO1
      ("repetitive_action decrementing globals->still_to_do. New value: %d",
       globals->still_to_do - 1);

  globals->still_to_do--;       /* should be the last line of the action since value=0 stops the program */
}                               /* end_of_repetitive_action */

static void delayed_action(void)
{
  my_globals *globals = (my_globals *) gras_userdata_get();

  INFO1("delayed_action setting globals->still_to_do to %d", LOOP_COUNT);

  globals->still_to_do = LOOP_COUNT;
}                               /* end_of_delayed_action */

int client(int argc, char *argv[])
{
  my_globals *globals;

  gras_init(&argc, argv);
  globals = gras_userdata_new(my_globals);
  globals->still_to_do = -1;

  INFO1("Programming the repetitive_action with a frequency of %f sec",
        REPEAT_INTERVAL);
  gras_timer_repeat(REPEAT_INTERVAL, repetitive_action);

  INFO1("Programming the delayed_action for after %f sec", DELAY_INTERVAL);
  gras_timer_delay(REPEAT_INTERVAL, delayed_action);

  INFO0("Have a rest");
  gras_os_sleep(DELAY_INTERVAL / 2.0);

  INFO0("Canceling the delayed_action.");
  gras_timer_cancel_delay(REPEAT_INTERVAL, delayed_action);

  INFO1("Re-programming the delayed_action for after %f sec",
        DELAY_INTERVAL);
  gras_timer_delay(REPEAT_INTERVAL, delayed_action);

  while (globals->still_to_do == -1 ||  /* Before delayed action runs */
         globals->still_to_do >
         0 /* after delayed_action, and not enough repetitive_action */ ) {

    DEBUG1("Prepare to handle messages for 5 sec (still_to_do=%d)",
           globals->still_to_do);
    gras_msg_handle(5.0);
  }
  gras_exit();
  return 0;
}                               /* end_of_client */
