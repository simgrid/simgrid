/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

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

import org.simgrid.s4u.*;

/* Our first class of actors takes a single string as parameter.
 *
 * Later, this actor class is instantiated within the simulation.
 */
class Receiver extends Actor {
  String mailboxName;
  public Receiver(String mailboxName) { this.mailboxName = mailboxName; }
  public void run() throws SimgridException
  {
    Mailbox mailbox = this.get_engine().mailbox_by_name(mailboxName);

    Engine.info("Hello s4u, I'm ready to get any message you'd want on %s", mailbox.get_name());

    String msg1 = (String)mailbox.get();
    String msg2 = (String)mailbox.get();
    String msg3 = (String)mailbox.get();
    Engine.info("I received '%s', '%s' and '%s'", msg1, msg2, msg3);
    Engine.info("I'm done. See you.");
  }
}

/* Our second class of actors is created from the deployment file, so it needs a (String, String, String[]) constructor
 */
class Forwarder extends Actor {
  Mailbox in;
  Mailbox out;

  Forwarder(String[] args)
  {
    if (args.length < 2)
      Engine.die("Actor forwarder requires 2 parameters, but got only %d", args.length);
    this.in  = this.get_engine().mailbox_by_name(args[0]);
    this.out = this.get_engine().mailbox_by_name(args[1]);
  }
  public void run() throws SimgridException
  {
    String msg = (String)in.get();
    Engine.info("Forward '%s'.", msg);
    out.put(msg, msg.length());
  }
}

/* Declares a third class of actors which sends a message to the mailbox 'mb42'.
 * The sent message is what was passed as parameter on creation (or 'GaBuZoMeu' by default)
 *
 * Later, this actor class is instantiated twice in the simulation.
 */
class Sender extends Actor {
  String mbox = "mb42";
  String msg  = "GaBuZoMeu";
  Sender() { /* Sending the default message */ }
  Sender(String msg) { /* Sending the specified message */ this.msg = msg; }
  Sender(String[] args)
  {
    /* This constructor is used when we start the actor from the deployment file */
    /* In contrary to C, args[0] is the first parameter (the actor's name is not part of that vector) */
    if (args.length < 2)
      Engine.die("The sender is expecting 2 parameters from the deployment file but got %d ones instead.", args.length);
    msg  = args[0];
    mbox = args[1];
  }
  public void run() /* This is the main code of the actor */
  {
    Engine.info("Hello s4u, I have something to send");
    Mailbox mailbox = this.get_engine().mailbox_by_name(mbox);

    mailbox.put(msg, msg.length());
    Engine.info("I'm done. See you.");
  }
}

/* Here comes the main function of your program */
public class actor_create {
  public static void main(String[] args)
  {
    /* When your program starts, you have to first start a new simulation engine, as follows */
    Engine e = new Engine(args);

    /* Then you should load a platform file, describing your simulated platform */
    e.load_platform(args.length > 0 ? args[0] : "../../platforms/small_platform.xml");

    /* And now you have to ask SimGrid to actually start your actors.
     *
     * Once created, the actors must be explicitely added to the simulation engine.
     */
    e.host_by_name("Fafard").add_actor("receiver", new Receiver("mb42"));
    e.host_by_name("Tremblay").add_actor("sender1", new Sender());
    /* If you want to pass parameters to your class, that's very easy: just use your constructors */
    e.host_by_name("Jupiter").add_actor("sender2", new Sender("GloubiBoulga"));

    /* But starting actors directly is considered as a bad experimental habit, since it ties the code
     * you want to test with the experimental scenario. Starting your actors from an external deployment
     * file in XML ensures that you can test your code in several scenarios without changing the code itself.
     *
     * For that, Actor classes must have a (std::vector<std::string>) constructor. */
    /* Once actors and functions are registered, just load the deployment file */
    e.load_deployment(args.length == 2 ? args[1] : "actor_create_d.xml");

    /* Once every actors are started in the engine, the simulation can start */
    e.run();

    /* Once the simulation is done, the program is ended */

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
