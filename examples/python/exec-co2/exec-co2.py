# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to compute the carbon footprint of the hosts with the host carbon footprint plugin. The
carbon intensity of the hosts is either static (given as a property in the platform file) or dynamic (read from a
CSV trace file).

Usage: exec-co2.py platform_file co2_trace_file [other parameters]
"""

import sys
from simgrid import (Engine, this_actor, sg_host_carbon_footprint_plugin_init,
                     sg_host_carbon_footprint_load_trace_file)


def test_execution():
    host = this_actor.get_host()

    # Run one task (total 1 CPU core) for 1 hour
    flop_amount_1_hour = host.speed * 3600

    # Then run 2 tasks (total of 2 CPU cores) for 2 hours, 3 tasks for 3 hours, and 4 tasks for 4 hours
    for task_count in range(1, 5):
        activities = [this_actor.exec_init(flop_amount_1_hour * task_count) for _ in range(task_count)]
        for activity in activities:
            activity.start()
        for activity in activities:
            activity.wait()


if __name__ == '__main__':
    if len(sys.argv) < 3:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file co2_trace_file [other parameters]")

    sg_host_carbon_footprint_plugin_init()
    sg_host_carbon_footprint_load_trace_file(sys.argv[2])

    e = Engine(sys.argv)
    e.load_platform(sys.argv[1])

    host_br_static_co2 = e.host_by_name("host_br_static_co2")
    host_usa_static_co2 = e.host_by_name("host_usa_static_co2")

    host_br_dynamic_co2 = e.host_by_name("host_br_dynamic_co2")
    host_usa_dynamic_co2 = e.host_by_name("host_usa_dynamic_co2")

    host_br_static_co2.add_actor("execution_static_br", test_execution)
    host_br_dynamic_co2.add_actor("execution_dynamic_br", test_execution)

    host_br_static_co2.add_actor("turn_off usa static", lambda host: host.turn_off(), host_usa_static_co2)
    host_br_dynamic_co2.add_actor("turn_off usa dynamic", lambda host: host.turn_off(), host_usa_dynamic_co2)

    e.run()
