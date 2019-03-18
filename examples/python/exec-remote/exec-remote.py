# Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys
from simgrid import *


class Wizard:
    def __call__(self):

        fafard = Host.by_name("Fafard")
        ginette = Host.by_name("Ginette")
        boivin = Host.by_name("Boivin")

        this_actor.info("I'm a wizard! I can run a task on the Ginette host from the Fafard one! Look!")
        activity = this_actor.exec_init(48.492e6)
        activity.host = ginette
        activity.start()
        this_actor.info("It started. Running 48.492Mf takes exactly one second on Ginette (but not on Fafard).")

        this_actor.sleep_for(0.1)
        this_actor.info("Loads in flops/s: Boivin={:.0f}; Fafard={:.0f}; Ginette={:.0f}".format(boivin.load, fafard.load,
                                                                                                ginette.load))
        activity.wait()
        this_actor.info("Done!")

        this_actor.info("And now, harder. Start a remote task on Ginette and move it to Boivin after 0.5 sec")
        activity = this_actor.exec_init(73293500)
        activity.host = ginette
        activity.start()

        this_actor.sleep_for(0.5)
        this_actor.info(
            "Loads before the move: Boivin={:.0f}; Fafard={:.0f}; Ginette={:.0f}".format(
                boivin.load,
                fafard.load,
                ginette.load))

        activity.host = boivin

        this_actor.sleep_for(0.1)
        this_actor.info(
            "Loads after the move: Boivin={:.0f}; Fafard={:.0f}; Ginette={:.0f}".format(
                boivin.load,
                fafard.load,
                ginette.load))

        activity.wait()
        this_actor.info("Done!")


if __name__ == '__main__':
    e = Engine(sys.argv)

    e.load_platform(sys.argv[1])

    Actor.create("test", Host.by_name("Fafard"), Wizard())

    e.run()
