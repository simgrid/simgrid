# Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys
from argparse import ArgumentParser

import numpy as np

import simgrid
from simgrid import Engine, this_actor


def create_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    return parser


def pinger():
    simgrid.smpi.MPI.Init()
    MPI_COMM_WORLD = simgrid.smpi.MPI.Comm.WORLD
    size = int(0)
    size = simgrid.smpi.MPI.Comm.size(MPI_COMM_WORLD)
    this_actor.info(f"ping {size}")
    rank = simgrid.smpi.MPI.Comm.rank(MPI_COMM_WORLD)
    this_actor.info(f"ping {rank}")
    input_size = 10
    root = 0
    np.random.seed(5)
    if rank == root:
        np_in = np.random.randint(100, size=input_size, dtype=np.int32)
        this_actor.info(f"input buffer {np_in}")
    else:
        np_in = np.zeros(input_size, dtype=np.int32)
    simgrid.smpi.MPI.Bcast(np_in, input_size, simgrid.smpi.MPI.Datatype.INT, root, MPI_COMM_WORLD)
    if rank != root:
        this_actor.info(f"result buffer {np_in}")

    simgrid.smpi.MPI.Finalize()

def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    simgrid.smpi.init()
    e.load_platform(settings.platform)


    simgrid.smpi.app_instance_start("pinger", pinger, [e.host_by_name("Jupiter"), e.host_by_name("Tremblay")])

    e.run()

    this_actor.info(f"Total simulation time: {e.clock:.3f}")


if __name__ == "__main__":
    main()
