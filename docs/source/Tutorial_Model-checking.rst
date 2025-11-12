.. _usecase_modelchecking:

Formal Verification and Model checking
======================================

Mc SimGrid is a full-featured model-checker that is embedded in the SimGrid framework. It can be used to formally verify safety
properties on codes running on top of SimGrid, be them classical pthread applications, :ref:`simple algorithms
<usecase_simalgo>`, or :ref:`full MPI applications <usecase_smpi>`. This module is under heavy development since 15 years now.
It is less stable than the parts of SimGrid intended to predict the performance of your application, but should still be usable
if you're motivated and patient with us.

Primer on formal methods
------------------------

Formal methods are techniques leveraging mathematics to test and assess systems. They are routinely used to assess computer
hardware, transportation systems or any other complex engineering process. Among these methods, model checking is a technique to
automatically prove that a given model verifies a given property by systematically checking all states of the model. The
property and model are written in a mathematical language and fed to an automated tool called model checker.  When the model
does not verify the property, the model checker gives a counter-example that can be used to refine and improve the model.
Conversely, if no counter-example can be found after an exhaustive exploration of the model, we know that the property holds for
the model. It may also happen that the model is too large to be exhaustively explored, in which case the model checker is not
conclusive. Model checkers rely on so-called reduction techniques (based on symmetries and equivalence) to efficiently explore
the system state. `Spin <https://spinroot.com/spin/whatispin.html>`_ is a renowned model checker while `TLA+
<https://en.wikipedia.org/wiki/TLA%2B>`_ is a mathematical language that can be used in such context.

Software model checking applies similar ideas to programs, without requiring a mathematical model of the system. Instead, the
program itself is used as a model to verify against a property. Along these lines, Mc SimGrid is a stateless model checker: it
does not leverage static analysis nor symbolic execution. Instead, the program is simply executed through all possible outcomes.
On indecision points, the application is forked (in the `UNIX meaning <https://en.wikipedia.org/wiki/Fork_(system_call)>`_), the
first branch is executed exhaustively, and then the other fork of the system is used to explore the other execution branch.

Mc SimGrid targets parallel and distributed applications that interact through message passing or through synchronization
mechanisms (mutex, barrier, etc). It can be used to find crash and other property violations even if they only occur under rare
conditions. Starting with v4.1, it can also detect any race conditions under some conditions. In addition to these classical
`safety properties <https://en.wikipedia.org/wiki/Linear_time_property>`_, it can also ensure `communication determinism
<https://hal.inria.fr/hal-01953167/document>`_, a property that allows more efficient solutions toward fault-tolerance. It can
alleviate the state space explosion problem through several variants of `Dynamic Partial Ordering Reduction (DPOR)
<https://en.wikipedia.org/wiki/Partial_order_reduction>`_. Note that Mc SimGrid is currently less mature than other parts of the
framework, but it improves every month. Please report any question and issue so that we can further improve it.

Instrumentation solutions
^^^^^^^^^^^^^^^^^^^^^^^^^

The model checker can only find bugs related to the applicative events it can observe.  Several partially compatible solutions
exist observe events in Mc SimGrid. The first instrumentation solution leverage :ref:`SMPI <usecase_smpi>` to detect misuses of
the MPI interface. It can observe communications done with MPI, but not thread operations nor memory accesses. The second
instrumentation solution is called ``sthread``. This library can be injected with `LD_PRELOAD
<https://en.wikipedia.org/wiki/Dynamic_linker#Systems_using_ELF>`_ into the applications to observe thread operations and
several synchronizations (mutex, barrier, semaphores, etc).

Prior to SimGrid v4.1, memory accesses could not be observed, preventing Mc SimGrid from the detection of memory race conditions
and other bugs that can only be observed for specific ordering of the memory read and write events. As such, It could only be
used to detect misuses of the synchronization functions such as pthread calls or MPI calls (depending on the leveraged
instrumentation solution), such as the ones resulting in deadlocks.

Starting with v4.1 (not released at the time of writing), read and write memory events can be observed in programs compiled with
our ``mclang`` compilation wrapper that adds an extra pass to the ``clang`` compiler that tracks memory accesses at runtime. The
memory events are not interleaved explicitly. Instead, Mc SimGrid uses a race detector inspired from `FastTrack
<https://dl.acm.org/doi/abs/10.1145/1543135.1542490>`_ to detect races on the explored executions.


Limits
^^^^^^

The first limit lies in the observation of events. Many applicative events remain unobserved. Communications are not observed if
they don't use the MPI standard; Disk accesses are left completely unobserved, and so are the time-related APIs, to name a few.

The second practical limit lies in the huge amount of scenarios to explore. SimGrid tries to explore only non-redundant scenarios
thanks to classical reduction techniques (such as DPOR) but the exploration may well never finish if you don't carefully adapt
your application to this mode. The amount of event interleavings to explore grows exponentially with the amount of actors and
amount of events in each actor's history. Keep your program small to fully explore your scenarios.

A classical trap is that the model checker can only verify whether your application fits the properties provided, which is
useless if you have a bug in your property. Remember also that one way for your application to never violate a given assertion
is to not start at all, because of a stupid bug.

Another limit of this mode is that time becomes discrete in the model checker: You can say for example that the application took
42 steps to run, but there is no way to know how much time it took or the number of watts that were dissipated.

The model checker may well miss existing issues, as it computes the possible outcomes *from a given initial situation*. This
approach is not adapted to prove the correctness of your application in full generality.

Getting Mc SimGrid
------------------

The model checker is included in the SimGrid source code, and it's compiled by default with no extra dependencies. It's also
activated in the Debian package since v3.36-2. If your precompiled version of SimGrid lacks support for the model checker, you
could build SimGrid from source. Simply request it from cmake (``cmake -Denable_model-checking .``) and then compile SimGrid
:ref:`as usual <install_src>`. It should work out of the box on all major systems (Linux, FreeBSD, Windows with WSL, or Mac OS
X). Double check the value of the ``SIMGRID_HAVE_MC`` in the generated file ``include/simgrid/config.h``. If it's not defined to
1, read the configuration logs to understand why the model checker was not compiled in, and try again. 

To test SimGrid without installing it, you can use Docker. You can either pull the ``simgrid/tuto-mc`` (3.3Gb) image that
contains a full SimGrid installation and all needed files, or the ``simgrid/mc-slim`` image (850Mb) that contains the bare
minimum to use the model-checker. That's still quite a large image, but we need a complete clang installation in the docker to
recompile the C/C++ code with SimGrid.

