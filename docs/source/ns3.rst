.. _model_ns3:

ns-3 as a SimGrid model
#######################

You can use the well-known `ns-3 packet-level network simulator
<http://www.nsnam.org>`_ as a SimGrid model, for example to investigate the
validity of your simulation. Just install ns-3 and recompile SimGrid
accordingly.

The SimGrid/ns-3 binding only contains features that are common to both systems.
Not all ns-3 models are available from SimGrid (only the TCP and WiFi ones are),
while not all SimGrid platform files can be used in conjunction ns-3 (routes
must be of length 1). Also, the platform built in ns-3 from the SimGrid
description is very basic. Finally, communicating from an host to
itself is forbidden in ns-3, so every such communication completes
immediately upon startup.


Compiling the ns-3/SimGrid binding
**********************************

Installing ns-3
===============

SimGrid requires ns-3 version 3.26 or higher, and you probably want the most
recent version of both SimGrid and ns-3. While the Debian package of SimGrid
don't have the ns-3 bindings activated, you can still use the packaged version
of ns-3 by grabbing the ``libns3-dev ns3`` packages. Alternatively, you can
install ns-3 from scratch (see the `ns-3 documentation <http://www.nsnam.org>`_).

Enabling ns-3 in SimGrid
========================

SimGrid must be recompiled with the ``enable_ns3`` option activated in cmake.
Optionally, use ``NS3_HINT`` to tell cmake where ns3 is installed on
your disk.

.. code-block:: shell

   cmake . -Denable_ns3=ON -DNS3_HINT=/opt/ns3 # or change the path if needed

By the end of the configuration, cmake reports whether ns-3 was found,
and this information is also available in ``include/simgrid/config.h``
If your local copy defines the variable ``SIMGRID_HAVE_NS3`` to 1, then ns-3
was correctly detected. Otherwise, explore ``CMakeFiles/CMakeOutput.log`` and
``CMakeFiles/CMakeError.log`` to diagnose the problem.

Test that ns-3 was successfully integrated with the following (from your SimGrid
build directory). It will run all SimGrid tests that are related to the ns-3
integration. If no test is run at all, you probably forgot to enable ns-3 in cmake.

.. code-block:: shell

   ctest -R ns3

Troubleshooting
===============

If you use a version of ns-3 that is not known to SimGrid yet, edit
``tools/cmake/Modules/FindNS3.cmake`` in your SimGrid tree, according to the
comments on top of this file. Conversely, if something goes wrong with an old
version of either SimGrid or ns-3, try upgrading everything.

.. _ns3_use:

Using ns-3 from SimGrid
***********************

Platform files compatibility
============================

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

.. code-block:: shell

   ./network-ns3 --cfg=network/model:ns-3 (other parameters)

Many other files from the ``examples/platform`` directory are usable with the
ns-3 model, such as `examples/platforms/dogbone.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/dogbone.xml>`_.
Check the file  `examples/s4u/network-ns3/network-ns3.tesh <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/network-ns3/network-ns3.tesh>`_
to see which ones are used in our regression tests.

WiFi platforms
--------------

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
^^^^^^^^^^^^^^^^^^^^^^^^


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
=======================

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
===========

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
===============

If your simulation hangs in a communication, this is probably because one host
is sending data that is not routable in your platform. Make sure that you only
use routes of length 1, and that any host is connected to the platform.
Arguably, SimGrid could detect this situation and report it, but unfortunately,
this is still to be done.

.. |br| raw:: html

   <br />
