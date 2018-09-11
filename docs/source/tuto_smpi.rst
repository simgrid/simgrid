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

As a SMPI user, you are supposed to provide a description of your
virtual platform, that is mostly a set of simulated hosts and network
links with some performance characteristics. SimGrid provides a plenty
of :ref:`documentation <platform>`_ and examples (in the
`examples/platform <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms>`_
source directory), and this section only show a series of introductory
examples.

Simple Example with 3 hosts
---------------------------

At the most basic level, you can describe your simulated platform as a
graph of hosts and network links. For instance:

.. image:: /tuto_smpi/img/3hosts.png
   :align: center

.. hidden-code-block:: xml
    :starthidden: True
    :label: See the XML platform description file...

    <?xml version='1.0'?>
    <!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd">
    <platform version="4.1">
      <zone id="AS0" routing="Full">
        <host id="host0" speed="1Gf"/>
        <host id="host1" speed="2Gf"/>
        <host id="host2" speed="40Gf"/>
        <link id="link0" bandwidth="125MBps" latency="100us"/>
        <link id="link1" bandwidth="50MBps" latency="150us"/>
        <link id="link2" bandwidth="250MBps" latency="50us"/>
        <route src="host0" dst="host1"><link_ctn id="link0"/><link_ctn id="link1"/></route>
        <route src="host1" dst="host2"><link_ctn id="link1"/><link_ctn id="link2"/></route>
        <route src="host0" dst="host2"><link_ctn id="link0"/><link_ctn id="link2"/></route>
      </zone>
    </platform>

In this XML, note the way in which hosts, links, and routes are
defined. All hosts are defined with a power (i.e., compute speed in
Gflops), and links with a latency (in us) and bandwidth (in MBytes per
second). Other units are possible and written as expected. By default,
routes are symmetrical.
       
..  LocalWords:  SimGrid
