# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example implements the following scenario:
- Multiple workers consume jobs (Job) from a shared mailbox (worker)
- A client first dispatches several jobs (with a simulated 'cost' - i.e. time the worker will 'process' the job)
- The client then waits for all job results for a maximum time set by the 'wait timeout' (Comm.wait_all_for)
- The client then displays the status of individual jobs.
"""


from argparse import ArgumentParser
from dataclasses import dataclass
from typing import List
from uuid import uuid4
import sys

from simgrid import Actor, Comm, Engine, Host, Mailbox, PyGetAsync, this_actor


SIMULATED_JOB_SIZE_BYTES = 1024
SIMULATED_RESULT_SIZE_BYTES = 1024 * 1024


def parse_requests(requests_str: str) -> List[float]:
    return [float(item.strip()) for item in requests_str.split(",")]


def create_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=1,
        help="number of worker actors to start"
    )
    parser.add_argument(
        "--jobs",
        type=parse_requests,
        default="1,2,3,4,5",
        help="duration of individual jobs sent to the workers by the client"
    )
    parser.add_argument(
        "--wait-timeout",
        type=float,
        default=5.0,
        help="number of seconds before the client gives up waiting for results and aborts the simulation"
    )
    return parser


@dataclass
class Job:
    job_id: str
    duration: float
    result_mailbox: Mailbox


def worker(worker_id: str):
    this_actor.info(f"{worker_id} started")
    mailbox: Mailbox = Mailbox.by_name("worker")
    while True:
        job: Job = mailbox.get()
        this_actor.info(f"{worker_id} working on {job.job_id} (will take {job.duration}s to complete)")
        this_actor.sleep_for(job.duration)
        job.result_mailbox.put(f"{worker_id}", SIMULATED_RESULT_SIZE_BYTES)


@dataclass
class AsyncJobResult:
    job: Job
    result_comm: Comm
    async_data: PyGetAsync

    @property
    def complete(self) -> bool:
        return self.result_comm.test()

    @property
    def status(self) -> str:
        return "complete" if self.complete else "pending"


def client(client_id: str, jobs: List[float], wait_timeout: float):
    worker_mailbox: Mailbox = Mailbox.by_name("worker")
    this_actor.info(f"{client_id} started")
    async_job_results: list[AsyncJobResult] = []
    for job_idx, job_duration in enumerate(jobs):
        result_mailbox: Mailbox = Mailbox.by_name(str(uuid4()))
        job = Job(job_id=f"job-{job_idx}", duration=job_duration, result_mailbox=result_mailbox)
        out_comm = worker_mailbox.put_init(Job(
            job_id=f"job-{job_idx}",
            duration=job_duration,
            result_mailbox=result_mailbox
        ), SIMULATED_JOB_SIZE_BYTES)
        out_comm.detach()
        result_comm, async_data = result_mailbox.get_async()
        async_job_results.append(AsyncJobResult(
            job=job,
            result_comm=result_comm,
            async_data=async_data
        ))
    this_actor.info(f"awaiting results for all jobs (timeout={wait_timeout}s)")
    completed_comms = Comm.wait_all_for([entry.result_comm for entry in async_job_results], wait_timeout)
    logger = this_actor.warning if completed_comms < len(async_job_results) else this_actor.info
    logger(f"received {completed_comms}/{len(async_job_results)} results")
    for result in async_job_results:
        this_actor.info(f"{result.job.job_id}"
                        f" status={result.status}"
                        f" result_payload={result.async_data.get() if result.complete else ''}")


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)
    Actor.create("client", Host.by_name("Tremblay"), client, "client", settings.jobs, settings.wait_timeout)
    for worker_idx in range(settings.workers):
        Actor.create("worker", Host.by_name("Ruby"), worker, f"worker-{worker_idx}").daemonize()
    e.run()


if __name__ == "__main__":
    main()
