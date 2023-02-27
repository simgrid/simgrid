.. _intro_concepts:

Introduction
============

.. raw:: html

   <object data="graphical-toc.svg" type="image/svg+xml"></object>
   <br/>
   <br/>

What is SimGrid
---------------

SimGrid is a framework for developing simulators of distributed applications targeting distributed platforms, which can in turn
be used to prototype, evaluate and compare relevant platform configurations, system designs, and algorithmic approaches.

Typical Study based on SimGrid
------------------------------

Here are some questions on which SimGrid is particularly relevant:

 - **Compare an Application to another**. This is a classical use case for scientists, who use SimGrid to test how their
   contributed solution compares to the existing solutions from the literature.

 - **Design the best [Simulated] Platform for a given Application.** Tweaking the platform file is much easier than building a
   new real platform for testing purposes. SimGrid also allows for the co-design of the platform and the application by
   modifying both of them.

 - **Debug Real Applications**. With real systems, is sometimes difficult to reproduce the exact run leading to the bug that you
   are tracking. With SimGrid, you are *clairvoyant* about your *reproducible experiments*: you can explore every part of the
   system, and your probe will not change the simulated state. It also makes it easy to mock some parts of the real system that
   are not under study.

 - **Formally assess an algorithm**. Inspirated from model checking, this execution mode does not use the performance models to
   determine the application outcome, but instead explore all causally possible outcomes of your application. This exhaustive
   search is perfect to find bugs that are difficult to trigger otherwise, but it will probably not manage to completely cover
   large applications.

Any SimGrid study entails the following components:

 - The studied **application**. This can be either a distributed algorithm described in our simple API (either in C++, Python or
   C) or a full-featured real parallel application using for example the MPI interface :ref:`(more info) <application>`.

 - The **simulated platform**. This is a description (in either XML or C++) of a given distributed system (machines, links,
   disks, clusters, etc). SimGrid makes it easy to augment the simulated platform with a dynamic scenario where for example the
   links are slowed down (because of external usage) or the machines fail. You even have support to specify the applicative
   workload that you want to feed to your application :ref:`(more info) <platform>`.

 - The application's **deployment description**. To run your application, you have to describe how it should be deployed on the
   simulated platform. You need to specify which process is mapped onto which machine, along with their parameters :ref:`(more
   info) <scenario>`.

 - The **platform models**. They describe how the simulated platform reacts to the activities of the application. For example,
   the models compute the time taken by a given communication on the simulated platform. These models are already included in
   SimGrid, and you only need to pick one and maybe adapt its configuration to get your results :ref:`(more info) <models>`.

These components are put together to run a **simulation**, that is an experiment or a probe. Simulations produce **outcomes**
(logs, visualization, or statistical analysis) that help to answer the **question** targeted by your study. It provides
information on the timing performance and the energy consumption of your application, taking network, CPU and disk resources
into account by default, and memory can also be modeled. SimGrid differs from many other tools by accurately modeling the contention
resulting from concurrent network usages.

We work hard to make SimGrid easy to use, but you should not blindly trust your results and always strive to double-check the
predictions. Questioning the realism of your results will lead you to better :ref:`calibrate the models <models_calibration>`,
which is the best way to ensure accurate predictions. Please also refer to the section :ref:`howto_science`.

Using SimGrid in practice
-------------------------

SimGrid is versatile and can be used in many ways, but the most typical setup is to specify your algorithm as a C++ or Python
program using our API, along with one of the provided XML platform files as shown in the **first tutorial** on
:ref:`usecase_simalgo`. If your application is already written in MPI, then you are lucky because SimGrid comes with a very good
support of this communication interface, as explained in our **second tutorial** on :ref:`usecase_smpi`. The **third tutorial** is on
:ref:`usecase_modelchecking`. Docker images are provided to run these tutorials without installing anything.

SimGrid comes with an :ref:`extensive amount of examples <s4u_examples>`, so that you can quick-start your simulator by
assembling and modifying some of the provided examples (see :ref:`this section <setup_your_own>` on how to get your own project
to compile with SimGrid). An extensive documentation is available from the left menu bar. If you want to get an idea of how
SimGrid works to better use it, you can refer to the :ref:`framework design presentation <design>`.

SimGrid Success Stories
-----------------------

SimGrid was cited in over 3,000 scientific papers (according to Google
Scholar). Among them,
`over 500 publications <https://simgrid.org/usages.html>`_
(written by hundreds of individuals) use SimGrid as a scientific
instrument to conduct their experimental evaluation. These
numbers do not include the articles contributing to SimGrid.
This instrument was used in many research communities, such as
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
(`paper <http://hal.inria.fr/hal-00919507>`_). In this case,
some differences between the prediction and the real timings were due to
misconfigurations with the real platform. To some extent,
SimGrid could even be used to debug the real platform :)

SimGrid is also used to debug, improve, and tune several large
applications.
`BigDFT <http://bigdft.org>`_ (a massively parallel code
computing the electronic structure of chemical elements developed by
the CEA), `StarPU <http://starpu.gforge.inria.fr/>`_ (a
Unified Runtime System for Heterogeneous Multicore Architectures
developed by Inria Bordeaux), and
`TomP2P <https://tomp2p.net/dev/simgrid/>`_ (a high-performance
key-value pair storage library developed at the University of Zurich).
Some of these applications enjoy large user communities themselves.

SimGrid Limits
--------------

This framework is by no means the holy grail, able to solve
every problem on Earth.

**SimGrid scope is limited to distributed systems.** Real-time
multi-threaded systems are out of this scope. You could probably tweak
SimGrid for such studies (or the framework could be extended
in this direction), but another framework specifically targeting such a
use case would probably be more suited.

**There is currently no support for 5G or LoRa networks**.
The framework could certainly be improved in this direction, but this
still has to be done.

**There is no perfect model, only models adapted to your study.** The SimGrid
models target fast and large studies, and yet they target realistic results. In
particular, our models abstract away parameters and phenomena that are often
irrelevant to reality in our context.

SimGrid is obviously not intended for a study of any phenomenon that our
abstraction removes. Here are some **studies that you should not do with
SimGrid**:

 - Studying the effect of L3 vs. L2 cache effects on your application
 - Comparing kernel schedulers and policies
 - Comparing variants of TCP
 - Exploring pathological cases where TCP breaks down, resulting in
   abnormal executions.
 - Studying security aspects of your application, in presence of
   malicious agents.


..  LocalWords:  SimGrid
