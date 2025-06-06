.. S4U (SimGrid for you) is the modern interface of SimGrid, which new project should use.
..
.. This file follows the ReStructured syntax to be included in the
.. documentation, but it should remain readable directly.

.. _s4u_examples:

S4U Examples
############

SimGrid comes with an extensive set of examples, documented on this
page. Most of them only demonstrate one single feature, with some
larger exemplars listed below.

The C++ examples can be found under examples/cpp while python examples
are in examples/python. Each such directory contains the source code (also listed
from this page), and the so-called tesh file containing how to call
the binary obtained by compiling this example and also the expected
output. Tesh files are used to turn each of our examples into an
integration test. Some examples also contain other files, on need.

A good way to bootstrap your own project is to copy and combine some
of the provided examples to constitute the skeleton of what you plan
to simulate.

.. _s4u_ex_actors:

***************************
Actors: the Active Entities
***************************

Starting and Stopping Actors
============================

.. _s4u_ex_actors_create:

Creating actors
---------------

Most actors are started from the deployment XML file because this
is a :ref:`better scientific habit <howto_science>`, but you can
also create them directly from your code.

.. tabs::

   .. example-tab:: examples/cpp/actor-create/s4u-actor-create.cpp

      You create actors either:

      - Directly with :cpp:func:`simgrid::s4u::Actor::create`
      - From XML with :cpp:func:`simgrid::s4u::Engine::register_actor` (if your actor is a class)
        or :cpp:func:`simgrid::s4u::Engine::register_function` (if your actor is a function)
        and then :cpp:func:`simgrid::s4u::Engine::load_deployment`

   .. example-tab:: examples/python/actor-create/actor-create.py

      You create actors either:

      - Directly with :py:func:`simgrid.Actor.create()`
      - From XML with :py:func:`simgrid.Engine.register_actor()` and then :py:func:`simgrid.Engine.load_deployment()`

   .. example-tab:: examples/c/actor-create/actor-create.c

      You create actors either:

      - Directly with :cpp:func:`sg_actor_create` followed by :cpp:func:`sg_actor_start`.
      - From XML with :cpp:func:`simgrid_register_function` and then :cpp:func:`simgrid_load_deployment`.

   .. example-tab:: examples/java/actor_create/actor_create.java

   .. example-tab:: examples/python/actor-create/actor-create_d.xml

      This configuration file is all languages.

Reacting to actors' end
-----------------------

You can attach callbacks to the end of actors. There are several ways of doing so, depending on whether you want to
attach your callback to a given actor and on how you define the end of a
given actor. User code probably wants to react to the termination of an actor
while some plugins want to react to the destruction (memory collection) of
actors.

.. tabs::

   .. example-tab:: examples/cpp/actor-exiting/s4u-actor-exiting.cpp

      This example shows how to attach a callback to:

      - the end of a specific actor: :cpp:func:`simgrid::s4u::Actor::on_exit()`
      - the end of any actor: :cpp:func:`simgrid::s4u::Actor::on_termination_cb`
      - the destruction of any actor: :cpp:func:`simgrid::s4u::Actor::on_destruction_cb`

   .. example-tab:: examples/c/actor-exiting/actor-exiting.c

      This example shows how to attach a callback to the end of a specific actor with
      :cpp:func:`sg_actor_on_exit()`.

   .. example-tab:: examples/java/actor_exiting/actor_exiting.java

Killing actors
--------------

Actors can forcefully stop other actors.

.. tabs::

   .. example-tab:: examples/cpp/actor-kill/s4u-actor-kill.cpp

      See also :cpp:func:`void simgrid::s4u::Actor::kill(void)`, :cpp:func:`void simgrid::s4u::Actor::kill_all()`,
      :cpp:func:`simgrid::s4u::this_actor::exit`, :cpp:func:`simgrid::s4u::Actor::on_exit`.

   .. example-tab:: examples/python/actor-kill/actor-kill.py

      See also :py:func:`simgrid.Actor.kill()`, :py:func:`simgrid.Actor.kill_all()`, :py:func:`simgrid.this_actor.exit()`,
      :py:func:`simgrid.this_actor.on_exit`.

   .. example-tab:: examples/c/actor-kill/actor-kill.c

      See also :cpp:func:`sg_actor_kill`, :cpp:func:`sg_actor_kill_all`, :cpp:func:`sg_actor_exit`, :cpp:func:`sg_actor_on_exit`.

   .. example-tab:: examples/java/actor_kill/actor_kill.java

Actors' life cycle from XML_reference
-------------------------------------

You can specify a start time and a kill time in the deployment file.

.. tabs::

   .. example-tab:: examples/cpp/actor-lifetime/s4u-actor-lifetime.cpp

      This file is not really interesting: the important matter is in the XML file.

   .. example-tab:: examples/cpp/actor-lifetime/s4u-actor-lifetime_d.xml

      This demonstrates the ``start_time`` and ``kill_time`` attribute of the :ref:`pf_tag_actor` tag.

   .. example-tab:: examples/python/actor-lifetime/actor-lifetime.py

      This file is not really interesting: the important matter is in the XML file.

   .. example-tab:: examples/c/actor-lifetime/actor-lifetime.c

      This file is not really interesting: the important matter is in the XML file.

   .. example-tab:: examples/java/actor_lifetime/actor_lifetime.java

      This file is not really interesting: the important matter is in the XML file.

Daemon actors
-------------

Some actors may be intended to simulate daemons that run in the background.
This example shows how to transform a regular
actor into a daemon that will be automatically killed once the simulation is over.

.. tabs::

   .. example-tab:: examples/cpp/actor-daemon/s4u-actor-daemon.cpp

      See also :cpp:func:`simgrid::s4u::Actor::daemonize()` and :cpp:func:`simgrid::s4u::Actor::is_daemon()`.

   .. example-tab:: examples/python/actor-daemon/actor-daemon.py

      See also :py:func:`simgrid.Actor.daemonize()` and :py:func:`simgrid.Actor.is_daemon()`.

   .. example-tab:: examples/c/actor-daemon/actor-daemon.c

      See also :cpp:func:`sg_actor_daemonize` and :cpp:func:`sg_actor_is_daemon`.

   .. example-tab:: examples/java/actor_daemon/actor_daemon.java

