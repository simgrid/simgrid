.. _S4U_doc:

The S4U Interface
#################

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" width="100%" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("S4UBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1";
   }
   </script>
   <br/>
   <br/>

The S4U interface (SimGrid for you) mixes the full power of SimGrid
with the full power of C++. This is the preferred interface to describe
abstract algorithms in the domains of Cloud, P2P, HPC, IoT, and similar
settings.

Currently (v3.21), S4U is definitely the way to go for long-term
projects. It is feature complete, but may still evolve slightly in the
future releases. It can already be used to do everything that can be
done in SimGrid, but you may have to adapt your code in future
releases. When this happens, compiling your code will produce
deprecation warnings for 4 releases (one year) before the removal of
the old symbols. 
If you want an API that will never ever evolve in the future, you
should use the deprecated MSG API instead. 

Main Concepts
*************

A typical SimGrid simulation is composed of several |API_s4u_Actors|_, that
execute user-provided functions. The actors have to explicitly use the
S4U interface to express their :ref:`computation <API_s4u_Exec>`,
:ref:`communication <API_s4u_Comm>`, :ref:`disk usage <API_s4u_Io>`,
and other |API_s4u_Activities|_, so that they get reflected within the
simulator. These activities take place on resources such as |API_s4u_Hosts|_,
|API_s4u_Links|_ and |API_s4u_Storages|_. SimGrid predicts the time taken by each
activity and orchestrates the actors accordingly, waiting for the
completion of these activities.


When **communicating**, data is not directly sent to other actors but
posted onto a |API_s4u_Mailbox|_ that serves as a rendez-vous point between
communicating actors. This means that you don't need to know who you
are talking to, you just put your communication `Put` request in a
mailbox, and it will be matched with a complementary `Get`
request.  Alternatively, actors can interact through **classical
synchronization mechanisms** such as |API_s4u_Barrier|_, |API_s4u_Semaphore|_,
|API_s4u_Mutex|_ and |API_s4u_ConditionVariable|_.

Each actor is located on a simulated |API_s4u_Host|_. Each host is located
itself in a |API_s4u_NetZone|_, that knows the networking path between one
resource to another. Each NetZone is included in another one, forming
a tree of NetZones which root zone contains the whole platform.

The :ref:`simgrid::s4u::this_actor <API_s4u_this_actor>` namespace
provides many helper functions to simplify the code of actors.

- **Global Classes**

  - :ref:`class s4u::Actor <API_s4u_Actor>`:
    Active entities executing your application.
  - :ref:`class s4u::Engine <API_s4u_Engine>`
    Simulation engine (singleton).
  - :ref:`class s4u::Mailbox <API_s4u_Mailbox>`
    Communication rendez-vous.

- **Platform Elements**

  - :ref:`class s4u::Host <API_s4u_Host>`:
    Actor location, providing computational power.
  - :ref:`class s4u::Link <API_s4u_Link>`
    Interconnecting hosts.
  - :ref:`class s4u::NetZone <API_s4u_NetZone>`:
    Sub-region of the platform, containing resources (Hosts, Links, etc).
  - :ref:`class s4u::Storage <API_s4u_Storage>`
    Resource on which actors can write and read data.
  - :ref:`class s4u::VirtualMachine <API_s4u_VirtualMachine>`:
    Execution containers that can be moved between Hosts.

- **Activities** (:ref:`class s4u::Activity <API_s4u_Activity>`):
  The things that actors can do on resources

  - :ref:`class s4u::Comm <API_s4u_Comm>`
    Communication activity, started on Mailboxes and consuming links.
  - :ref:`class s4u::Exec <API_s4u_Exec>`
    Computation activity, started on Host and consuming CPU resources.
  - :ref:`class s4u::Io <API_s4u_Io>`
    I/O activity, started on and consumming Storages.

- **Synchronization Mechanisms**: Classical IPC that actors can use

  - :ref:`class s4u::Barrier <API_s4u_Barrier>`
  - :ref:`class s4u::ConditionVariable <API_s4u_ConditionVariable>`
  - :ref:`class s4u::Mutex <API_s4u_Mutex>`
  - :ref:`class s4u::Semaphore <API_s4u_Semaphore>`


