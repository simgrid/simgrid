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

- **Simulation Elements**

  - :ref:`class Actor <API_s4u_Actor>`:
    Active entities executing your application.
  - :ref:`class Engine <API_s4u_Engine>`
    Simulation engine (singleton).
  - :ref:`class Mailbox <API_s4u_Mailbox>`
    Communication rendez-vous, with which actors meet each other.

- **Resources**

  - :ref:`class Disk <API_s4u_Disk>`
    Resource on which actors can write and read data.
  - :ref:`class Host <API_s4u_Host>`:
    Actor location, providing computational power.
  - :ref:`class Link <API_s4u_Link>`
    Interconnecting hosts.
  - :ref:`class NetZone <API_s4u_NetZone>`:
    Sub-region of the platform, containing resources (Hosts, Links, etc).
  - :ref:`class VirtualMachine <API_s4u_VirtualMachine>`:
    Execution containers that can be moved between Hosts.

- **Activities** (:ref:`class Activity <API_s4u_Activity>`):
  The things that actors can do on resources

  - :ref:`class Comm <API_s4u_Comm>`
    Communication activity, started on Mailboxes and consuming links.
  - :ref:`class Exec <API_s4u_Exec>`
    Computation activity, started on Host and consuming CPU resources.
  - :ref:`class Io <API_s4u_Io>`
    I/O activity, started on and consumming disks.

- **Synchronization Objects**: Classical IPC that actors can use

  - :ref:`class Barrier <API_s4u_Barrier>`
  - :ref:`class ConditionVariable <API_s4u_ConditionVariable>`
  - :ref:`class Mutex <API_s4u_Mutex>`
  - :ref:`class Semaphore <API_s4u_Semaphore>`


.. |API_s4u_Actors| replace:: **Actors**
.. _API_s4u_Actors: #api-s4u-actor

.. |API_s4u_Activities| replace:: **Activities**
.. _API_s4u_Activities: #api-s4u-activity

.. |API_s4u_Hosts| replace:: **Hosts**
.. _API_s4u_Hosts: #api-s4u-host

.. |API_s4u_Links| replace:: **Links**
.. _API_s4u_Links: #api-s4u-link

.. |API_s4u_Disks| replace:: **Disks**
.. _API_s4u_Disks: #api-s4u-disk

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

.. _s4u_Activities:

Activities
**********

Activities represent the actions that consume a resource, such as a
:ref:`Comm <API_s4u_Comm>` that consumes the *transmitting power* of 
:ref:`Link <API_s4u_Link>` resources, or an :ref:`Exec <API_s4u_Exec>`
that consumes the *computing power* of :ref:`Host <API_s4u_Host>` resources.
See also the :ref:`full API <API_s4u_Activity>` below.

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

.. _s4u_raii:

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

.. _API_s4u_simulation_object:

==================
Simulation objects
==================

.. _API_s4u_Actor:

==============
⁣  class Actor
==============

.. autodoxyclass:: simgrid::s4u::Actor

.. doxygentypedef:: ActorPtr

.. doxygentypedef:: aid_t

Creating actors
---------------

.. tabs::

   .. group-tab:: C++

      .. code:: C++

         #include <simgrid/s4u/Engine.hpp>

      .. autodoxymethod:: simgrid::s4u::Actor::create(const std::string &name, s4u::Host *host, const std::function< void()> &code)
      .. autodoxymethod:: simgrid::s4u::Actor::create(const std::string &name, s4u::Host *host, F code)
      .. autodoxymethod:: simgrid::s4u::Actor::create(const std::string &name, s4u::Host *host, F code, Args... args)
      .. autodoxymethod:: simgrid::s4u::Actor::create(const std::string &name, s4u::Host *host, const std::string &function, std::vector< std::string > args)

      .. autodoxymethod:: simgrid::s4u::Actor::init(const std::string &name, s4u::Host *host)
      .. autodoxymethod:: simgrid::s4u::Actor::start(const std::function< void()> &code)

   .. group-tab:: Python

      .. code:: Python

         from simgrid import Actor

      .. automethod:: simgrid.Actor.create

   .. group-tab:: C

      .. code:: C

         #include <simgrid/actor.h>

      .. doxygentypedef:: sg_actor_t
      .. cpp:type:: const s4u_Actor* const_sg_actor_t

         Pointer to a constant actor object.

      .. autodoxymethod:: sg_actor_init(const char *name, sg_host_t host)
      .. autodoxymethod:: sg_actor_start(sg_actor_t actor, xbt_main_func_t code, int argc, char **argv)

      .. autodoxymethod:: sg_actor_attach(const char *name, void *data, sg_host_t host, xbt_dict_t properties)
      .. autodoxymethod:: sg_actor_detach()

      .. autodoxymethod:: sg_actor_ref(const_sg_actor_t actor)
      .. autodoxymethod:: sg_actor_unref(const_sg_actor_t actor)

