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
        '--workers',
        type=int,
        default=6,
        help='number of workers to start'
    )
    return parser


@dataclass
class ResultHolder:
    value: int


def worker_context_manager(mutex: Mutex, result: ResultHolder):
    """ Worker that uses a context manager to acquire/release the shared mutex
    :param mutex: Shared mutex that guards read/write access to the shared result
    :param result: Shared result which will be updated by the worker
    """
    this_actor.info(f"I just started")
    with mutex:
        this_actor.info(f"acquired the mutex with context manager")
        result.value += 1
        this_actor.info(f"updated shared result, which is now {result.value}")
    this_actor.info(f"released the mutex after leaving the context manager")
    this_actor.info("Bye now!")


def worker(mutex: Mutex, result: ResultHolder):
    """ Worker that manually acquires/releases the shared mutex
    :param mutex: Shared mutex that guards read/write access to the shared result
    :param result: Shared result which will be updated by the worker
    """
    this_actor.info(f"I just started")
    mutex.lock()
    this_actor.info(f"acquired the mutex manually")
    result.value += 1
    this_actor.info(f"updated shared result, which is now {result.value}")
    mutex.unlock()
    this_actor.info(f"released the mutex manually")
    this_actor.info("Bye now!")


def master(settings):
    """ Spawns `--workers` workers and wait until they have all updated the shared result, then displays it before
        leaving. Alternatively spawns `worker_context_manager()` and `worker()` workers.
    :param settings: Simulation settings
    """
    result = ResultHolder(value=0)
    mutex = Mutex()
    actors = []
    for i in range(settings.workers):
        use_worker_context_manager = i % 2 == 0
        actors.append(
            Actor.create(
                f"worker-{i}(mgr)" if use_worker_context_manager else f"worker-{i}",
                Host.by_name("Jupiter" if use_worker_context_manager else "Tremblay"),
                worker_context_manager if use_worker_context_manager else worker,
                mutex,
                result
            )
        )
    this_actor.sleep_for(10)
    this_actor.info(f"The final result is: {result.value}")


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)
    Actor.create("master", Host.by_name("Tremblay"), master, settings)
    e.run()


if __name__ == "__main__":
    main()
