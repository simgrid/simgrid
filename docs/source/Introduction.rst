.. _intro_concepts:

Introduction
============

.. raw:: html

   <object data="graphical-toc.svg" type="image/svg+xml"></object>
   <br/>
   <br/>

What is SimGrid
---------------

SimGrid is a framework for developing simulators of distributed application executions on distributed platforms. It can 
be used to prototype, evaluate and compare relevant platform configurations, system designs, and algorithmic approaches.

What SimGrid allows you to do
----------------------------

Here are some objectives for which SimGrid is particularly relevant and has been used extensively:

 - **Compare designs**. This is a classical use case for researchers/developers, who use SimGrid to assess how their contributed solution (a platform, system, application, and/or algorithm design) compares to existing solutions from the literature.

 - **Design the best [Simulated] Platform for a given Application.** Modifying a platform file use to drive simulations is much easier than building 
   real-world platforms for testing purposes. SimGrid also allows for the co-design of the platform and the application, as both can be modified with little work.

 - **Debug Real Applications**. With real systems it is often difficult to reproduce the exact run that would lead to a bug that is being tracked. 
   With SimGrid, you are *clairvoyant* about your *reproducible experiments*: you can explore every part of the
   system, and your exploration will not change the simulated state. It also makes it easy to mock or abstract away parts of the real system that
   are not under study.

 - **Formally assess an algorithm**. Inspired by model checking, SimGrid provides an execution mode that does not 
   quantify an application's performance behavior, but that instead explores all causally possible outcomes of the application so as to evaluate application correctness. This exhaustive
   search is ideal for finding bugs that are difficult to trigger experimentally. But because it is exhaustive, there is a limit to the scale of the applications for which it can be used. 

Anatomy of a project that uses SimGrid
--------------------------------------

Any project that uses SimGrid as its simulation framework comprises the following components:

 - An **application**. An application consists of one or more process that can either implement distributed algorithms described using a simple API (either in C++, Python or
   C) or be part of a full-featured real  parallel application implemented with, for example, the MPI standard :ref:`(more info) <application>`.

 - A **simulated platform**. This is a description (in either XML or C++) of a distributed system's hardware (machines, links,
   disks, clusters, etc). SimGrid makes it straightforward to augment the simulated platform with dynamic behaviors where, for example, the
   links are slowed down (because of external usage) or the machines fail :ref:`(more info) <platform>`.

 - An application's **deployment description**. To simulate the execution of the application on the platform, they way in which the application is
   deployed on the platform must be described.  This is done by specifying which process is mapped onto which machine :ref:`(more
   info) <scenario>`.

 - **Platform models**. SimGrid implements models that describe how the simulated platform reacts to the simulated activities performed my
  application processes.  SimGrid provides a range of documented models,
  which the user can select and configure for their particular use case.  A
  big selling point of SimGrid, which sets it apart from its competitors,
  is that it can accurately model the network contention that results from
  concurrent communications. :ref:`(more info) <models>`.


The above components are put together to run a **simulation experiment**
that produces **outcomes** (logs, visualization, statistics) that help
answer the user's research and development **question**. The outcomes
typically include a timeline of the application execution and information
about its energy consumption.  


We work hard to make SimGrid easy to use, but you should not blindly trust your results and always strive to validate
the simulation outcomes. Assessing the realism of these outcomes will lead you to better :ref:`calibrate the models <models_calibration>`,
which is the best way to achieved high simulation accuracy. Please refer to the section :ref:`howto_science`.

Using SimGrid in practice
-------------------------

SimGrid is versatile and can be used in many ways, but the most typical setup is to specify your algorithm as a C++ or Python
program using our API, along with one of the provided XML platform files as shown in the **first tutorial** on
:ref:`usecase_simalgo`. If your application is already written in MPI, then you are in luck because SimGrid comes with MPI support, 
as explained in our **second tutorial** on :ref:`usecase_smpi`. The **third tutorial** is on
:ref:`usecase_modelchecking`. Docker images are provided to run these tutorials without installing any software, other than Docker, on your machine.