Specifying the stack size
-------------------------

The stack size can be specified by default on the command line,
globally by changing the configuration with :cpp:func:`simgrid::s4u::Engine::set_config`,
or for a specific actor using :cpp:func:`simgrid::s4u::Actor::set_stacksize` before its start.

.. tabs::

   .. example-tab:: examples/cpp/actor-stacksize/s4u-actor-stacksize.cpp

   .. example-tab:: examples/c/actor-stacksize/actor-stacksize.c

Inter-Actors Interactions
=========================

See also the examples on :ref:`inter-actors communications
<s4u_ex_communication>` and the ones on :ref:`classical
synchronization objects <s4u_ex_IPC>`.

Suspending/resuming Actors
--------------------------

Actors can be suspended and resumed during their executions.

.. tabs::

   .. example-tab:: examples/cpp/actor-suspend/s4u-actor-suspend.cpp

      See also :cpp:func:`simgrid::s4u::this_actor::suspend()`,
      :cpp:func:`simgrid::s4u::Actor::suspend()`, :cpp:func:`simgrid::s4u::Actor::resume()`, and
      :cpp:func:`simgrid::s4u::Actor::is_suspended()`.

   .. example-tab:: examples/python/actor-suspend/actor-suspend.py

      See also :py:func:`simgrid.this_actor.suspend()`,
      :py:func:`simgrid.Actor.suspend()`, :py:func:`simgrid.Actor.resume()`, and
      :py:func:`simgrid.Actor.is_suspended()`.

   .. example-tab:: examples/c/actor-suspend/actor-suspend.c

      See also :cpp:func:`sg_actor_suspend()`, :cpp:func:`sg_actor_resume()`, and
      :cpp:func:`sg_actor_is_suspended()`.

Migrating Actors
----------------

Actors can move or be moved from a host to another very easily. It amounts to setting them on a new host.

.. tabs::

   .. example-tab:: examples/cpp/actor-migrate/s4u-actor-migrate.cpp

      See also :cpp:func:`simgrid::s4u::this_actor::set_host()` and :cpp:func:`simgrid::s4u::Actor::set_host()`.

   .. example-tab:: examples/python/actor-migrate/actor-migrate.py

      See also :py:attr:`simgrid.Actor.host`.

   .. example-tab:: examples/c/actor-migrate/actor-migrate.c

      See also :cpp:func:`sg_actor_set_host()`.

   .. example-tab:: examples/java/actor_migrate/actor_migrate.java

Waiting for the termination of an actor (joining on it)
-------------------------------------------------------

You can block the current actor until the end of another actor.

.. tabs::

   .. example-tab:: examples/cpp/actor-join/s4u-actor-join.cpp

      See also :cpp:func:`simgrid::s4u::Actor::join()`.

   .. example-tab:: examples/python/actor-join/actor-join.py

      See also :py:func:`simgrid.Actor.join()`.

   .. example-tab:: examples/c/actor-join/actor-join.c

      See also :cpp:func:`sg_actor_join`.

   .. example-tab:: examples/java/actor_join/actor_join.java

Yielding to other actors
------------------------

The ```yield()``` function interrupts the execution of the current
actor, leaving a chance to the other actors that are ready to run
at this timestamp.

.. tabs::

   .. example-tab:: examples/cpp/actor-yield/s4u-actor-yield.cpp

      See also :cpp:func:`simgrid::s4u::this_actor::yield()`.

   .. example-tab:: examples/python/actor-yield/actor-yield.py

      See also :py:func:`simgrid.this_actor.yield_()`.

   .. example-tab:: examples/c/actor-yield/actor-yield.c

      See also :cpp:func:`sg_actor_yield()`.

   .. example-tab:: examples/java/actor_yield/actor_yield.java

Traces Replay as a Workload
===========================

This section details how to run trace-driven simulations. It is very
handy when you want to test an algorithm or protocol that only reacts
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

Communication replay
--------------------

Presents a set of event handlers reproducing classical communication primitives (asynchronous send/receive at the moment).

.. tabs::

   .. example-tab:: examples/cpp/replay-comm/s4u-replay-comm.cpp

I/O replay
----------

Presents a set of event handlers reproducing classical I/O primitives (open, read, close).

.. tabs::

   .. example-tab:: examples/cpp/replay-io/s4u-replay-io.cpp

**************************
Activities: what Actors do
**************************

.. _s4u_ex_communication:

Communications on the Network
=============================

Basic communications
--------------------

This simple example just sends one message back and forth.
The tesh file laying in the directory shows how to start the simulator binary, highlighting how to pass options to
the simulators (as detailed in Section :ref:`options`).

.. tabs::

   .. example-tab:: examples/cpp/comm-pingpong/s4u-comm-pingpong.cpp

   .. example-tab:: examples/python/comm-pingpong/comm-pingpong.py

   .. example-tab:: examples/c/comm-pingpong/comm-pingpong.c

   .. example-tab:: examples/java/comm_pingpong/comm_pingpong.java

Basic asynchronous communications
---------------------------------

Illustrates how to have non-blocking communications, that are communications running in the background leaving the process
free to do something else during their completion.

.. tabs::

   .. example-tab:: examples/cpp/comm-wait/s4u-comm-wait.cpp

      See also :cpp:func:`simgrid::s4u::Mailbox::put_async()` and :cpp:func:`simgrid::s4u::Comm::wait()`.

   .. example-tab:: examples/python/comm-wait/comm-wait.py

      See also :py:func:`simgrid.Mailbox.put_async()` and :py:func:`simgrid.Comm.wait()`.

   .. example-tab:: examples/c/comm-wait/comm-wait.c

      See also :cpp:func:`sg_mailbox_put_async()` and :cpp:func:`sg_comm_wait()`.

Waiting for communications with timeouts
----------------------------------------

There is two ways of declaring timeouts in SimGrid. ``waituntil`` let you specify the deadline until when you want to wait, while
``waitfor`` expects the maximal wait duration.
This example is very similar to the previous one, simply adding how to declare timeouts when waiting on asynchronous communication.

