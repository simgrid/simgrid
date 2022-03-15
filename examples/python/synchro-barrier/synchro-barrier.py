# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from argparse import ArgumentParser
import sys

from simgrid import Actor, Barrier, Engine, Host, this_actor


def create_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    parser.add_argument(
        "--actors",
        type=int,
        required=True,
        help="Number of actors to start"
    )
    return parser


def worker(barrier: Barrier):
    """ Wait on the barrier and exits.
    :param barrier: Barrier to be awaited
    """
    this_actor.info(f"Waiting on the barrier")
    barrier.wait()
    this_actor.info("Bye")


def master(actor_count: int):
    """ Create barrier with `actor_count` expected actors, spawns `actor_count - 1` workers, then wait on the barrier
    and exits.
    :param actor_count: Spawn actor_count-1 workers and do a barrier with them
    """
    barrier = Barrier(actor_count)
    workers_count = actor_count - 1
    this_actor.info(f"Spawning {workers_count} workers")
    for i in range(workers_count):
        Actor.create(f"worker-{i}", Host.by_name("Jupiter"), worker, barrier)
    this_actor.info(f"Waiting on the barrier")
    barrier.wait()
    this_actor.info("Bye")


def main():
    settings = create_parser().parse_known_args()[0]
    if settings.actors < 1:
        raise ValueError("--actors must be greater than 0")
    e = Engine(sys.argv)
    e.load_platform(settings.platform)
    Actor.create("master", Host.by_name("Tremblay"), master, settings.actors)
    e.run()


if __name__ == "__main__":
    main()
