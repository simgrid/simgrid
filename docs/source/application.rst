.. _application:

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" width="100%" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("ApplicationBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

Describing your Application
***************************

Every SimGrid simulation entails a distributed application, that
virtually executes on the simulated platform. This application can
be either an existing MPI program (if you use the SMPI interface), or
a program specifically written to execute within SimGrid, using one of
the dedicated APIs.

.. _S4U_doc:

The S4U Interface
=================

The S4U interface (SimGrid for you) mixes the full power of SimGrid
with the full power of C++. This is the prefered interface to describe
abstract algorithms in the domains of Cloud, P2P, HPC, IoT and similar
settings.

Main Concepts
-------------

A typical SimGrid simulation is composed of several |Actors|_, that
execute user-provided functions. The actors have to explicitly use the
S4U interface to express their computation, communication, disk usage
and other |Activities|_, so that they get reflected within the
simulator. These activities take place on resources: |Hosts|_,
|Links|_, |Storages|_ and |VirtualMachines|_. SimGrid predicts the
time taken by each activity and orchestrates accordingly the actors
waiting for the completion of these activities.


Each actor executes a user-provided function on a simulated |Host|_,
with which it can interact using the :ref:`simgrid::s4u::this_actor
<namespace_simgrid__s4u__this_actor>` namespace.  **Communications**
are not directly sent to actors, but posted onto a |Mailbox|_ that
serve as rendez-vous point between communicating actors.  Actors can
also interact through **classical synchronization mechanisms** such as
|Barrier|_, |Semaphore|_, |Mutex|_ and |ConditionVariable|_.

.. todo:: Add NetZone

.. |Actors| replace:: **Actors**
.. _Actors: api/classsimgrid_1_1s4u_1_1Actor.html

.. |Activities| replace:: **Activities**
.. _Activities: api/classsimgrid_1_1s4u_1_1Activity.html

.. |Hosts| replace:: **Hosts**
.. _Hosts: api/classsimgrid_1_1s4u_1_1Host.html

.. |Links| replace:: **Links**
.. _Links: api/classsimgrid_1_1s4u_1_1Link.html

.. |Storages| replace:: **Storages**
.. _Storages: api/classsimgrid_1_1s4u_1_1Storage.html

.. |VirtualMachines| replace:: **VirtualMachines**
.. _VirtualMachines: api/classsimgrid_1_1s4u_1_1VirtualMachine.html

.. |Host| replace:: **Host**
.. _Host: api/classsimgrid_1_1s4u_1_1Host.html

.. |Mailbox| replace:: **Mailbox**
.. _Mailbox: api/classsimgrid_1_1s4u_1_1Mailbox.html

.. |Barrier| replace:: **Barrier**
.. _Barrier: api/classsimgrid_1_1s4u_1_1Barrier.html

.. |ConditionVariable| replace:: **ConditionVariable**
.. _ConditionVariable: api/classsimgrid_1_1s4u_1_1ConditionVariable.html

.. |Mutex| replace:: **Mutex**
.. _Mutex: api/classsimgrid_1_1s4u_1_1Mutex.html


.. include:: app_smpi.rst

.. include:: app_legacy.rst
