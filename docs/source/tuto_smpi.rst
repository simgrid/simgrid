.. _usecase_smpi:

Simulating MPI Applications
===========================

Discover SMPI
-------------

SimGrid can not only :ref:`simulate algorithms <usecase_simalgo>`, but
it can also be used to execute real MPI applications on top of
virtual, simulated platforms with the SMPI module. Even complex
C/C++/F77/F90 applications should run out of the box in this
environment. In fact, almost all proxy apps provided by the `ExaScale
Project <https://proxyapps.exascaleproject.org/>`_ only require minor
modifications to `run on top of SMPI
<https://github.com/simgrid/SMPI-proxy-apps/>`_.

This setting permits to debug your MPI applications in a perfectly
reproducible setup, with no Heisenbugs. Enjoy the full Clairevoyance
provided by the simulator while running what-if analysis on platforms
that are still to be built! Several `production-grade MPI applications
<https://framagit.org/simgrid/SMPI-proxy-apps#full-scale-applications>`_
use SimGrid for their integration and performance testing.

MPI 2.2 is already partially covered: over 160 primitives are
supported. Some parts of the standard are still missing: MPI-IO, MPI3
collectives, spawning ranks, and some others. If one of the functions
you use is still missing, please drop us an email. We may find the
time to implement it for you.

Multi-threading support is very limited in SMPI. Only funneled
applications are supported: at most one thread per rank can issue any
MPI calls. For better timing predictions, your application should even
be completely mono-threaded. Using OpenMP (or pthreads directly) may
greatly decrease SimGrid predictive power. That may still be OK if you
only plan to debug your application in a reproducible setup, without
any performance-related analysis.

How does it work?
^^^^^^^^^^^^^^^^^

In SMPI, communications are simulated while computations are
emulated. This means that while computations occur as they would in
the real systems, communication calls are intercepted and achived by
the simulator.

To start using SMPI, you just need to compile your application with
``smpicc`` instead of ``mpicc``, or with ``smpiff`` instead of
``mpiff``, or with ``smpicxx`` instead of ``mpicxx``. Then, the only
difference between the classical ``mpirun`` and the new ``smpirun`` is
that it requires a new parameter ``-platform`` with a file describing
the virtual platform on which your application shall run.

Internally, all ranks of your application are executed as threads of a
single unix process. That's not a problem if your application has
global variables, because ``smpirun`` loads one application instance
per MPI rank as if it was another dynamic library. Then, MPI
communication calls are implemented using SimGrid: data is exchanged
through memory copy, while the simulator's performance models are used
to predict the time taken by each communications. Any computations
occuring between two MPI calls are benchmarked, and the corresponding
time is reported into the simulator.

.. image:: /tuto_smpi/img/big-picture.png
   :align: center


Describing Your Platform
========================

       
..  LocalWords:  SimGrid
