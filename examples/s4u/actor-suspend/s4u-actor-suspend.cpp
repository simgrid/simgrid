/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_suspend, "Messages specific for this s4u example");

/* The Lazy guy only wants to sleep, but can be awaken by the dream_master process. */
static void lazy_guy()
{
  XBT_INFO("Nobody's watching me ? Let's go to sleep.");
  simgrid::s4u::this_actor::suspend(); /* - Start by suspending itself */
  XBT_INFO("Uuuh ? Did somebody call me ?");

  XBT_INFO("Going to sleep..."); /* - Then repetitively go to sleep, but got awaken */
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("Mmm... waking up.");

  XBT_INFO("Going to sleep one more time (for 10 sec)...");
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("Waking up once for all!");

  XBT_INFO("Ok, let's do some work, then (for 10 sec on Boivin).");
  simgrid::s4u::this_actor::execute(980.95e6);

  XBT_INFO("Mmmh, I'm done now. Goodbye.");
}

/* The Dream master: */
static void dream_master()
{
  XBT_INFO("Let's create a lazy guy."); /* - Create a lazy_guy process */
  simgrid::s4u::ActorPtr lazy = simgrid::s4u::Actor::create("Lazy", simgrid::s4u::this_actor::get_host(), lazy_guy);
  XBT_INFO("Let's wait a little bit...");
  simgrid::s4u::this_actor::sleep_for(10); /* - Wait for 10 seconds */
  XBT_INFO("Let's wake the lazy guy up! >:) BOOOOOUUUHHH!!!!");
  if (lazy->is_suspended())
    lazy->resume(); /* - Then wake up the lazy_guy */
  else
    XBT_ERROR("I was thinking that the lazy guy would be suspended now");

  simgrid::s4u::this_actor::sleep_for(5); /* Repeat two times: */
  XBT_INFO("Suspend the lazy guy while he's sleeping...");
  lazy->suspend(); /* - Suspend the lazy_guy while he's asleep */
  XBT_INFO("Let him finish his siesta.");
  simgrid::s4u::this_actor::sleep_for(10); /* - Wait for 10 seconds */
  XBT_INFO("Wake up, lazy guy!");
  lazy->resume(); /* - Then wake up the lazy_guy again */

  simgrid::s4u::this_actor::sleep_for(5);
  XBT_INFO("Suspend again the lazy guy while he's sleeping...");
  lazy->suspend();
  XBT_INFO("This time, don't let him finish his siesta.");
  simgrid::s4u::this_actor::sleep_for(2);
  XBT_INFO("Wake up, lazy guy!");
  lazy->resume();

  simgrid::s4u::this_actor::sleep_for(5);
  XBT_INFO("Give a 2 seconds break to the lazy guy while he's working...");
  lazy->suspend();
  simgrid::s4u::this_actor::sleep_for(2);
  XBT_INFO("Back to work, lazy guy!");
  lazy->resume();

  XBT_INFO("OK, I'm done here.");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]); /* - Load the platform description */
  std::vector<simgrid::s4u::Host*> list = e.get_all_hosts();
  simgrid::s4u::Actor::create("dream_master", list.front(), dream_master);

  e.run(); /* - Run the simulation */

  return 0;
}
