/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

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

#include <simgrid/s4u.hpp>
#include <string>

// This declares a logging channel so that XBT_INFO can be used later
XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_create, "The logging channel used in this example");

/* Declares a first class of actors which sends a message to the mailbox 'mb42'.
 * The sent message is what was passed as parameter on creation (or 'GaBuZoMeu' by default)
 *
 * Later, this actor class is instantiated twice in the simulation.
 */
class Sender {
public:
  std::string msg = "GaBuZoMeu";
  explicit Sender() = default;
  explicit Sender(std::vector<std::string> args)
  {
    /* This constructor is used when we pass parameters to the actor */
    if (args.size() > 0)
      msg = args[0];
  }
  void operator()()
  {
    XBT_INFO("Hello s4u, I have something to send");
    simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName("mb42");

    mailbox->put(new std::string(msg), msg.size());
    XBT_INFO("I'm done. See you.");
  }
};

/* Declares a second class of actor which receive two messages on the mailbox which
 * name is passed as parameter ('thingy' by default, ie the wrong one).
 *
 * Later, this actor class is instantiated once in the simulation.
 */
class Receiver {
public:
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName("thingy");

  explicit Receiver() = default;
  explicit Receiver(std::vector<std::string> args)
  {
    /* This constructor is used when we pass parameters to the actor */
    /* as with argc/argv, args[0] is the actor's name, so the first parameter is args[1] */

    /* FIXME: this is a bug as this does not happen when starting the process directly
     * We should fix it by not adding the process name as argv[0] from the deployment file,
     * which is useless anyway since it's always the function name in this setting.
     * But this will break MSG...
     */
    if (args.size() > 1)
      mailbox = simgrid::s4u::Mailbox::byName(args[1]);
  }
  void operator()()
  {
    XBT_INFO("Hello s4u, I'm ready to get any message you'd want on %s", mailbox->getCname());

    std::string* msg1 = static_cast<std::string*>(mailbox->get());
    std::string* msg2 = static_cast<std::string*>(mailbox->get());
    XBT_INFO("I received '%s' and '%s'", msg1->c_str(), msg2->c_str());
    delete msg1;
    delete msg2;
    XBT_INFO("I'm done. See you.");
  }
};

/* Here comes the main function of your program */
int main(int argc, char** argv)
{
  /* When your program starts, you have to first start a new simulation engine, as follows */
  simgrid::s4u::Engine e(&argc, argv);

  /* Then you should load a platform file, describing your simulated platform */
  e.loadPlatform("../../platforms/small_platform.xml");

  /* And now you have to ask SimGrid to actually start your actors.
   *
   * You can first directly start your actor, as follows. Note the last parameter: 'Sender()',
   * as if you would call the Sender function.
   */
  simgrid::s4u::Actor::createActor("sender1", simgrid::s4u::Host::by_name("Tremblay"), Sender());

  /* The second way is to first register your function, and then retrieve it */
  e.registerFunction<Sender>("sender");  // The sender is passed as a template parameter here
  std::vector<std::string> args;         // Here we declare the parameter that the actor will get
  args.push_back("GloubiBoulga");        // Add a parameter to the set (we could have done it in the first approach too)

  simgrid::s4u::Actor::createActor("sender2", simgrid::s4u::Host::by_name("Jupiter"), "sender", args);

  /* The third way to start your actors is to use a deployment file. */
  e.registerFunction<Receiver>("receiver");   // You first have to register the actor as with the second approach
  e.loadDeployment("s4u-actor-create_d.xml"); // And then, you load the deployment file

  /* Once every actors are started in the engine, the simulation can start */
  e.run();

  /* Once the simulation is done, the program is ended */
  return 0;
}