Searching specific actors
-------------------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Actor::by_pid(aid_t pid)
      .. autodoxymethod:: simgrid::s4u::Actor::self()

   .. group-tab:: Python

      .. automethod:: simgrid.Actor.by_pid
      .. automethod:: simgrid.Actor.self

   .. group-tab:: C

      .. autodoxymethod:: sg_actor_by_PID(aid_t pid)
      .. autodoxymethod:: sg_actor_self()

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
      .. autodoxymethod:: simgrid::s4u::Actor::set_host

      .. autodoxymethod:: simgrid::s4u::Actor::get_refcount()
      .. autodoxymethod:: simgrid::s4u::Actor::get_impl

   .. group-tab:: Python
                  
      .. autoattribute:: simgrid.Actor.name
      .. autoattribute:: simgrid.Actor.host
      .. autoattribute:: simgrid.Actor.pid
      .. autoattribute:: simgrid.Actor.ppid

   .. group-tab:: C

      .. autodoxymethod:: sg_actor_get_name(const_sg_actor_t actor)
      .. autodoxymethod:: sg_actor_get_PID(const_sg_actor_t actor)
      .. autodoxymethod:: sg_actor_get_PPID(const_sg_actor_t actor)
      .. autodoxymethod:: sg_actor_get_properties(const_sg_actor_t actor)
      .. autodoxymethod:: sg_actor_get_property_value(const_sg_actor_t actor, const char *name)

      .. autodoxymethod:: sg_actor_get_host(const_sg_actor_t actor)
      .. autodoxymethod:: sg_actor_set_host(sg_actor_t actor, sg_host_t host)

      .. autodoxymethod:: sg_actor_data(const_sg_actor_t actor)
      .. autodoxymethod:: sg_actor_data_set(sg_actor_t actor, void *userdata)        

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

   .. group-tab:: C

      .. autodoxymethod:: sg_actor_suspend(sg_actor_t actor)
      .. autodoxymethod:: sg_actor_resume(sg_actor_t actor)
      .. autodoxymethod:: sg_actor_is_suspended(sg_actor_t actor)

Specifying when actors should terminate
---------------------------------------

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

   .. group-tab:: C

      .. autodoxymethod:: sg_actor_kill(sg_actor_t actor)
      .. autodoxymethod:: sg_actor_kill_all()
      .. autodoxymethod:: sg_actor_set_kill_time(sg_actor_t actor, double kill_time)

      .. autodoxymethod:: sg_actor_restart(sg_actor_t actor)
      .. autodoxymethod:: sg_actor_daemonize(sg_actor_t actor)

.. _API_s4u_Actor_end:

Reacting to the end of actors
-----------------------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Actor::on_exit
      .. autodoxymethod:: simgrid::s4u::Actor::join()
      .. autodoxymethod:: simgrid::s4u::Actor::join(double timeout)
      .. autodoxymethod:: simgrid::s4u::Actor::set_auto_restart(bool autorestart)

   .. group-tab:: Python

      .. automethod:: simgrid.Actor.join

   .. group-tab:: C

      .. autodoxymethod:: sg_actor_join(sg_actor_t actor, double timeout)
      .. autodoxymethod:: sg_actor_set_auto_restart(sg_actor_t actor, int auto_restart)

