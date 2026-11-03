# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to exchange messages through a message queue. Unlike the communications through a mailbox,
these messages do not involve any network transfer.

Usage: mess-wait.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, MessageQueue, this_actor


def sender(messages_count):
    mqueue = MessageQueue.by_name("control")

    this_actor.sleep_for(0.5)

    this_actor.info("Send 'hello' to 'receiver'")
    mqueue.put("hello")

    for i in range(messages_count):
        msg_content = f"Message {i}"
        # Create a control message and put it in the message queue
        mess = mqueue.put_async(msg_content)
        this_actor.info(f"Send '{msg_content}' to '{mqueue.name}'")
        mess.wait()

    # Send message to let the receiver know that it should stop
    this_actor.info("Send 'finalize' to 'receiver'")
    mqueue.put("finalize")


def receiver():
    mqueue = MessageQueue.by_name("control")

    this_actor.sleep_for(1)
    # Messages can be received asynchronously. In that case, you retrieve the payload once the Mess is over by
    # calling its get_payload() method.
    hello = mqueue.get_async()
    hello.wait()
    this_actor.info(f"I got a '{hello.get_payload()}'.")

    this_actor.info("Wait for my first message")
    while True:
        mess = mqueue.get_async()

        this_actor.sleep_for(0.1)
        mess.wait()

        received = mess.get_payload()
        this_actor.info(f"I got a '{received}'.")
        if received == "finalize":
            break  # If it's a finalize message, we're done.


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("Tremblay").add_actor("sender", sender, 3)
    e.host_by_name("Fafard").add_actor("receiver", receiver)

    e.run()