SimGrid comes with :ref:`many examples <s4u_examples>`, so that you can quick-start your simulator by
assembling and modifying some of the provided examples (see :ref:`this section <setup_your_own>` on how to get your own project
to compile with SimGrid). An extensive documentation is available from the left menu bar. If you want to get an idea of how
SimGrid works you can read about its :ref:`design goals <design>`.

SimGrid Success Stories
-----------------------

SimGrid was cited in over 3,000 scientific papers (according to Google
Scholar). Among them,
`over 500 publications <https://simgrid.org/usages.html>`_
(written by hundreds of individuals) use SimGrid as a scientific
instrument to conduct experimental evaluations. These
numbers do not include those articles that directly contribute to SimGrid itself.
SimGrid was used in many research communities, such as
`High-Performance Computing <https://hal.inria.fr/inria-00580599/>`_,
`Cloud Computing <http://dx.doi.org/10.1109/CLOUD.2015.125>`_,
`Workflow Scheduling <http://dl.acm.org/citation.cfm?id=2310096.2310195>`_,
`Big Data <https://hal.inria.fr/hal-01199200/>`_ and
`MapReduce <http://dx.doi.org/10.1109/WSCAD-SSC.2012.18>`_,
`Data Grid <http://ieeexplore.ieee.org/document/7515695/>`_,
`Volunteer Computing <http://www.sciencedirect.com/science/article/pii/S1569190X17301028>`_,
`Peer-to-Peer Computing <https://hal.archives-ouvertes.fr/hal-01152469/>`_,
`Network Architecture <http://dx.doi.org/10.1109/TPDS.2016.2613043>`_,
`Fog Computing <http://ieeexplore.ieee.org/document/7946412/>`_, or
`Batch Scheduling <https://hal.archives-ouvertes.fr/hal-01333471>`_.

If your platform description is accurate enough (see
`here <http://hal.inria.fr/hal-00907887>`_ or
`there <https://hal.inria.fr/hal-01523608>`_),
SimGrid can provide high-quality performance predictions. For example,
we determined the speedup achieved by the Tibidabo ARM-based
cluster before its construction
(`paper <http://hal.inria.fr/hal-00919507>`_). Some
differences between the simulated and the real timings were observed, and
turned out to be due to
misconfigurations in the real platform! 
SimGrid can thus even be used to debug a real platform :)

SimGrid is also used to debug, improve, and tune several large
applications.
`BigDFT <http://bigdft.org>`_ (a massively parallel code
for computing the electronic structure of chemical elements developed by
the CEA), `StarPU <http://starpu.gforge.inria.fr/>`_ (a
Unified Runtime System for Heterogeneous Multicore Architectures
developed by Inria Bordeaux), and
`TomP2P <https://tomp2p.net/dev/simgrid/>`_ (a high-performance
key-value pair storage library developed at the University of Zurich).
Some of these applications enjoy large user communities themselves.

SimGrid Limits
--------------

SimGrid is by no means the holy grail that is able to solve every conceivable simulation problem.

**SimGrid's scope is limited to distributed systems.** Real-time
multi-threaded systems are out of this scope. You could probably use and/or
extend SimGrid for this purpose, but another framework that specifically
targets this use case would probably be more suitable.

**There is currently no support for 5G or LoRa networks**.
SimGrid could certainly be extended with models for these networks, but this
yet to be done.

**There is no perfect model, only models adapted to your purposes.** SimGrid's
models were designed to make it possible to run fast and accurate
simulations of large systems. As a result, the models abstract away many
parameters and phenomena that are often irrelevant for most use cases in the
field. This means that SimGrid cannot be used to study any phenomenon that our
model do not capture.  Here are some **phenomena that you currently cannot study with
SimGrid**:

 - Studying the effect of L3 vs. L2 cache effects on your application;
 - Comparing kernel schedulers and policies;
 - Comparing variants of TCP;
 - Exploring pathological cases where TCP breaks down, resulting in
   abnormal executions;
 - Studying security aspects of your application, in the presence of
   malicious agents.


..  LocalWords:  SimGrid
