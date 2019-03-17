# Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# This example shows how to declare and start your actors.
#
# The first step is to declare the code of your actors (what they do exactly does not matter to this example) and then
# you ask SimGrid to start your actors. There is three ways of doing so:
# - Directly, by instantiating your actor as parameter to Actor::create()
# - By first registering your actors before instantiating it;
# - Through the deployment file.
#
# This example shows all these solutions, even if you obviously should use only one of these solutions to start your
# actors. The most advised solution is to use a deployment file, as it creates a clear separation between your
# application and the settings to test it. This is a better scientific methodology. Actually, starting an actor with
# Actor.create() is mostly useful to start an actor from another actor.

import sys
from simgrid import *


def receiver(mailbox_name):
    """
    Our first class of actors is simply implemented with a function, that takes a single string as parameter.
    Later, this actor class is instantiated within the simulation.
    """
    mailbox = Mailbox.by_name(mailbox_name)

    this_actor.info(
        "Hello s4u, I'm ready to get any message you'd want on {:s}".format(mailbox.name))

    msg1 = mailbox.get()
    msg2 = mailbox.get()
    msg3 = mailbox.get()
    this_actor.info(
        "I received '{:s}', '{:s}' and '{:s}'".format(msg1, msg2, msg3))
    this_actor.info("I'm done. See you.")


def forwarder(*args):
    """Our second class of actors is also a function"""
    if len(args) < 2:
        raise AssertionError(
            "Actor forwarder requires 2 parameters, but got only {:d}".format(len(args)))
    mb_in = Mailbox.by_name(args[0])
    mb_out = Mailbox.by_name(args[1])

    msg = mb_in.get()
    this_actor.info("Forward '{:s}'.".format(msg))
    mb_out.put(msg, len(msg))


class Sender:
    """
    Declares a third class of actors which sends a message to the mailbox 'mb42'.
    The sent message is what was passed as parameter on creation (or 'GaBuZoMeu' by default)

    Later, this actor class is instantiated twice in the simulation.
    """

    def __init__(self, msg="GaBuZoMeu", mbox="mb42"):
        self.msg = msg
        self.mbox = mbox

    # Actors that are created as object will execute their __call__ method.
    # So, the following constitutes the main function of the Sender actor.
    def __call__(self):
        this_actor.info("Hello s4u, I have something to send")
        mailbox = Mailbox.by_name(self.mbox)

        mailbox.put(self.msg, len(self.msg))
        this_actor.info("I'm done. See you.")


if __name__ == '__main__':
    # Here comes the main function of your program

    # When your program starts, you have to first start a new simulation engine, as follows
    e = Engine(sys.argv)

    # Then you should load a platform file, describing your simulated platform
    e.load_platform("../../platforms/small_platform.xml")

    # And now you have to ask SimGrid to actually start your actors.
    #
    # The easiest way to do so is to implement the behavior of your actor in a single function,
    # as we do here for the receiver actors. This function can take any kind of parameters, as
    # long as the last parameters of Actor::create() match what your function expects.
    Actor.create("receiver", Host.by_name("Fafard"), receiver, "mb42")

    # If your actor is getting more complex, you probably want to implement it as a class instead,
    # as we do here for the sender actors. The main behavior goes into operator()() of the class.
    #
    # You can then directly start your actor, as follows:
    Actor.create("sender1", Host.by_name("Tremblay"), Sender())
    # If you want to pass parameters to your class, that's very easy: just use your constructors
    Actor.create("sender2", Host.by_name("Jupiter"), Sender("GloubiBoulga"))

    # But starting actors directly is considered as a bad experimental habit, since it ties the code
    # you want to test with the experimental scenario. Starting your actors from an external deployment
    # file in XML ensures that you can test your code in several scenarios without changing the code itself.
    #
    # For that, you first need to register your function or your actor as follows.
    e.register_actor("sender", Sender)
    e.register_actor("forwarder", forwarder)
    # Once actors and functions are registered, just load the deployment file
    e.load_deployment("actor-create_d.xml")

    # Once every actors are started in the engine, the simulation can start
    e.run()

    # Once the simulation is done, the program is ended
