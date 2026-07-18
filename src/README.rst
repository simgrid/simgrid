.. This file follows the ReStructured syntax to be included in the documentation (see docs/source/Contributors_Documentation.rst), 
.. but it should remain readable directly when browsing the source tree.

.. _src_architecture:

Codebase architecture
######################

Before diving into the code, you may want to read :ref:`intro_concepts` for a user-level view of what SimGrid does, and
:ref:`design` for the concepts (actors, activities, resources, maestro, simcalls, the LMM solver) that the whole codebase is
built around.

Note that this section describes the organization as of this writing and is maintained by hand; it is not guaranteed to be
exhaustive or perfectly up to date. When in doubt, ``git grep -n`` and the header files are the ground truth.

Interesting top-level directories
*********************************

- The ``src/`` directory contains all of SimGrid source code. Its content is detailed below.
- The ``examples/`` directory contains :ref:`many examples <s4u_examples>` of the S4U interface that can be copied and assembled
  to build your own project. These programs are also included in the :ref:`test suite <contrib_tests>` to ensure that they work
  as intended.
- The ``docs/`` directory contains the source of the Sphinx documentation that is available online:
  `stable release <https://simgrid.org/doc/latest/>`_ or `current git version <https://simgrid.frama.io/simgrid/index.html>`_.
- The ``include/`` directory contains all public headers.
- The ``teshsuite/`` directory contains :ref:`tests <contrib_tests>` that are not intended to be usable by users as building
  blocks for their own project.
- The ``tools/`` directory contains various helper scripts and tools used to build and maintain SimGrid.
- ``src/3rd-party`` contains vendored third-party code (e.g. the Catch2 test framework).

User-facing ``src/`` subdirectories
***********************************

The S4U interface, for algorithms.

- ``src/s4u``: The public C++ API (actors, activities, resources) that most users program against, described in :ref:`S4U_doc`.
- ``src/bindings/python`` and ``src/bindings/java``: Bindings of the S4U API to the Python and Java languages, respectively.
  Their user documentation is blended within the :ref:`s4u documentation <S4U_doc>`.

- ``src/plugins``: Built-in plugins (host/link energy, host/link load, Wi-Fi, batteries, solar panels, CRAH, etc.), documented
  in :ref:`cfg=plugin`. The plugin mechanism itself is documented in :ref:`plugins`. 


The SMPI interface, for MPI applications.

- ``src/smpi``: SMPI, the simulated MPI implementation (built on top of ``src/s4u``).

  - ``src/smpi/mpi``: The user-facing ``MPI_*`` functions.
  - ``src/smpi/colls``: Implementations of MPI collective communication algorithms, borrowed in the project your simulation wants to mimick (OpenMPI, MPICH, etc).
  - ``src/smpi/internals``:  Internal machinery of SMPI (process management, memory sharing, benchmarking, etc.).
  - ``src/smpi/bindings``:  Fortran bindings of SMPI.
  - ``src/smpi/plugins``: Plugins specific to SMPI simulations. Currently only contains an old attempt toward Adaptative MPI,
    back from 2018. We may want to remove this old and poorly documented piece of code.
  - ``src/smpi/include``: Internal headers shared across the SMPI implementation.

Other interfaces and generic files.

- ``src/simgrid``: Top-level support code shared across the project (configuration, exceptions, versioning).
- ``src/xbt``: The XBT toolbox: logging, data structures, and other support code used throughout the project (see :ref:`xbt`).
- ``src/sthread``: Interception of pthread-based programs. This is mostly useful in conjonction with Mc SimGrid.
- ``src/dag``: The DAG-of-activities programming model, including the DAX format loader. TODO: move it to src/s4u/
- ``src/instr``: Execution tracing, producing Paje traces for post-mortem visualization.

Kernel code architecture
************************

As explained in :ref:`design`, user-facing abstractions (in ``src/s4u``) and their kernel-side counterparts (in ``src/kernel``)
are kept separate on purpose: expect to find a matching pair of classes on both sides for most abstractions.

- ``src/kernel/actor``: The actor and maestro implementation, and simcall handling.
- ``src/kernel/activity``: Kernel-side implementation of activities (executions, communications, I/O, synchronization objects).
- ``src/kernel/resource``: Kernel-side resource models (CPU, network link, disk...).
- ``src/kernel/routing``: Network zones and routing between hosts.
- ``src/kernel/lmm``: The linear max-min (LMM) solver at the core of the resource-sharing models.
- ``src/kernel/context``: The context-switching mechanisms between actors and the maestro (the "context factories").
- ``src/kernel/timer``: Simulated timers.
- ``src/kernel/xml``: The XML platform-file parser.

Model checker code architecture
*******************************

As explained in the :ref:`usecase_modelchecking` tutorial, Mc SimGrid is a model-checker that can exhaustively explore the state
space of any application that can run in SimGrid (be it with S4U directly or with the SMPI or sthread shims). Its source code is
located in ``src/mc``.

- ``src/mc/explo``: The exploration algorithms (DFS, UDPOR/DPOR-based reduction, etc.).
- ``src/mc/api``: The API used by the exploration algorithms to interact with the checked application.
- ``src/mc/remote``: Communication with, and inspection of, the separate process being model-checked.
- ``src/mc/smemory``: Memory-access instrumentation and tracking, used to detect race conditions in the verified application.
- ``src/mc/transition``: Representation of the transitions (simcalls) explored by the model checker.