.. tabs::

   .. example-tab:: examples/cpp/comm-waituntil/s4u-comm-waituntil.cpp

      See also :cpp:func:`simgrid::s4u::Activity::wait_until()` and :cpp:func:`simgrid::s4u::Comm::wait_for()`.

   .. example-tab:: examples/python/comm-waituntil/comm-waituntil.py

      See also :py:func:`simgrid.Comm.wait_until()`

.. _s4u_ex_mailbox_ready:

Checking for incoming communications
------------------------------------

This example uses ``Mailbox.ready()`` to check for completed communications. When this function returns true, then at least a message
is arrived, so you know that ``Mailbox.get()`` will complete immediately. This is thus another way toward asynchronous communications.

.. tabs::

   .. example-tab:: examples/cpp/comm-ready/s4u-comm-ready.cpp

      See also :cpp:func:`simgrid::s4u::Mailbox::ready()`.

   .. example-tab:: examples/python/comm-ready/comm-ready.py

      See also :py:func:`simgrid.Mailbox.ready()`


Suspending communications
-------------------------

The ``suspend()`` and ``resume()`` functions block the progression of a given communication for a while and then unblock it.
``is_suspended()`` returns whether that activity is currently blocked or not.

.. tabs::

   .. example-tab:: examples/cpp/comm-suspend/s4u-comm-suspend.cpp

      See also :cpp:func:`simgrid::s4u::Activity::suspend()`
      :cpp:func:`simgrid::s4u::Activity::resume()` and
      :cpp:func:`simgrid::s4u::Activity::is_suspended()`.

   .. example-tab:: examples/python/comm-suspend/comm-suspend.py

      See also :py:func:`simgrid.Comm.suspend()` and
      :py:func:`simgrid.Comm.resume()`.

   .. example-tab:: examples/java/actor_suspend/actor_suspend.java

.. _s4u_ex_comm_failure:

Dealing with network failures
-----------------------------

This examples shows how to survive to network exceptions that occurs when a link is turned off, or when the actor with whom
you communicate fails because its host is turned off. In this case, any blocking operation such as ``put``, ``get`` or
``wait`` will raise an exception that you can catch and react to. See also :ref:`howto_churn`,
:ref:`this example <s4u_ex_platform_state_profile>` on how to attach a state profile to hosts and
:ref:`that example <s4u_ex_exec_failure>` on how to react to host failures.

.. tabs::

   .. example-tab:: examples/cpp/comm-failure/s4u-comm-failure.cpp

   .. example-tab:: examples/python/comm-failure/comm-failure.py

.. _s4u_ex_comm_host2host:

Direct host-to-host communication
---------------------------------

This example demonstrates the direct communication mechanism, that allows to send data from one host to another without
relying on the mailbox mechanism.

.. tabs::

   .. example-tab:: examples/cpp/comm-host2host/s4u-comm-host2host.cpp

      See also :cpp:func:`simgrid::s4u::Comm::sendto_init()` and  :cpp:func:`simgrid::s4u::Comm::sendto_async()`.

   .. example-tab:: examples/python/comm-host2host/comm-host2host.py

      See also :py:func:`simgrid.Comm.sendto_init()` and  :py:func:`simgrid.Comm.sendto_async()`.

.. _s4u_ex_execution:

Executions on the CPU
=====================

Basic execution
---------------

The computations done in your program are not reported to the
simulated world unless you explicitly request the simulator to pause
the actor until a given amount of flops gets computed on its simulated
host. Some executions can be given a higher priority so that they
get more resources.

.. tabs::

   .. example-tab:: examples/cpp/exec-basic/s4u-exec-basic.cpp

      See also :cpp:func:`void simgrid::s4u::this_actor::execute(double)`
      and :cpp:func:`void simgrid::s4u::this_actor::execute(double, double)`.

   .. example-tab:: examples/python/exec-basic/exec-basic.py

      See also :py:func:`simgrid.this_actor.execute()`.

   .. example-tab:: examples/c/exec-basic/exec-basic.c

      See also :cpp:func:`void sg_actor_execute(double)`
      and :cpp:func:`void sg_actor_execute_with_priority(double, double)`.

Asynchronous execution
----------------------

You can start asynchronous executions, just like you would fire background threads.

.. tabs::

   .. example-tab:: examples/cpp/exec-async/s4u-exec-async.cpp

      See also :cpp:func:`simgrid::s4u::this_actor::exec_init()`,
      :cpp:func:`simgrid::s4u::Activity::start()`,
      :cpp:func:`simgrid::s4u::Activity::wait()`,
      :cpp:func:`simgrid::s4u::Activity::get_remaining()`,
      :cpp:func:`simgrid::s4u::Exec::get_remaining_ratio()`,
      :cpp:func:`simgrid::s4u::this_actor::exec_async()` and
      :cpp:func:`simgrid::s4u::Activity::cancel()`.

   .. example-tab:: examples/python/exec-async/exec-async.py

      See also :py:func:`simgrid.this_actor.exec_init()`,
      :py:func:`simgrid.Exec.start()`,
      :py:func:`simgrid.Exec.wait()`,
      :py:attr:`simgrid.Exec.remaining`,
      :py:attr:`simgrid.Exec.remaining_ratio`,
      :py:func:`simgrid.this_actor.exec_async()` and
      :py:func:`simgrid.Exec.cancel()`.

   .. example-tab:: examples/c/exec-async/exec-async.c

      See also :cpp:func:`sg_actor_exec_init()`,
      :cpp:func:`sg_exec_start()`,
      :cpp:func:`sg_exec_wait()`,
      :cpp:func:`sg_exec_get_remaining()`,
      :cpp:func:`sg_exec_get_remaining_ratio()`,
      :cpp:func:`sg_actor_exec_async()` and
      :cpp:func:`sg_exec_cancel()`,

Remote execution
----------------

You can start executions on remote hosts, or even change the host on which they occur during their execution.
This is naturally not very realistic, but it's something handy to have.

.. tabs::

   .. example-tab:: examples/cpp/exec-remote/s4u-exec-remote.cpp

      See also :cpp:func:`simgrid::s4u::Exec::set_host()`.

   .. example-tab:: examples/python/exec-remote/exec-remote.py

      See also :py:attr:`simgrid.Exec.host`.

   .. example-tab:: examples/c/exec-remote/exec-remote.c

      See also :cpp:func:`sg_exec_set_host()`.

