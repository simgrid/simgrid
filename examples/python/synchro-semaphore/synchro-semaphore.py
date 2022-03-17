# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from typing import List, Optional
from dataclasses import dataclass
from argparse import ArgumentParser
import sys

from simgrid import Actor, Engine, Host, Semaphore, this_actor


@dataclass
class Shared:
    buffer: str


END_MARKER = ""


shared = Shared("")
sem_empty = Semaphore(1)  # indicates whether the buffer is empty
sem_full = Semaphore(0)  # indicates whether the buffer is full


def create_parser():
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    parser.add_argument(
        '--words',
        type=lambda raw_words: raw_words.split(","),
        default=["one", "two", "three"],
        help='Comma-delimited list of words sent by the producer to the consumer'
    )
    return parser


def producer(words: List[str]):
    this_actor.info("starting consuming")
    for word in words + [END_MARKER]:
        sem_empty.acquire()
        this_actor.sleep_for(1)
        this_actor.info(f"Pushing '{word}'")
        shared.buffer = word
        sem_full.release()
    this_actor.info("Bye!")


def consumer():
    this_actor.info("starting producing")
    word: Optional[str] = None
    while word != END_MARKER:
        sem_full.acquire()
        word = str(shared.buffer)
        sem_empty.release()
        this_actor.info(f"Receiving '{word}'")
    this_actor.info("Bye!")


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)
    Actor.create("producer", Host.by_name("Tremblay"), producer, settings.words)
    Actor.create("consumer", Host.by_name("Jupiter"), consumer)
    e.run()


if __name__ == "__main__":
    main()