Signals
-------

.. tabs::

   .. group-tab:: C++

      .. autodoxyvar:: simgrid::s4u::Actor::on_creation
      .. autodoxyvar:: simgrid::s4u::Actor::on_suspend
      .. autodoxyvar:: simgrid::s4u::Actor::on_resume
      .. autodoxyvar:: simgrid::s4u::Actor::on_sleep
      .. autodoxyvar:: simgrid::s4u::Actor::on_wake_up
      .. autodoxyvar:: simgrid::s4u::Actor::on_termination
      .. autodoxyvar:: simgrid::s4u::Actor::on_destruction

.. _API_s4u_this_actor:

====================
⁣  The current actor
====================

These functions can be used in your user code to interact with the actor
currently running (the one retrieved with :cpp:func:`simgrid::s4u::Actor::self`).
Using these functions can greatly improve the code readability.

Querying info
-------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::this_actor::get_cname()
      .. autodoxymethod:: simgrid::s4u::this_actor::get_name()
      .. autodoxymethod:: simgrid::s4u::this_actor::get_pid()
      .. autodoxymethod:: simgrid::s4u::this_actor::get_ppid()
      .. autodoxymethod:: simgrid::s4u::this_actor::is_maestro()

      .. autodoxymethod:: simgrid::s4u::this_actor::get_host()
      .. autodoxymethod:: simgrid::s4u::this_actor::set_host(Host *new_host)

   .. group-tab:: Python

      .. autofunction:: simgrid.this_actor.get_host
      .. autofunction:: simgrid.this_actor.set_host

   .. group-tab:: C

      .. autodoxymethod:: sg_actor_self_data()
      .. autodoxymethod:: sg_actor_self_data_set(void *data)
      .. autodoxymethod:: sg_actor_self_get_name()
      .. autodoxymethod:: sg_actor_self_get_pid()
      .. autodoxymethod:: sg_actor_self_get_ppid()
      .. autodoxymethod:: sg_host_self()
      .. autodoxymethod:: sg_host_self_get_name()

Suspending and resuming
-----------------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::this_actor::suspend()
      .. autodoxymethod:: simgrid::s4u::this_actor::yield()

   .. group-tab:: Python

      .. autofunction:: simgrid.this_actor.suspend
      .. autofunction:: simgrid.this_actor.yield_

Logging messages
----------------

.. tabs::

   .. group-tab:: Python

       .. autofunction:: simgrid.this_actor.info
       .. autofunction:: simgrid.this_actor.error

Sleeping
--------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::this_actor::sleep_for(double duration)
      .. autodoxymethod:: simgrid::s4u::this_actor::sleep_for(std::chrono::duration< Rep, Period > duration)
      .. autodoxymethod:: simgrid::s4u::this_actor::sleep_until(const SimulationTimePoint< Duration > &wakeup_time)
      .. autodoxymethod:: simgrid::s4u::this_actor::sleep_until(double wakeup_time)

   .. group-tab:: Python

      .. autofunction:: simgrid.this_actor.sleep_for
      .. autofunction:: simgrid.this_actor.sleep_until

   .. group-tab:: C

      .. autodoxymethod:: sg_actor_sleep_for(double duration)

Simulating executions
---------------------

