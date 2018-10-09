.. S4U (Simgrid for you) is the next interface of SimGrid, expected to be released with SimGrid 4.0.
..
.. Even if it is not completely rock stable yet, it may well already fit
.. your needs. You are welcome to try it and report any interface
.. glitches that you see. Be however warned that the interface may change
.. until the final release.  You will have to adapt your code on the way.
.. 
.. This file follows the ReStructured syntax to be included in the
.. documentation, but it should remain readable directly.


S4U Examples
************

SimGrid comes with an extensive set of examples, documented on this
page. Most of them only demonstrate one single feature, with some
larger examplars listed below. 

Each of these examples can be found in a subdirectory under
examples/s4u in the archive. It contains the source code (also listed
from this page), and the so-called tesh file containing how to call
the binary obtained by compiling this example and also the expected
output. Tesh files are used to turn each of our examples into an
integration test. Some examples also contain other files, on need.

A good way to bootstrap your own project is to copy and combine some
of the provided examples to constitute the skeleton of what you plan
to simulate.

===========================
Actors: the Active Entities
===========================


Starting and Stoping Actors
---------------------------

  - **Creating actors:**
    Most actors are started from the deployment XML file, but there is other methods.
    This example show them all.
    |br| `examples/s4u/actor-create/s4u-actor-create.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-create/s4u-actor-create.cpp>`_
    
  - **Kill actors:**
    Actors can forcefully stop other actors with the 
    :cpp:func:`void simgrid::s4u::Actor::kill(void)` or the 
    :cpp:func:`void simgrid::s4u::Actor::kill(aid_t)` methods.
    |br| `examples/s4u/actor-kill/s4u-actor-kill.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-kill/s4u-actor-kill.cpp>`_

  - **Controling the actor life cycle from the XML:**
    You can specify a start time and a kill time in the deployment
    file.
    |br| `examples/s4u/actor-lifetime/s4u-actor-lifetime.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-lifetime/s4u-actor-lifetime.cpp>`_
    |br| `examples/s4u/actor-lifetime/s4u-actor-lifetime_d.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-lifetime/s4u-actor-lifetime_d.xml>`_

  - **Daemonize actors:**
    Some actors may be intended to simulate daemons that run in background. This example show how to transform a regular
    actor into a daemon that will be automatically killed once the simulation is over.
    |br| `examples/s4u/actor-daemon/s4u-actor-daemon.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-daemon/s4u-actor-daemon.cpp>`_
    
Inter-Actors Interactions
-------------------------

  - **Suspend and Resume actors:**    
    Actors can be suspended and resumed during their executions thanks
    to :cpp:func:`simgrid::s4u::Actor::suspend()` and
    :cpp:func:`simgrid::s4u::Actor::resume()`.
    |br| `examples/s4u/actor-suspend/s4u-actor-suspend.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-suspend/s4u-actor-suspend.cpp>`_

  - **Migrating Actors:**
    Actors can move or be moved from a host to another with
    :cpp:func:`simgrid::s4u::this_actor::migrate()`.
    |br| `examples/s4u/actor-migration/s4u-actor-migration.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-migration/s4u-actor-migration.cpp>`_

  - **Waiting for the termination of an actor:** (joining on it)
    :cpp:func:`simgrid::s4u::Actor::join()` allows to block the current
    actor until the end of the receiving actor.
    |br| `examples/s4u/actor-join/s4u-actor-join.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-join/s4u-actor-join.cpp>`_

  - **Yielding to other actor**.
    The :cpp:func:`simgrid::s4u::this_actor::yield()` function interrupts the
    execution of the current actor, leaving a chance to the other actors
    that are ready to run at this timestamp.
    |br| `examples/s4u/actor-yield/s4u-actor-yield.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-yield/s4u-actor-yield.cpp>`_

Traces Replay as a Workload
---------------------------

