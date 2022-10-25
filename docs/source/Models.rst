.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("ModelBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

.. _models:

The SimGrid Models
##################

As for any simulator, the models are very important components in SimGrid. This page first introduces the several kind of models
used in SimGrid before focusing on the **performance models** that compute the duration of :ref:`every activities
<S4U_main_concepts>` in the simulator depending on the platform characteristics and on the other activities that are sharing the
resources.

The **routing models** constitute advanced elements of the platform description. This description naturally entails
:ref:`components<platform>` that are very related to the performance models, because determining for example the execution time
of a task obviously depends on the characteristics of the machine executing it. Furthermore, networking zones can be
interconnected to form larger platforms `in a scalable way <http://hal.inria.fr/hal-00650233/>`_. Each of these zone can be given
a specific :ref:`routing model<platform_routing>` that efficiently computes the list of links entailing a network path between
two given hosts.

The model checker uses an abstraction of the performance simulations. Mc SimGrid explores every causally possible executions of
the application, completely abstracting the performance away. The simulated time not even computed in this mode! The abstraction
involved in this process also models the mutual impacts between actions, to not re-explore histories that only differ by the
order of independent and unrelated actions. As with the rest of the model checker, these models are unfortunately still to be
documented properly.

Finally, the `SimGrid-FMI external plugin <https://framagit.org/simgrid/simgrid-FMI>`_ can be used to integrate any FMI-based
models into SimGrid. This was used to accurately study a *Smart grid* through co-simulation: `PandaPower
<http://www.pandapower.org/>`_ was used to simulate the power grid, `ns-3 <https://nsnam.org/>`_ co-simulate was used the
communication network while SimGrid was simulating the IT infrastructure. Please refer to the `relevant publication
<https://hal.archives-ouvertes.fr/hal-01762540/>`_ for more details.

Modeled resources
*****************

The main objective of SimGrid is to provide timing information for three kind of resources: network, CPU and disk.

The **network models** are improved and assessed since almost 20 years. It should be possible to get accurate predictions once
you properly :ref:`calibrate the models for your settings<models_calibration>`. As detailed in the next section, SimGrid
provides several network models. Two plugins can be used to compute the network energy consumption: One for the :ref:`wired
networks<plugin_link_energy>`, and another one for the :ref:`Wi-Fi networks<plugin_link_energy>`. Some users find :ref:`TCP
simulated performance counter-intuitive<understanding_lv08>` at first in SimGrid, sometimes because of a misunderstanding of the
TCP behavior in real networks.

The **computing models** are less developed in SimGrid. With the S4U interface, the user specifies the amount of flops that each
computation "consumes", and the model simply divides this amount by the host's flops rate to compute the duration of this
execution. In SMPI, the user code is automatically timed, and the :ref:`computing speed<cfg=smpi/host-speed>` of the host
machine is used to evaluate the corresponding amount of flops. This model should be sufficient for most users, even if assuming
a constant flops rate for each machine is a simplification. In reality, the flops rate varies because of I/O, memory and cache
effects. It is somehow possible to :ref:`overcome this simplification<cfg=smpi/comp-adjustment-file>`, but the required
calibration process is rather intricate and not documented yet (feel free to :ref:`contact the community<community>` on need).
In the future, more advanced models may be added but the existing model proved good enough for all experiments done on
distributed applications during the last two decades. The CPU energy consumption can be computed with the :ref:`relevant
plugin<plugin_host_energy>`.

The **disk models** of SimGrid are more recent than for the network and computing resources, but they should still be correct
for most users. `Studies have shown <https://hal.inria.fr/hal-01197128>`_ that they are sensible under some conditions, and a
:ref:`calibration process<howto_disk>` is provided. As usual, you probably want to double-check their predictions through an
appropriate validation campaign.

SimGrid main models
*******************

SimGrid aims at the sweet spot between accuracy and simulation speed. Concerning the accuracy, our goal is to report correct
performance trends when comparing competing designs with minimal burden on the user, while allowing power users to fine tune the
simulation models for predictions that are within 5% or below of the results on real machines. For example, we determined the
`speedup achieved by the Tibidabo ARM-based cluster <http://hal.inria.fr/hal-00919507>`_ before its construction. On the other
side, the tool must be fast and scalable enough to study modern IT systems at scale. SimGrid was for example used to `simulate a
Chord ring involving millions of actors <https://hal.inria.fr/inria-00602216>`_ (even if that not really more instructive for
this protocol than smaller simulations), or `a qualification run at full-scale of the Stampede supercomputer
<https://hal.inria.fr/hal-02096571>`_.

Most of our models are based on a linear max-min solver (LMM), as depicted below. The actors' activities are represented by
actions in the simulation kernel, accounting the initial amount of work of the corresponding activity (in flops for computing
activities or bytes for networking and disk activities), and the remaining amount of work. At each simulation step, the
instantaneous computing and communicating speed of each action is computed according to the model. A set of constraints is used
to express for example that the instantaneous speed of actions on a given resource must remain smaller than the instantaneous
speed of that resource. In the example below, it is stated that the speed :math:`\varrho_1` of activity 1 plus the speed :math:`\varrho_n`
of activity :math:`n` must remain smaller than the capacity :math:`C_A` of the corresponding host A.

.. image:: img/lmm-overview.svg

There is obviously many valuation of :math:`\varrho_1 \ldots{} \varrho_n` that respect such as set of constraints. SimGrid usually computes
the instantaneous speeds according to a Max-Mix objective function, that maximizing the minimum over all :math:`\varrho_i`. The
coefficients associated to each variable in the inequalities are used to model some performance effects, such as the fact that
TCP tend to favor communications with small RTTs. These coefficients computed from both hardcoded values and from the
:ref:`latency and bandwidth factors<cfg=network/latency-factor>`.

Once the instantaneous speeds are computed, the simulation kernel computes the earliest terminating action from their speeds and
remaining work. The simulated time is then updated along with the values in the LMM. The corresponding activities terminate,
unblocking the corresponding actors that can further execute.

Most of the SimGrid models build upon the LMM solver, that they adapt and configure for a given usage. **CM02** is the simplest
LMM model as it does not introduce any correction factors. This model should be used if you prefer understandable results over
realistic ones. **LV08** (the default model) uses constant factors that are intended to capture common effects such as
slow-start, or the fact that TCP headers reduce the *effective* bandwidth. **SMPI** use more advanced factors that also capture
the MPI-specific effects such as the eager vs. rendez-vous communication mode. You can :ref:`pick another
model<options_model_select>` on the command line, and these models can be :ref:`further configured<options_model>`.

**L07** is rather distinct because it uses another objective function called *bottleneck*. This is because this model is
intended to be used for parallel tasks that are actions mixing flops and bytes while the Max-Min objective function requires
that all variables are expressed using the same unit. This is also why in reality, we have one LMM system per resource kind in
the simulation, but the idea remains similar.

.. _understanding_lv08:

The default TCP model
=====================

When simulating a data transfer between two hosts, you may be surprised by the obtained simulation time. Lets consider the
following platform:

.. code-block:: xml

   <host id="A" speed="1Gf" />
   <host id="B" speed="1Gf" />

   <link id="link1" latency="10ms" bandwidth="1Mbps" />

   <route src="A" dst="B">
     <link_ctn id="link1" />
   </route>

If host `A` sends `100kB` (a hundred kilobytes) to host `B`, one could expect that this communication would take `0.81` seconds
to complete according to a simple latency-plus-size-divided-by-bandwidth model (0.01 + 8e5/1e6 = 0.81). However, the default TCP
model of SimGrid is a bit more complex than that. It accounts for three phenomena that directly impact the simulation time even
on such a simple example:

  - The size of a message at the application level (i.e., 100kB in this example) is not the size that will actually be
    transferred over the network. To mimic the fact that TCP and IP headers are added to each packet of the original payload,
    the TCP model of SimGrid empirically considers that `only 97% of the nominal bandwidth` are available. In other words, the
    size of your message is increased by a few percents, whatever this size be.

  - In the real world, the TCP protocol is not able to fully exploit the bandwidth of a link from the emission of the first
    packet. To reflect this `slow start` phenomenon, the latency declared in the platform file is multiplied by `a factor of
    13.01`. Here again, this is an empirically determined value that may not correspond to every TCP implementations on every
    networks. It can be tuned when more realistic simulated times for short messages are needed though.

  - When data is transferred from A to B, some TCP ACK messages travel in the opposite direction. To reflect the impact of this
    `cross-traffic`, SimGrid simulates a flow from B to A that represents an additional bandwidth consumption of `0.05`. The
    route from B to A is implicitly declared in the platform file and uses the same link `link1` as if the two hosts were
    connected through a communication bus. The bandwidth share allocated to the flow from A to B is then the available bandwidth
    of `link1` (i.e., 97% of the nominal bandwidth of 1Mb/s) divided by 1.05 (i.e., the total consumption). This feature,
    activated by default, can be disabled by adding the `--cfg=network/crosstraffic:0` flag to command line.

As a consequence, the time to transfer 100kB from A to B as simulated by the default TCP model of SimGrid is not 0.81 seconds
but

.. code-block:: python

    0.01 * 13.01 + 800000 / ((0.97 * 1e6) / 1.05) =  0.996079 seconds.


WiFi zones
==========

In SimGrid, WiFi networks are modeled with WiFi zones, where a zone contains the access point of the WiFi network and the hosts
connected to it (called station in the WiFi world). Links inside WiFi zones are modeled as regular links with a specific
attribute, and these links are then added to routes between hosts. The main difference of WiFi networks is that their
performance is not given by the link bandwidth and latency but by both the access point WiFi characteristics and the distance
between the access point and the hosts.

Such WiFi zones can be used in both the LMM-based model or with ns-3, and are supposed to behave similarly in both cases.

Declaring a WiFi zone
---------------------

To declare a new WiFi network, simply declare a network zone with the ``WIFI`` routing.

.. code-block:: xml

	<zone id="SSID_1" routing="WIFI">

Inside this zone you must declare which host or router will be the access point of the WiFi network.

.. code-block:: xml

	<prop id="access_point" value="alice"/>

Afterward simply declare the hosts and routers inside the WiFi network. Remember that one must have the same name as declared in
the property "access point".

.. code-block:: xml

	<router id="alice" speed="1Gf"/>
	<host id="STA0-0" speed="1Gf"/>
	<host id="STA0-1" speed="1Gf"/>

Finally, close the WiFi zone.

.. code-block:: xml

	</zone>

The WiFi zone may be connected to another zone using a traditional link and a zoneRoute. Note that the connection between two
zones is always wired.

.. code-block:: xml

	<link id="wireline" bandwidth="100Mbps" latency="2ms" sharing_policy="SHARED"/>

	<zoneRoute src="SSID_1" dst="SSID_2" gw_src="alice" gw_dst="bob">
	    <link_ctn id="wireline"/>
	</zoneRoute>

WiFi network performance
------------------------

The performance of a wifi network is controlled by 3 property that can be added to hosts connected to the wifi zone:

 * ``mcs`` (`Modulation and Coding Scheme <https://en.wikipedia.org/wiki/Link_adaptation>`_)
   Roughly speaking, it defines the speed at which the access point is
   exchanging data with all stations. It depends on its model and configuration,
   and the possible values are listed for example on Wikipedia.
   |br| By default, ``mcs=3``.
   It is a property of the WiFi zone.
 * ``nss`` (Number of Spatial Streams, or `number of antennas <https://en.wikipedia.org/wiki/IEEE_802.11n-2009#Number_of_antennas>`_)
   defines the amount of simultaneous data streams that the AP can sustain.
   Not all value of MCS and NSS are valid nor compatible (cf. `802.11n standard <https://en.wikipedia.org/wiki/IEEE_802.11n-2009#Data_rates>`_).
   |br| By default, ``nss=1``.
   It is a property of the WiFi zone.
 * ``wifi_distance`` is the distance from the station to the access point. Each
   station can have a specific value.
   |br| By default, ``wifi_distance=10``.
   It is a property of stations of the WiFi network.

Here is an example of a zone changing ``mcs`` and ``nss`` values.

.. code-block:: xml

	<zone id="SSID_1" routing="WIFI">
	    <prop id="access_point" value="alice"/>
	    <prop id="mcs" value="2"/>
	    <prop id="nss" value="2"/>
	...
	</zone>

Here is an example of a host changing ``wifi_distance`` value.

.. code-block:: xml

	<host id="STA0-0" speed="1Gf">
	    <prop id="wifi_distance" value="37"/>
	</host>

Other models
************

SimGrid provides two other models in addition to the LMM-based ones.

First, the **constant-time model** is a simplistic network model where all communication take a constant time (one second). It
provides the lowest realism, but is marginally faster and much simpler to understand. This model may reveal interesting if you
plan to study abstract distributed algorithms such as leader election or causal broadcast.

On the contrary, the **ns-3 based model** is the most accurate network model that you can get in SimGrid. It relies on the
well-known `ns-3 packet-level network simulator <http://www.nsnam.org>`_ to compute every timing information of your simulation.
For example, this may be used to investigate the validity of a simulation. Note that this model is much slower than the
LMM-based models, because ns-3 simulates every network packet involved in as communication while SimGrid only recompute the
instantaneous speeds when one of the communications starts or stops. Both simulators are linear in the size of their input, but
ns-3 has a much larger input in case of large steady communications.

ns-3 as a SimGrid model
=======================

You need to install ns-3 and recompile SimGrid accordingly to use this model.

The SimGrid/ns-3 binding only contains features that are common to both systems.
Not all ns-3 models are available from SimGrid (only the TCP and WiFi ones are),
while not all SimGrid platform files can be used in conjunction ns-3 (routes
must be of length 1). Also, the platform built in ns-3 from the SimGrid
description is very basic. Finally, communicating from a host to
itself is forbidden in ns-3, so every such communication completes
immediately upon startup.


Compiling the ns-3/SimGrid binding
----------------------------------

Installing ns-3
^^^^^^^^^^^^^^^

SimGrid requires ns-3 version 3.26 or higher, and you probably want the most
recent version of both SimGrid and ns-3. While the Debian package of SimGrid
don't have the ns-3 bindings activated, you can still use the packaged version
of ns-3 by grabbing the ``libns3-dev ns3`` packages. Alternatively, you can
install ns-3 from scratch (see the `ns-3 documentation <http://www.nsnam.org>`_).

Enabling ns-3 in SimGrid
^^^^^^^^^^^^^^^^^^^^^^^^

SimGrid must be recompiled with the ``enable_ns3`` option activated in cmake.
Optionally, use ``NS3_HINT`` to tell cmake where ns3 is installed on
your disk.

.. code-block:: console

   $ cmake . -Denable_ns3=ON -DNS3_HINT=/opt/ns3 # or change the path if needed

By the end of the configuration, cmake reports whether ns-3 was found,
and this information is also available in ``include/simgrid/config.h``
If your local copy defines the variable ``SIMGRID_HAVE_NS3`` to 1, then ns-3
was correctly detected. Otherwise, explore ``CMakeFiles/CMakeOutput.log`` and
``CMakeFiles/CMakeError.log`` to diagnose the problem.

Test that ns-3 was successfully integrated with the following (from your SimGrid
build directory). It will run all SimGrid tests that are related to the ns-3
integration. If no test is run at all, you probably forgot to enable ns-3 in cmake.

.. code-block:: console

   $ ctest -R ns3

Troubleshooting
^^^^^^^^^^^^^^^

If you use a version of ns-3 that is not known to SimGrid yet, edit
``tools/cmake/Modules/FindNS3.cmake`` in your SimGrid tree, according to the
comments on top of this file. Conversely, if something goes wrong with an old
version of either SimGrid or ns-3, try upgrading everything.

Note that there is a known bug with version 3.31 of ns3, when it's built with
MPI support, like it is with the package libns3-dev in Debian 11 « Bullseye ».
A simple workaround is to edit the file
``/usr/include/ns3.31/ns3/point-to-point-helper.h`` to remove the ``#ifdef NS3_MPI``
include guard.  This can be achieved with the following command (as root):

.. code-block:: console

   # sed -i '/^#ifdef NS3_MPI/,+2s,^#,//&,' /usr/include/ns3.31/ns3/point-to-point-helper.h

.. _ns3_use:

Using ns-3 from SimGrid
-----------------------

Platform files compatibility
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Any route longer than one will be ignored when using ns-3. They are
harmless, but you still need to connect your hosts using one-hop routes.
The best solution is to add routers to split your route. Here is an
example of an invalid platform:

.. code-block:: xml

   <?xml version='1.0'?>
   <!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
   <platform version="4.1">
     <zone id="zone0" routing="Floyd">
       <host id="alice" speed="1Gf" />
       <host id="bob"   speed="1Gf" />

       <link id="l1" bandwidth="1Mbps" latency="5ms" />
       <link id="l2" bandwidth="1Mbps" latency="5ms" />

       <route src="alice" dst="bob">
         <link_ctn id="l1"/>            <!-- !!!! IGNORED WHEN USED WITH ns-3       !!!! -->
         <link_ctn id="l2"/>            <!-- !!!! ROUTES MUST CONTAIN ONE LINK ONLY !!!! -->
       </route>
     </zone>
   </platform>

This can be reformulated as follows to make it usable with the ns-3 binding.
There is no direct connection from alice to bob, but that's OK because ns-3
automatically routes from point to point (using
``ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables``).

.. code-block:: xml

   <?xml version='1.0'?>
   <!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
   <platform version="4.1">
     <zone id="zone0" routing="Full">
       <host id="alice" speed="1Gf" />
       <host id="bob"   speed="1Gf" />

       <router id="r1" /> <!-- routers are compute-less hosts -->

       <link id="l1" bandwidth="1Mbps" latency="5ms"/>
       <link id="l2" bandwidth="1Mbps" latency="5ms"/>

       <route src="alice" dst="r1">
         <link_ctn id="l1"/>
       </route>

       <route src="r1" dst="bob">
         <link_ctn id="l2"/>
       </route>
     </zone>
   </platform>

Once your platform is OK, just change the :ref:`network/model
<options_model_select>` configuration option to `ns-3` as follows. The other
options can be used as usual.

.. code-block:: console

   $ ./network-ns3 --cfg=network/model:ns-3 (other parameters)

Many other files from the ``examples/platform`` directory are usable with the
ns-3 model, such as `examples/platforms/dogbone.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/dogbone.xml>`_.
Check the file  `examples/cpp/network-ns3/network-ns3.tesh <https://framagit.org/simgrid/simgrid/tree/master/examples/cpp/network-ns3/network-ns3.tesh>`_
to see which ones are used in our regression tests.

Alternatively, you can manually modify the ns-3 settings by retrieving
the ns-3 node from any given host with the
:cpp:func:`simgrid::get_ns3node_from_sghost` function (defined in
``simgrid/plugins/ns3.hpp``).

.. doxygenfunction:: simgrid::get_ns3node_from_sghost

Random seed
-----------
It is possible to define a fixed or random seed to the ns3 random number generator using the config tag.

.. code-block:: xml

	<?xml version='1.0'?><!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
	<platform version="4.1">
	    <config>
		    <prop id = "network/model" value = "ns-3" />
		    <prop id = "ns3/seed" value = "time" />
	    </config>
	...
	</platform>

The first property defines that this platform will be used with the ns3 model.
The second property defines the seed that will be used. Defined to ``time``
it will use a random seed, defined to a number it will use this number as
the seed.

Limitations
-----------

A ns-3 platform is automatically created from the provided SimGrid
platform. However, there are some known caveats:

  * The default values (e.g., TCP parameters) are the ns-3 default values.
  * ns-3 networks are routed using the shortest path algorithm, using ``ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables``.
  * End hosts cannot have more than one interface card. So, your SimGrid hosts
    should be connected to the platform through only one link. Otherwise, your
    SimGrid host will be considered as a router (FIXME: is it still true?).

Our goal is to keep the ns-3 plugin of SimGrid as easy (and hopefully readable)
as possible. If the current state does not fit your needs, you should modify
this plugin, and/or create your own plugin from the existing one. If you come up
with interesting improvements, please contribute them back.

Troubleshooting
---------------

If your simulation hangs in a communication, this is probably because one host
is sending data that is not routable in your platform. Make sure that you only
use routes of length 1, and that any host is connected to the platform.
Arguably, SimGrid could detect this situation and report it, but unfortunately,
this is still to be done.


.. |br| raw:: html

   <br />
