# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.


from argparse import ArgumentParser
import sys
from simgrid import Engine, Host, this_actor, Actor, sg_host_load_plugin_init

def parse():
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    return parser.parse_args()

def execute_load_test():
  host = Host.by_name('MyHost1')
  this_actor.info(f'Initial peak speed: {host.speed:.0E} flop/s; number of flops computed so far: {host.computed_flops:.0E} (should be 0) and current average load: {host.avg_load} (should be 0)')

  start = Engine.clock
  this_actor.info('Sleep for 10 seconds')
  this_actor.sleep_for(10)

  speed = host.speed
  this_actor.info(f'Done sleeping {Engine.clock - start}s; peak speed: {host.speed:.0E} flop/s; number of flops computed so far: {host.computed_flops:.0E} (nothing should have changed)')

  # Run an activity
  start = e.clock
  this_actor.info(f'Run an activity of {200E6:.0E} flops at current speed of {host.speed:.0E} flop/s')
  this_actor.execute(200E6)

  this_actor.info(f'Done working on my activity; this took {Engine.clock - start}s; current peak speed: {host.speed:.0E} flop/s (when I started the computation, \
the speed was set to {speed:.0E} flop/s); number of flops computed so \
far: {host.computed_flops:.0E}, average load as reported by the HostLoad plugin: {host.avg_load:.5f} (should be {200E6 / (10.5 * speed * host.core_count + (Engine.clock - start - 0.5) * host.speed * host.core_count):.5f})')

  # ========= Change power peak =========
  pstate = 1
  host.pstate = pstate
  this_actor.info(f'========= Requesting pstate {pstate} (speed should be of {host.pstate_speed(pstate):.0E} flop/s and is of {host.speed:.0E} flop/s, average load is {host.avg_load:.5f})')

  # Run a second activity
  start = Engine.clock
  this_actor.info(f'Run an activity of {100E6:.0E} flops')
  this_actor.execute(100E6)
  this_actor.info(f'Done working on my activity; this took {Engine.clock - start}s; current peak speed: {host.speed:.0E} flop/s; number of flops computed so far: {host.computed_flops:.0E}')

  start = Engine.clock
  this_actor.info("========= Requesting a reset of the computation and load counters")
  host.reset_load()
  this_actor.info(f'After reset: {host.computed_flops:.0E} flops computed; load is {host.avg_load}')
  this_actor.info('Sleep for 4 seconds')
  this_actor.sleep_for(4)
  this_actor.info(f'Done sleeping {Engine.clock - start}s; peak speed: {host.speed:.0E} flop/s; number of flops computed so far: {host.computed_flops:.0E}')

  # =========== Turn the other host off ==========
  host2 = Host.by_name('MyHost2')
  this_actor.info(f'Turning MyHost2 off, and sleeping another 10 seconds. MyHost2 computed {host2.computed_flops:.0E} flops so far and has an average load of {host2.avg_load}')
  host2.turn_off()
  start = Engine.clock
  this_actor.sleep_for(10)
  this_actor.info(f'Done sleeping {Engine.clock - start}s; peak speed: {host.speed:.0E} flop/s; number of flops computed so far: {host.computed_flops:.0E}')

def change_speed():
  host = Host.by_name('MyHost1')
  this_actor.sleep_for(10.5)
  this_actor.info("I slept until now, but now I'll change the speed of this host while the other actor is still computing! This should slow the computation down.")
  host.pstate = 2

if __name__ == '__main__':
  args = parse()
  
  sg_host_load_plugin_init()
  e = Engine(sys.argv)
  e.load_platform(args.platform)

  Actor.create('load_test', e.host_by_name('MyHost1'), execute_load_test)
  Actor.create('change_speed', e.host_by_name('MyHost1'), change_speed)

  e.run()

  this_actor.info(f'Total simulation time: {Engine.clock}')