This section details how to run trace-driven simulations. It is very
handy when you want to test an algorithm or protocol that only react
to external events. For example, many P2P protocols react to user
requests, but do nothing if there is no such event.

In such situations, you should write your protocol in C++, and separate
the workload that you want to play onto your protocol in a separate
text file. Declare a function handling each type of the events in your
trace, register them using :cpp:func:`xbt_replay_action_register()` in
your main, and then run the simulation.

Then, you can either have one trace file containing all your events,
or a file per simulated process: the former may be easier to work
with, but the second is more efficient on very large traces. Check
also the tesh files in the example directories for details.

  - **Communication replay:**
    Presents a set of event handlers reproducing classical communication
    primitives (asynchronous send/receive at the moment).
    |br| `examples/s4u/replay-comm/s4u-replay-comm.cpp  <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/replay-comm/s4u-replay-comm.cpp>`_

  - **I/O replay:**
    Presents a set of event handlers reproducing classical I/O
    primitives (open, read, close).
    |br| `examples/s4u/replay-storage/s4u-replay-storage.cpp  <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/replay-storage/s4u-replay-storage.cpp>`_

==========================
Activities: what Actors do
==========================

Communications on the Network
-----------------------------

 - **Basic asynchronous communications:**
   Illustrates how to have non-blocking communications, that are
   communications running in the background leaving the process free
   to do something else during their completion. The main functions
   involved are :cpp:func:`simgrid::s4u::Mailbox::put_async()` and 
   :cpp:func:`simgrid::s4u::Comm::wait()`.
   |br| `examples/s4u/async-wait/s4u-async-wait.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/async-wait/s4u-async-wait.cpp>`_

 - **Waiting for all communications in a set:**
   The :cpp:func:`simgrid::s4u::Comm::wait_all()` function is useful
   when you want to block until all activities in a given set have
   completed. 
   |br| `examples/s4u/async-waitall/s4u-async-waitall.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/async-waitall/s4u-async-waitall.cpp>`_

 - **Waiting for the first completed communication in a set:**
   The :cpp:func:`simgrid::s4u::Comm::wait_any()` function is useful
   when you want to block until one activity of the set completes, no
   matter which terminates first.    
   |br| `examples/s4u/async-waitany/s4u-async-waitany.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/async-waitany/s4u-async-waitany.cpp>`_

.. todo:: add the `ready` example here
   
Executions on the CPU
---------------------

  - **Basic execution:**
    The computations done in your program are not reported to the
    simulated world, unless you explicitely request the simulator to pause
    the actor until a given amount of flops gets computed on its simulated
    host. Some executions can be given an higher priority so that they
    get more resources.
    |br| `examples/s4u/exec-basic/s4u-exec-basic.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-basic/s4u-exec-basic.cpp>`_

  - **Asynchronous execution:**
    You can start asynchronous executions, just like you would fire
    background threads.
    |br| `examples/s4u/exec-async/s4u-exec-async.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-async/s4u-exec-async.cpp>`_
    
  - **Monitoring asynchronous executions:**
    This example shows how to start an asynchronous execution, and
    monitor its status.
    |br| `examples/s4u/exec-monitor/s4u-exec-monitor.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-monitor/s4u-exec-monitor.cpp>`_
    
  - **Remote execution:**
    Before its start, you can change the host on which a given execution will occur.
    |br| `examples/s4u/exec-remote/s4u-exec-remote.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-remote/s4u-exec-remote.cpp>`_

  - **Using Pstates on a host:**
    Shows how define a set of pstatesfor a host in the XML, and how the current
    pstate can be accessed/changed with :cpp:func:`simgrid::s4u::Host::get_pstate_speed` and :cpp:func:`simgrid::s4u::Host::set_pstate`.
    |br| `examples/s4u/exec-dvfs/s4u-exec-dvfs.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-dvfs/s4u-exec-dvfs.cpp>`_
    |br| `examples/platforms/energy_platform.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/energy_platform.xml>`_

  - **Parallel tasks:**
    These objects are convenient abstractions of parallel
    computational kernels that span over several machines. 
    |br| `examples/s4u/exec-ptask/s4u-exec-ptask.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-ptask/s4u-exec-ptask.cpp>`_

