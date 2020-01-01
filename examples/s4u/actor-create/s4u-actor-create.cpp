/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to declare and start your actors.
 *
 * The first step is to declare the code of your actors (what they do exactly does not matter to this example) and then
 * you ask SimGrid to start your actors. There is three ways of doing so:
 *  - Directly, by instantiating your actor as parameter to Actor::create()
 *  - By first registering your actors before instantiating it
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

/* Our first class of actors is simply implemented with a function, that takes a single string as parameter.
 *
 * Later, this actor class is instantiated within the simulation.
 */
static void receiver(const std::string& mailbox_name)
{
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(mailbox_name);

  XBT_INFO("Hello s4u, I'm ready to get any message you'd want on %s", mailbox->get_cname());

  const std::string* msg1 = static_cast<std::string*>(mailbox->get());
  const std::string* msg2 = static_cast<std::string*>(mailbox->get());
  const std::string* msg3 = static_cast<std::string*>(mailbox->get());
  XBT_INFO("I received '%s', '%s' and '%s'", msg1->c_str(), msg2->c_str(), msg3->c_str());
  delete msg1;
  delete msg2;
  delete msg3;
  XBT_INFO("I'm done. See you.");
}

/* Our second class of actors is also a function */
static int forwarder(int argc, char** argv)
{
  xbt_assert(argc >= 3, "Actor forwarder requires 2 parameters, but got only %d", argc - 1);
  simgrid::s4u::Mailbox* in    = simgrid::s4u::Mailbox::by_name(argv[1]);
  simgrid::s4u::Mailbox* out   = simgrid::s4u::Mailbox::by_name(argv[2]);
  std::string* msg             = static_cast<std::string*>(in->get());
  XBT_INFO("Forward '%s'.", msg->c_str());
  out->put(msg, msg->size());
  return 0;
}

/* Declares a third class of actors which sends a message to the mailbox 'mb42'.
 * The sent message is what was passed as parameter on creation (or 'GaBuZoMeu' by default)
 *
 * Later, this actor class is instantiated twice in the simulation.
 */
class Sender {
public:
  std::string mbox  = "mb42";
  std::string msg = "GaBuZoMeu";
  explicit Sender() = default; /* Sending the default message */
  explicit Sender(const std::string& arg) : msg(arg) { /* Sending the specified message */}
  explicit Sender(std::vector<std::string> args)
  {
    /* This constructor is used when we start the actor from the deployment file */
    /* args[0] is the actor's name, so the first parameter is args[1] */

    xbt_assert(args.size() >= 3, "The sender is expecting 2 parameters from the deployment file but got %zu",
               args.size() - 1);
    msg  = args[1];
    mbox = args[2];
  }
  void operator()() /* This is the main code of the actor */
  {
    XBT_INFO("Hello s4u, I have something to send");
    simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(mbox);

    mailbox->put(new std::string(msg), msg.size());
    XBT_INFO("I'm done. See you.");
  }
};

/* Here comes the main function of your program */
int main(int argc, char** argv)
{
  /* When your program starts, you have to first start a new simulation engine, as follows */
  simgrid::s4u::Engine e(&argc, argv);

  /* Then you should load a platform file, describing your simulated platform */
  e.load_platform("../../platforms/small_platform.xml");

  /* And now you have to ask SimGrid to actually start your actors.
   *
   * The easiest way to do so is to implement the behavior of your actor in a single function,
   * as we do here for the receiver actors. This function can take any kind of parameters, as
   * long as the last parameters of Actor::create() match what your function expects.
   */
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("Fafard"), &receiver, "mb42");

  /* If your actor is getting more complex, you probably want to implement it as a class instead,
   * as we do here for the sender actors. The main behavior goes into operator()() of the class.
   *
   * You can then directly start your actor, as follows: */
  simgrid::s4u::Actor::create("sender1", simgrid::s4u::Host::by_name("Tremblay"), Sender());
  /* If you want to pass parameters to your class, that's very easy: just use your constructors */
  simgrid::s4u::Actor::create("sender2", simgrid::s4u::Host::by_name("Jupiter"), Sender("GloubiBoulga"));

  /* But starting actors directly is considered as a bad experimental habit, since it ties the code
   * you want to test with the experimental scenario. Starting your actors from an external deployment
   * file in XML ensures that you can test your code in several scenarios without changing the code itself.
   *
   * For that, you first need to register your function or your actor as follows.
   * Actor classes must have a (std::vector<std::string>) constructor,
   * and actor functions must be of type int(*)(int argc, char**argv). */
  e.register_actor<Sender>("sender"); // The sender class is passed as a template parameter here
  e.register_function("forwarder", &forwarder);
  /* Once actors and functions are registered, just load the deployment file */
  e.load_deployment("s4u-actor-create_d.xml");

  /* Once every actors are started in the engine, the simulation can start */
  e.run();

  /* Once the simulation is done, the program is ended */
  return 0;
}