.. |API_s4u_Actors| replace:: **Actors**
.. _API_s4u_Actors: #s4u-actor

.. |API_s4u_Activities| replace:: **Activities**
.. _API_s4u_Activities: #s4u-activity

.. |API_s4u_Hosts| replace:: **Hosts**
.. _API_s4u_Hosts: #s4u-host

.. |API_s4u_Links| replace:: **Links**
.. _API_s4u_Links: #s4u-link

.. |API_s4u_Storages| replace:: **Storages**
.. _API_s4u_Storages: #s4u-storage

.. |API_s4u_VirtualMachines| replace:: **VirtualMachines**
.. _API_s4u_VirtualMachines: #s4u-virtualmachine

.. |API_s4u_Host| replace:: **Host**

.. |API_s4u_Mailbox| replace:: **Mailbox**

.. |API_s4u_NetZone| replace:: **NetZone**

.. |API_s4u_Barrier| replace:: **Barrier**

.. |API_s4u_Semaphore| replace:: **Semaphore**

.. |API_s4u_ConditionVariable| replace:: **ConditionVariable**

.. |API_s4u_Mutex| replace:: **Mutex**

.. THE EXAMPLES

.. include:: ../../examples/s4u/README.rst

Activities
**********

Activities represent the actions that consume a resource, such as a
:ref:`s4u::Comm <API_s4u_Comm>` that consumes the *transmiting power* of
:ref:`s4u::Link <API_s4u_Link>` resources.

=======================
Asynchronous Activities
=======================

Every activity can be either **blocking** or **asynchronous**. For
example, :cpp:func:`s4u::Mailbox::put() <simgrid::s4u::Mailbox::put>`
and :cpp:func:`s4u::Mailbox::get() <simgrid::s4u::Mailbox::get>`
create blocking communications: the actor is blocked until the
completion of that communication. Asynchronous communications do not
block the actor during their execution but progress on their own.

Once your asynchronous activity is started, you can test for its
completion using :cpp:func:`s4u::Activity::test() <simgrid::s4u::Activity::test>`.
This function returns ``true`` if the activity completed already.
You can also use :cpp:func:`s4u::Activity::wait() <simgrid::s4u::Activity::wait>`
to block until the completion of the activity. To wait for at most a given amount of time,
use  :cpp:func:`s4u::Activity::wait_for() <simgrid::s4u::Activity::wait_for>`.
Finally, to wait at most until a specified time limit, use
:cpp:func:`s4u::Activity::wait_until() <simgrid::s4u::Activity::wait_until>`.

.. todo::

   wait_for and wait_until are currently not implemented for Exec and Io activities.

Every kind of activities can be asynchronous:

  - :ref:`s4u::CommPtr <API_s4u_Comm>` are created with 
    :cpp:func:`s4u::Mailbox::put_async() <simgrid::s4u::Mailbox::put_async>` and
    :cpp:func:`s4u::Mailbox::get_async() <simgrid::s4u::Mailbox::get_async>`.
  - :ref:`s4u::IoPtr <API_s4u_Io>` are created with 
    :cpp:func:`s4u::Storage::read_async() <simgrid::s4u::Storage::read_async>` and
    :cpp:func:`s4u::Storage::write_async() <simgrid::s4u::Storage::write_async>`.    
  - :ref:`s4u::ExecPtr <API_s4u_Exec>` are created with
    :cpp:func:`s4u::Host::exec_async() <simgrid::s4u::Host::exec_async>`.
  - In the future, it will become possible to have asynchronous IPC
    such as asynchronous mutex lock requests.

The following example shows how to have several concurrent
communications ongoing.  First, you have to declare a vector in which
we will store the ongoing communications. It is also useful to have a
vector of mailboxes.

.. literalinclude:: ../../examples/s4u/async-waitall/s4u-async-waitall.cpp
   :language: c++
   :start-after: init-begin
   :end-before: init-end
   :dedent: 4

