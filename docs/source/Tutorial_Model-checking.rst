.. _usecase_modelchecking:

Formal Verification and Model-checking
======================================

SimGrid can not only predict the performance of your application, but also assess its correctness through formal methods. Mc SimGrid is
a full-featured model-checker that is embedded in the SimGrid framework. It can be used to formally verify safety and liveness
properties on codes running on top of SimGrid, be it :ref:`simple algorithms <usecase_simalgo>` or :ref:`full MPI applications
<usecase_smpi>`.

Primer on formal methods
------------------------

Formal methods are techniques leveraging mathematics to test and assess systems. They are routinely used to assess computer hardware,
transportation systems or any other complex engineering process. Among these methods, model-checking is a technique to automatically
prove that a given model verifies a given property by systematically checking all states of the model. The property and model are
written in a mathematical language and fed to an automated tool called model checker. When the model does not verify the property, the
model checker gives a counter-example that can be used to refine and improve the model. Conversely, if no counter-example can be found
after an exhaustive exploration of the model, we know that the property holds for the model. It may also happen that the model is too
large to be exhaustively explored, in which case the model-checker is not conclusive. Model checkers rely on so-called reduction
techniques (based on symmetries and equivalence) to efficiently explore the system state.

Dynamic verification applies similar ideas to programs, without requiring a mathematical model of the system. Instead, the program
itself is used as a model to verify against a property. Along these lines, Mc SimGrid is a stateful model checker: it does not leverage
static analysis nor symbolic execution. Instead, the program is simply executed through all possible outcomes. On indecision points, a
system checkpoint is taken, the first branch is executed exhaustively, and then the system is roll back to that point to explore the
other branch.

Mc SimGrid targets distributed applications that interact through message passing or through synchronization mechanisms (mutex,
barrier, etc). Since it does not explicitly observe memory accesses, Mc SimGrid cannot automatically detect race conditions in
multithreaded programs. It can however be used to detect misuses of the synchronization functions, such as the ones resulting in
deadlocks.

Mc SimGrid can be used to verify classical `safety and liveness properties <https://en.wikipedia.org/wiki/Linear_time_property>`_, but
also `communication determinism <https://hal.inria.fr/hal-01953167/document>`_, a property that allows more efficient solutions toward
fault-tolerance. It can alleviate the state space explosion problem through `Dynamic Partial Ordering Reduction (DPOR)
<https://en.wikipedia.org/wiki/Partial_order_reduction>`_ and `state equality <https://hal.inria.fr/hal-01900120/document>`_. Note that
Mc SimGrid is currently less mature than other parts of the framework, but it improves every month. Please report any question and
issue so that we can further improve it.

Getting Mc SimGrid
------------------

It is included in the SimGrid source code, but it is not compiled in by default as it induces a small performance overhead to the
simulations. It is also not activated in the Debian package, nor in the Java or Python binary distributions. If you just plan to
experiment with Mc SimGrid, the easiest is to get the corresponding docker image. On the long term, you probably want to install it on
your machine: it works out of the box on Linux, Windows (with WSL2) and FreeBSD. Simply request it from cmake (``cmake
-Denable_model-checking .``) and then compile SimGrid :ref:`as usual <install_src>`. Unfortunately, Mc SimGrid does not work natively
on Mac OS X yet, so mac users should stick to the docker method for now.

.. code-block:: console

   $ docker image pull simgrid/tuto-mc
   $ mkdir ~/tuto-mcsimgrid # or chose another directory to share between your computer and the docker container
   $ docker run --user $UID:$GID -it --rm --name mcsimgrid --volume ~/tuto-mcsimgrid:/source/tutorial simgrid/tuto-mc bash

In the container, you have access to the following directories of interest:

- ``/source/tutorial``: A view to the ``~/tuto-mcsimgrid`` directory on your disk, out of the container.
  Edit the files you want from your computer and save them in ``~/tuto-mcsimgrid``;
  Compile and use them immediately within the container in ``/source/tutorial``.
- ``/source/tuto-mc.git``: Files provided with this tutorial.
- ``/source/simgrid.git``: Source code of SimGrid, pre-configured in MC mode. The framework is also installed in ``/usr``
  so the source code is only provided for your information.

Lab1: non-deterministic receive
-------------------------------

Motivational example
^^^^^^^^^^^^^^^^^^^^

Let's go with a first example of a bugged program. Once in the container, copy all files from the tutorial into the directory shared
between your host computer and the container.

