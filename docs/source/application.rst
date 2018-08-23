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
S4U interface to express their
:ref:`computation <exhale_class_classsimgrid_1_1s4u_1_1Exec>`,
:ref:`communication <exhale_class_classsimgrid_1_1s4u_1_1Comm>`,
:ref:`disk usage <exhale_class_classsimgrid_1_1s4u_1_1Io>`,
and other |Activities|_, so that they get reflected within the
simulator. These activities take place on resources such as |Hosts|_,
|Links|_ and |Storages|_. SimGrid predicts the time taken by each
activity and orchestrates accordingly the actors waiting for the
completion of these activities. 


When **communicating**, data is not directly sent to other actors but
posted onto a |Mailbox|_ that serve as rendez-vous point between
communicating actors. This means that you don't need to know who you
are talking to, you just put your communication `Send` request in a
mailbox, and it will be matched with a complementary `Receive`
request.  Alternatively, actors can interact through **classical
synchronization mechanisms** such as |Barrier|_, |Semaphore|_,
|Mutex|_ and |ConditionVariable|_.

Each actor is located on a simulated |Host|_. Each host is located
itself in a |NetZone|_, that route communications through the
links. Each NetZone is included in another one, forming a tree of
NetZones which root zone contains the whole platform.

The :ref:`simgrid::s4u::this_actor
<namespace_simgrid__s4u__this_actor>` namespace provides many helper
functions to simplify the code of actors.

- **Global Classes**

  - :ref:`class s4u::Actor <exhale_class_classsimgrid_1_1s4u_1_1Actor>`:
    Active entities executing your application.
  - :ref:`class s4u::Engine <exhale_class_classsimgrid_1_1s4u_1_1Engine>`
    Simulation engine (singleton).
  - :ref:`class s4u::Mailbox <exhale_class_classsimgrid_1_1s4u_1_1Mailbox>`
    Communication rendez-vous.
    
- **Platform Elements**
  
  - :ref:`class s4u::Host <exhale_class_classsimgrid_1_1s4u_1_1Host>`:
    Actor location, providing computational power.
  - :ref:`class s4u::Link <exhale_class_classsimgrid_1_1s4u_1_1Link>`
    Interconnecting hosts.
  - :ref:`class s4u::NetZone <exhale_class_classsimgrid_1_1s4u_1_1NetZone>`:
    Sub-region of the platform, containing resources (Hosts, Link, etc).
  - :ref:`class s4u::Storage <exhale_class_classsimgrid_1_1s4u_1_1Storage>`
    Resource on which actors can write and read data. 
  - :ref:`class s4u::VirtualMachine <exhale_class_classsimgrid_1_1s4u_1_1VirtualMachine>`:
    Execution containers that can be moved between Hosts.
    
- **Activities** (:ref:`class s4u::Activity <exhale_class_classsimgrid_1_1s4u_1_1Activity>`):
  The things that actors can do on resources

  - :ref:`class s4u::Comm <exhale_class_classsimgrid_1_1s4u_1_1Comm>`
    Communication activity, started on Mailboxes and consuming links.
  - :ref:`class s4u::Exec <exhale_class_classsimgrid_1_1s4u_1_1Exec>`
    Computation activity, started on Host and consuming CPU resources.
  - :ref:`class s4u::Io <exhale_class_classsimgrid_1_1s4u_1_1Io>`
    I/O activities, started on and consumming Storages.
  
- **Synchronization Mechanisms**: Classical IPC that actors can use
  
  - :ref:`class s4u::Barrier <exhale_class_classsimgrid_1_1s4u_1_1Barrier>`
  - :ref:`class s4u::ConditionVariable <exhale_class_classsimgrid_1_1s4u_1_1ConditionVariable>`
  - :ref:`class s4u::Mutex <exhale_class_classsimgrid_1_1s4u_1_1Mutex>`
  - :ref:`class s4u::Semaphore <exhale_class_classsimgrid_1_1s4u_1_1Semaphore>`


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

.. |NetZone| replace:: **NetZone**
.. _NetZone: api/classsimgrid_1_1s4u_1_1NetZone.html

.. |Barrier| replace:: **Barrier**
.. _Barrier: api/classsimgrid_1_1s4u_1_1Barrier.html

.. |ConditionVariable| replace:: **ConditionVariable**
.. _ConditionVariable: api/classsimgrid_1_1s4u_1_1ConditionVariable.html

.. |Mutex| replace:: **Mutex**
.. _Mutex: api/classsimgrid_1_1s4u_1_1Mutex.html


.. include:: app_smpi.rst

.. include:: app_legacy.rst