.. tabs::

   .. group-tab:: simgrid/mc-slim

      If you go for the small image, the following script ``simgrid.sh`` will execute a command within the docker.
   
      .. code-block:: shell
   
         #! /bin/sh
   
         exec docker run --rm -i -v $PWD:$PWD -w $PWD -u $UID:$UID simgrid/mc-slim $*
   
      The ``-v $PWD:$PWD`` makes the current directory visible from within the docker while ``-w $PWD`` uses that directory as a
      working directory. ``-u $UID:$UID`` uses your identity within the docker so that the files created within the directory
      have the right ownership and permissions. Add ``--ulimit nofile=1024:1024`` if you want to run valgrind within the
      container, but it's not installed for now.
   
      Make the ``simgrid.sh`` executable with a ``chmod``. Once it's done, you can use it to run a command within the docker.
   
      .. code-block:: console
   
         $ chmod +x simgrid.sh
         $ ./simgrid.sh hostname
         01e5b887ce44  # this is the name of the docker container. It will thus change each time you run it
         $ hostname
           (here comes the name of your linux system. It will not change with time)
         $ ./simgrid.sh clang --version
           (here comes the clang version installed in the docker container -- 19.1.7 as of writing)
         $ clang --version
           (here comes the clang version on your machine. You may even not have it installed, that's OK)

   .. group-tab:: simgrid/tuto-mc

      If you go for the large image, the following command starts a terminal within the Docker container, where SimGrid is already installed.

      .. code-block:: console
      
         $ docker image pull simgrid/tuto-mc
         $ mkdir ~/tuto-mcsimgrid # or chose another directory to share between your computer and the docker container
         $ docker run --user $UID:$GID -it --rm --name mcsimgrid --volume ~/tuto-mcsimgrid:/source/tutorial simgrid/tuto-mc bash

      If the copy command fails, complaining that you do not have the needed right, try removing the ``sudo`` in front of your
      ``docker`` command, or the ``--user $UID:$GID`` part of the docker command if the ``sudo`` is mandatory in your setup.

      More info if you want to understand that command. Skip it if you wish. The ``--user $UID:$GID`` part request docker to use
      your login name and group within the container too. ``-it`` requests to run the command interactively in a terminal.
      ``--rm`` asks to remove the container once the command is done. ``--name`` gives a name to the container. ``--volume``
      makes one directory of your machine visible from within the container. The part on the left of ``:`` is the name outside
      while the right part is the name within the container. The last words on the line are the docker image to use as a basis
      for the container (here, ``simgrid/tuto-mc``) and the program to run when the container starts (here, ``bash``).

      In the container, you have access to the following directories of interest:

      - ``/source/tutorial``: A view to the ``~/tuto-mcsimgrid`` directory on your disk, out of the container.
        Edit the files you want from your computer and save them in ``~/tuto-mcsimgrid``;
        Compile and use them immediately within the container in ``/source/tutorial``.
      - ``/source/tutorial-model-checking.git``: Files provided with this tutorial.
      - ``/source/simgrid-v???``: Source code of SimGrid, pre-configured in MC mode. The framework is also installed in ``/usr``
        so the source code is only provided for your information.

      To use valgrind from within the container, you probably need to run the following command

      .. code-block:: console
         $ ulimit -n 1024

Lab1: Dining philosophers with sthread
--------------------------------------

Let's first explore the behavior of bugged implementation of the `dining philosophers problem
<https://en.wikipedia.org/wiki/Dining_philosophers_problem>`_. 

.. tabs::

   .. group-tab:: simgrid/mc-slim

      First download the tutorial files from git.

      .. code-block:: console

         $ git clone --depth=1 https://framagit.org/simgrid/tutorial-model-checking.git
         $ cd tutorial-model-checking.git

   .. group-tab:: simgrid/tuto-mc

      Once in the container, copy all files from the tutorial into the directory shared between your host computer and the container.

      .. code-block:: console

         # From within the container
         $ cp -r /source/tutorial-model-checking.git/* /source/tutorial/
         $ cd /source/tutorial/

      Several files should have appeared in the ``~/tuto-mcsimgrid`` directory of your computer (out of the container).

This lab uses `philosophers.c <https://framagit.org/simgrid/tutorial-model-checking/-/blob/main/philosophers.c>`_

.. toggle-header::
   :header: Code of ``philosophers.c``: click here to open

   You can also `view it online <https://framagit.org/simgrid/tutorial-model-checking/-/blob/main/philosophers.c>`_

   .. literalinclude:: tuto_mc/philosophers.c
      :language: cpp

|br|
The provided code is as simple as possible. It simply declares a ``philosopher_code`` function, representing a philosopher that
first picks its left fork and then right fork before eating. This code is obviously wrong: if all philosopher manage to get
their left fork at the same time, no one will manage to get its right fork (because it's the left fork of someone else), and the
execution will deadlock.

.. tabs::

   .. group-tab:: simgrid/mc-slim

      Suprisingly, this code works when you run it:

      .. code-block:: console

         # Do not forget the ./simgrid.sh at the beginning to enter the docker for each command

         $ ./simgrid.sh clang -g philosophers.c -o philosophers
         $ ./simgrid.sh ./philosophers 5 1 # launch 5 philosophers, enabling debug
         Philosopher 0 just ate.
         Philosopher 1 just ate.
         Philosopher 2 just ate.
         Philosopher 3 just ate.
         Philosopher 4 just ate.
         $

   .. group-tab:: simgrid/tuto-mc

      Suprisingly, this code works when you run it:

      .. code-block:: console
      
         # From within the container, directory /source/tutorial/

         $ cmake . && make philosophers
         $ ./philosophers 5 1 # launch 5 philosophers, enabling debug
         Philosopher 0 just ate.
         Philosopher 2 just ate.
         Philosopher 3 just ate.
         Philosopher 1 just ate.
         Philosopher 4 just ate.
         $

The philosophers may well eat in another order in your case, but it is really unlikely that you manage to trigger the bug in
your first run. Actually, you can probably run the code ten thousands times without triggering the issue.

.. tabs::

   .. group-tab:: simgrid/mc-slim

      .. code-block:: console

         $ for i in `seq 1 10000` ; do echo "XXX Run $i" ; ./simgrid.sh ./philosophers 5 1 ; done
         (10,000 non-buggy executions -- most likely)

   .. group-tab:: simgrid/tuto-mc

      .. code-block:: console
      
         # From within the container, directory /source/tutorial/
         $ for i in `seq 1 10000` ; do echo "XXX Run $i" ; ./philosophers 5 1 ; done
         (10,000 non-buggy executions -- most likely)

This is exactly what makes debugging multithreaded applications so frustrating. It often happens that even if you know for sure
that your code is wrong, you fail to trigger the issue with your tests. The second source of frustration comes from the fact
that when you get an unexpected deadlock in your test, you fail to understand how your application reached that buggy state. And
if you add any logs to your application, its behavior changes and the bug disappear (such bugs are often called `heisenbugs
<https://en.wikipedia.org/wiki/Heisenbug>`_). 

Fortunately, SimGrid can catch the bug of such a small program very quickly and provides a large amount of information about the
bugs it finds. You just have to run your code within the ``simgrid-mc`` program, asking for *sthread* replacement of
``pthread``.

.. tabs::

   .. group-tab:: simgrid/mc-slim

      .. code-block:: console

         $ ./simgrid.sh simgrid-mc --sthread ./philosophers 5 0
         (output explained below)

   .. group-tab:: simgrid/tuto-mc

      .. code-block:: console
      
         # From within the container, directory /source/tutorial/

         $ simgrid-mc --sthread ./philosophers 5 0
         (output explained below)

If you get an error such as ``Channel::receive failure: Connection reset by peer``, read further the logs. It's probably that
the binary does not exist or something. SimGrid is very verbose when things go wrong but the cause to your problem should be
described somewhere in the verbiage.

If simgrid-mc fails with the error ``[root/CRITICAL] Could not wait for the model-checker.``, you need to explicitly add the
PTRACE capability to your docker. Restart your docker with the additional parameter ``--cap-add SYS_PTRACE``.

Since Mc SimGrid is a software model checker, it exhaustively explores all possible outcomes of your application, so you can
take for granted that it will find a bug if there is any. If the exploration terminates without finding any bug, then you can be
reasonably confident that your program is bug-free. It's not a proof either, because Mc SimGrid itself is a complex program
which may contain bugs, preventing it from finding existing bugs in your application. If your program is too large, its
exhaustive exploration may be too large to be practical. But in our case, Mc SimGrid produces a counter example in one tenth of
a second:

.. code-block:: console

   [0.000000] [xbt_cfg/INFO] Configuration change: Set 'model-check/setenv' to 'LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libsthread.so'
   [0.000000] [mc_checkerside/INFO] setenv 'LD_PRELOAD'='/usr/lib/x86_64-linux-gnu/libsthread.so'
   sthread is intercepting the execution of ./philosophers. If it's not what you want, export STHREAD_IGNORE_BINARY=./philosophers
   [0.000000] [mc_dfs/INFO] Start a DFS exploration. Reduction is: dpor.
   [0.000000] [mc_global/INFO] **************************
   [0.000000] [mc_global/INFO] *** DEADLOCK DETECTED ***
   [0.000000] [mc_global/INFO] **************************
   (more info omitted)

The first few lines are debug and various informative messages, such as the used version of sthread and the fact that sthread
successfully intercepts our binary. Then the exploration starts, quickly leading to the deadlock. Then comes the current state
of the system when the deadlock arises:

.. code-block:: console

   [0.000000] [ker_engine/INFO] 6 actors are still active, awaiting something. Here is their status:
   [0.000000] [ker_engine/INFO]  - pid 1 (main thread@Lilibeth) simcall ActorJoin(pid:2)
   [0.000000] [ker_engine/INFO]  - pid 2 (thread 1@Lilibeth) simcall MUTEX_WAIT(mutex_id:1 owner:3)
   [0.000000] [ker_engine/INFO]  - pid 3 (thread 2@Lilibeth) simcall MUTEX_WAIT(mutex_id:2 owner:4)
   [0.000000] [ker_engine/INFO]  - pid 4 (thread 3@Lilibeth) simcall MUTEX_WAIT(mutex_id:3 owner:5)
   [0.000000] [ker_engine/INFO]  - pid 5 (thread 4@Lilibeth) simcall MUTEX_WAIT(mutex_id:4 owner:6)
   [0.000000] [ker_engine/INFO]  - pid 6 (thread 5@Lilibeth) simcall MUTEX_WAIT(mutex_id:0 owner:2)

The main thread of our program (the first one, the one given pid 1 by SimGrid) is waiting in a ``pthread_join`` (SimGrid
converts it into its internal ActorJoin *simcall* i.e. transition i.e. observed program event). We even learn that this thread
is trying to join on the thread of pid 2 at that point. We then see the expected loop of locks characterizing the deadlock:
pid 2 owns mutex 0 and wants mutex 1; pid 3 owns mutex 1 and wants 2; pid 4 owns mutex 2 and wants 3; pid 5 owns mutex 3 and
wants 4; pid 6 owns mutex 4 and wants 0. That's exactly the bug we were expecting from that code. 

SimGrid then details the execution trace leading to this deadlock.

.. code-block:: console

   [0.000000] [mc_global/INFO] Counter-example execution trace:
   [0.000000] [mc_global/INFO]   Actor 2 in simcall MUTEX_ASYNC_LOCK(mutex: 0, owner: 2)
   [0.000000] [mc_global/INFO]   Actor 2 in simcall MUTEX_WAIT(mutex: 0, owner: 2)
   [0.000000] [mc_global/INFO]   Actor 3 in simcall MUTEX_ASYNC_LOCK(mutex: 1, owner: 3)
   [0.000000] [mc_global/INFO]   Actor 2 in simcall MUTEX_ASYNC_LOCK(mutex: 1, owner: 3)
   [0.000000] [mc_global/INFO]   Actor 3 in simcall MUTEX_WAIT(mutex: 1, owner: 3)
   [0.000000] [mc_global/INFO]   Actor 4 in simcall MUTEX_ASYNC_LOCK(mutex: 2, owner: 4)
   [0.000000] [mc_global/INFO]   Actor 3 in simcall MUTEX_ASYNC_LOCK(mutex: 2, owner: 4)
   [0.000000] [mc_global/INFO]   Actor 4 in simcall MUTEX_WAIT(mutex: 2, owner: 4)
   [0.000000] [mc_global/INFO]   Actor 5 in simcall MUTEX_ASYNC_LOCK(mutex: 3, owner: 5)
   [0.000000] [mc_global/INFO]   Actor 4 in simcall MUTEX_ASYNC_LOCK(mutex: 3, owner: 5)
   [0.000000] [mc_global/INFO]   Actor 5 in simcall MUTEX_WAIT(mutex: 3, owner: 5)
   [0.000000] [mc_global/INFO]   Actor 6 in simcall MUTEX_ASYNC_LOCK(mutex: 4, owner: 6)
   [0.000000] [mc_global/INFO]   Actor 5 in simcall MUTEX_ASYNC_LOCK(mutex: 4, owner: 6)
   [0.000000] [mc_global/INFO]   Actor 6 in simcall MUTEX_WAIT(mutex: 4, owner: 6)
   [0.000000] [mc_global/INFO]   Actor 6 in simcall MUTEX_ASYNC_LOCK(mutex: 0, owner: 2)

SimGrid execution traces are not that easy to read because the internal events do not perfectly match the API we used. Most
notably, ``pthread_lock`` is split into two events: ``MUTEX_ASYNC_LOCK`` (where the actor declares it intend to lock the mutex
without blocking. It puts its name in the waiting list of that mutex) and ``MUTEX_WAIT`` (where it actually blocks until its
name is becomes the first from that list). When ``MUTEX_ASYNC_LOCK`` appears in the execution trace, it means that this action
was successfully run by the corresponding actor (intend to wait on the mutex do not appear in the trace, only successful waits
appear). 

You can read ``MUTEX_ASYNC_LOCK`` as ``pthread_lock_begin`` while ``MUTEX_WAIT`` would be  ``pthread_lock_end``.
``pthread_unlock`` simply becomes ``MUTEX_UNLOCK``, even if there is no such operation in that execution trace.

With this information and our previous understanding of the issue, we can read the trace as follows:

 - Actor 2 takes mutex 0 (``MUTEX_ASYNC_LOCK`` + ``MUTEX_WAIT``)
 - Actor 3 declares its intend to take mutex 1 (``MUTEX_ASYNC_LOCK``)
 - Actor 2 declares its intend to take mutex 1 (``MUTEX_ASYNC_LOCK``)

This is already a dangerous move, as actor 2 is the owner of mutex 0 and wants the mutex 1, that is owned by actor 3 that will
need the mutex 2 to release the mutex 1. But the deadlock is not granted yet, as nobody owns mutex 2 yet, so actor 3 could still
get it. When exactly does the trap close in on our threads?

If we read the output further, SimGrid displays the critical transition, which is the first transition after which no valid
execution exist. Before that critical transition, some possible executions still manage to avoid any issue, but after that
transition all executions are buggy.

.. code-block:: console

   [0.000000] [mc_ct/INFO] *********************************
   [0.000000] [mc_ct/INFO] *** CRITICAL TRANSITION FOUND ***
   [0.000000] [mc_ct/INFO] *********************************
   [0.000000] [mc_ct/INFO] Current knowledge of explored stack:
   [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 2 in  ==> simcall: MUTEX_ASYNC_LOCK(mutex: 0, owner: 2)
   [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 2 in  ==> simcall: MUTEX_WAIT(mutex: 0, owner: 2)
   [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 3 in  ==> simcall: MUTEX_ASYNC_LOCK(mutex: 1, owner: 3)
   [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 2 in  ==> simcall: MUTEX_ASYNC_LOCK(mutex: 1, owner: 3)
   [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 3 in  ==> simcall: MUTEX_WAIT(mutex: 1, owner: 3)
   [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 4 in  ==> simcall: MUTEX_ASYNC_LOCK(mutex: 2, owner: 4)
   [0.000000] [mc_ct/INFO]   (INCORRECT) Actor 3 in  ==> simcall: MUTEX_ASYNC_LOCK(mutex: 2, owner: 4)
   [0.000000] [mc_ct/INFO]   (INCORRECT) Actor 4 in  ==> simcall: MUTEX_WAIT(mutex: 2, owner: 4)
   [0.000000] [mc_ct/INFO]   (INCORRECT) Actor 4 in  ==> simcall: MUTEX_ASYNC_LOCK(mutex: 0, owner: 2)
   [0.000000] [mc_ct/INFO] Found the critical transition: Actor 4 ==> simcall: MUTEX_ASYNC_LOCK(mutex: 2, owner: 4)   

Once the actor 4 becomes the owner of mutex 2 while any other philosopher owns a mutex, the deadlock becomes inevitable.

Before that critical transition, SimGrid displays some information on how to reproduce the bug out of the model checker as well as additional statistics.

.. code-block:: console

   [0.000000] [mc_Session/INFO] You can debug the problem (and see the whole details) by rerunning out of simgrid-mc 
                                with --cfg=model-check/replay:'1;1;1;1;1;2;2;3;2;3;4;3;4;5;4;5;6;5;6;6'
   [0.000000] [mc_dfs/INFO] DFS exploration ended. 390 unique states visited; 15 explored traces (322 transition replays, 696 states visited overall)

As stated in the first message, you can rerun the faulty execution trace directly with the given extra parameter. This can be
useful to run that execution within valgrind, you probably don't want to slow down your application with valgrind while running
the time consuming model checker. But the real advantage of that command is that SimGrid provides much more information when
replaying a given trace. As you can see below, that's probably more information than you could dream of. 

.. tabs::

   .. group-tab:: simgrid/mc-slim

      .. code-block:: console

         $ ./simgrid.sh simgrid-mc --sthread ./philosophers 5 0 --cfg=model-check/replay:'1;1;1;1;1;2;2;3;2;3;4;3;4;5;4;5;6;5;6;6'
         sthread is intercepting the execution of ./philosophers. If it's not what you want, export STHREAD_IGNORE_BINARY=./philosophers
         [0.000000] [xbt_cfg/INFO] Configuration change: Set 'model-check/replay' to '1;1;1;1;1;2;2;3;2;3;4;3;4;5;4;5;6;5;6;6'
         [0.000000] [mc_record/INFO] path=1;1;1;1;1;2;2;3;2;3;4;3;4;5;4;5;6;5;6;6
         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #1 '1/0' Actor main thread(pid:1): ActorCreate(-1)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor main thread):
           ->  #0 simgrid::xbt::BacktraceImpl::BacktraceImpl() at src/xbt/backtrace.cpp:48
           ->  #1 xbt_backtrace_display_current at src/xbt/backtrace.cpp:36
           ->  #2 simgrid::kernel::actor::ActorImpl::yield() at src/kernel/actor/ActorImpl.cpp:297
           ->  #3 simcall(simgrid::kernel::actor::Simcall::Type, std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at src/kernel/actor/Simcall.cpp:58
           ->  #4 simcall_run_answered(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at src/kernel/actor/Simcall.cpp:67
           ->  #5 boost::intrusive_ptr<simgrid::s4u::Actor> simgrid::s4u::Host::add_actor<std::_Bind<sthread_create::$_0 (void* (*)(void*), void*)> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, std::_Bind<sthread_create::$_0 (void* (*)(void*), void*)>) at include/simgrid/s4u/Host.hpp:255
           ->  #6 pthread_create at src/sthread/sthread.c:144
           ->  #7 main at /tmp/tutorial-model-checking.git/philosophers.c:52

         (some more ActorCreate() transitions ignored)

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #6 '2/0' Actor thread 1(pid:2): MUTEX_ASYNC_LOCK(mutex_id:0 owner:none)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 1):
           ->  #0 simgrid::xbt::BacktraceImpl::BacktraceImpl() at src/xbt/backtrace.cpp:48
           ->  #1 xbt_backtrace_display_current at src/xbt/backtrace.cpp:36
           ->  #2 simgrid::kernel::actor::ActorImpl::yield() at src/kernel/actor/ActorImpl.cpp:297
           ->  #3 simcall(simgrid::kernel::actor::Simcall::Type, std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at src/kernel/actor/Simcall.cpp:58
           ->  #4 simcall_run_answered(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at src/kernel/actor/Simcall.cpp:67
           ->  #5 sthread_mutex_lock at src/sthread/sthread_impl.cpp:252
           ->  #6 pthread_mutex_lock at src/sthread/sthread.c:160
           ->  #7 philosopher_code at /tmp/tutorial-model-checking.git/philosophers.c:19
           ->  #8 auto sthread_create::$_0::operator()<void* (void*), void>(void* (*)(void*), void*) const at src/sthread/sthread_impl.cpp:154
           ->  #9 std::function<void ()>::operator()() const at /usr/lib/gcc/x86_64-linux-gnu/15/../../../../include/c++/15/bits/std_function.h:593
           ->  #10 

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #7 '2/0' Actor thread 1(pid:2): MUTEX_WAIT(mutex_id:0 owner:2)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 1):
           ->  #0 simgrid::xbt::BacktraceImpl::BacktraceImpl() at src/xbt/backtrace.cpp:48
           ->  #1 xbt_backtrace_display_current at src/xbt/backtrace.cpp:36
           ->  #2 simgrid::kernel::actor::ActorImpl::yield() at src/kernel/actor/ActorImpl.cpp:297
           ->  #3 simcall(simgrid::kernel::actor::Simcall::Type, std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at src/kernel/actor/Simcall.cpp:58
           ->  #4 simcall_run_blocking(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at src/kernel/actor/Simcall.cpp:74
           ->  #5 sthread_mutex_lock at src/sthread/sthread_impl.cpp:252
           ->  #6 pthread_mutex_lock at src/sthread/sthread.c:160
           ->  #7 philosopher_code at /tmp/tutorial-model-checking.git/philosophers.c:19
           ->  #8 auto sthread_create::$_0::operator()<void* (void*), void>(void* (*)(void*), void*) const at src/sthread/sthread_impl.cpp:154
           ->  #9 std::function<void ()>::operator()() const at /usr/lib/gcc/x86_64-linux-gnu/15/../../../../include/c++/15/bits/std_function.h:593
           ->  #10 

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #8 '3/0' Actor thread 2(pid:3): MUTEX_ASYNC_LOCK(mutex_id:1 owner:none)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 2):
           ->  #0 simgrid::xbt::BacktraceImpl::BacktraceImpl() at src/xbt/backtrace.cpp:48
           ->  #1 xbt_backtrace_display_current at src/xbt/backtrace.cpp:36
           ->  #2 simgrid::kernel::actor::ActorImpl::yield() at src/kernel/actor/ActorImpl.cpp:297
           ->  #3 simcall(simgrid::kernel::actor::Simcall::Type, std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at src/kernel/actor/Simcall.cpp:58
           ->  #4 simcall_run_answered(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at src/kernel/actor/Simcall.cpp:67
           ->  #5 sthread_mutex_lock at src/sthread/sthread_impl.cpp:252
           ->  #6 pthread_mutex_lock at src/sthread/sthread.c:160
           ->  #7 philosopher_code at /tmp/tutorial-model-checking.git/philosophers.c:19
           ->  #8 auto sthread_create::$_0::operator()<void* (void*), void>(void* (*)(void*), void*) const at src/sthread/sthread_impl.cpp:154
           ->  #9 std::function<void ()>::operator()() const at /usr/lib/gcc/x86_64-linux-gnu/15/../../../../include/c++/15/bits/std_function.h:593

         (some more execution chunks ignored)

         [0.000000] [mc_record/INFO] The replay of the trace is complete. DEADLOCK detected.
         [0.000000] [ker_engine/INFO] 6 actors are still active, awaiting something. Here is their status:
         [0.000000] [ker_engine/INFO]  - pid 1 (main thread@Lilibeth) simcall ActorJoin(pid:2)
         [0.000000] [ker_engine/INFO]  - pid 2 (thread 1@Lilibeth) simcall MUTEX_WAIT(mutex_id:1 owner:3)
         [0.000000] [ker_engine/INFO]  - pid 3 (thread 2@Lilibeth) simcall MUTEX_WAIT(mutex_id:2 owner:4)
         [0.000000] [ker_engine/INFO]  - pid 4 (thread 3@Lilibeth) simcall MUTEX_WAIT(mutex_id:3 owner:5)
         [0.000000] [ker_engine/INFO]  - pid 5 (thread 4@Lilibeth) simcall MUTEX_WAIT(mutex_id:4 owner:6)
         [0.000000] [ker_engine/INFO]  - pid 6 (thread 5@Lilibeth) simcall MUTEX_WAIT(mutex_id:0 owner:2)
         [0.000000] [sthread/INFO] All threads exited. Terminating the simulation.

   .. group-tab:: simgrid/tuto-mc

      .. code-block:: console
      
         $ simgrid-mc --sthread ./philosophers 5 0 --cfg=model-check/replay:'1;1;1;1;1;2;2;3;2;3;4;3;4;5;4;5;6;5;6;6'
         sthread is intercepting the execution of ./philosophers. If it's not what you want, export STHREAD_IGNORE_BINARY=./philosophers
         [0.000000] [xbt_cfg/INFO] Configuration change: Set 'model-check/replay' to '1;1;1;1;1;2;2;3;2;3;4;3;4;5;4;5;6;5;6;6'
         [0.000000] [mc_record/INFO] path=2;2;3;2;3;4;3;4;4
         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #1 '2/0' Actor thread 1(pid:2): MUTEX_ASYNC_LOCK(mutex_id:0 owner:none)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 1):
           ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
           ->  #1 simcall_run_answered(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at /src/kernel/actor/Simcall.cpp:67
           ->  #2 simgrid::s4u::Mutex::lock() at /src/s4u/s4u_Mutex.cpp:24
           ->  #3 sthread_mutex_lock at /src/sthread/sthread_impl.cpp:223
           ->  #4 pthread_mutex_lock at /usr/include/pthread.h:738
           ->  #5 philosopher_code at /source/tutorial/philosophers.c:19
           ->  #6 std::_Function_handler<void (), std::_Bind<sthread_create::{lambda(auto:1*, auto:2*)#1} (void* (*)(sthread_create::{lambda(auto:1*, auto:2*)#1}), sthread_create::{lambda(auto:1*, auto:2*)#1})> >::_M_invoke(std::_Any_data const&) at /usr/include/c++/10/bits/std_function.h:293
           ->  #7 smx_ctx_wrapper at /src/kernel/context/ContextSwapped.cpp:43

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #2 '2/0' Actor thread 1(pid:2): MUTEX_WAIT(mutex_id:0 owner:2)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 1):
           ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
           ->  #1 simcall_run_blocking(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at /src/kernel/actor/Simcall.cpp:74
           ->  #2 simgrid::s4u::Mutex::lock() at /src/s4u/s4u_Mutex.cpp:28
           ->  #3 sthread_mutex_lock at /src/sthread/sthread_impl.cpp:223
           ->  #4 pthread_mutex_lock at /usr/include/pthread.h:738
           ->  #5 philosopher_code at /source/tutorial/philosophers.c:19
           ->  #6 std::_Function_handler<void (), std::_Bind<sthread_create::{lambda(auto:1*, auto:2*)#1} (void* (*)(sthread_create::{lambda(auto:1*, auto:2*)#1}), sthread_create::{lambda(auto:1*, auto:2*)#1})> >::_M_invoke(std::_Any_data const&) at /usr/include/c++/10/bits/std_function.h:293
           ->  #7 smx_ctx_wrapper at /src/kernel/context/ContextSwapped.cpp:43

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #3 '3/0' Actor thread 2(pid:3): MUTEX_ASYNC_LOCK(mutex_id:1 owner:none)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 2):
           ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
           ->  #1 simcall_run_answered(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at /src/kernel/actor/Simcall.cpp:67
           ->  #2 simgrid::s4u::Mutex::lock() at /src/s4u/s4u_Mutex.cpp:24
           ->  #3 sthread_mutex_lock at /src/sthread/sthread_impl.cpp:223
           ->  #4 pthread_mutex_lock at /usr/include/pthread.h:738
           ->  #5 philosopher_code at /source/tutorial/philosophers.c:19
           ->  #6 std::_Function_handler<void (), std::_Bind<sthread_create::{lambda(auto:1*, auto:2*)#1} (void* (*)(sthread_create::{lambda(auto:1*, auto:2*)#1}), sthread_create::{lambda(auto:1*, auto:2*)#1})> >::_M_invoke(std::_Any_data const&) at /usr/include/c++/10/bits/std_function.h:293
           ->  #7 smx_ctx_wrapper at /src/kernel/context/ContextSwapped.cpp:43

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #4 '2/0' Actor thread 1(pid:2): MUTEX_ASYNC_LOCK(mutex_id:1 owner:3)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 1):
           ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
           ->  #1 simcall_run_answered(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at /src/kernel/actor/Simcall.cpp:67
           ->  #2 simgrid::s4u::Mutex::lock() at /src/s4u/s4u_Mutex.cpp:24
           ->  #3 sthread_mutex_lock at /src/sthread/sthread_impl.cpp:223
           ->  #4 pthread_mutex_lock at /usr/include/pthread.h:738
           ->  #5 philosopher_code at /source/tutorial/philosophers.c:21
           ->  #6 std::_Function_handler<void (), std::_Bind<sthread_create::{lambda(auto:1*, auto:2*)#1} (void* (*)(sthread_create::{lambda(auto:1*, auto:2*)#1}), sthread_create::{lambda(auto:1*, auto:2*)#1})> >::_M_invoke(std::_Any_data const&) at /usr/include/c++/10/bits/std_function.h:293
           ->  #7 smx_ctx_wrapper at /src/kernel/context/ContextSwapped.cpp:43

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #5 '3/0' Actor thread 2(pid:3): MUTEX_WAIT(mutex_id:1 owner:3)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 2):
           ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
           ->  #1 simcall_run_blocking(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at /src/kernel/actor/Simcall.cpp:74
           ->  #2 simgrid::s4u::Mutex::lock() at /src/s4u/s4u_Mutex.cpp:28
           ->  #3 sthread_mutex_lock at /src/sthread/sthread_impl.cpp:223
           ->  #4 pthread_mutex_lock at /usr/include/pthread.h:738
           ->  #5 philosopher_code at /source/tutorial/philosophers.c:19
           ->  #6 std::_Function_handler<void (), std::_Bind<sthread_create::{lambda(auto:1*, auto:2*)#1} (void* (*)(sthread_create::{lambda(auto:1*, auto:2*)#1}), sthread_create::{lambda(auto:1*, auto:2*)#1})> >::_M_invoke(std::_Any_data const&) at /usr/include/c++/10/bits/std_function.h:293
           ->  #7 smx_ctx_wrapper at /src/kernel/context/ContextSwapped.cpp:43

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #6 '4/0' Actor thread 3(pid:4): MUTEX_ASYNC_LOCK(mutex_id:2 owner:none)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 3):
           ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
           ->  #1 simcall_run_answered(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at /src/kernel/actor/Simcall.cpp:67
           ->  #2 simgrid::s4u::Mutex::lock() at /src/s4u/s4u_Mutex.cpp:24
           ->  #3 sthread_mutex_lock at /src/sthread/sthread_impl.cpp:223
           ->  #4 pthread_mutex_lock at /usr/include/pthread.h:738
           ->  #5 philosopher_code at /source/tutorial/philosophers.c:19
           ->  #6 std::_Function_handler<void (), std::_Bind<sthread_create::{lambda(auto:1*, auto:2*)#1} (void* (*)(sthread_create::{lambda(auto:1*, auto:2*)#1}), sthread_create::{lambda(auto:1*, auto:2*)#1})> >::_M_invoke(std::_Any_data const&) at /usr/include/c++/10/bits/std_function.h:293
           ->  #7 smx_ctx_wrapper at /src/kernel/context/ContextSwapped.cpp:43

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #7 '3/0' Actor thread 2(pid:3): MUTEX_ASYNC_LOCK(mutex_id:2 owner:4)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 2):
           ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
           ->  #1 simcall_run_answered(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at /src/kernel/actor/Simcall.cpp:67
           ->  #2 simgrid::s4u::Mutex::lock() at /src/s4u/s4u_Mutex.cpp:24
           ->  #3 sthread_mutex_lock at /src/sthread/sthread_impl.cpp:223
           ->  #4 pthread_mutex_lock at /usr/include/pthread.h:738
           ->  #5 philosopher_code at /source/tutorial/philosophers.c:21
           ->  #6 std::_Function_handler<void (), std::_Bind<sthread_create::{lambda(auto:1*, auto:2*)#1} (void* (*)(sthread_create::{lambda(auto:1*, auto:2*)#1}), sthread_create::{lambda(auto:1*, auto:2*)#1})> >::_M_invoke(std::_Any_data const&) at /usr/include/c++/10/bits/std_function.h:293
           ->  #7 smx_ctx_wrapper at /src/kernel/context/ContextSwapped.cpp:43

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #8 '4/0' Actor thread 3(pid:4): MUTEX_WAIT(mutex_id:2 owner:4)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 3):
           ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
           ->  #1 simcall_run_blocking(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at /src/kernel/actor/Simcall.cpp:74
           ->  #2 simgrid::s4u::Mutex::lock() at /src/s4u/s4u_Mutex.cpp:28
           ->  #3 sthread_mutex_lock at /src/sthread/sthread_impl.cpp:223
           ->  #4 pthread_mutex_lock at /usr/include/pthread.h:738
           ->  #5 philosopher_code at /source/tutorial/philosophers.c:19
           ->  #6 std::_Function_handler<void (), std::_Bind<sthread_create::{lambda(auto:1*, auto:2*)#1} (void* (*)(sthread_create::{lambda(auto:1*, auto:2*)#1}), sthread_create::{lambda(auto:1*, auto:2*)#1})> >::_M_invoke(std::_Any_data const&) at /usr/include/c++/10/bits/std_function.h:293
           ->  #7 smx_ctx_wrapper at /src/kernel/context/ContextSwapped.cpp:43

         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #9 '4/0' Actor thread 3(pid:4): MUTEX_ASYNC_LOCK(mutex_id:3 owner:none)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         Backtrace (displayed in actor thread 3):
           ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
           ->  #1 simcall_run_answered(std::function<void ()> const&, simgrid::kernel::actor::SimcallObserver*) at /src/kernel/actor/Simcall.cpp:67
           ->  #2 simgrid::s4u::Mutex::lock() at /src/s4u/s4u_Mutex.cpp:24
           ->  #3 sthread_mutex_lock at /src/sthread/sthread_impl.cpp:223
           ->  #4 pthread_mutex_lock at /usr/include/pthread.h:738
           ->  #5 philosopher_code at /source/tutorial/philosophers.c:21
           ->  #6 std::_Function_handler<void (), std::_Bind<sthread_create::{lambda(auto:1*, auto:2*)#1} (void* (*)(sthread_create::{lambda(auto:1*, auto:2*)#1}), sthread_create::{lambda(auto:1*, auto:2*)#1})> >::_M_invoke(std::_Any_data const&) at /usr/include/c++/10/bits/std_function.h:293
           ->  #7 smx_ctx_wrapper at /src/kernel/context/ContextSwapped.cpp:43

         [0.000000] [mc_record/INFO] The replay of the trace is complete. The application could run further.
         [0.000000] [sthread/INFO] All threads exited. Terminating the simulation.
         [0.000000] /src/kernel/EngineImpl.cpp:275: [ker_engine/WARNING] Process called exit when leaving - Skipping cleanups
         [0.000000] /src/kernel/EngineImpl.cpp:275: [ker_engine/WARNING] Process called exit when leaving - Skipping cleanups

We hope this tool proves useful for debugging your multithreaded code. We encourage you to share your feedback, whether positive
or negative. Additionally, we would appreciate learning about any bugs you have identified using this tool. Our team will strive
to address any challenges you encounter while working with Mc SimGrid.

Lab2: race condition with sthread
---------------------------------

This short lab aims at searching for race conditions with the latest version of SimGrid. It requires SimGrid v4.1 (that is not
yet released), and was only tested with the Docker image. See above to pull and start the Docker image.

If you want to search for race conditions in your code, you must use our compilation wrapper to compile the programs.
Do not forget the ``-g`` flag, or the messages will be less informative.

.. tabs::

   .. group-tab:: simgrid/mc-slim

      .. code-block:: console
      
         $ git clone --depth=1 https://framagit.org/simgrid/tutorial-model-checking.git
         $ cd tutorial-model-checking.git
         $ ./simgrid.sh mclang plusplus.c -o plusplus -g
         Successfully instrumented 4 read operations and 3 writes operations

   .. group-tab:: simgrid/tuto-mc

      .. code-block:: console
      
         # From within the container, directory /source/tutorial/
      
         $ cp /source/tutorial-model-checking.git/plusplus.c .
         $ mclang plusplus.c -o plusplus -g
         Successfully instrumented 4 read operations and 3 writes operations

Looking at the `source code <https://framagit.org/simgrid/tutorial-model-checking/-/blob/main/philosophers.c>`_, there is a
clear race condition between the two threads on the variable ``i``. This is because incrementing an integer is not an atomic
operation, so ``i`` could have the value of ``1`` if the threads compete for its increment. But if you run the program, it is
very unlikely that you observe any issue, even if you run it 10,000 times in a row. 

.. tabs::

   .. group-tab:: simgrid/mc-slim

      .. code-block:: console
      
         $ ./simgrid.sh ./plusplus
         $ for i in `seq 1 10000` ; do echo "XXX Run $i" ; ./simgrid.sh ./plusplus ; done

   .. group-tab:: simgrid/tuto-mc

      .. code-block:: console

         # From within the container, directory /source/tutorial/
         
         $ ./plusplus
         $ for i in `seq 1 10000` ; do echo "XXX Run $i" ; ./plusplus ; done

But if you run this program within the model checker, it detects the issue instantaneously, and properly report the race condition:

.. tabs::

   .. group-tab:: simgrid/mc-slim

      .. code-block:: console

         $ ./simgrid.sh simgrid-mc --sthread ./plusplus
         [0.000000] [mc_checkerside/INFO] setenv 'LD_PRELOAD'='/usr/lib/x86_64-linux-gnu/libsthread.so'
         sthread is intercepting the execution of ./plusplus. If it's not what you want, export STHREAD_IGNORE_BINARY=./plusplus
         [0.000000] [mc_dfs/INFO] Start a DFS exploration. Reduction is: dpor.
         [0.000000] [mc_explo/INFO] Found a datarace at location 0x561ce34d004c between actor 2 and 3 after the follwing execution:
         [0.000000] [mc_explo/INFO]   Actor 1 in simcall ActorCreate(child 2)
         [0.000000] [mc_explo/INFO]   Actor 1 in simcall ActorCreate(child 3)
         [0.000000] [mc_explo/INFO]   Actor 2 in simcall ActorExit()
         [0.000000] [mc_explo/INFO]   Actor 1 in simcall ActorJoin(target 2, no timeout)
         [0.000000] [mc_explo/INFO]   Actor 3 in simcall ActorExit()
         [0.000000] [mc_explo/INFO] You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with --cfg=model-check/replay:'1;1;2;1;3'
         [0.000000] [mc_dfs/INFO] DFS exploration ended. 6 unique states visited; 0 explored traces (0 transition replays, 5 states visited overall)

   .. group-tab:: simgrid/tuto-mc

      .. code-block:: console
      
         # From within the container, directory /source/tutorial/

         $ simgrid-mc --sthread ./plusplus
         [0.000000] [mc_checkerside/INFO] setenv 'LD_PRELOAD'='/usr/lib/x86_64-linux-gnu/libsthread.so'
         sthread is intercepting the execution of ./plusplus. If it's not what you want, export STHREAD_IGNORE_BINARY=./plusplus
         [0.000000] [mc_dfs/INFO] Start a DFS exploration. Reduction is: dpor.
         [0.000000] [mc_explo/INFO] Found a datarace at location 0x55d463ce604c between actor 2 and 3 after the follwing execution:
         [0.000000] [mc_explo/INFO]   Actor 1 in simcall ActorCreate(child 2)
         [0.000000] [mc_explo/INFO]   Actor 1 in simcall ActorCreate(child 3)
         [0.000000] [mc_explo/INFO]   Actor 2 in simcall ActorExit()
         [0.000000] [mc_explo/INFO]   Actor 1 in simcall ActorJoin(target 2, no timeout)
         [0.000000] [mc_explo/INFO]   Actor 3 in simcall ActorExit()
         [0.000000] [mc_explo/INFO] You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with --cfg=model-check/replay:'1;1;2;1;3'
         [0.000000] [mc_dfs/INFO] DFS exploration ended. 6 unique states visited; 0 explored traces (0 transition replays, 5 states visited overall)

To replay the execution to get more details, you must inject sthread manually through ``LD_PRELOAD``. The syntax is given in the
first line of the ``simgrid-mc`` output shown above.

.. tabs::

   .. group-tab:: simgrid/mc-slim

      .. code-block:: console
      
         # From within the container, directory /source/tutorial/
      
         $ ./simgrid.sh simgrid-mc --sthread ./plusplus --cfg=model-check/replay:'1;1;2;1;3'
         # (verbose description of the first execution steps, that are not that informative)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #5 '3/0' Actor thread 2(pid:3): ActorExit()
         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] Found a datarace at location 0x562a1c30c04c
         [0.000000] [mc_record/INFO] First operation was a WRITE made by actor 2:
           ->  #0 simgrid::xbt::BacktraceImpl::BacktraceImpl() at src/xbt/backtrace.cpp:48
           ->  #1 thread_code at /tmp/tutorial-model-checking.git/plusplus.c:8
           ->  #2 auto sthread_create::$_0::operator()<void* (void*), void>(void* (*)(void*), void*) const at src/sthread/sthread_impl.cpp:154
           ->  #3 std::function<void ()>::operator()() const at /usr/lib/gcc/x86_64-linux-gnu/15/../../../../include/c++/15/bits/std_function.h:593
           ->  #4 
      
         [0.000000] [mc_record/INFO] Second operation was a READ made by actor 3:
           ->  #0 simgrid::xbt::BacktraceImpl::BacktraceImpl() at src/xbt/backtrace.cpp:48
           ->  #1 thread_code at /tmp/tutorial-model-checking.git/plusplus.c:8
           ->  #2 auto sthread_create::$_0::operator()<void* (void*), void>(void* (*)(void*), void*) const at src/sthread/sthread_impl.cpp:154
           ->  #3 std::function<void ()>::operator()() const at /usr/lib/gcc/x86_64-linux-gnu/15/../../../../include/c++/15/bits/std_function.h:593
           ->  #4 

   .. group-tab:: simgrid/tuto-mc

      .. code-block:: console
      
         # From within the container, directory /source/tutorial/

         $ simgrid-mc --sthread ./plusplus --cfg=model-check/replay:'1;1;2;1;3'
         # (verbose description of the first execution steps, that are not that informative)
         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] * Path chunk #5 '3/0' Actor thread 2(pid:3): ActorExit()
         [0.000000] [mc_record/INFO] ***********************************************************************************
         [0.000000] [mc_record/INFO] Found a datarace at location 0x562a1c30c04c
         [0.000000] [mc_record/INFO] First operation was a WRITE made by actor 2:
           ->  #0 simgrid::xbt::BacktraceImpl::BacktraceImpl() at src/xbt/backtrace.cpp:48
           ->  #1 thread_code at /tmp/tutorial-model-checking.git/plusplus.c:8
           ->  #2 auto sthread_create::$_0::operator()<void* (void*), void>(void* (*)(void*), void*) const at src/sthread/sthread_impl.cpp:154
           ->  #3 std::function<void ()>::operator()() const at /usr/lib/gcc/x86_64-linux-gnu/15/../../../../include/c++/15/bits/std_function.h:593
           ->  #4 

         [0.000000] [mc_record/INFO] Second operation was a READ made by actor 3:
           ->  #0 simgrid::xbt::BacktraceImpl::BacktraceImpl() at src/xbt/backtrace.cpp:48
           ->  #1 thread_code at /tmp/tutorial-model-checking.git/plusplus.c:8
           ->  #2 auto sthread_create::$_0::operator()<void* (void*), void>(void* (*)(void*), void*) const at src/sthread/sthread_impl.cpp:154
           ->  #3 std::function<void ()>::operator()() const at /usr/lib/gcc/x86_64-linux-gnu/15/../../../../include/c++/15/bits/std_function.h:593
           ->  #4 

When you know where to look, you from this output that the bug comes from a READ and a WRITE, both occuring at line
``plusplus.c:8`` in the function ``thread_code`` of the provided example.

Lab3: non-deterministic receive (S4U or MPI)
--------------------------------------------

Motivational example
^^^^^^^^^^^^^^^^^^^^

Let's go with another example of a bugged program, this time using message passing in a distributed setting. 

.. tabs::

   .. group-tab:: simgrid/mc-slim

      First download the files from the tutorial on need.

         $ git clone --depth=1 https://framagit.org/simgrid/tutorial-model-checking.git
         $ cd tutorial-model-checking.git

   .. group-tab:: simgrid/mc-slim

      Once in the container, copy all files from the tutorial into the directory shared between your host computer and the container.

      .. code-block:: console
      
        # From within the container

        $ cp -r /source/tutorial-model-checking.git/* /source/tutorial/
        $ cd /source/tutorial/

      Several files should have appeared in the ``~/tuto-mcsimgrid`` directory of your computer.

This lab uses `ndet-receive-s4u.cpp <https://framagit.org/simgrid/tutorial-model-checking/-/blob/main/ndet-receive-s4u.cpp>`_,
that relies the :ref:`S4U interface <S4U_doc>` of SimGrid, but we provide a
`MPI version <https://framagit.org/simgrid/tutorial-model-checking/-/blob/main/ndet-receive-mpi.cpp>`_
if you prefer (see below for details on using the MPI version).

.. toggle-header::
   :header: Code of ``ndet-receive-s4u.cpp``: click here to open

   You can also `view it online <https://framagit.org/simgrid/tutorial-model-checking/-/blob/main/ndet-receive-s4u.cpp>`_

   .. literalinclude:: tuto_mc/ndet-receive-s4u.cpp
      :language: cpp

|br|
The provided code is rather simple: Three ``client`` are launched with an integer from ``1, 2, 3`` as a parameter. These actors simply
send their parameter to a given mailbox. A ``server`` receives 3 messages and assumes that the last received message is the number ``3``.
If you compile and run it, it simply works:

.. tabs::

   .. group-tab:: simgrid/mc-slim

      .. code-block:: console
      
         $ ./simgrid.sh clang++ ndet-receive-s4u.cpp -lsimgrid -Wno-deprecated-declarations -o ndet
         $ ./simgrid.sh ./ndet small_platform.xml
         [Jupiter:client:(2) 0.000000] [example/INFO] Sending 1
         [Bourassa:client:(3) 0.000000] [example/INFO] Sending 2
         [Ginette:client:(4) 0.000000] [example/INFO] Sending 3
         [Jupiter:client:(2) 0.020516] [example/INFO] Sent!
         [Bourassa:client:(3) 0.047027] [example/INFO] Sent!
         [Ginette:client:(4) 0.064651] [example/INFO] Sent!
         [Tremblay:server:(1) 0.064651] [example/INFO] OK

   .. group-tab:: simgrid/tuto-mc

      .. code-block:: console
      
         $ cmake . && make
         (output omitted)
         $ ./ndet-receive-s4u small_platform.xml
         [Jupiter:client:(2) 0.000000] [example/INFO] Sending 1
         [Bourassa:client:(3) 0.000000] [example/INFO] Sending 2
         [Ginette:client:(4) 0.000000] [example/INFO] Sending 3
         [Jupiter:client:(2) 0.020516] [example/INFO] Sent!
         [Bourassa:client:(3) 0.047027] [example/INFO] Sent!
         [Ginette:client:(4) 0.064651] [example/INFO] Sent!
         [Tremblay:server:(1) 0.064651] [example/INFO] OK

Running and understanding Mc SimGrid
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you think about it, that's weird that this code works: all the messages are sent at the exact same time (t=0), so there is no reason for
the message ``3`` to arrive last. Depending on the link speed, any order should be possible. To trigger the bug, you could fiddle with the
source code and/or the platform file, but this is not a method. It's time to start Mc SimGrid, the SimGrid model checker, to exhaustively test
all message orders. For that, you simply launch your simulation as a parameter to the ``simgrid-mc`` binary as you would do with ``valgrind``:

.. tabs::

   .. group-tab:: simgrid/mc-slim

      .. code-block:: console
      
         $ ./simgrid.sh simgrid-mc ./ndet-receive-s4u small_platform.xml
         (some output ignored)
         [Tremblay:server:(1) 0.000000] (...) Assertion value_got == 3 failed
         (more output ignored)

   .. group-tab:: simgrid/tuto-mc

      .. code-block:: console
      
         $ simgrid-mc ./ndet-receive-s4u small_platform.xml
         (some output ignored)
         [Tremblay:server:(1) 0.000000] (...) Assertion value_got == 3 failed
         (more output ignored)

If it fails with the error ``[root/CRITICAL] Could not wait for the model-checker.``, you need to explicitly add the PTRACE capability to
your docker. Restart your docker with the additional parameter ``--cap-add SYS_PTRACE``.

At the end, it works: Mc SimGrid successfully triggers the bug. But the produced output is somewhat long and hairy. Don't worry, we will
now read it together. It can be split in several parts:

- First, you have some information coming from the application.

  - On top, you see the output of the application, but somewhat stuttering. This is exactly what happens: since Mc SimGrid is exploring
    all possible outcome of the code, the execution is sometimes rewind to explore another possible branch (here: another possible
    message ordering). Note also that all times are always 0 in the model checker, since the time is abstracted away in this mode.

    .. code-block:: console

       [Jupiter:client:(2) 0.000000] [example/INFO] Sending 1
       [Bourassa:client:(3) 0.000000] [example/INFO] Sending 2
       [Ginette:client:(4) 0.000000] [example/INFO] Sending 3
       [0.000000] [mc_dfs/INFO] Start a DFS exploration. Reduction is: dpor.
       [Jupiter:client:(2) 0.000000] [example/INFO] Sent!
       [Bourassa:client:(3) 0.000000] [example/INFO] Sent!
       [Tremblay:server:(1) 0.000000] [example/INFO] OK
       [Ginette:client:(4) 0.000000] [example/INFO] Sent!
       [Jupiter:client:(2) 0.000000] [example/INFO] Sent!
       [Bourassa:client:(3) 0.000000] [example/INFO] Sent!
       [Jupiter:client:(2) 0.000000] [example/INFO] Sent!

  - Then you have the error message, along with a backtrace of the application at the point where the assertion fails. Not all the frames of
    the backtrace are useful, and some are omitted here.

    .. code-block:: console

       [Tremblay:server:(1) 0.000000] /source/tutorial/ndet-receive-s4u.cpp:27: [root/CRITICAL] Assertion value_got == 3 failed
       Backtrace (displayed in actor server):
         ->  #0 xbt_backtrace_display_current at /src/xbt/backtrace.cpp:31
         ->  #1 server() in ./ndet-receive-s4u
         (uninformative frames omitted)

-  After that comes a lot of information from the model-checker.

  - First, the error message itself. The ``xbt_assert`` in the code result in an ``abort()`` in the application, that is interpreted as an
    application crash by the model-checker.

    .. code-block:: console

       [0.000000] [mc_ModelChecker/INFO] **************************
       [0.000000] [mc_ModelChecker/INFO] ** CRASH IN THE PROGRAM **
       [0.000000] [mc_ModelChecker/INFO] **************************
       [0.000000] [mc_ModelChecker/INFO] From signal: Aborted
       [0.000000] [mc_ModelChecker/INFO] A core dump was generated by the system.

  - An execution trace is then given, listing all the actions that led to that faulty execution. This is not easy to read, because the API
    calls we made (put/get) are split in atomic calls (iSend+Wait/iRecv+Wait), and all executions are interleaved. Also, Mc SimGrid
    reports the first faulty execution it finds: it may not be the shorter one.

    .. code-block:: console

       [0.000000] [mc_explo/INFO] Counter-example execution trace:
       [0.000000] [mc_explo/INFO]   Actor 1 in Irecv ==> simcall: iRecv(mbox=0)
       [0.000000] [mc_explo/INFO]   Actor 2 in Isend ==> simcall: iSend(mbox=0)
       [0.000000] [mc_explo/INFO]   Actor 1 in Wait ==> simcall: WaitComm(from 2 to 1, mbox=0, no timeout)
       [0.000000] [mc_explo/INFO]   Actor 1 in Irecv ==> simcall: iRecv(mbox=0)
       [0.000000] [mc_explo/INFO]   Actor 2 in Wait ==> simcall: WaitComm(from 2 to 1, mbox=0, no timeout)
       [0.000000] [mc_explo/INFO]   Actor 4 in Isend ==> simcall: iSend(mbox=0)
       [0.000000] [mc_explo/INFO]   Actor 1 in Wait ==> simcall: WaitComm(from 4 to 1, mbox=0, no timeout)
       [0.000000] [mc_explo/INFO]   Actor 1 in Irecv ==> simcall: iRecv(mbox=0)
       [0.000000] [mc_explo/INFO]   Actor 3 in Isend ==> simcall: iSend(mbox=0)
       [0.000000] [mc_explo/INFO]   Actor 1 in Wait ==> simcall: WaitComm(from 3 to 1, mbox=0, no timeout)

  - Then, the execution path is given.

    .. code-block:: console

       [0.000000] [mc_explo/INFO] You can debug the problem (and see the whole details) by rerunning out 
                                  of simgrid-mc with --cfg=model-check/replay:'1;2;1;1;2;4;1;1;3;1'

    This is the magical string (here: ``1;2;1;1;2;4;1;1;3;1``) that you should pass to your simulator to follow the same
    execution path, but this time with many additional information. You can also add valgrind or a sanitizer to the party to
    ensure that you have no memory error in your code.


   .. tabs::

      .. group-tab:: simgrid/mc-slim

         Valgrind is not available in this docker image, but using the Address Sanitizer is quite easy:

          $ ./simgrid.sh clang++ ndet-receive-s4u.cpp -lsimgrid -fsanitize=address -fno-omit-frame-pointer -o ndet
          $ ./simgrid.sh valgrind ./ndet-receive-s4u small_platform.xml --cfg=model-check/replay:'1;2;1;1;2;4;1;1;3;1'
          ==402== Memcheck, a memory error detector
          ==402== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
          ==402== Using Valgrind-3.16.1 and LibVEX; rerun with -h for copyright info
          ==402== Command: ./ndet-receive-s4u small_platform.xml --cfg=model-check/replay:1;2;1;1;2;4;1;1;3;1
          ==402==
          [0.000000] [xbt_cfg/INFO] Configuration change: Set 'model-check/replay' to '1;2;1;1;2;4;1;1;3;1'
          [0.000000] [mc_record/INFO] path=1;2;1;1;2;4;1;1;3;1
          [Jupiter:client:(2) 0.000000] [example/INFO] Sending 1
          [Bourassa:client:(3) 0.000000] [example/INFO] Sending 2
          [Ginette:client:(4) 0.000000] [example/INFO] Sending 3
          [Jupiter:client:(2) 0.000000] [example/INFO] Sent!
          [Tremblay:server:(1) 0.000000] /source/tutorial/ndet-receive-s4u.cpp:27: [root/CRITICAL] Assertion value_got == 3 failed
          (some output ignored)
          ==402==
          ==402== Process terminating with default action of signal 6 (SIGABRT): dumping core
          ==402==    at 0x550FCE1: raise (raise.c:51)
          ==402==    by 0x54F9536: abort (abort.c:79)
          ==402==    by 0x10C696: server() (ndet-receive-s4u.cpp:27)
          (more valgrind output ignored)

      .. group-tab:: simgrid/tuto-mc

          .. code-block:: console

          $ valgrind ./ndet-receive-s4u small_platform.xml --cfg=model-check/replay:'1;2;1;1;2;4;1;1;3;1'
          ==402== Memcheck, a memory error detector
          ==402== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
          ==402== Using Valgrind-3.16.1 and LibVEX; rerun with -h for copyright info
          ==402== Command: ./ndet-receive-s4u small_platform.xml --cfg=model-check/replay:1;2;1;1;2;4;1;1;3;1
          ==402==
          [0.000000] [xbt_cfg/INFO] Configuration change: Set 'model-check/replay' to '1;2;1;1;2;4;1;1;3;1'
          [0.000000] [mc_record/INFO] path=1;2;1;1;2;4;1;1;3;1
          [Jupiter:client:(2) 0.000000] [example/INFO] Sending 1
          [Bourassa:client:(3) 0.000000] [example/INFO] Sending 2
          [Ginette:client:(4) 0.000000] [example/INFO] Sending 3
          [Jupiter:client:(2) 0.000000] [example/INFO] Sent!
          [Tremblay:server:(1) 0.000000] /source/tutorial/ndet-receive-s4u.cpp:27: [root/CRITICAL] Assertion value_got == 3 failed
          (some output ignored)
          ==402==
          ==402== Process terminating with default action of signal 6 (SIGABRT): dumping core
          ==402==    at 0x550FCE1: raise (raise.c:51)
          ==402==    by 0x54F9536: abort (abort.c:79)
          ==402==    by 0x10C696: server() (ndet-receive-s4u.cpp:27)
          (more valgrind output ignored)

  - Then, Mc SimGrid displays some statistics about the amount of states and traces visited to find this bug.

    .. code-block:: console

       [0.000000] [mc_dfs/INFO] DFS exploration ended. 19 unique states visited; 1 explored traces (12 transition replays, 31 states visited overall)

  - Finally, the model checker searches for the critical transition, that is, the execution step afer which the problem becomes
    unavoidable. Before that transition, some executions manage to avoid any issue and reach a non-faulty program execution,
    while after that transition, only faulty executions can be reached. We believe that this information could help you to
    better understand the issue, and we would love to hear what you think about this feature.

    .. code-block:: console

       [0.000000] [mc_ct/INFO] *********************************
       [0.000000] [mc_ct/INFO] *** CRITICAL TRANSITION FOUND ***
       [0.000000] [mc_ct/INFO] *********************************
       [0.000000] [mc_ct/INFO] Current knowledge of explored stack:
       [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 1 in Irecv ==> simcall: iRecv(mbox=0, comm=1, tag=0))
       [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 2 in Isend ==> simcall: iSend(mbox=0, comm=1, tag=0)
       [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 1 in Wait ==> simcall: WaitComm(from 2 to 1, mbox=0, no timeout, comm=1)
       [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 1 in Irecv ==> simcall: iRecv(mbox=0, comm=3, tag=0))
       [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 2 in Wait ==> simcall: WaitComm(from 2 to 1, mbox=0, no timeout, comm=1)
       [0.000000] [mc_ct/INFO]   (  CORRECT) Actor 4 in Isend ==> simcall: iSend(mbox=0, comm=3, tag=0)
       [0.000000] [mc_ct/INFO]   (INCORRECT) Actor 1 in Wait ==> simcall: WaitComm(from 4 to 1, mbox=0, no timeout, comm=3)
       [0.000000] [mc_ct/INFO]   (INCORRECT) Actor 1 in Irecv ==> simcall: iRecv(mbox=0, comm=5, tag=0))
       [0.000000] [mc_ct/INFO]   (INCORRECT) Actor 3 in Isend ==> simcall: iSend(mbox=0, comm=5, tag=0)
       [0.000000] [mc_ct/INFO]   (INCORRECT) Actor 1 in Wait ==> simcall: WaitComm(from 3 to 1, mbox=0, no timeout, comm=5)
       [0.000000] [mc_ct/INFO] Found the critical transition: Actor 4 ==> simcall: iSend(mbox=0, comm=3, tag=0)

Using MPI instead of S4U
^^^^^^^^^^^^^^^^^^^^^^^^

If you prefer, you can use MPI instead of the SimGrid-specific interface. Inspect the provided ``ndet-receive-mpi.c`` file: that's just a
translation of ``ndet-receive-s4u.cpp`` to MPI.

.. toggle-header::
   :header: Code of ``ndet-receive-mpi.c``: click here to open

   You can also `view it online <https://framagit.org/simgrid/tutorial-model-checking/-/blob/main/ndet-receive-mpi.c>`_.

   .. literalinclude:: tuto_mc/ndet-receive-mpi.c
      :language: cpp

|br|
You can compile and run it on top of SimGrid as follows (don't forget to add ``./simgrid.sh`` in front of all commands if you
use the ``simgrid/mc-slim`` image).

.. code-block:: console

   $ smpicc ndet-receive-mpi.c -o ndet-receive-mpi
   $ smpirun -np 4 -platform small_platform.xml ndet-receive-mpi

Interestingly enough, the bug is triggered on my machine even without Mc SimGrid, because the simulator happens to use the execution path
leading to it. It may not be the case on your machine, as this depends on the iteration order of an unsorted collection. Instead, we
should use Mc SimGrid to exhaustively explore the state space and trigger the bug in all cases.

.. code-block:: console

   $ smpirun -wrapper simgrid-mc -np 4 -platform small_platform.xml ndet-receive-mpi

The produced output is then very similar to the one you get with S4U, even if the exact execution path leading to the bug may
differs. You can also trigger a given execution path out of the model-checker, for example to explore it with valgrind (if
you're not in the ``simgrid/mc-slim`` docker image) or to get more information.

.. code-block:: console

   # With valgrind out of simgrid-mc if you're not in mc-slim
   $ smpirun -wrapper valgrind -np 4 -platform small_platform.xml --cfg=model-check/replay:'1;2;1;1;4;1;1;3;1' ndet-receive-mpi
   # Still within simgrid-mc but with the full logs, the only way to go if you're in mc-slim
   $ ./simgrid.sh smpirun -wrapper simgrid-mc -np 4 -platform small_platform.xml --cfg=model-check/replay:'1;2;1;1;4;1;1;3;1' ndet-receive-mpi

Under the hood
^^^^^^^^^^^^^^

If you want to run such analysis on your own code, out of the provided docker, there is some steps that you should take.

- SimGrid should naturally :ref:`be compiled <install_src>` with model-checking support. This requires some extra dependencies
  (documented on the :ref:`relevant page <install_src>`). 
- Mc SimGrid uses the ``ptrace`` system call to spy on the verified application. Some versions of Docker forbid the use of this call by
  default for security reason (it could be used to escape the docker containment with older versions of Linux). If you encounter this
  issue, you should either update your settings (the security issue was solved in later versions of Linux), or add ``--cap-add
  SYS_PTRACE`` to the docker parameters, as hinted by the text.

Going further
-------------

This tutorial is not complete yet, as there is nothing on reduction techniques. For now, the best source of information on these
topics is `this old tutorial <https://simgrid.org/tutorials/simgrid-mc-101.pdf>`_ and `that old presentation
<http://people.irisa.fr/Martin.Quinson/blog/2018/0123/McSimGrid-Boston.pdf>`_. But be warned that these source of information
are very old: the liveness verification was removed in v3.35, even if these docs still mention it.

.. |br| raw:: html

   <br />