.. code-block:: console

  # From within the container
  $ cp -r /source/tuto-mc.git/* /source/tutorial/
  $ cd /source/tutorial/

Several files should have appeared in the ``~/tuto-mcsimgrid`` directory of your computer.
This tutorial uses `ndet-receive-s4u.cpp <https://framagit.org/simgrid/tutorial-model-checking/-/blob/main/ndet-receive-s4u.cpp>`_,
that uses the :ref:`S4U interface <S4U_doc>` of SimGrid, but we provide a
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

If you think of it, that's weird that this code works: all the messages are sent at the exact same time (t=0), so there is no reason for
the message ``3`` to arrive last. Depending on the link speed, any order should be possible. To trigger the bug, you could fiddle with the
source code and/or the platform file, but this is not a method. Time to start Mc SimGrid, the SimGrid model checker, to exhaustively test
all message orders. For that, you simply launch your simulation as a parameter to the ``simgrid-mc`` binary as you would do with ``valgrind``:

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

       [0.000000] [mc_safety/INFO] Check a safety property. Reduction is: dpor.
       [Jupiter:client:(2) 0.000000] [example/INFO] Sending 1
       [Bourassa:client:(3) 0.000000] [example/INFO] Sending 2
       [Ginette:client:(4) 0.000000] [example/INFO] Sending 3
       [Jupiter:client:(2) 0.000000] [example/INFO] Sent!
       [Bourassa:client:(3) 0.000000] [example/INFO] Sent!
       [Tremblay:server:(1) 0.000000] [example/INFO] OK
       [Ginette:client:(4) 0.000000] [example/INFO] Sent!
       [Jupiter:client:(2) 0.000000] [example/INFO] Sent!
       [Bourassa:client:(3) 0.000000] [example/INFO] Sent!
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
         ->  0# xbt_backtrace_display_current at /source/simgrid.git/src/xbt/backtrace.cpp:30
         ->  1# server() at /source/tutorial/ndet-receive-s4u.cpp:27

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
    reports the first faulty execution it finds: it may not be the shorter possible one.

    .. code-block:: console

       [0.000000] [mc_ModelChecker/INFO] Counter-example execution trace:
       [0.000000] [mc_ModelChecker/INFO]   1: iRecv(mbox=0)
       [0.000000] [mc_ModelChecker/INFO]   2: iSend(mbox=0)
       [0.000000] [mc_ModelChecker/INFO]   1: WaitComm(from 2 to 1, mbox=0, no timeout)
       [0.000000] [mc_ModelChecker/INFO]   1: iRecv(mbox=0)
       [0.000000] [mc_ModelChecker/INFO]   2: WaitComm(from 2 to 1, mbox=0, no timeout)
       [0.000000] [mc_ModelChecker/INFO]   4: iSend(mbox=0)
       [0.000000] [mc_ModelChecker/INFO]   1: WaitComm(from 4 to 1, mbox=0, no timeout)
       [0.000000] [mc_ModelChecker/INFO]   1: iRecv(mbox=0)
       [0.000000] [mc_ModelChecker/INFO]   3: iSend(mbox=0)
       [0.000000] [mc_ModelChecker/INFO]   1: WaitComm(from 3 to 1, mbox=0, no timeout)

  - Then, the execution path is given.

    .. code-block:: console

       [0.000000] [mc_record/INFO] Path = 1;2;1;1;2;4;1;1;3;1

    This is the magical string (here: ``1;2;1;1;2;4;1;1;3;1``) that you should pass to your simulator to follow the same execution path
    without ``simgrid-mc``. This is because ``simgrid-mc`` forbids to use a debugger such as gdb or valgrind on the code during the
    model-checking. For example, you can trigger the same execution in valgrind as follows:

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

  - Then, Mc SimGrid displays some statistics about the amount of expanded states (the unique states in which your program was at a given
    point of the exploration), the visited states (the amount of times we visited another state -- the same state may have been visited
    several times) and the amount of transitions.

    .. code-block:: console

       [0.000000] [mc_dfs/INFO] DFS exploration ended. 22 unique states visited; 4 backtracks (56 transition replays, 30 states visited overall)

  - Finally, the application stack trace is displayed as the model-checker sees it. It should be the same as the one displayed from the
    application side, unless you found a bug our tools.

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
You can compile and run it on top of SimGrid as follows.

.. code-block:: console

   $ smpicc ndet-receive-mpi.c -o ndet-receive-mpi
   $ smpirun -np 4 -platform small_platform.xml ndet-receive-mpi

Interestingly enough, the bug is triggered on my machine even without Mc SimGrid, because the simulator happens to use the execution path
leading to it. It may not be the case on your machine, as this depends on the iteration order of an unsorted collection. Instead, we
should use Mc SimGrid to exhaustively explore the state space and trigger the bug in all cases.

.. code-block:: console

   $ smpirun -wrapper simgrid-mc -np 4 -platform small_platform.xml ndet-receive-mpi

The produced output is then very similar to the one you get with S4U, even if the exact execution path leading to the bug may differs. You
can also trigger a given execution path out of the model-checker, for example to explore it with valgrind.

.. code-block:: console

   $ smpirun -wrapper valgrind -np 4 -platform small_platform.xml --cfg=model-check/replay:'1;2;1;1;4;1;1;3;1' ndet-receive-mpi

Under the hood
^^^^^^^^^^^^^^

If you want to run such analysis on your own code, out of the provided docker, there is some steps that you should take.

- SimGrid should naturally :ref:`be compiled <install_src>` with model-checking support. This requires a full set of dependencies
  (documented on the :ref:`relevant page <install_src>`) and should not be activated by default as there is a small performance penalty for
  codes using a SimGrid with MC enabled (even if you don't activate the model-checking at run time).
- You should pass some specific flags to the linker when compiling your application: ``-Wl,-znorelro -Wl,-znoseparate-code`` In the
  docker, the provided CMakeLists.txt provides them for you when compiling the provided code. ``smpicc`` and friends also add this
  parameter automatically.
- Also install ``libboost-stacktrace-dev`` to display nice backtraces from the application side (the one from the model-checking side is
  available in any case, but it contains less details).
- Mc SimGrid uses the ``ptrace`` system call to spy on the verified application. Some versions of Docker forbid the use of this call by
  default for security reason (it could be used to escape the docker containment with older versions of Linux). If you encounter this
  issue, you should either update your settings (the security issue was solved in later versions of Linux), or add ``--cap-add
  SYS_PTRACE`` to the docker parameters, as hinted by the text.

Going further
-------------

This tutorial is not complete yet, as there is nothing on reduction
techniques nor on liveness properties. For now, the best source of
information on these topics is `this old tutorial
<https://simgrid.org/tutorials/simgrid-mc-101.pdf>`_ and `that old
presentation
<http://people.irisa.fr/Martin.Quinson/blog/2018/0123/McSimGrid-Boston.pdf>`_.

.. |br| raw:: html

   <br />
