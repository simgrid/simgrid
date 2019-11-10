.. _S4U_doc:

The S4U Interface
#################

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("ActorBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

The S4U interface (SimGrid for you) mixes the full power of SimGrid
with the full power of C++. This is the preferred interface to describe
abstract algorithms in the domains of Cloud, P2P, HPC, IoT, and similar
settings.

Since v3.20 (June 2018), S4U is definitely the way to go for long-term
projects. It is feature complete, but may still evolve slightly in the
future releases. It can already be used to do everything that can be
done in SimGrid, but you may have to adapt your code in future
releases. When this happens, compiling your code will produce
deprecation warnings for 4 releases (one year) before the removal of
the old symbols. 
If you want an API that will never ever evolve in the future, you
should use the :ref:`deprecated MSG API <MSG_doc>` instead. 

Main Concepts
*************

A typical SimGrid simulation is composed of several |API_s4u_Actors|_, that
execute user-provided functions. The actors have to explicitly use the
S4U interface to express their :ref:`computation <API_s4u_Exec>`,
:ref:`communication <API_s4u_Comm>`, :ref:`disk usage <API_s4u_Io>`,
and other |API_s4u_Activities|_, so that they get reflected within the
simulator. These activities take place on resources such as |API_s4u_Hosts|_,
|API_s4u_Links|_ and |API_s4u_Disks|_. SimGrid predicts the time taken by each
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
a tree of NetZones which root zone contains the whole platform. The
actors can also be located on a |API_s4U_VirtualMachine|_ that may
restrict the activities it contains to a limited amount of cores.
Virtual machines can also be migrated between hosts.

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

  - :ref:`class s4u::Disk <API_s4u_Disk>`
    Resource on which actors can write and read data.
  - :ref:`class s4u::Host <API_s4u_Host>`:
    Actor location, providing computational power.
  - :ref:`class s4u::Link <API_s4u_Link>`
    Interconnecting hosts.
  - :ref:`class s4u::NetZone <API_s4u_NetZone>`:
    Sub-region of the platform, containing resources (Hosts, Links, etc).
  - :ref:`class s4u::VirtualMachine <API_s4u_VirtualMachine>`:
    Execution containers that can be moved between Hosts.

- **Activities** (:ref:`class s4u::Activity <API_s4u_Activity>`):
  The things that actors can do on resources

  - :ref:`class s4u::Comm <API_s4u_Comm>`
    Communication activity, started on Mailboxes and consuming links.
  - :ref:`class s4u::Exec <API_s4u_Exec>`
    Computation activity, started on Host and consuming CPU resources.
  - :ref:`class s4u::Io <API_s4u_Io>`
    I/O activity, started on and consumming disks.

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

.. |API_s4u_Disks| replace:: **Disks**
.. _API_s4u_Disks: #s4u-disk

.. |API_s4u_VirtualMachine| replace:: **VirtualMachines**

.. |API_s4u_Host| replace:: **Host**

.. |API_s4u_Mailbox| replace:: **Mailbox**

.. |API_s4u_Mailboxes| replace:: **Mailboxes**
.. _API_s4u_Mailboxes: #s4u-mailbox

.. |API_s4u_NetZone| replace:: **NetZone**

.. |API_s4u_Barrier| replace:: **Barrier**

.. |API_s4u_Semaphore| replace:: **Semaphore**

.. |API_s4u_ConditionVariable| replace:: **ConditionVariable**

.. |API_s4u_Mutex| replace:: **Mutex**

Activities
**********

Activities represent the actions that consume a resource, such as a
:ref:`s4u::Comm <API_s4u_Comm>` that consumes the *transmitting power* of
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

Every kind of activity can be asynchronous:

  - :ref:`s4u::CommPtr <API_s4u_Comm>` are created with 
    :cpp:func:`s4u::Mailbox::put_async() <simgrid::s4u::Mailbox::put_async>` and
    :cpp:func:`s4u::Mailbox::get_async() <simgrid::s4u::Mailbox::get_async>`.
  - :ref:`s4u::IoPtr <API_s4u_Io>` are created with 
    :cpp:func:`s4u::Disk::read_async() <simgrid::s4u::Disk::read_async>` and
    :cpp:func:`s4u::Disk::write_async() <simgrid::s4u::Disk::write_async>`.
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

.. _s4u_mailbox:
	  
Mailboxes
*********

Please also refer to the :ref:`API reference for s4u::Mailbox
<API_s4u_Mailbox>`.

===================
What are Mailboxes?
===================

|API_s4u_Mailboxes|_ are rendez-vous points for network communications,
similar to URLs on which you could post and retrieve data. Actually,
the mailboxes are not involved in the communication once it starts,
but only to find the contact with which you want to communicate.

They are similar to many common things: The phone number, which allows
the caller to find the receiver. The twitter hashtag, which help
senders and receivers to find each others. In TCP, the pair 
``{host name, host port}`` to which you can connect to find your peer.
In HTTP, URLs through which the clients can connect to the servers. 
In ZeroMQ, the queues are used to match senders and receivers.

One big difference with most of these systems is that no actor is the
exclusive owner of a mailbox, neither in sending nor in receiving.
Many actors can send into and/or receive from the same mailbox.  TCP
socket ports for example are shared on the sender side but exclusive
on the receiver side (only one process can receive from a given socket
at a given point of time).

A big difference with TCP sockets or MPI communications is that
communications do not start right away after a
:cpp:func:`Mailbox::put() <simgrid::s4u::Mailbox::put()>`, but wait
for the corresponding :cpp:func:`Mailbox::get() <simgrid::s4u::Mailbox::get()>`.
You can change this by :ref:`declaring a receiving actor <s4u_receiving_actor>`.

A big difference with twitter hashtags is that SimGrid does not
offer easy support to broadcast a given message to many
receivers. So that would be like a twitter tag where each message
is consumed by the first receiver.

A big difference with the ZeroMQ queues is that you cannot filter
on the data you want to get from the mailbox. To model such settings
in SimGrid, you'd have one mailbox per potential topic, and subscribe
to each topic individually with a 
:cpp:func:`get_async() <simgrid::s4u::Mailbox::get_async()>` on each mailbox.
Then, use :cpp:func:`Comm::wait_any() <simgrid::s4u::Comm::wait_any()>` 
to get the first message on any of the mailbox you are subscribed onto.

The mailboxes are not located on the network, and you can access
them without any latency. The network delay are only related to the
location of the sender and receiver once the match between them is
done on the mailbox. This is just like the phone number that you
can use locally, and the geographical distance only comes into play
once you start the communication by dialing this number.

=====================
How to use Mailboxes?
=====================

You can retrieve any existing mailbox from its name (which is a
unique string, just like a twitter tag). This results in a
versatile mechanism that can be used to build many different
situations.

To model classical socket communications, use "hostname:port" as
mailbox names, and make sure that only one actor reads into a given
mailbox. This does not make it easy to build a perfectly realistic
model of the TCP sockets, but in most cases, this system is too
cumbersome for your simulations anyway. You probably want something
simpler, that turns our to be easy to build with the mailboxes.

Many SimGrid examples use a sort of yellow page system where the
mailbox names are the name of the service (such as "worker",
"master" or "reducer"). That way, you don't have to know where your
peer is located to contact it. You don't even need its name. Its
function is enough for that. This also gives you some sort of load
balancing for free if more than one actor pulls from the mailbox:
the first actor that can deal with the request will handle it.

=========================================
How are put() and get() requests matched?
=========================================

The matching algorithm simple: first come, first serve. When a new
send arrives, it matches the oldest enqueued receive. If no receive is
currently enqueued, then the incoming send is enqueued. As you can
see, the mailbox cannot contain both send and receive requests: all
enqueued requests must be of the same sort.

.. _s4u_receiving_actor:

===========================
Declaring a Receiving Actor
===========================

The last twist is that by default in the simulator, the data starts
to be exchanged only when both the sender and the receiver are
announced (it waits until both :cpp:func:`put() <simgrid::s4u::Mailbox::put()>`
and :cpp:func:`get() <simgrid::s4u::Mailbox::get()>` are posted). 
In TCP, since you establish connections beforehand, the data starts to
flow as soon as the sender posts it, even if the receiver did not post
its :cpp:func:`recv() <simgrid::s4u::Mailbox::recv()>` yet. 

To model this in SimGrid, you can declare a specific receiver to a
given mailbox (with the function 
:cpp:func:`set_receiver() <simgrid::s4u::Mailbox::set_receiver()>`). 
That way, any :cpp:func:`put() <simgrid::s4u::Mailbox::put()>`
posted to that mailbox will start as soon as possible, and the data
will already be there on the receiver host when the receiver actor
posts its :cpp:func:`get() <simgrid::s4u::Mailbox::get()>`

Note that being permanent receivers of a mailbox prevents actors to be
garbage-collected. If your simulation creates many short-lived actors
that marked as permanent receiver, you should call
``mailbox->set_receiver(nullptr)`` by the end of the actors so that their
memory gets properly reclaimed. This call should be at the end of the
actor's function, not in a on_exit callback.

Memory Management
*****************

For sake of simplicity, we use `RAII
<https://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization>`_
for many classes in S4U. This is an idiom where resources are automatically
managed through the context. Provided that you never manipulate
objects of type Foo directly but always FooPtr references (which are
defined as `boost::intrusive_ptr
<http://www.boost.org/doc/libs/1_61_0/libs/smart_ptr/intrusive_ptr.html>`_
<Foo>), you will never have to explicitly release the resource that
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

Note that Mailboxes, Hosts and Links are not handled thought smart
pointers (yet?). This means that it is currently impossible to destroy a
mailbox or a link. You can still destroy an host (but probably
shouldn't), using :cpp:func:`simgrid::s4u::Host::destroy`.

.. THE EXAMPLES

.. include:: ../../examples/README.rst

API Reference
*************

.. _API_s4u_this_actor:

==================================
Interacting with the current actor
==================================

Static methods working on the current actor (see :ref:`API_s4u_Actor`).

.. doxygennamespace:: simgrid::s4u::this_actor

.. _API_s4u_Activity:

=============
s4u::Activity
=============

.. doxygenclass:: simgrid::s4u::Activity
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Actor:

===========
class Actor
===========

.. doxygentypedef:: ActorPtr

.. doxygentypedef:: aid_t

.. autodoxyclass:: simgrid::s4u::Actor

Creating actors
---------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Actor::create(const std::string &name, s4u::Host *host, const std::function< void()> &code)
      .. autodoxymethod:: simgrid::s4u::Actor::create(const std::string &name, s4u::Host *host, F code)
      .. autodoxymethod:: simgrid::s4u::Actor::create(const std::string &name, s4u::Host *host, F code, Args... args)
      .. autodoxymethod:: simgrid::s4u::Actor::create(const std::string &name, s4u::Host *host, const std::string &function, std::vector< std::string > args)

      .. autodoxymethod:: simgrid::s4u::Actor::init(const std::string &name, s4u::Host *host)
      .. autodoxymethod:: simgrid::s4u::Actor::start(const std::function< void()> &code)

   .. group-tab:: Python

      .. automethod:: simgrid.Actor.create

Searching specific actors
-------------------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Actor::by_pid(aid_t pid)
      .. autodoxymethod:: simgrid::s4u::Actor::self()

   .. group-tab:: Python

      .. automethod:: simgrid.Actor.by_pid
      .. automethod:: simgrid.Actor.self

Querying info about actors
--------------------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Actor::get_cname
      .. autodoxymethod:: simgrid::s4u::Actor::get_name
      .. autodoxymethod:: simgrid::s4u::Actor::get_pid
      .. autodoxymethod:: simgrid::s4u::Actor::get_ppid
      .. autodoxymethod:: simgrid::s4u::Actor::get_properties() const
      .. autodoxymethod:: simgrid::s4u::Actor::get_property(const std::string &key) const
      .. autodoxymethod:: simgrid::s4u::Actor::set_property(const std::string &key, const std::string &value) 

      .. autodoxymethod:: simgrid::s4u::Actor::get_host
      .. autodoxymethod:: simgrid::s4u::Actor::migrate

      .. autodoxymethod:: simgrid::s4u::Actor::get_refcount()
      .. autodoxymethod:: simgrid::s4u::Actor::get_impl()

   .. group-tab:: Python
                  
      .. autoattribute:: simgrid.Actor.name
      .. autoattribute:: simgrid.Actor.host
      .. autoattribute:: simgrid.Actor.pid
      .. autoattribute:: simgrid.Actor.ppid

      .. automethod:: simgrid.Actor.migrate

Suspending and resuming actors
------------------------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Actor::suspend()
      .. autodoxymethod:: simgrid::s4u::Actor::resume()
      .. autodoxymethod:: simgrid::s4u::Actor::is_suspended()

   .. group-tab:: Python

      .. automethod:: simgrid.Actor.resume
      .. automethod:: simgrid.Actor.suspend
      .. automethod:: simgrid.Actor.is_suspended

Killing actors
--------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Actor::kill()
      .. autodoxymethod:: simgrid::s4u::Actor::kill_all()
      .. autodoxymethod:: simgrid::s4u::Actor::set_kill_time(double time)
      .. autodoxymethod:: simgrid::s4u::Actor::get_kill_time()

      .. autodoxymethod:: simgrid::s4u::Actor::restart()
      .. autodoxymethod:: simgrid::s4u::Actor::daemonize()
      .. autodoxymethod:: simgrid::s4u::Actor::is_daemon

   .. group-tab:: Python

      .. automethod:: simgrid.Actor.kill
      .. automethod:: simgrid.Actor.kill_all

      .. automethod:: simgrid.Actor.daemonize
      .. automethod:: simgrid.Actor.is_daemon

Reacting to the end of actors
-----------------------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Actor::on_exit(const std::function< void(bool)> &fun)
      .. autodoxymethod:: simgrid::s4u::Actor::join()
      .. autodoxymethod:: simgrid::s4u::Actor::join(double timeout)
      .. autodoxymethod:: simgrid::s4u::Actor::set_auto_restart(bool autorestart)

   .. group-tab:: Python

      .. automethod:: simgrid.Actor.join

Signals
-------

.. tabs::

   .. group-tab:: C++

      .. autodoxyvar:: simgrid::s4u::Actor::on_creation
      .. autodoxyvar:: simgrid::s4u::Actor::on_suspend
      .. autodoxyvar:: simgrid::s4u::Actor::on_resume
      .. autodoxyvar:: simgrid::s4u::Actor::on_sleep
      .. autodoxyvar:: simgrid::s4u::Actor::on_wake_up
      .. autodoxyvar:: simgrid::s4u::Actor::on_migration_start
      .. autodoxyvar:: simgrid::s4u::Actor::on_migration_end
      .. autodoxyvar:: simgrid::s4u::Actor::on_termination
      .. autodoxyvar:: simgrid::s4u::Actor::on_destruction

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

.. _API_s4u_Disk:

============
s4u::Disk
============

.. doxygenclass:: simgrid::s4u::Disk
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

.. _API_s4u_ExecSeq:

============
s4u::ExecSeq
============

.. doxygentypedef:: ExecSeqPtr

.. doxygenclass:: simgrid::s4u::ExecSeq
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_ExecPar:

============
s4u::ExecPar
============

.. doxygentypedef:: ExecParPtr

.. doxygenclass:: simgrid::s4u::ExecPar
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

Please also refer to the :ref:`full doc on s4u::Mailbox <s4u_mailbox>`.

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

.. _API_s4u_VirtualMachine:

===================
s4u::VirtualMachine
===================

.. doxygenclass:: simgrid::s4u::VirtualMachine
   :members:
   :protected-members:
   :undoc-members:

C API Reference
***************

==============
Main functions
==============

.. doxygenfunction:: simgrid_init
.. doxygenfunction:: simgrid_get_clock
.. doxygenfunction:: simgrid_load_deployment
.. doxygenfunction:: simgrid_load_platform
.. doxygenfunction:: simgrid_register_default
.. doxygenfunction:: simgrid_register_function
.. doxygenfunction:: simgrid_run

==================
Condition Variable
==================

See also the :ref:`C++ API <API_s4u_ConditionVariable>`.

.. doxygenfunction:: sg_cond_init
.. doxygenfunction:: sg_cond_notify_all
.. doxygenfunction:: sg_cond_notify_one
.. doxygenfunction:: sg_cond_wait
.. doxygenfunction:: sg_cond_wait_for

Python API Reference
********************

The Python API is automatically generated with pybind11. It closely mimicks the C++
API, to which you should refer for more information.

==========
this_actor
==========

.. automodule:: simgrid.this_actor
   :members:

===========
Class Actor
===========

.. autoclass:: simgrid.Actor
   :members:

==========
Class Comm
==========

.. autoclass:: simgrid.Comm
   :members:

============
Class Engine
============

.. autoclass:: simgrid.Engine
   :members:

==========
Class Exec
==========

.. autoclass:: simgrid.Exec
   :members:

==========
Class Host
==========

.. autoclass:: simgrid.Host
   :members:

=============
Class Mailbox
=============

.. autoclass:: simgrid.Mailbox
   :members:

.. |hr| raw:: html

   <hr />
