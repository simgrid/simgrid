/* $Id$ */

/* timer: repetitive and delayed actions                                    */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

int client(int argc,char *argv[]); /* Placed here to not bother doxygen inclusion */

#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging specific to this test");

#define REPEAT_INTERVAL 1.0
#define DELAY_INTERVAL 2.0
#define LOOP_COUNT 5

typedef struct {
   int still_to_do;
} my_globals;

static void repetitive_action(void) {
   my_globals *globals=(my_globals*)gras_userdata_get();
   
   /* Stop if nothing to do yet */
   if (globals->still_to_do <= 0) {
     INFO1("[%.0f] Repetitive_action has nothing to do yet",gras_os_time());
     return;
   }
   
   if (globals->still_to_do == 1) {
      /* Unregister myself if I'm done */
      gras_timer_cancel_repeat(REPEAT_INTERVAL,repetitive_action);
   }

   INFO2("[%.0f] repetitive_action decrementing globals->still_to_do. New value: %d",gras_os_time(),
	 globals->still_to_do-1);
   
   globals->still_to_do--; /* should be the last line of the action since value=0 stops the program */
} /* end_of_repetitive_action */

static void delayed_action(void) {
   my_globals *globals=(my_globals*)gras_userdata_get();
   
   INFO2("[%.0f] delayed_action setting globals->still_to_do to %d",
	 gras_os_time(),LOOP_COUNT);

   globals->still_to_do = LOOP_COUNT;
} /* end_of_delayed_action */

int client(int argc,char *argv[]) {
  xbt_error_t errcode;
  int cpt;
  my_globals *globals;

  gras_init(&argc,argv);
  globals=gras_userdata_new(my_globals);
  globals->still_to_do = -1;

  INFO2("[%.0f] Programming the repetitive_action with a frequency of %f sec", gras_os_time(), REPEAT_INTERVAL);
  gras_timer_repeat(REPEAT_INTERVAL,repetitive_action);
   
  INFO2("[%.0f] Programming the delayed_action for after %f sec", gras_os_time(), DELAY_INTERVAL);
  gras_timer_delay(REPEAT_INTERVAL,delayed_action);

  INFO1("[%.0f] Have a rest", gras_os_time());  
  gras_os_sleep(DELAY_INTERVAL / 2.0);
   
  INFO1("[%.0f] Canceling the delayed_action.",gras_os_time());
  gras_timer_cancel_delay(REPEAT_INTERVAL,delayed_action);

  INFO2("[%.0f] Re-programming the delayed_action for after %f sec", gras_os_time(),DELAY_INTERVAL);
  gras_timer_delay(REPEAT_INTERVAL,delayed_action);

  while (globals->still_to_do == -1 ||  /* Before delayed action runs */
	 globals->still_to_do > 0 /* after delayed_action, and not enough repetitive_action */) {

     DEBUG2("[%.0f] Prepare to handle messages for 5 sec (still_to_do=%d)",
	    gras_os_time(), globals->still_to_do);
     gras_msg_handle(5.0);
  }
  gras_exit();
  return 0;
} /* end_of_client */