.. _s4u_ex_ptasks:

Parallel executions
-------------------

These objects are convenient abstractions of parallel
computational kernels that span over several machines, such as a
PDGEM and the other ScaLAPACK routines. Note that this only works
with the "ptask_L07" host model (``--cfg=host/model:ptask_L07``).

This example demonstrates several kinds of parallel tasks: regular
ones, communication-only (without computation), computation-only
(without communication), synchronization-only (neither
communication nor computation). It also shows how to reconfigure a
task after its start, to change the number of hosts it runs onto.
This allows simulating malleable tasks.

.. tabs::

   .. example-tab:: examples/cpp/exec-ptask/s4u-exec-ptask.cpp

      See also :cpp:func:`simgrid::s4u::this_actor::parallel_execute()`.

   .. example-tab:: examples/python/exec-ptask/exec-ptask.py

      See also :ref:`simgrid.this_actor.parallel_execute()`

Ptasks play well with the host energy plugin, as shown in this example.
There is not much new compared to the above ptask example or the
:ref:`examples about energy <s4u_ex_energy>`. It just works.

.. tabs::

   .. example-tab:: examples/cpp/energy-exec-ptask/s4u-energy-exec-ptask.cpp

   .. example-tab:: examples/c/energy-exec-ptask/energy-exec-ptask.c

.. _s4u_ex_exec_failure:

Dealing with host failures
--------------------------

This examples shows how to survive to host failure exceptions that occur when an host is turned off. The actors do not get notified when the host
on which they run is turned off: they are just terminated in this case, and their ``on_exit()`` callback gets executed. For remote executions on
failing hosts however, any blocking operation such as ``exec`` or ``wait`` will raise an exception that you can catch and react to. See also
:ref:`howto_churn`,
:ref:`this example <s4u_ex_platform_state_profile>` on how to attach a state profile to hosts, and
:ref:`that example <s4u_ex_comm_failure>` on how to react to network failures.

.. tabs::

   .. example-tab:: examples/cpp/exec-failure/s4u-exec-failure.cpp

.. _s4u_ex_dvfs:

DVFS and pstates
----------------

This example shows how to define a set of pstates in the XML. The current pstate
of a host can then be accessed and changed from the program.

.. tabs::

   .. example-tab:: examples/cpp/exec-dvfs/s4u-exec-dvfs.cpp

      See also :cpp:func:`simgrid::s4u::Host::get_pstate_speed` and :cpp:func:`simgrid::s4u::Host::set_pstate`.

   .. example-tab:: examples/c/exec-dvfs/exec-dvfs.c

      See also :cpp:func:`sg_host_get_pstate_speed` and :cpp:func:`sg_host_set_pstate`.

   .. example-tab:: examples/python/exec-dvfs/exec-dvfs.py

      See also :py:func:`simgrid.Host.pstate_speed()` and :py:attr:`simgrid.Host.pstate`.

   .. example-tab:: examples/platforms/energy_platform.xml

      The important parts are in the :ref:`pf_tag_host` tag. The ``pstate`` attribute is the initial pstate while the ``speed`` attribute must
      be a comma-separated list of values: the speed at each pstate. This platform file also describes the ``wattage_per_state`` and
      ``wattage_off`` properties, that are used by the :ref:`plugin_host_energy` plugin.

.. _s4u_ex_disk_io:

I/O on Disks and Files
======================

SimGrid provides two levels of abstraction to interact with the
simulated disks. At the simplest level, you simply create read and
write actions on the disk resources.

Access to raw disk devices
--------------------------

This example illustrates how to simply read and write data on a simulated disk resource.

.. tabs::

   .. example-tab:: examples/cpp/io-disk-raw/s4u-io-disk-raw.cpp

   .. example-tab:: examples/c/io-disk-raw/io-disk-raw.c

   .. example-tab:: examples/platforms/hosts_with_disks.xml

      This shows how to declare disks in XML.

Asynchronous raw accesses
-------------------------

As most other activities, raw IO accesses can be used asynchronously, as illustrated in this example.

.. tabs::

   .. example-tab:: examples/cpp/io-async/s4u-io-async.cpp

Filesystem plugin
-----------------

The FileSystem plugin provides a more detailed view, with the
classical operations over files: open, move, unlink, and of course,
read and write. The file and disk sizes are also dealt with and can
result in short reads and short writes, as in reality.

  - **File Management:**
    This example illustrates the use of operations on files
    (read, write, seek, tell, unlink, etc).

    .. tabs::

       .. example-tab:: examples/cpp/io-file-system/s4u-io-file-system.cpp

       .. example-tab:: examples/c/io-file-system/io-file-system.c

  - **Remote I/O:**
    I/O operations on files can also be done remotely,
    i.e. when the accessed disk is not mounted on the caller's host.

    .. tabs::

       .. example-tab:: examples/cpp/io-file-remote/s4u-io-file-remote.cpp

       .. example-tab:: examples/c/io-file-remote/io-file-remote.c

.. _s4u_ex_activityset:

Bags of activities
==================

Sometimes, you want to block on a set of activities, getting unblocked when any activity of the set unblocks, or waiting for the
completion of all activities in the set. This is where the ActivitySet become useful.

Waiting for all activities in a set
-----------------------------------

The ``wait_all()`` function is useful when you want to block until all activities in a given set have been completed.

.. tabs::

   .. example-tab:: examples/cpp/activityset-waitall/s4u-activityset-waitall.cpp

      See also :cpp:func:`simgrid::s4u::ActivitySet::wait_all()`.

   .. example-tab:: examples/python/activityset-waitall/activityset-waitall.py

      See also :py:func:`simgrid.ActivitySet.wait_all()`.

   .. example-tab:: examples/c/activityset-waitall/activityset-waitall.c

      See also :cpp:func:`sg_activity_set_wait_all()`.

   .. example-tab:: examples/java/activityset_awaitall/activityset_awaitall.java

Waiting for all activities in a set (with timeout)
--------------------------------------------------