Simulate the execution of some code on this actor. You can either simulate
parallel or sequential code, and you can either block upon the termination of
the execution, or start an asynchronous activity.

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::this_actor::exec_async(double flops_amounts)
      .. autodoxymethod:: simgrid::s4u::this_actor::exec_init(const std::vector< s4u::Host * > &hosts, const std::vector< double > &flops_amounts, const std::vector< double > &bytes_amounts)
      .. autodoxymethod:: simgrid::s4u::this_actor::exec_init(double flops_amounts)
      .. autodoxymethod:: simgrid::s4u::this_actor::execute(double flop)
      .. autodoxymethod:: simgrid::s4u::this_actor::execute(double flop, double priority)
      .. autodoxymethod:: simgrid::s4u::this_actor::parallel_execute(const std::vector< s4u::Host * > &hosts, const std::vector< double > &flops_amounts, const std::vector< double > &bytes_amounts)
      .. autodoxymethod:: simgrid::s4u::this_actor::parallel_execute(const std::vector< s4u::Host * > &hosts, const std::vector< double > &flops_amounts, const std::vector< double > &bytes_amounts, double timeout)

   .. group-tab:: Python

      .. autofunction:: simgrid.this_actor.exec_init
      .. autofunction:: simgrid.this_actor.execute

   .. group-tab:: C

      .. autodoxymethod:: sg_actor_self_execute(double flops)

Exiting
-------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::this_actor::exit()
      .. autodoxymethod:: simgrid::s4u::this_actor::on_exit(const std::function< void(bool)> &fun)

   .. group-tab:: Python

      .. autofunction:: simgrid.this_actor.exit
      .. autofunction:: simgrid.this_actor.on_exit

.. _API_s4u_Engine:

====================
⁣  Simulation Engine
====================

.. autodoxyclass:: simgrid::s4u::Engine

Initialization
--------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Engine::Engine(int *argc, char **argv)
      .. autodoxymethod:: simgrid::s4u::Engine::is_initialized()
      .. autodoxymethod:: simgrid::s4u::Engine::shutdown()
      .. autodoxymethod:: simgrid::s4u::Engine::set_config(const std::string &str)

      .. autodoxymethod:: simgrid::s4u::Engine::load_deployment(const std::string &deploy)
      .. autodoxymethod:: simgrid::s4u::Engine::load_platform(const std::string &platf)
      .. autodoxymethod:: simgrid::s4u::Engine::register_actor(const std::string &name)
      .. autodoxymethod:: simgrid::s4u::Engine::register_actor(const std::string &name, F code)
      .. autodoxymethod:: simgrid::s4u::Engine::register_default(int(*code)(int, char **))
      .. autodoxymethod:: simgrid::s4u::Engine::register_function(const std::string &name, int(*code)(int, char **))
      .. autodoxymethod:: simgrid::s4u::Engine::register_function(const std::string &name, void(*code)(std::vector< std::string >))

   .. group-tab:: Python
   
       .. automethod:: simgrid.Engine.load_deployment
       .. automethod:: simgrid.Engine.load_platform
       .. automethod:: simgrid.Engine.register_actor

   .. group-tab:: C

      .. autodoxymethod:: simgrid_init

      .. autodoxymethod:: simgrid_load_deployment
      .. autodoxymethod:: simgrid_load_platform
      .. autodoxymethod:: simgrid_register_default
      .. autodoxymethod:: simgrid_register_function

Run the simulation
------------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Engine::get_clock()
      .. autodoxymethod:: simgrid::s4u::Engine::run()

   .. group-tab:: Python
   
      .. automethod:: simgrid.Engine.get_clock
      .. automethod:: simgrid.Engine.run

   .. group-tab:: C

      .. autodoxymethod:: simgrid_get_clock
      .. autodoxymethod:: simgrid_run

Retrieving actors
-----------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Engine::get_actor_count()
      .. autodoxymethod:: simgrid::s4u::Engine::get_all_actors()
      .. autodoxymethod:: simgrid::s4u::Engine::get_filtered_actors(const std::function< bool(ActorPtr)> &filter)

   .. group-tab:: C

      .. autodoxymethod:: simgrid_get_actor_count()

Retrieving hosts
----------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Engine::get_all_hosts()
      .. autodoxymethod:: simgrid::s4u::Engine::get_host_count()
      .. autodoxymethod:: simgrid::s4u::Engine::get_filtered_hosts(const std::function< bool(Host *)> &filter)
      .. autodoxymethod:: simgrid::s4u::Engine::host_by_name(const std::string &name)
      .. autodoxymethod:: simgrid::s4u::Engine::host_by_name_or_null(const std::string &name)

   .. group-tab:: Python

      .. automethod:: simgrid.Engine.get_all_hosts

