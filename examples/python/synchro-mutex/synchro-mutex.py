# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from argparse import ArgumentParser
from dataclasses import dataclass
import sys

from simgrid import Actor, Engine, Host, Mutex, this_actor


def create_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    parser.add_argument(
        '--actors',
        type=int,
        default=6,
        help='how many pairs of actors should be started'
    )
    return parser


@dataclass
class ResultHolder:
    value: int


def worker_context_manager(mutex: Mutex, result: ResultHolder):
    with mutex:
        this_actor.info(f"Hello simgrid, I'm ready to compute after acquiring the mutex from a context manager")
        result.value += 1
    this_actor.info(f"I'm done, good bye")


def worker(mutex: Mutex, result: ResultHolder):
    mutex.lock()
    this_actor.info("Hello simgrid, I'm ready to compute after a regular lock")
    result.value += 1
    mutex.unlock()
    this_actor.info("I'm done, good bye")


def master(settings):
    results = [ResultHolder(value=0) for _ in range(settings.actors)]
    for i in range(settings.actors):
        mutex = Mutex()
        Actor.create(f"worker-{i}(mgr)", Host.by_name("Jupiter"), worker_context_manager, mutex, results[i])
        Actor.create(f"worker-{i}", Host.by_name("Tremblay"), worker, mutex, results[i])
    this_actor.sleep_for(10)
    for i in range(settings.actors):
        this_actor.info(f"Result[{i}] -> {results[i].value}")
    this_actor.info("I'm done, good bye")


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)
    Actor.create("master", Host.by_name("Tremblay"), master, settings)
    e.run()


if __name__ == "__main__":
    main()
