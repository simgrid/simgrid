.. _models:

The SimGrid Models
##################

.. todo::

   - Main existing models (contention, cste, LM07)
   - Main concepts (Routing, LMM) + link to the papers
   - How to switch on the command line

.. _understanding_lv08:

The default TCP model
*********************

When simulating a data transfer between two hosts, you may be surprised
by the obtained simulation time. Lets consider the following platform:

.. code-block:: xml

   <host id="A" speed="1Gf" />
   <host id="B" speed="1Gf" />

   <link id="link1" latency="10ms" bandwidth="1Mbps" />

   <route src="A" dst="B">
     <link_ctn id="link1" />
   </route>

If host `A` sends `100kB` (a hundred kilobytes) to host `B`, one could expect
that this communication would take `0.81` seconds to complete according to a
simple latency-plus-size-divided-by-bandwidth model (0.01 + 8e5/1e6 = 0.81).
However, the default TCP model of SimGrid is a bit more complex than that. It

accounts for three phenomena that directly impact the simulation time even
on such a simple example:

  - The size of a message at the application level (i.e., 100kB in this
    example) is not the size that will actually be transferred over the
    network. To mimic the fact that TCP and IP headers are added to each packet of
    the original payload, the TCP model of SimGrid empirically considers that
    `only 97% of the nominal bandwidth` are available. In other words, the
    size of your message is increased by a few percents, whatever this size be.

  - In the real world, the TCP protocol is not able to fully exploit the
    bandwidth of a link from the emission of the first packet. To reflect this
    `slow start` phenomenon, the latency declared in the platform file is
    multiplied by `a factor of 13.01`. Here again, this is an empirically
    determined value that may not correspond to every TCP implementations on
    every networks. It can be tuned when more realistic simulated times for
    short messages are needed though.

  - When data is transferred from A to B, some TCP ACK messages travel in the
    opposite direction. To reflect the impact of this `cross-traffic`, SimGrid
    simulates a flow from B to A that represents an additional bandwidth
    consumption of `0.05`. The route from B to A is implicitly declared in the
    platform file and uses the same link `link1` as if the two hosts were
    connected through a communication bus. The bandwidth share allocated to the
    flow from A to B is then the available bandwidth of `link1` (i.e., 97% of
    the nominal bandwidth of 1Mb/s) divided by 1.05 (i.e., the total consumption).
    This feature, activated by default, can be disabled by adding the
    `--cfg=network/crosstraffic:0` flag to command line.

As a consequence, the time to transfer 100kB from A to B as simulated by the
default TCP model of SimGrid is not 0.81 seconds but

.. code-block:: python

    0.01 * 13.01 + 800000 / ((0.97 * 1e6) / 1.05) =  0.996079 seconds.

.. _model_ns3:

ns-3 as a SimGrid model
***********************

You can use the well-known `ns-3 packet-level network simulator
<http://www.nsnam.org>`_ as a SimGrid model, for example to investigate the
validity of your simulation. Just install ns-3 and recompile SimGrid
accordingly.

The SimGrid/ns-3 binding only contains features that are common to both systems.
Not all ns-3 models are available from SimGrid (only the TCP and WiFi ones are),
while not all SimGrid platform files can be used in conjunction ns-3 (routes
must be of length 1). Also, the platform built in ns-3 from the SimGrid
description is very basic. Finally, communicating from a host to
itself is forbidden in ns-3, so every such communication completes
immediately upon startup.


Compiling the ns-3/SimGrid binding
==================================

Installing ns-3
---------------

SimGrid requires ns-3 version 3.26 or higher, and you probably want the most
recent version of both SimGrid and ns-3. While the Debian package of SimGrid
don't have the ns-3 bindings activated, you can still use the packaged version
of ns-3 by grabbing the ``libns3-dev ns3`` packages. Alternatively, you can
install ns-3 from scratch (see the `ns-3 documentation <http://www.nsnam.org>`_).

Enabling ns-3 in SimGrid
------------------------

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
---------------

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
=======================

Platform files compatibility
----------------------------

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


WiFi platforms
^^^^^^^^^^^^^^

In SimGrid, WiFi networks are modeled with WiFi zones, where a zone contains
the access point of the WiFi network and the hosts connected to it (called
station in the WiFi world). Links inside WiFi zones are modeled as regular
links with a specific attribute, and these links are then added to routes
between hosts. The main difference When using ns-3 WiFi networks is that
the network performance is not given by the link bandwidth and latency but
by the access point WiFi characteristics, and the distance between the access
point and the hosts.

So, to declare a new WiFi network, simply declare a zone with the ``WIFI``
routing.

.. code-block:: xml

	<zone id="SSID_1" routing="WIFI">

Inside this zone you must declare which host or router will be the access point
of the WiFi network.

.. code-block:: xml

	<prop id="access_point" value="alice"/>

Afterward simply declare the hosts and routers inside the WiFi network. Remember
that one must have the same name as declared in the property "access point".

.. code-block:: xml

	<router id="alice" speed="1Gf"/>
	<host id="STA0-0" speed="1Gf"/>
	<host id="STA0-1" speed="1Gf"/>

Finally, close the WiFi zone.

.. code-block:: xml

	</zone>

The WiFi zone may be connected to another zone using a traditional link and
a zoneRoute. Note that the connection between two zones is always wired.

.. code-block:: xml

	<link id="wireline" bandwidth="100Mbps" latency="2ms" sharing_policy="SHARED"/>

	<zoneRoute src="SSID_1" dst="SSID_2" gw_src="alice" gw_dst="bob">
	    <link_ctn id="wireline"/>
	</zoneRoute>

WiFi network performance
""""""""""""""""""""""""

The performance of a wifi network is controlled by 3 property that can be added
to hosts connected to the wifi zone:

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

Random Number Generator
-----------------------

It is possible to define a fixed or random seed to the ns3 random number
generator using the config tag.

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