Retrieving links
----------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Engine::get_all_links()
      .. autodoxymethod:: simgrid::s4u::Engine::get_link_count()
      .. autodoxymethod:: simgrid::s4u::Engine::get_filtered_links
      .. autodoxymethod:: simgrid::s4u::Engine::link_by_name(const std::string &name)
      .. autodoxymethod:: simgrid::s4u::Engine::link_by_name_or_null(const std::string &name)

Interacting with the routing
----------------------------

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Engine::get_all_netpoints()
      .. autodoxymethod:: simgrid::s4u::Engine::get_filtered_netzones()
      .. autodoxymethod:: simgrid::s4u::Engine::get_instance()
      .. autodoxymethod:: simgrid::s4u::Engine::get_netzone_root()
      .. autodoxymethod:: simgrid::s4u::Engine::netpoint_by_name_or_null(const std::string &name)
      .. autodoxymethod:: simgrid::s4u::Engine::netzone_by_name_or_null(const std::string &name)
      .. autodoxymethod:: simgrid::s4u::Engine::set_netzone_root(const NetZone *netzone)

Signals
-------

.. tabs::

   .. group-tab:: C++

      .. autodoxyvar:: simgrid::s4u::Engine::on_deadlock
      .. autodoxyvar:: simgrid::s4u::Engine::on_platform_created
      .. autodoxyvar:: simgrid::s4u::Engine::on_platform_creation
      .. autodoxyvar:: simgrid::s4u::Engine::on_simulation_end
      .. autodoxyvar:: simgrid::s4u::Engine::on_time_advance

.. _API_s4u_Mailbox:

================
⁣  class Mailbox
================

.. autodoxyclass:: simgrid::s4u::Mailbox

Please also refer to the :ref:`full doc on s4u::Mailbox <s4u_mailbox>`.

Basic management
----------------

.. tabs::

   .. group-tab:: C++

      .. code-block:: C++

         #include <simgrid/s4u/Mailbox.hpp>

      Note that there is no MailboxPtr type, and that you cannot use the RAII
      idiom on mailboxes because they are internal objects to the simulation
      engine. Once created, there is no way to destroy a mailbox before the end
      of the simulation.
         
      .. autodoxymethod:: simgrid::s4u::Mailbox::by_name(const std::string &name)

   .. group-tab:: Python

      .. automethod:: simgrid.Mailbox.by_name

Querying info
.............

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Mailbox::get_cname()
      .. autodoxymethod:: simgrid::s4u::Mailbox::get_name()

   .. group-tab:: Python

      .. autoattribute:: simgrid.Mailbox.name

Sending data
............

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Mailbox::put(void *payload, uint64_t simulated_size_in_bytes)
      .. autodoxymethod:: simgrid::s4u::Mailbox::put(void *payload, uint64_t simulated_size_in_bytes, double timeout)
      .. autodoxymethod:: simgrid::s4u::Mailbox::put_async(void *data, uint64_t simulated_size_in_bytes)
      .. autodoxymethod:: simgrid::s4u::Mailbox::put_init()
      .. autodoxymethod:: simgrid::s4u::Mailbox::put_init(void *data, uint64_t simulated_size_in_bytes)

   .. group-tab:: Python

      .. automethod:: simgrid.Mailbox.put
      .. automethod:: simgrid.Mailbox.put_async


Receiving data
..............

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Mailbox::empty()
      .. autodoxymethod:: simgrid::s4u::Mailbox::front()
      .. autodoxymethod:: simgrid::s4u::Mailbox::get()
      .. autodoxymethod:: simgrid::s4u::Mailbox::get(double timeout)
      .. autodoxymethod:: simgrid::s4u::Mailbox::get_async(void **data)
      .. autodoxymethod:: simgrid::s4u::Mailbox::get_init()
      .. autodoxymethod:: simgrid::s4u::Mailbox::iprobe(int type, bool(*match_fun)(void *, void *, kernel::activity::CommImpl *), void *data)
      .. autodoxymethod:: simgrid::s4u::Mailbox::listen()
      .. autodoxymethod:: simgrid::s4u::Mailbox::ready()

   .. group-tab:: Python

       .. automethod:: simgrid.Mailbox.get

   .. group-tab:: C

      .. autodoxymethod:: sg_mailbox_listen(const char *alias)