I/O on Disks and Files
----------------------

SimGrid provides two levels of abstraction to interact with the
simulated storages. At the simplest level, you simply create read and
write actions on the storage resources.

  - **Access to raw storage devices:**
    This example illustrates how to simply read and write data on a
    simulated storage resource.
    |br| `examples/s4u/io-storage-raw/s4u-io-storage-raw.cpp  <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/io-storage-raw/s4u-io-storage-raw.cpp>`_

The FileSystem plugin provides a more detailed view, with the
classical operations over files: open, move, unlink, and of course
read and write. The file and disk sizes are also dealt with and can
result in short reads and short write, as in reality.

  - **File Management:**
    This example illustrates the use of operations on files
    (read, write, seek, tell, unlink, etc).
    |br| `examples/s4u/io-file-system/s4u-io-file-system.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/io-file-system/s4u-io-file-system.cpp>`_

  - **Remote I/O:**
    I/O operations on files can also be done in a remote fashion, 
    i.e. when the accessed disk is not mounted on the caller's host.
    |br| `examples/s4u/io-file-remote/s4u-io-file-remote.cpp  <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/io-file-remote/s4u-io-file-remote.cpp>`_

Classical synchronization objects
---------------------------------

 - **Mutex:**
   Shows how to use simgrid::s4u::Mutex synchronization objects.
   |br| `examples/s4u/synchro-mutex/s4u-synchro-mutex.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/synchro-mutex/s4u-synchro-mutex.cpp>`_

 - **Barrier:**
   Shows how to use simgrid::s4u::Barrier synchronization objects.
   |br| `examples/s4u/synchro-barrier/s4u-synchro-barrier.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/synchro-barrier/s4u-synchro-barrier.cpp>`_

 - **Semaphore:**
   Shows how to use simgrid::s4u::Semaphore synchronization objects.
   |br| `examples/s4u/synchro-semaphore/s4u-synchro-semaphore.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/synchro-semaphore/s4u-synchro-semaphore.cpp>`_

=============================
Interacting with the Platform
=============================

 - **Retrieving the list of hosts matching a given criteria:**
   Shows how to filter the actors that match a given criteria.
   |br| `examples/s4u/engine-filtering/s4u-engine-filtering.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/engine-filtering/s4u-engine-filtering.cpp>`_

 - **User-defined properties:**
   You can attach arbitrary information to most platform elements from
   the XML file, and then interact with these values from your
   program. Note that the changes are not written permanently on disk,
   in the XML file nor anywhere else. They only last until the end of
   your simulation.
   
   - :cpp:func:`simgrid::s4u::Actor::get_property()` and :cpp:func:`simgrid::s4u::Actor::set_property()`
   - :cpp:func:`simgrid::s4u::Host::get_property()` and :cpp:func:`simgrid::s4u::Host::set_property()`
   - :cpp:func:`simgrid::s4u::Link::get_property()` and :cpp:func:`simgrid::s4u::Link::set_property()`
   - :cpp:func:`simgrid::s4u::NetZone::get_property()` and :cpp:func:`simgrid::s4u::NetZone::set_property()`
     
   |br| `examples/s4u/platform-properties/s4u-platform-properties.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/platform-properties/s4u-platform-properties.cpp>`_
   |br| `examples/s4u/platform-properties/s4u-platform-properties_d.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/platform-properties/s4u-platform-properties_d.xml>`_
   |br| `examples/platforms/prop.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/prop.xml>`_