Then, you start all the communications that should occur concurrently with
:cpp:func:`s4u::Mailbox::put_async() <simgrid::s4u::Mailbox::put_async>`.  
Finally, the actor waits for the completion of all of them at once
with 
:cpp:func:`s4u::Comm::wait_all() <simgrid::s4u::Comm::wait_all>`.  
     
.. literalinclude:: ../../examples/s4u/async-waitall/s4u-async-waitall.cpp
   :language: c++
   :start-after: put-begin
   :end-before: put-end
   :dedent: 4


=====================
Activities Life cycle
=====================

Sometimes, you want to change the setting of an activity before it even starts. 

.. todo:: write this section

Memory Management
*****************

For sake of simplicity, we use `RAII
<https://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization>`_
everywhere in S4U. This is an idiom where resources are automatically
managed through the context. Provided that you never manipulate
objects of type Foo directly but always FooPtr references (which are
defined as `boost::intrusive_ptr
<http://www.boost.org/doc/libs/1_61_0/libs/smart_ptr/intrusive_ptr.html>`_
<Foo>), you will never have to explicitely release the resource that
you use nor to free the memory of unused objects.

Here is a little example:

.. code-block:: cpp

   void myFunc() 
   {
     simgrid::s4u::MutexPtr mutex = simgrid::s4u::Mutex::create(); // Too bad we cannot use `new`

     mutex->lock();   // use the mutex as a simple reference
     //  bla bla
     mutex->unlock(); 
  
   } // The mutex gets automatically freed because the only existing reference gets out of scope

API Reference
*************

.. _API_s4u_Activity:

=============
s4u::Activity
=============

.. doxygenclass:: simgrid::s4u::Activity
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Actor:

==========
s4u::Actor
==========

.. doxygentypedef:: ActorPtr

.. doxygenclass:: simgrid::s4u::Actor
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Barrier:

============
s4u::Barrier
============

.. doxygentypedef:: BarrierPtr

.. doxygenclass:: simgrid::s4u::Barrier
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Comm:

=========
s4u::Comm
=========

.. doxygentypedef:: CommPtr

.. doxygenclass:: simgrid::s4u::Comm
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_ConditionVariable:

======================
s4u::ConditionVariable
======================

.. doxygentypedef:: ConditionVariablePtr

.. doxygenclass:: simgrid::s4u::ConditionVariable
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Engine:

===========
s4u::Engine
===========

.. doxygenclass:: simgrid::s4u::Engine
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Exec:

=========
s4u::Exec
=========

.. doxygentypedef:: ExecPtr

.. doxygenclass:: simgrid::s4u::Exec
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Host:

=========
s4u::Host
=========

.. doxygenclass:: simgrid::s4u::Host
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Io:

=======
s4u::Io
=======

.. doxygentypedef:: IoPtr

.. doxygenclass:: simgrid::s4u::Io
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Link:

=========
s4u::Link
=========

.. doxygenclass:: simgrid::s4u::Link
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Mailbox:

============
s4u::Mailbox
============

.. doxygentypedef:: MailboxPtr

.. doxygenclass:: simgrid::s4u::Mailbox
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Mutex:

==========
s4u::Mutex
==========

.. doxygentypedef:: MutexPtr

.. doxygenclass:: simgrid::s4u::Mutex
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_NetZone:

============
s4u::NetZone
============

.. doxygenclass:: simgrid::s4u::NetZone
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Semaphore:

==============
s4u::Semaphore
==============

.. doxygentypedef:: SemaphorePtr

.. doxygenclass:: simgrid::s4u::Semaphore
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Storage:

============
s4u::Storage
============

.. doxygenclass:: simgrid::s4u::Storage
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_VirtualMachine:

===================
s4u::VirtualMachine
===================

.. doxygenclass:: simgrid::s4u::VirtualMachine
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_this_actor:

=========================
namespace s4u::this_actor
=========================


.. doxygennamespace:: simgrid::s4u::this_actor