Receiving actor
...............

See :ref:`s4u_receiving_actor`.

.. tabs::

   .. group-tab:: C++

      .. autodoxymethod:: simgrid::s4u::Mailbox::get_receiver()
      .. autodoxymethod:: simgrid::s4u::Mailbox::set_receiver(ActorPtr actor)

   .. group-tab:: C

      .. autodoxymethod:: ::sg_mailbox_set_receiver(const char *alias)
                  
.. _API_s4u_Resource:

=========
Resources
=========

.. _API_s4u_Disk:

=============
⁣  class Disk
=============

.. doxygenclass:: simgrid::s4u::Disk
   :members:
   :protected-members:
   :undoc-members:



.. _API_s4u_Host:

=============
⁣  class Host
=============

.. doxygenclass:: simgrid::s4u::Host
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Link:

=============
⁣  class Link
=============

.. doxygenclass:: simgrid::s4u::Link
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_NetZone:

================
⁣  class NetZone
================

.. doxygenclass:: simgrid::s4u::NetZone
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_VirtualMachine:

=======================
⁣  class VirtualMachine
=======================

.. doxygenclass:: simgrid::s4u::VirtualMachine
   :members:
   :protected-members:
   :undoc-members:

.. autodoxymethod:: sg_vm_create_core
.. autodoxymethod:: sg_vm_create_multicore
.. autodoxymethod:: sg_vm_get_name
.. autodoxymethod:: sg_vm_get_pm
.. autodoxymethod:: sg_vm_is_created
.. autodoxymethod:: sg_vm_is_running
.. autodoxymethod:: sg_vm_is_suspended
.. autodoxymethod:: sg_vm_start
.. autodoxymethod:: sg_vm_suspend
.. autodoxymethod:: sg_vm_resume
.. autodoxymethod:: sg_vm_shutdown
.. autodoxymethod:: sg_vm_destroy

.. _API_s4u_Activity:

==============
class Activity
==============

.. autodoxyclass:: simgrid::s4u::Activity

   **Known subclasses:**
   :ref:`Communications <API_s4u_Comm>` (started on Mailboxes and consuming links),
   :ref:`Executions <API_s4u_Exec>` (started on Host and consuming CPU resources)
   :ref:`I/O <API_s4u_Io>` (started on and consumming disks).
   See also the :ref:`section on activities <s4u_Activities>` above.

Querying info about activities
------------------------------

   .. autodoxymethod:: simgrid::s4u::Activity::get_remaining()
   .. autodoxymethod:: simgrid::s4u::Activity::get_state()
   .. autodoxymethod:: simgrid::s4u::Activity::set_remaining(double remains)
   .. autodoxymethod:: simgrid::s4u::Activity::get_impl

Activities lifecycle
--------------------

   .. autodoxymethod:: simgrid::s4u::Activity::start
   .. autodoxymethod:: simgrid::s4u::Activity::cancel
   .. autodoxymethod:: simgrid::s4u::Activity::test
   .. autodoxymethod:: simgrid::s4u::Activity::wait
   .. autodoxymethod:: simgrid::s4u::Activity::wait_for
   .. autodoxymethod:: simgrid::s4u::Activity::wait_until(double time_limit)

.. _API_s4u_Comm:

=============
⁣  class Comm
=============

.. doxygentypedef:: CommPtr

.. doxygenclass:: simgrid::s4u::Comm
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Exec:

=============
⁣  class Exec
=============

.. doxygentypedef:: ExecPtr

.. doxygenclass:: simgrid::s4u::Exec
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_ExecSeq:

==================
⁣    class ExecSeq
==================

.. doxygentypedef:: ExecSeqPtr

.. doxygenclass:: simgrid::s4u::ExecSeq
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_ExecPar:

==================
⁣    class ExecPar
==================

.. doxygentypedef:: ExecParPtr

.. doxygenclass:: simgrid::s4u::ExecPar
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Io:

===========
⁣  class Io
===========

.. doxygentypedef:: IoPtr

.. doxygenclass:: simgrid::s4u::Io
   :members:
   :protected-members:
   :undoc-members:

.. _API_s4u_Synchronizations:

=======================
Synchronization Objects
=======================

.. _API_s4u_Mutex:

==============
⁣  Mutex
==============

.. autodoxyclass:: simgrid::s4u::Mutex

Basic management
----------------

   .. tabs::

      .. group-tab:: C++

         .. code-block:: C++

            #include <simgrid/s4u/Mutex.hpp>

         .. doxygentypedef:: MutexPtr

         .. autodoxymethod:: simgrid::s4u::Mutex::Mutex(kernel::activity::MutexImpl *mutex)
         .. autodoxymethod:: simgrid::s4u::Mutex::create()
         .. autodoxymethod:: simgrid::s4u::Mutex::~Mutex()

      .. group-tab:: C

         .. code-block:: C

            #include <simgrid/mutex.h>

         .. doxygentypedef:: sg_mutex_t
         .. cpp:type:: const s4u_Mutex* const_sg_mutex_t

            Pointer to a constant mutex object.

         .. autodoxymethod:: sg_mutex_init()
         .. autodoxymethod:: sg_mutex_destroy(const_sg_mutex_t mutex)

Locking
-------

   .. tabs::

      .. group-tab:: C++

         .. autodoxymethod:: simgrid::s4u::Mutex::lock()
         .. autodoxymethod:: simgrid::s4u::Mutex::try_lock()
         .. autodoxymethod:: simgrid::s4u::Mutex::unlock()

      .. group-tab:: C

         .. autodoxymethod:: sg_mutex_lock(sg_mutex_t mutex)
         .. autodoxymethod:: sg_mutex_try_lock(sg_mutex_t mutex)
         .. autodoxymethod:: sg_mutex_unlock(sg_mutex_t mutex)

.. _API_s4u_Barrier:

================
⁣  Barrier
================

.. autodoxyclass:: simgrid::s4u::Barrier

   .. tabs::

      .. group-tab:: C++

         .. code-block:: C++

            #include <simgrid/s4u/Barrier.hpp>

         .. doxygentypedef:: BarrierPtr

         .. autodoxymethod:: simgrid::s4u::Barrier::Barrier(unsigned int expected_actors)
         .. autodoxymethod:: simgrid::s4u::Barrier::create(unsigned int expected_actors)
         .. autodoxymethod:: simgrid::s4u::Barrier::wait()

      .. group-tab:: C

         .. code-block:: C

            #include <simgrid/barrier.hpp>

         .. doxygentypedef:: sg_bar_t
         .. cpp:type:: const s4u_Barrier* const_sg_bar_t

            Pointer to a constant barrier object.

         .. autodoxymethod:: sg_barrier_init(unsigned int count)
         .. autodoxymethod:: sg_barrier_destroy(const_sg_bar_t bar)
         .. autodoxymethod:: sg_barrier_wait(sg_bar_t bar)


.. _API_s4u_ConditionVariable:

==========================
⁣  Condition variable
==========================

.. autodoxyclass:: simgrid::s4u::ConditionVariable

Basic management
----------------

   .. tabs::

      .. group-tab:: C++

         .. code-block:: C++

            #include <simgrid/s4u/ConditionVariable.hpp>

         .. doxygentypedef:: ConditionVariablePtr

         .. autodoxymethod:: simgrid::s4u::ConditionVariable::create()

      .. group-tab:: C

         .. code-block:: C

            #include <simgrid/cond.h>

         .. doxygentypedef:: sg_cond_t
         .. doxygenfunction:: sg_cond_init
         .. doxygenfunction:: sg_cond_destroy