=================
Energy Simulation
=================

  - **Describing the energy profiles in the platform:**
    This platform file contains the energy profile of each links and
    hosts, which is necessary to get energy consumption predictions.
    As usual, you should not trust our example, and you should strive
    to double-check that your instanciation matches your target platform.
    |br| `examples/platforms/energy_platform.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/energy_platform.xml>`_

  - **Consumption due to the CPU:** 
    This example shows how to retrieve the amount of energy consumed
    by the CPU during computations, and the impact of the pstate.
    |br| `examples/s4u/energy-exec/s4u-energy-exec.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/energy-exec/s4u-energy-exec.cpp>`_

  - **Consumption due to the network:**
    This example shows how to retrieve and display the energy consumed
    by the network during communications.
    |br| `examples/s4u/energy-link/s4u-energy-link.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/energy-link/s4u-energy-link.cpp>`_

  - **Modeling the shutdown and boot of hosts:**
    Simple example of model of model for the energy consumption during
    the host boot and shutdown periods.
    |br| `examples/s4u/energy-boot/platform_boot.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/energy-boot/platform_boot.xml>`_
    |br| `examples/s4u/energy-boot/s4u-energy-boot.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/energy-boot/s4u-energy-boot.cpp>`_

=======================
Tracing and Visualizing
=======================

Tracing can be activated by various configuration options which
are illustrated in these example. See also the 
:ref:`full list of options related to tracing <tracing_tracing_options>`.

It is interesting to run the process-create example with the following
options to see the task executions:

  - **Platform Tracing:**
    This program is a toy example just loading the platform, so that
    you can play with the platform visualization. Recommanded options:
    ``--cfg=tracing:yes --cfg=tracing/categorized:yes``
    |br| `examples/s4u/trace-platform/s4u-trace-platform.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/trace-platform/s4u-trace-platform.cpp>`_

========================
Larger SimGrid Examplars
========================

This section contains application examples that are somewhat larger
than the previous examples.

  - **Ping Pong:**
    This simple example just sends one message back and forth.
    The tesh file laying in the directory show how to start the simulator binary, highlighting how to pass options to 
    the simulators (as detailed in Section :ref:`options`). 
    |br| `examples/s4u/app-pingpong/s4u-app-pingpong.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/app-pingpong/s4u-app-pingpong.cpp>`_

  - **Token ring:**
    Shows how to implement a classical communication pattern, where a
    token is exchanged along a ring to reach every participant.
    |br| `examples/s4u/app-token-ring/s4u-app-token-ring.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/app-token-ring/s4u-app-token-ring.cpp>`_

  - **Master Workers:**
    Another good old example, where one Master process has a bunch of task to dispatch to a set of several Worker 
    processes. This example comes in two equivalent variants, one
    where the actors are specified as simple functions (which is easier to
    understand for newcomers) and one where the actors are specified
    as classes (which is more powerful for the users wanting to build
    their own projects upon the example).
    |br| `examples/s4u/app-masterworkers/s4u-app-masterworkers-class.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/app-masterworkers/s4u-app-masterworkers-class.cpp>`_
    |br| `examples/s4u/app-masterworkers/s4u-app-masterworkers-fun.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/app-masterworkers/s4u-app-masterworkers-fun.cpp>`_
    
Data diffusion
--------------

  - **Bit Torrent:** 
    Classical protocol for Peer-to-Peer data diffusion.
    |br| `examples/s4u/app-bittorrent/s4u-bittorrent.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/app-bittorrent/s4u-bittorrent.cpp>`_
    
  - **Chained Send:** 
    Data broadcast over a ring of processes.
    |br| `examples/s4u/app-chainsend/s4u-app-chainsend.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/app-chainsend/s4u-app-chainsend.cpp>`_

Distributed Hash Tables (DHT)
-----------------------------

  - **Chord Protocol** 
    One of the most famous DHT protocol.
    |br| `examples/s4u/dht-chord/s4u-dht-chord.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/dht-chord/s4u-dht-chord.cpp>`_

.. TODO:: document here the examples about plugins

.. |br| raw:: html

   <br />
