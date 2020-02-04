/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to declare and start your actors.
 *
 * The first step is to declare the code of your actors (what they do exactly does not matter to this example) and then
 * you ask SimGrid to start your actors. There is three ways of doing so:
 *  - Directly, by instantiating your actor as paramter to Actor::create()
 *  - By first registering your actors before instantiating it;
 *  - Through the deployment file.
 *
 * This example shows all these solutions, even if you obviously should use only one of these solutions to start your
 * actors. The most advised solution is to use a deployment file, as it creates a clear separation between your
 * application and the settings to test it. This is a better scientific methodology. Actually, starting an actor with
 * Actor::create() is mostly useful to start an actor from another actor.
 */

#include <simgrid/actor.h>
#include <simgrid/engine.h>
#include <simgrid/host.h>
#include <simgrid/mailbox.h>
#include <xbt/asserts.h>
#include <xbt/log.h>

// This declares a logging channel so that XBT_INFO can be used later
XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_create, "The logging channel used in this example");

/* Our first class of actors is simply implemented with a function.
 * As every CSG actors, its parameters are in a vector of string (argc/argv), and it does not return anything.
 *
 * One 'receiver' actor is instantiated within the simulation later in this file.
 */
static int receiver(int argc, char** argv)
{
  xbt_assert(argc == 2, "This actor expects a single argument: the mailbox on which to get messages");
  sg_mailbox_t mailbox = sg_mailbox_by_name(argv[1]);

  XBT_INFO("Hello s4u, I'm ready to get any message you'd want on %s", argv[1]);

  const char* msg1 = sg_mailbox_get(mailbox);
  const char* msg2 = sg_mailbox_get(mailbox);
  const char* msg3 = sg_mailbox_get(mailbox);
  XBT_INFO("I received '%s', '%s' and '%s'", msg1, msg2, msg3);
  XBT_INFO("I'm done. See you.");
  return 0;
}

/* Our second class of actors, in charge of sending stuff */
static int sender(int argc, char** argv)
{
  xbt_assert(argc == 3, "Actor 'sender' requires 2 parameters (mailbox and data to send), but got only %d", argc - 1);
  XBT_INFO("Hello s4u, I have something to send");
  sg_mailbox_t mailbox = sg_mailbox_by_name(argv[1]);
  char* sent_data      = argv[2];

  sg_mailbox_put(mailbox, xbt_strdup(sent_data), strlen(sent_data));
  XBT_INFO("I'm done. See you.");
  return 0;
}

/* Here comes the main function of your program */
int main(int argc, char** argv)
{
  /* When your program starts, you have to first start a new simulation engine, as follows */
  simgrid_init(&argc, argv);

  /* Then you should load a platform file, describing your simulated platform */
  simgrid_load_platform("../../platforms/small_platform.xml");

  /* And now you have to ask SimGrid to actually start your actors.
   *
   * The easiest way to do so is to implement the behavior of your actor in a single function,
   * as we do here for the receiver actors. This function can take any kind of parameters, as
   * long as the last parameters of Actor::create() match what your function expects.
   */
  int recv_argc           = 2;
  const char* recv_argv[] = {"receiver", "mb42", NULL};
  sg_actor_t recv         = sg_actor_init("receiver", sg_host_by_name("Fafard"));
  sg_actor_start(recv, receiver, recv_argc, recv_argv);

  /* But starting actors directly is considered as a bad experimental habit, since it ties the code
   * you want to test with the experimental scenario. Starting your actors from an external deployment
   * file in XML ensures that you can test your code in several scenarios without changing the code itself.
   *
   * For that, you first need to register your function or your actor as follows.
   * Actor classes must have a (std::vector<std::string>) constructor,
   * and actor functions must be of type int(*)(int argc, char**argv). */
  simgrid_register_function("sender", &sender);
  /* Once actors and functions are registered, just load the deployment file */
  simgrid_load_deployment("csg-actor-create_d.xml");

  /* Once every actors are started in the engine, the simulation can start */
  simgrid_run();

  /* Once the simulation is done, the program is ended */
  return 0;
}
