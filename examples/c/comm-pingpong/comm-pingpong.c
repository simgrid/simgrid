/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "simgrid/forward.h"
#include "simgrid/mailbox.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(app_pingpong, "Messages specific for this example");

static void pinger(int argc, char* argv[])
{
  sg_mailbox_t mailbox_in  = sg_mailbox_by_name("Mailbox 1");
  sg_mailbox_t mailbox_out = sg_mailbox_by_name("Mailbox 2");

  XBT_INFO("Ping from mailbox %s to mailbox %s", sg_mailbox_get_name(mailbox_in), sg_mailbox_get_name(mailbox_out));

  /* - Do the ping with a 1-Byte task (latency bound) ... */
  double* now = (double*)xbt_malloc(sizeof(double));
  *now        = simgrid_get_clock();
  sg_mailbox_put(mailbox_out, now, 1);

  /* - ... then wait for the (large) pong */
  double* sender_time = (double*)sg_mailbox_get(mailbox_in);

  double communication_time = simgrid_get_clock() - *sender_time;
  XBT_INFO("Task received : large communication (bandwidth bound)");
  XBT_INFO("Pong time (bandwidth bound): %.3f", communication_time);
  xbt_free(sender_time);
}

static void ponger(int argc, char* argv[])
{
  sg_mailbox_t mailbox_in  = sg_mailbox_by_name("Mailbox 2");
  sg_mailbox_t mailbox_out = sg_mailbox_by_name("Mailbox 1");

  XBT_INFO("Pong from mailbox %s to mailbox %s", sg_mailbox_get_name(mailbox_in), sg_mailbox_get_name(mailbox_out));

  /* - Receive the (small) ping first ....*/
  double* sender_time       = (double*)sg_mailbox_get(mailbox_in);
  double communication_time = simgrid_get_clock() - *sender_time;
  XBT_INFO("Task received : small communication (latency bound)");
  XBT_INFO(" Ping time (latency bound) %f", communication_time);
  xbt_free(sender_time);

  /*  - ... Then send a 1GB pong back (bandwidth bound) */
  double* payload = (double*)xbt_malloc(sizeof(double));
  *payload        = simgrid_get_clock();
  XBT_INFO("task_bw->data = %.3f", *payload);
  sg_mailbox_put(mailbox_out, payload, 1e9);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s ../../platforms/small_platform.xml app-pingpong_d.xml\n",
             argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  simgrid_register_function("pinger", pinger);
  simgrid_register_function("ponger", ponger);
  simgrid_load_deployment(argv[2]);

  simgrid_run();

  XBT_INFO("Total simulation time: %.3f", simgrid_get_clock());

  return 0;
}
