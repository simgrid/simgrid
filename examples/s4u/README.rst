.. S4U (Simgrid for you) is the modern interface of SimGrid, which new project should use.
..
.. This file follows the ReStructured syntax to be included in the
.. documentation, but it should remain readable directly.


S4U Examples
************

SimGrid comes with an extensive set of examples, documented on this
page. Most of them only demonstrate one single feature, with some
larger examplars listed below. 

The C++ examples can be found under examples/s4u while python examples
are in examples/python. Each such directory contains the source code (also listed
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

.. _s4u_ex_actors:

Starting and Stoping Actors
---------------------------

  - **Creating actors:**
    Most actors are started from the deployment XML file, because this
    is a :ref:`better scientific habbit <howto_science>`, but you can
    also create them directly from your code.

    .. tabs::
    
       .. group-tab:: C++
       
          You create actors either:
             
          - Directly with :cpp:func:`simgrid::s4u::Actor::create`
          - From XML with :cpp:func:`simgrid::s4u::Engine::register_actor` (if your actor is a class)
            or :cpp:func:`simgrid::s4u::Engine::register_function` (if your actor is a function)
            and then :cpp:func:`simgrid::s4u::Engine::load_deployment`
             
          .. showfile:: examples/s4u/actor-create/s4u-actor-create.cpp
             :language: cpp
             
       .. group-tab:: Python
       
          You create actors either:
            
          - Directly with :py:func:`simgrid.Actor.create()`
          - From XML with :py:func:`simgrid.Engine.register_actor()` and then :py:func:`simgrid.Engine.load_deployment()`
               
          .. showfile:: examples/python/actor-create/actor-create.py 
             
       .. group-tab:: XML
       
          The following file is used in both C++ and Python.
          
          .. showfile:: examples/python/actor-create/actor-create_d.xml
             :language: xml

  - **React to the end of actors:** You can attach callbacks to the end of
    actors. There is several ways of doing so, depending on whether you want to
    attach your callback to a given actor and on how you define the end of a
    given actor. User code probably want to react to the termination of an actor
    while some plugins want to react to the destruction (memory collection) of
    actors.

    .. tabs::
    
       .. group-tab:: C++

          This example shows how to attach a callback to:

          - the end of a specific actor: :cpp:func:`simgrid::s4u::this_actor::on_exit()`
          - the end of any actor: :cpp:member:`simgrid::s4u::Actor::on_termination()`
          - the destruction of any actor: :cpp:member:`simgrid::s4u::Actor::on_destruction()`

          .. showfile:: examples/s4u/actor-exiting/s4u-actor-exiting.cpp
             :language: cpp

  - **Kill actors:**
    Actors can forcefully stop other actors.
    
    .. tabs::
    
       .. group-tab:: C++
       
          See also :cpp:func:`void simgrid::s4u::Actor::kill(void)`, :cpp:func:`void simgrid::s4u::Actor::kill_all()`, :cpp:func:`simgrid::s4u::this_actor::exit`.

          .. showfile:: examples/s4u/actor-kill/s4u-actor-kill.cpp
             :language: cpp
                
       .. group-tab:: Python

          See also :py:func:`simgrid.Actor.kill`, :py:func:`simgrid.Actor.kill_all`, :py:func:`simgrid.this_actor.exit`.

          .. showfile:: examples/python/actor-kill/actor-kill.py

  - **Controling the actor life cycle from the XML:**
    You can specify a start time and a kill time in the deployment file.

    .. tabs::

       .. group-tab:: C++

          This file is not really interesting: the important matter is in the XML file.

          .. showfile:: examples/s4u/actor-lifetime/s4u-actor-lifetime.cpp
             :language: cpp

       .. group-tab:: XML

          This demonstrates the ``start_time`` and ``kill_time`` attribute of the :ref:`pf_tag_actor` tag.

          .. showfile:: examples/s4u/actor-lifetime/s4u-actor-lifetime_d.xml
             :language: xml

  - **Daemonize actors:**
    Some actors may be intended to simulate daemons that run in background. This example show how to transform a regular
    actor into a daemon that will be automatically killed once the simulation is over.
    
    .. tabs::

       .. group-tab:: C++

          See also :cpp:func:`simgrid::s4u::Actor::daemonize()` and :cpp:func:`simgrid::s4u::Actor::is_daemon()`.

          .. showfile:: examples/s4u/actor-daemon/s4u-actor-daemon.cpp
             :language: cpp

       .. group-tab:: Python

          See also :py:func:`simgrid.Actor.daemonize()` and :py:func:`simgrid.Actor.is_daemon()`.

          .. showfile:: examples/python/actor-daemon/actor-daemon.py
    
Inter-Actors Interactions
-------------------------

See also the examples on :ref:`inter-actors communications
<s4u_ex_communication>` and the ones on :ref:`classical
synchronization objects <s4u_ex_IPC>`.

  - **Suspend and Resume actors:**    
    Actors can be suspended and resumed during their executions.

    - |cpp| `examples/s4u/actor-suspend/s4u-actor-suspend.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-suspend/s4u-actor-suspend.cpp>`_
      :cpp:func:`simgrid::s4u::this_actor::suspend()`,
      :cpp:func:`simgrid::s4u::Actor::suspend()`, :cpp:func:`simgrid::s4u::Actor::resume()`, :cpp:func:`simgrid::s4u::Actor::is_suspended()`.
    - |py|  `examples/python/actor-suspend/actor-suspend.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/actor-suspend/actor-suspend.py>`_
      :py:func:`simgrid.this_actor.suspend()`,
      :py:func:`simgrid.Actor.suspend()`, :py:func:`simgrid.Actor.resume()`, :py:func:`simgrid.Actor.is_suspended()`.

  - **Migrating Actors:**
    Actors can move or be moved from a host to another very easily.
    
    - |cpp| `examples/s4u/actor-migrate/s4u-actor-migrate.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-migrate/s4u-actor-migrate.cpp>`_
      :cpp:func:`simgrid::s4u::this_actor::migrate()`
    - |py|  `examples/python/actor-migrate/actor-migrate.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/actor-migrate/actor-migrate.py>`_
      :py:func:`simgrid.this_actor.migrate()`

  - **Waiting for the termination of an actor:** (joining on it)
    You can block the current actor until the end of another actor.
    
    - |cpp| `examples/s4u/actor-join/s4u-actor-join.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-join/s4u-actor-join.cpp>`_
      :cpp:func:`simgrid::s4u::Actor::join()`
    - |py|  `examples/python/actor-join/actor-join.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/actor-join/actor-join.py>`_
      :py:func:`simgrid.Actor.join()`

  - **Yielding to other actors**.
    The ```yield()``` function interrupts the execution of the current
    actor, leaving a chance to the other actors that are ready to run
    at this timestamp.
    
    - |cpp| `examples/s4u/actor-yield/s4u-actor-yield.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/actor-yield/s4u-actor-yield.cpp>`_
      :cpp:func:`simgrid::s4u::this_actor::yield()`
    - |py|  `examples/python/actor-yield/actor-yield.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/actor-yield/actor-yield.py>`_
      :py:func:`simgrid.this_actor.yield_()`

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
    |br| `examples/s4u/replay-io/s4u-replay-io.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/replay-io/s4u-replay-io.cpp>`_

==========================
Activities: what Actors do
==========================

.. _s4u_ex_communication:

Communications on the Network
-----------------------------

 - **Basic asynchronous communications:**
   Illustrates how to have non-blocking communications, that are
   communications running in the background leaving the process free
   to do something else during their completion. 
   
   - |cpp| `examples/s4u/async-wait/s4u-async-wait.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/async-wait/s4u-async-wait.cpp>`_
     :cpp:func:`simgrid::s4u::Mailbox::put_async()` and :cpp:func:`simgrid::s4u::Comm::wait()`
   - |py|  `examples/python/async-wait/async-wait.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/async-wait/async-wait.py>`_
     :py:func:`simgrid.Mailbox.put_async()` :py:func:`simgrid.Comm.wait()`

 - **Waiting for all communications in a set:**
   The `wait_all()` function is useful when you want to block until
   all activities in a given set have completed. 
   
   - |cpp| `examples/s4u/async-waitall/s4u-async-waitall.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/async-waitall/s4u-async-waitall.cpp>`_
     :cpp:func:`simgrid::s4u::Comm::wait_all()`
   - |py| `examples/python/async-waitall/async-waitall.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/async-waitall/async-waitall.py>`_
     :py:func:`simgrid.Comm.wait_all()`

 - **Waiting for the first completed communication in a set:**
   The `wait_any()` function is useful
   when you want to block until one activity of the set completes, no
   matter which terminates first.
   
   - |cpp| `examples/s4u/async-waitany/s4u-async-waitany.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/async-waitany/s4u-async-waitany.cpp>`_
     :cpp:func:`simgrid::s4u::Comm::wait_any()`
   - |py| `examples/python/async-waitany/async-waitany.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/async-waitany/async-waitany.py>`_
     :py:func:`simgrid.Comm.wait_any()`
     
.. todo:: review the `ready` and `waituntil` examples and add them here.
   
.. _s4u_ex_execution:

Executions on the CPU
---------------------

  - **Basic execution:**
    The computations done in your program are not reported to the
    simulated world, unless you explicitly request the simulator to pause
    the actor until a given amount of flops gets computed on its simulated
    host. Some executions can be given an higher priority so that they
    get more resources.
    
    - |cpp| `examples/s4u/exec-basic/s4u-exec-basic.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-basic/s4u-exec-basic.cpp>`_
    - |py|  `examples/python/exec-basic/exec-basic.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/exec-basic/exec-basic.py>`_

  - **Asynchronous execution:**
    You can start asynchronous executions, just like you would fire
    background threads.
    
    - |cpp| `examples/s4u/exec-async/s4u-exec-async.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-async/s4u-exec-async.cpp>`_
    - |py|  `examples/python/exec-async/exec-async.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/exec-async/exec-async.py>`_
    
  - **Remote execution:**
    You can start executions on remote hosts, or even change the host
    on which they occur during their execution.
    
    - |cpp| `examples/s4u/exec-remote/s4u-exec-remote.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-remote/s4u-exec-remote.cpp>`_
    - |py| `examples/python/exec-remote/exec-remote.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/exec-remote/exec-remote.py>`_

  - **Parallel executions:**
    These objects are convenient abstractions of parallel
    computational kernels that span over several machines, such as a
    PDGEM and the other ScaLAPACK routines. Note that this only works
    with the "ptask_L07" host model (``--cfg=host/model:ptask_L07``).
    |br| `examples/s4u/exec-ptask/s4u-exec-ptask.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-ptask/s4u-exec-ptask.cpp>`_
    
  - **Using Pstates on a host:**
    `examples/platforms/energy_platform.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/energy_platform.xml>`_
    shows how define a set of pstates in the XML. The current pstate
    of an host can then be accessed and changed from the program.

    - |cpp| `examples/s4u/exec-dvfs/s4u-exec-dvfs.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-dvfs/s4u-exec-dvfs.cpp>`_
      :cpp:func:`simgrid::s4u::Host::get_pstate_speed` and :cpp:func:`simgrid::s4u::Host::set_pstate`.
    - |py|  `examples/python/exec-dvfs/exec-dvfs.py <https://framagit.org/simgrid/simgrid/tree/master/examples/python/exec-dvfs/exec-dvfs.py>`_
      :py:func:`Host.get_pstate_speed` and :py:func:`Host.set_pstate`.

I/O on Disks and Files
----------------------

SimGrid provides two levels of abstraction to interact with the
simulated disks. At the simplest level, you simply create read and
write actions on the disk resources.

  - **Access to raw disk devices:**
    This example illustrates how to simply read and write data on a
    simulated disk resource.
    |br| `examples/s4u/io-disk-raw/s4u-io-disk-raw.cpp  <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/io-disk-raw/s4u-io-disk-raw.cpp>`_

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

.. _s4u_ex_IPC:

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

 - **Specifying state profiles:** shows how to specify when the
   resources must be turned off and on again, and how to react to such
   failures in your code.
   
   |br| `examples/platforms/small_platform_failures.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/small_platform_failures.xml>`_
   |br| The state profiles in `examples/platforms/profiles <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/profiles>`_

 - **Specifying speed profiles:** shows how to specify an external
   load to resources, variating their peak speed over time.
   
   |br| `examples/platforms/small_platform_profile.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/small_platform_profile.xml>`_
   |br| The speed, bandwidth and latency profiles in `examples/platforms/profiles  <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/profiles>`_

=================
Energy Simulation
=================

  - **Describing the energy profiles in the platform:**
    This platform file contains the energy profile of each links and
    hosts, which is necessary to get energy consumption predictions.
    As usual, you should not trust our example, and you should strive
    to double-check that your instantiation matches your target platform.
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

.. _s4u_ex_clouds:

Simulating Clouds
-----------------

  - **Cloud basics**
    This example starts some computations both on PMs and VMs, and
    migrates some VMs around.
    |br| `examples/s4u/cloud-simple/s4u-cloud-simple.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/cloud-simple/s4u-cloud-simple.cpp>`_

.. TODO:: document here the examples about clouds and plugins

=======================
Model-Checking Examples
=======================

The model-checker can be used to exhaustively search for issues in the
tested application. It must be activated at compile time, but this
mode is rather experimental in SimGrid (as of v3.22). You should not
enable it unless you really want to formally verify your applications:
SimGrid is slower and maybe less robust when MC is enabled.

  - **Failing assert**
    In this example, two actors send some data to a central server,
    which asserts that the messages are always received in the same order.
    This is obviously wrong, and the model-checker correctly finds a
    counter-example to that assertion.
    |br| `examples/s4u/mc-failing-assert/s4u-mc-failing-assert.cpp <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/mc-failing-assert/s4u-mc-failing-assert.cpp>`_

.. |br| raw:: html

   <br />

.. |cpp| image:: /img/lang_cpp.png
   :align: middle
   :width: 12

.. |py| image:: /img/lang_python.png
   :align: middle
   :width: 12
