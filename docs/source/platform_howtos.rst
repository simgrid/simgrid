.. _platform:

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" width="100%" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("PlatformBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

Modeling Hints
##############

There is no perfect model, but only models that are adapted to the
specific study that you want to do. SimGrid provide several advanced
mechanisms that you can adapt to model the situation that you are
interested into, and it is often uneasy to see where to start with.
This page collects several hints and tricks on modeling situations.
Even if you are looking for a very advanced, specific use case, these
examples may help you to design the solution you need.

Doing Science with SimGrid
**************************

Many users are using SimGrid as a scientific instrument for their
research. This tool was indeed invented to that extend, and we strive
to streamline this kind of usage. But SimGrid is no magical tool, and
it is of your responsability that the tool actually provides sensible
results. Fortunately, there is a vast literature on how to avoid
Modeling & Simulations pitfalls. We review here two particular works.

In `An Integrated Approach to Evaluating Simulation Credibility
<http://www.dtic.mil/dtic/tr/fulltext/u2/a405051.pdf>`_, the authors
provide a methodology enabling the users to increase their confidence
in the simulation tools they use. First of all, you must know what you
actually expect to discover whether the tool actually covers your
needs. Then, as they say, "a fool with a tool is still a fool", so you
need to think about your methodology before you submit your articles.
You really should read this article, that contains many other
interesting advices.

`Seven Pitfalls in Modeling and Simulation Research
<https://dl.acm.org/citation.cfm?id=2430188>`_ is even more
specific. Here are the listed pitfalls: (1) Don't know whether it's
modeling or simulation, (2) No separation of concerns, (3) No clear
scientific question, (4) Implementing everything from scratch, (5)
Unsupported claims, (6) Toy duck approach, and (7) The tunnel view. As
you can see, this article is a must read. It's a pitty that it's not
freely available, though.

.. _howto_churn:

Modeling Churn in P2P
*********************

One of the biggest challenges in P2P settings is to cope with the
churn, meaning that resources keep appearing and disappearing. In
SimGrid, you can always change the state of each host manually, with
eg :cpp:func:`simgrid::s4u::Host::turn_on`. To reduce the burden when
the churn is high, you can also attach a **state profile** to the host
directly.

This is not possible from S4U yet (TODO), and you should use the
``state_file`` attribute of :ref:`pf_tag_host`, :ref:`pf_tag_cluster`
or :ref:`pf_tag_link`.

Every lines (but the last) of such files describe timed events with
the form "date value". Example:

.. code-block:: python
		
   1 0
   2 1
   LOOPAFTER 8

- At time t=1, the host is turned off (value 0 means OFF)
- At time t=2, it is turned back on (other values means ON)
- At time t=10, the history is reset (because that's 8 seconds after
  the last event). So the host will be turned off again at t=11.

If your trace does not contain a LOOPAFTER line, then your profile is
only executed once and not repetitively.

.. _howto_multicore:

Modeling Multicore Machines
***************************

Default Model
=============

Multicore machines are very complex, and there is many way to model
them. The default models of SimGrid are coarse grain and capture some
elements of this reality. Here is how to declare simple multicore hosts:

.. code-block:: xml
		
   <host id="mymachine" speed="8Gf" core="4"/>

It declares a 4-cores host called "mymachine", each core computing 8
GFlops per second. If you put one activity of 8 GFlop on this host, it
will be computed in 1 second (by default, activities are
single-threaded and cannot leverage the computing power of more than
one core). If you put two of them together, they will still be
computed in one second, and so on up to 4 tasks. If you put 5 tasks,
they will share the total computing resource, and all tasks will be
computed at 5/4 = 1.25 second. That's a very simple model, but that's
all what you will get by default from SimGrid.

Pinning tasks to cores
======================

The default model does not account for task pinning, where you
manually select on which core each of the existing activity should
execute. The best solution to model this is probably to model your
4-core processor as 4 separte hosts, and assigning the activities to
cores by migrating them to the declared hosts. In some sense, this 
takes the whole Network-On-Chip idea really seriously.

Some extra complications may arise here. If you have more tasks than
cores, you'll have to `schedule your tasks
<https://en.wikipedia.org/wiki/Scheduling_%28computing%29#Operating_system_process_scheduler_implementations)>`_
yourself on the cores (so you'd better avoid this complexity). Since
you cannot have more than one network model in a given SimGrid
simulation, you will end up with a TCP connexion between your cores. A
possible work around is to never start any simulated communication
between the cores and have the same routes from each core to the
rest of the external network.

Modeling a multicore CPU as a set of SimGrid hosts may seem strange
and unconvincing, but some users achieved very realistic simulations
of multi-core and GPU machines this way.

Modeling machine bootup and shutdown periods
********************************************

When a physical host boots up, a lot of things happen. It takes time
during which the machine is not usable but dissipates energy, and
programs actually die and restart during a reboot. Since there is many
ways to model it, SimGrid does not do any modeling choice for you but
the most obvious ones.

Any actor (or process in MSG) running on an host that is shut down
will be killed and all its activities (tasks in MSG) will be
automatically canceled. If killed the actor was marked as
auto-restartable (with
:cpp:func:`simgrid::s4u::Actor::set_auto_restart` or with
:cpp:func:`MSG_process_auto_restart_set`), it will start anew with the
same parameters when the host boots back up.

By default, shutdowns and bootups are instantaneous. If you want to
add an extra delay, you have to do that yourself, for example from an
`controler` actor that runs on another host. The best way to do so is
to declare a fictionous pstate where the CPU delivers 0 flop per
second (so every activity on that host will be frozen when the host is
in this pstate). When you want to switch the host off, your controler
switches the host to that specific pstate (with
:cpp:func:`simgrid::s4u::Host::set_pstate`), waits for the amount of
time that you decided necessary for your host to shut down, and turns
the host off (with :cpp:func:`simgrid::s4u::Host::turn_off`). To boot
up, switch the host on, go into the specific pstate, wait a while and
go to a more regular pstate.

To model the energy dissipation, you need to put the right energy
consumption in your startup/shutdown specific pstate. Remember that
the energy consumed is equal to the instantaneous consumption
multiplied by the time in which the host keeps in that state. Do the
maths, and set the right instantaneous consumption to your pstate, and
you'll get the whole boot period to consume the amount of energy that
you want. You may want to have one fictionous pstate for the bootup
period and another one for the shutdown period.

Of course, this is only one possible way to model these things. YMMV ;)