Waiting and notifying
---------------------

   .. tabs::

      .. group-tab:: C++

         .. autodoxymethod:: simgrid::s4u::ConditionVariable::notify_all()
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::notify_one()
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait(s4u::MutexPtr lock)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait(const std::unique_lock< s4u::Mutex > &lock)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait(const std::unique_lock< Mutex > &lock, P pred)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait_for(const std::unique_lock< s4u::Mutex > &lock, double duration)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait_for(const std::unique_lock< s4u::Mutex > &lock, double duration, P pred)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait_for(const std::unique_lock< s4u::Mutex > &lock, std::chrono::duration< Rep, Period > duration)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait_for(const std::unique_lock< s4u::Mutex > &lock, std::chrono::duration< Rep, Period > duration, P pred)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait_until(const std::unique_lock< s4u::Mutex > &lock, const SimulationTimePoint< Duration > &timeout_time)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait_until(const std::unique_lock< s4u::Mutex > &lock, const SimulationTimePoint< Duration > &timeout_time, P pred)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait_until(const std::unique_lock< s4u::Mutex > &lock, double timeout_time)
         .. autodoxymethod:: simgrid::s4u::ConditionVariable::wait_until(const std::unique_lock< s4u::Mutex > &lock, double timeout_time, P pred)

      .. group-tab:: C

         .. doxygenfunction:: sg_cond_notify_all
         .. doxygenfunction:: sg_cond_notify_one
         .. doxygenfunction:: sg_cond_wait
         .. doxygenfunction:: sg_cond_wait_for

.. _API_s4u_Semaphore:

==================
⁣  Semaphore
==================

.. autodoxyclass:: simgrid::s4u::Semaphore


Basic management
----------------

   .. tabs::

      .. group-tab:: C++

         .. code-block:: C++

            #include <simgrid/s4u/Semaphore.hpp>

         .. doxygentypedef:: SemaphorePtr
         .. autodoxymethod:: simgrid::s4u::Semaphore::Semaphore(unsigned int initial_capacity)
         .. autodoxymethod:: simgrid::s4u::Semaphore::~Semaphore()
         .. autodoxymethod:: simgrid::s4u::Semaphore::create(unsigned int initial_capacity)

      .. group-tab:: C

         .. code-block:: C

            #include <simgrid/semaphore.h>

         .. doxygentypedef:: sg_sem_t
         .. cpp:type:: const s4u_Semaphore* const_sg_sem_t

            Pointer to a constant semaphore object.

         .. autodoxymethod:: sg_sem_init(int initial_value)
         .. autodoxymethod:: sg_sem_destroy(const_sg_sem_t sem)

Locking
-------

   .. tabs::

      .. group-tab:: C++

         .. autodoxymethod:: simgrid::s4u::Semaphore::acquire()
         .. autodoxymethod:: simgrid::s4u::Semaphore::acquire_timeout(double timeout)
         .. autodoxymethod:: simgrid::s4u::Semaphore::get_capacity()
         .. autodoxymethod:: simgrid::s4u::Semaphore::release()
         .. autodoxymethod:: simgrid::s4u::Semaphore::would_block()

      .. group-tab:: C

         .. autodoxymethod:: sg_sem_acquire(sg_sem_t sem)
         .. autodoxymethod:: sg_sem_acquire_timeout(sg_sem_t sem, double timeout)
         .. autodoxymethod:: sg_sem_get_capacity(sg_sem_t sem)
         .. autodoxymethod:: sg_sem_release(sg_sem_t sem)
         .. autodoxymethod:: sg_sem_would_block(sg_sem_t sem)

Python API Reference
********************

The Python API is automatically generated with pybind11. It closely mimicks the C++
API, to which you should refer for more information.

==========
this_actor
==========

.. automodule:: simgrid.this_actor
   :members:

==========
Class Comm
==========

.. autoclass:: simgrid.Comm
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