The ``wait_all_for()`` function is very similar to ``wait_all()`` but allows to specify a timeout.

.. tabs::

   .. example-tab:: examples/cpp/activityset-waitallfor/s4u-activityset-waitallfor.cpp

      See also :cpp:func:`simgrid::s4u::ActivitySet::wait_all_for()`.

   .. example-tab:: examples/python/activityset-waitallfor/activityset-waitallfor.py

      See also :py:func:`simgrid.ActivitySet.wait_all_for()`.

   .. example-tab:: examples/c/activityset-waitallfor/activityset-waitallfor.c

      See also :cpp:func:`sg_activity_set_wait_all_for()`.

   .. example-tab:: examples/java/activityset_awaitallfor/activityset_awaitallfor.java

Waiting for the first completed activity in a set
-------------------------------------------------

The ``wait_any()`` blocks until one activity of the set completes, no matter which terminates first.

.. tabs::

   .. example-tab:: examples/cpp/activityset-waitany/s4u-activityset-waitany.cpp

      See also :cpp:func:`simgrid::s4u::ActivitySet::wait_any()`.

   .. example-tab:: examples/python/activityset-waitany/activityset-waitany.py

      See also :py:func:`simgrid.ActivitySet.wait_any()`.

   .. example-tab:: examples/c/activityset-waitany/activityset-waitany.c

      See also :cpp:func:`sg_activity_set_wait_any`.

   .. example-tab:: examples/java/activityset_awaitany/activityset_awaitany.java

Testing whether at least one activity completed
-----------------------------------------------

The ``test_any()`` returns whether at least one activity of the set has completed.

.. tabs::

   .. example-tab:: examples/cpp/activityset-testany/s4u-activityset-testany.cpp

      See also :cpp:func:`simgrid::s4u::ActivitySet::test_any()`.

   .. example-tab:: examples/python/activityset-testany/activityset-testany.py

      See also :py:func:`simgrid.ActivitySet.test_any()`.

   .. example-tab:: examples/c/activityset-testany/activityset-testany.c

      See also :cpp:func:`sg_activity_set_test_any`.

   .. example-tab:: examples/java/activityset_testany/activityset_testany.java

.. _s4u_ex_dag:

Dependencies between activities
===============================

SimGrid makes it easy to express dependencies between activities, where a given activity cannot start until the completion of
all its predecessors. You can even have simulation not involving any actors, where the main thread (called maestro) creates and
schedules activities itself.

Simple dependencies
-------------------

When you declare dependencies between two activities, the dependent will not actually start until all its dependencies complete,
as shown in the following examples. The first one declare dependencies between executions while the second one declare
dependencies between communications. You could declare such dependencies between arbitrary activities.

.. tabs::

   .. example-tab:: examples/cpp/exec-dependent/s4u-exec-dependent.cpp

.. tabs::

   .. example-tab:: examples/cpp/comm-dependent/s4u-comm-dependent.cpp

Assigning activities
--------------------

To actually start, an activity needs to be assigned to a given resource. This examples illustrates how an execution that is not
assigned will not actually start until being assigned. In some sense, activities' assignment can be seen as a specific
dependency that can withdraw their execution.

.. tabs::

   .. example-tab:: examples/cpp/exec-unassigned/s4u-exec-unassigned.cpp

Simple DAG of activities
------------------------

This example shows how to create activities from the maestro directly without relying on an actor, organize the dependencies of
activities as a DAG (direct acyclic graph), and start them. Each activity will start as soon as its dependencies are fulfilled.

.. tabs::

   .. example-tab:: examples/cpp/dag-simple/s4u-dag-simple.cpp

DAG with communication
----------------------

This is a little example showing how add communication activities to your DAG, representing inter-task data exchanges.

.. tabs::

   .. example-tab:: examples/cpp/dag-comm/s4u-dag-comm.cpp

DAG with I/O 
------------

This is a little example showing how add I/O activities to your DAG, representing disk buffers.

.. tabs::

   .. example-tab:: examples/cpp/dag-io/s4u-dag-io.cpp

Scheduling activities
---------------------

This example illustrates a simple scheduling algorithm, where the activities are placed on the "most adapted" host. Of course, there is many way 
to determine which host is the better fit for a given activity, and this example just uses a simple algorithm.

.. tabs::

   .. example-tab:: examples/cpp/dag-scheduling/s4u-dag-scheduling.cpp

Loading DAGs from file
----------------------

There is currently two file formats that you can load directly in SimGrid, but writing another loader for your beloved format should not be difficult.

.. tabs::

   .. example-tab:: examples/cpp/dag-from-dax/s4u-dag-from-dax.cpp

   .. group-tab:: input

      .. showfile:: examples/cpp/dag-from-dax/smalldax.xml
         :language: xml

.. tabs::

   .. example-tab:: examples/cpp/dag-from-dot/s4u-dag-from-dot.cpp

   .. group-tab:: input

      .. showfile:: examples/cpp/dag-from-dot/dag.dot
         :language: xml

Simulating a time slice
-----------------------

When you declare activities, :cpp:func:`simgrid::s4u::Engine::run()` runs up to the point of time where an activity completes.
Sometimes, you want to give a maximal duration to simulate up to a given date at most, for example to inject a new activity at that time.
This example shows how to do it.

.. tabs::

   .. example-tab:: examples/cpp/engine-run-partial/s4u-engine-run-partial.cpp

DAG and failures
----------------

This example shows how to deal with host or network failures while scheduling DAGs of activities.

.. tabs::

   .. example-tab:: examples/cpp/dag-failure/s4u-dag-failure.cpp

.. _s4u_ex_IPC:

Classical synchronization objects
=================================

Barrier
-------

Shows how to use :cpp:type:`simgrid::s4u::Barrier` synchronization objects.

.. tabs::

   .. example-tab:: examples/cpp/synchro-barrier/s4u-synchro-barrier.cpp

   .. example-tab:: examples/python/synchro-barrier/synchro-barrier.py

Condition variable: basic usage
-------------------------------

Shows how to use :cpp:type:`simgrid::s4u::ConditionVariable` synchronization objects.

.. tabs::

   .. example-tab:: examples/cpp/synchro-condition-variable/s4u-synchro-condition-variable.cpp

Condition variable: timeouts
----------------------------

Shows how to specify timeouts when blocking on condition variables.

.. tabs::

   .. example-tab:: examples/cpp/synchro-condition-variable-waituntil/s4u-synchro-condition-variable-waituntil.cpp

Mutex
-----

Shows how to use :cpp:type:`simgrid::s4u::Mutex` synchronization objects.

.. tabs::

   .. example-tab:: examples/cpp/synchro-mutex/s4u-synchro-mutex.cpp

   .. example-tab:: examples/python/synchro-mutex/synchro-mutex.py

Semaphore
---------

Shows how to use :cpp:type:`simgrid::s4u::Semaphore` synchronization objects.

.. tabs::

   .. example-tab:: examples/cpp/synchro-semaphore/s4u-synchro-semaphore.cpp

   .. example-tab:: examples/python/synchro-semaphore/synchro-semaphore.py

   .. example-tab:: examples/c/synchro-semaphore/synchro-semaphore.c

*****************************
Interacting with the Platform
*****************************

User-defined properties
=======================

You can attach arbitrary information to most platform elements from the XML file, and then interact with these values from your
program. Note that the changes are not written permanently on disk, in the XML file nor anywhere else. They only last until the end of
your simulation.

.. tabs::

   .. example-tab:: examples/cpp/platform-properties/s4u-platform-properties.cpp

      - :cpp:func:`simgrid::s4u::Actor::get_property()` and :cpp:func:`simgrid::s4u::Actor::set_property()`
      - :cpp:func:`simgrid::s4u::Host::get_property()` and :cpp:func:`simgrid::s4u::Host::set_property()`
      - :cpp:func:`simgrid::s4u::Link::get_property()` and :cpp:func:`simgrid::s4u::Link::set_property()`
      - :cpp:func:`simgrid::s4u::NetZone::get_property()` and :cpp:func:`simgrid::s4u::NetZone::set_property()`

   .. example-tab:: examples/c/platform-properties/platform-properties.c

      - :cpp:func:`sg_actor_get_property_value()`
      - :cpp:func:`sg_host_get_property_value()` and :cpp:func:sg_host_set_property_value()`
      - :cpp:func:`sg_zone_get_property_value()` and :cpp:func:`sg_zone_set_property_value()`

   .. group-tab:: XML

      **Platform file:**

      .. showfile:: examples/platforms/prop.xml
         :language: xml

Element filtering
=================

Retrieving the netzones matching given criteria
-----------------------------------------------

Shows how to filter the cluster netzones.

.. tabs::

   .. example-tab:: examples/cpp/routing-get-clusters/s4u-routing-get-clusters.cpp

Retrieving the list of hosts matching given criteria
----------------------------------------------------

Shows how to filter the actors that match given criteria.

.. tabs::

   .. example-tab:: examples/cpp/engine-filtering/s4u-engine-filtering.cpp

Profiles
========

.. _s4u_ex_platform_state_profile:

Specifying state profiles
-------------------------

Shows how to specify when the resources must be turned off and on again, and how to react to such
failures in your code. See also :ref:`howto_churn`,
:ref:`this example <s4u_ex_comm_failure>` on how to react to communication failures, and
:ref:`that example <s4u_ex_exec_failure>` on how to react to host failures.

.. tabs::

   .. example-tab:: examples/cpp/platform-failures/s4u-platform-failures.cpp

   .. example-tab:: examples/c/platform-failures/platform-failures.c

   .. example-tab:: examples/python/platform-failures/platform-failures.py

   .. group-tab:: XML

      .. showfile:: examples/platforms/small_platform_failures.xml
         :language: xml

      .. showfile:: examples/platforms/profiles/jupiter_state.profile

      .. showfile:: examples/platforms/profiles/fafard_state.profile

Specifying speed profiles
-------------------------

Shows how to specify an external load to resources, variating their peak speed over time.

   .. tabs::

      .. example-tab:: examples/cpp/platform-profile/s4u-platform-profile.cpp

      .. example-tab:: examples/python/platform-profile/platform-profile.py

      .. group-tab:: XML

         .. showfile:: examples/platforms/small_platform_profile.xml
            :language: xml

         .. showfile:: examples/platforms/profiles/jupiter_speed.profile

         .. showfile:: examples/platforms/profiles/link1_bandwidth.profile

         .. showfile:: examples/platforms/profiles/link1_latency.profile

Modifying the platform
======================

Serializing communications
--------------------------

This example shows how to limit the amount of communications going through a given link.
It is very similar to the other asynchronous communication examples, but messages get serialized by the platform.
Without this call to ``Link::set_concurrency_limit(2)``, all messages would be received at the exact same timestamp since
they are initiated at the same instant and are of the same size. But with this extra configuration to the link, at most 2
messages can travel through the link at the same time.

.. tabs::

   .. example-tab:: examples/cpp/platform-comm-serialize/s4u-platform-comm-serialize.cpp

      See also :cpp:func:`simgrid::s4u::Link::set_concurrency_limit()`.

   .. example-tab:: examples/python/platform-comm-serialize/platform-comm-serialize.py

      See also :py:func:`simgrid.Link.set_concurrency_limit()`.

.. _s4u_ex_energy:

*****************
Energy Simulation
*****************

Setup
=====

Describing the energy profiles in the platform
----------------------------------------------

The first platform file contains the energy profile of each link and host for a wired network, which is necessary to get energy consumption
predictions. The second platform file is the equivalent for a wireless network. As usual, you should not trust our example, and you should
strive to double-check that your instantiation matches your target platform.

.. tabs::

   .. group-tab:: XML

.. showfile:: examples/platforms/energy_platform.xml
   :language: xml

.. showfile:: examples/platforms/wifi_energy.xml
   :language: xml

Usage
=====

CPU energy consumption
----------------------

This example shows how to retrieve the amount of energy consumed by the CPU during computations, and the impact of the pstate.

.. tabs::

   .. example-tab:: examples/cpp/energy-exec/s4u-energy-exec.cpp

   .. example-tab:: examples/c/energy-exec/energy-exec.c

Virtual machines consumption
----------------------------

This example is very similar to the previous one, adding VMs to the picture.

.. tabs::

   .. example-tab:: examples/cpp/energy-vm/s4u-energy-vm.cpp

   .. example-tab:: examples/c/energy-vm/energy-vm.c

Wired network energy consumption
--------------------------------

This example shows how to retrieve and display the energy consumed by the wired network during communications.

.. tabs::

   .. example-tab:: examples/cpp/energy-link/s4u-energy-link.cpp

WiFi network energy consumption
-------------------------------

This example shows how to retrieve and display the energy consumed by the wireless network during communications.

.. tabs::

   .. example-tab:: examples/cpp/energy-wifi/s4u-energy-wifi.cpp

Modeling the shutdown and boot of hosts
---------------------------------------

Simple example of a model for the energy consumption during the host boot and shutdown periods.

.. tabs::

   .. example-tab:: examples/platforms/energy_boot.xml

   .. example-tab:: examples/cpp/energy-boot/s4u-energy-boot.cpp

***********************
Tracing and Visualizing
***********************

Tracing can be activated by various configuration options which are illustrated in these examples. See also the
:ref:`full list of options related to tracing <tracing_tracing_options>`.
The following introduces  some option sets of interest that you may want to pass to your simulators.

.. todo::
   These tracing examples should be integrated in the examples to not duplicate the C++ files.
   A full command line to see the result in the right tool (vite/FrameSoc) should be given along with some screenshots.

Platform Tracing
================

Basic example
-------------

This program is a toy example just loading the platform so that you can play with the platform visualization. Recommended options:
``--cfg=tracing:yes --cfg=tracing/categorized:yes``

.. tabs::

   .. example-tab:: examples/cpp/trace-platform/s4u-trace-platform.cpp

Setting Categories
------------------

This example declares several tracing categories that are used to
classify its tasks. When the program is executed, the tracing mechanism
registers the resource utilization of hosts and links according to these
categories. Recommended options:
``--cfg=tracing:yes --cfg=tracing/categorized:yes --cfg=tracing/uncategorized:yes``

.. tabs::

   .. example-tab:: examples/cpp/trace-categories/s4u-trace-categories.cpp

Master Workers tracing
----------------------

This is an augmented version of our basic master/worker example using
several tracing features. It traces resource usage, sorted out in several
categories; Trace marks and user variables are also used. Recommended
options: ``--cfg=tracing/categorized:yes --cfg=tracing/uncategorized:yes``

.. tabs::

   .. example-tab:: examples/cpp/trace-masterworkers/s4u-trace-masterworkers.cpp

   .. example-tab:: examples/python/app-masterworkers/app-masterworkers.py

Process migration tracing
-------------------------

This version is enhanced so that the process migrations can be displayed
as arrows in a Gantt-chart visualization. Recommended options to that
extend: ``--cfg=tracing:yes --cfg=tracing/actor:yes``

.. tabs::

   .. example-tab:: examples/cpp/trace-process-migration/s4u-trace-process-migration.cpp

Tracing user variables
======================

You can also attach your own variables to any resource described in the platform
file. The following examples illustrate this feature.  They have to be run with
the following options: ``--cfg=tracing:yes --cfg=tracing/platform:yes``

Attaching variables to Hosts
----------------------------

.. tabs::

   .. example-tab:: examples/cpp/trace-host-user-variables/s4u-trace-host-user-variables.cpp

Attaching variables to Links
----------------------------

The tricky part is that you have to know the name of the link you want to enhance with a variable.

.. tabs::

   .. example-tab:: examples/cpp/trace-link-user-variables/s4u-trace-link-user-variables.cpp

Attaching variables to network routes
-------------------------------------

It is often easier to update a given variable for all links of a given network path (identified by its source and destination hosts) instead of
knowing the name of each specific link.

.. tabs::

   .. example-tab::  examples/cpp/trace-route-user-variables/s4u-trace-route-user-variables.cpp

************************
Larger SimGrid Exemplars
************************

This section contains application examples that are somewhat larger than the previous examples.

Classical examples
==================

Token ring
----------

Shows how to implement a classical communication pattern, where a token is exchanged along a ring to reach every participant.

.. tabs::

   .. example-tab:: examples/cpp/app-token-ring/s4u-app-token-ring.cpp

   .. example-tab:: examples/c/app-token-ring/app-token-ring.c

Master Workers
--------------

Another good old example, where one Master actor has a bunch of tasks to dispatch to a set of several Worker actors.
This example is used in the :ref:`SimGrid tutorial <usecase_simalgo>`.

.. tabs::

   .. group-tab:: C++

      This example comes in two equivalent variants, one where the actors
      are specified as simple functions (which is easier to understand for
      newcomers) and one where the actors are specified as classes (which is
      more powerful for the users wanting to build their own projects upon
      the example).

      .. showfile:: examples/cpp/app-masterworkers/s4u-app-masterworkers-class.cpp
         :language: cpp

      .. showfile:: examples/cpp/app-masterworkers/s4u-app-masterworkers-fun.cpp
         :language: cpp

   .. group-tab:: C

      .. showfile:: examples/c/app-masterworker/app-masterworker.c
         :language: cpp

   .. example-tab:: examples/python/app-masterworkers/app-masterworkers.py

Data diffusion
==============

Bit Torrent
-----------

Classical protocol for Peer-to-Peer data diffusion.

.. tabs::

   .. group-tab:: C++

      .. showfile:: examples/cpp/app-bittorrent/s4u-bittorrent.cpp
         :language: cpp

      .. showfile:: examples/cpp/app-bittorrent/s4u-peer.cpp
         :language: cpp

      .. showfile:: examples/cpp/app-bittorrent/s4u-tracker.cpp
         :language: cpp

   .. group-tab:: C

      .. showfile:: examples/c/app-bittorrent/app-bittorrent.c
         :language: cpp

      .. showfile:: examples/c/app-bittorrent/bittorrent-peer.c
         :language: cpp

      .. showfile:: examples/c/app-bittorrent/tracker.c
         :language: cpp

Chained Send
------------

Data broadcast over a ring of processes.

.. tabs::

   .. example-tab:: examples/cpp/app-chainsend/s4u-app-chainsend.cpp

   .. group-tab:: C

      .. showfile:: examples/c/app-chainsend/chainsend.c
         :language: c

      .. showfile:: examples/c/app-chainsend/broadcaster.c
         :language: c

      .. showfile:: examples/c/app-chainsend/peer.c
         :language: c

Distributed Hash Tables (DHT)
=============================

Chord Protocol
--------------

One of the most famous DHT protocol.

.. tabs::

   .. group-tab:: C++

      .. showfile:: examples/cpp/dht-chord/s4u-dht-chord.cpp
         :language: cpp

      .. showfile:: examples/cpp/dht-chord/s4u-dht-chord-node.cpp
         :language: cpp

Kademlia
--------

Another well-known DHT protocol.

.. tabs::

   .. group-tab:: C++

      .. showfile:: examples/cpp/dht-kademlia/s4u-dht-kademlia.cpp
         :language: cpp

      .. showfile:: examples/cpp/dht-kademlia/routing_table.cpp
         :language: cpp

      .. showfile:: examples/cpp/dht-kademlia/answer.cpp
         :language: cpp

      .. showfile:: examples/cpp/dht-kademlia/node.cpp
         :language: cpp

   .. group-tab:: C

      .. showfile:: examples/c/dht-kademlia/dht-kademlia.c
         :language: cpp

      .. showfile:: examples/c/dht-kademlia/routing_table.c
         :language: cpp

      .. showfile:: examples/c/dht-kademlia/answer.c
         :language: cpp

      .. showfile:: examples/c/dht-kademlia/message.c
         :language: cpp

      .. showfile:: examples/c/dht-kademlia/node.c
         :language: cpp

Pastry
------

Yet another well-known DHT protocol.

.. tabs::

   .. example-tab:: examples/c/dht-pastry/dht-pastry.c

.. _s4u_ex_clouds:

Simulating Clouds
=================

Cloud basics
------------

This example starts some computations both on PMs and VMs and migrates some VMs around.

.. tabs::

   .. example-tab:: examples/cpp/cloud-simple/s4u-cloud-simple.cpp

   .. example-tab:: examples/c/cloud-simple/cloud-simple.c

Migrating VMs
-------------

This example shows how to migrate VMs between PMs.

.. tabs::

   .. example-tab:: examples/cpp/cloud-migration/s4u-cloud-migration.cpp

   .. example-tab:: examples/c/cloud-migration/cloud-migration.c

***********************
Model-Related Examples
***********************

ns-3 as a model
===============

This simple ping-pong example demonstrates how to use the bindings to the Network
Simulator. The most interesting is probably not the C++ files since
they are unchanged from the other simulations, but the associated files,
such as the platform file to see how to declare a platform to be used
with the ns-3 bindings of SimGrid and the tesh file to see how to
start a simulation in these settings.

.. tabs::

   .. example-tab:: examples/cpp/network-ns3/s4u-network-ns3.cpp

   .. group-tab:: XML

      **Platform files:**

      .. showfile:: examples/platforms/small_platform_one_link_routes.xml
         :language: xml

WiFi links
==========

This demonstrates how to declare a wifi zone in your platform and
how to use it in your simulation. For that, you should have a link
whose sharing policy is set to `WIFI`. Such links can have more
than one bandwidth value (separated by commas), corresponding to
the several SNR level of your wifi link.

In this case, SimGrid automatically switches to validated
performance models of wifi networks, where the time is shared
between users instead of the bandwidth for wired links (the
corresponding publication is currently being written).

If your wifi link provides more than one SNR level, you can switch
the level of a given host using
:cpp:func:`simgrid::s4u::Link::set_host_wifi_rate`. By default,
the first level is used.

.. tabs::

   .. example-tab:: examples/cpp/network-wifi/s4u-network-wifi.cpp

   .. group-tab:: XML

      **Platform files:**

      .. showfile:: examples/platforms/wifi.xml
         :language: xml

You can also use the **ns-3 models on your wifi networks** as follows:

.. tabs::

   .. example-tab:: examples/cpp/network-ns3-wifi/s4u-network-ns3-wifi.cpp

   .. group-tab:: XML

      **Platform files:**

      .. showfile:: examples/platforms/wifi_ns3.xml
         :language: xml


***************
Plugin Examples
***************

It is possible to extend SimGrid without modifying its internals by
attaching code to the existing signals and by adding extra data to the
simulation objects through extensions. How to do that is not exactly
documented yet, and you should look for examples in the src/plugins
directory.

This section documents how the existing plugins can be used. Remember
that you are very welcome to modify the plugins to fit your needs. It
should be much easier than modifying the SimGrid kernel.

Monitoring the host load
========================

.. tabs::

   .. example-tab:: examples/cpp/plugin-host-load/s4u-plugin-host-load.cpp

   .. example-tab:: examples/c/plugin-host-load/plugin-host-load.c

Monitoring the link load
========================

.. tabs::

   .. example-tab:: examples/cpp/plugin-link-load/s4u-plugin-link-load.cpp

***********************
Model-Checking examples
***********************

The model-checker can be used to exhaustively search for issues in the tested application. It must be activated at compile-time, but this
mode is rather experimental in SimGrid (as of v3.25). We are working on it :)

Failing assert
==============

In this example, two actors send some data to a central server, which asserts that the messages are always received in the same order.
This is wrong, and the model-checker correctly finds a counter-example to that assertion.

.. tabs::

   .. example-tab:: examples/cpp/mc-failing-assert/s4u-mc-failing-assert.cpp

*****************
Advanced examples
*****************

.. _s4u_ex_actor_attach:

Changing maestro's thread
=========================

Usually, SimGrid's maestro executes in the main thread of your application, meaning that the main thread is in charge of
initializing the simulation and then scheduling the activities. If you really need it, it is possible to move away maestro in
another system thread, for example because another library absolutely wants to run as the system main thread. The following
example shows how to do that, using  :cpp:func:`sg_actor_attach` at the begining and :cpp:func:`sg_actor_detach` on
termination.

.. tabs::

   .. example-tab:: examples/cpp/maestro-set/s4u-maestro-set.cpp

.. |br| raw:: html

   <br />
