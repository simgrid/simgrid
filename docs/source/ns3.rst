.. _model_ns3:

ns-3 as a SimGrid model
#######################

You can use the well-known `ns-3 packet-level network simulator
<http://www.nsnam.org>`_ as a SimGrid model, for example to investigate the
validity of your simulation. Just install ns-3 and recompile SimGrid
accordingly.

Compiling the ns-3/SimGrid binding
**********************************

Installing ns-3
===============

SimGrid requires ns-3 version 3.26 or higher, and you probably want the most
recent version of both SimGrid and ns-3. While the Debian package of SimGrid
don't have the ns-3 bindings activated, you can still use the packaged version
of ns-3 by grabbing the ``libns3-dev ns3`` packages. Alternatively, you can
install ns-3 from scratch as follows:

.. code-block:: shell

  # Download the source
  wget http://www.nsnam.org/release/ns-allinone-3.29.tar.bz2
  tar -xf ns-allinone-3.29.tar.bz2
  cd ns-allinone-3.29/ns-3.29/
  # Configure, build and install
  ./waf configure --prefix="/opt/ns3" # or give another path if you prefer
  ./waf
  ./waf install

For more information, please refer to the ns-3 documentation
(`official website <http://www.nsnam.org>`_).

Enabling ns-3 in SimGrid
========================

SimGrid must be recompiled with the ``enable_ns3`` option activated in cmake.
Optionally, use ``NS3_HINT`` to hint cmake about where to find ns-3.

.. code-block:: shell

   cmake . -Denable_ns3=ON -DNS3_HINT=/opt/ns3 # or change the path if needed

By the end of the configuration, cmake reports whether ns-3 was found,
and this information is also available in ``include/simgrid/config.h``
If your local copy defines the variable ``SIMGRID_HAVE_NS3`` to 1, then ns-3
was correctly detected. Otherwise, explore ``CMakeFiles/CMakeOutput.log`` and
``CMakeFiles/CMakeError.log`` to diagnose the problem.

Test your installation after compilation as follows:

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

The SimGrid/ns-3 binding only contains features that are common to both
systems. Also, the platform built in ns-3 from the
SimGrid description is very basic.

Platform files compatibility
============================

Any route longer than one will be ignored when using ns-3. They are
harmless, but you still need to connect your hosts using one-hop routes.
The best solution is to add routers to split your route. Here is an
example of invalid platform:

.. code-block:: shell

	<?xml version='1.0'?><!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd">
	<platform version="4.1">
		<AS id="AS0" routing="Floyd">

			<host id="alice" speed="1Gf" />
			<host id="bob"   speed="1Gf" />
	
			<link id="l1" bandwidth="1Mbps" latency="5ms" />
			<link id="l2" bandwidth="1Mbps" latency="5ms" />
	
			<route src="alice" dst="bob">
				<link_ctn id="l1"/>            <!-- !!!! INVALID WITH ns-3    !!!! -->
				<link_ctn id="l2"/>            <!-- !!!! length=2 IS TOO MUCH !!!! -->
			</route>
		</AS>
	</platform>

This can be reformulated as follows to make it usable with the ns-3 binding.
There is no direct connection from alice to bob, but that's OK because
ns-3 automatically routes from point to point.

.. code-block:: shell

	<?xml version='1.0'?><!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd">
	<platform version="4.1">
		<AS id="AS0" routing="Floyd">

			<host id="alice" speed="1Gf"/>
			<host id="bob"   speed="1Gf"/>

			<router id="r1"/>	<!-- routers are compute-less hosts -->

			<link id="l1" bandwidth="1Mbps" latency="5ms"/>
			<link id="l2" bandwidth="1Mbps" latency="5ms"/>

			<route src="alice" dst="r1">
				<link_ctn id="l1"/> 
			</route>

			<route src="r1" dst="bob">
				<link_ctn id="l2"/> 
			</route>
		</AS>
	</platform>

Once your platform is OK, just change the :ref:`network/model
<options_model_select>`_ configuration option to "ns-3" as follows. The rest
is unchanged.

.. code-block:: shell

   ./network-ns3 --cfg=network/model:ns-3 (other parameters)

Many other files from the ``examples/platform directory`` are usable with the
ns-3 model.

Build a wifi-compatible platform
===================================

We describe here a simple platform allowing ns3 wifi communication
between two simgrid hosts.

First, here are the mandatory information necessary to create a
simgrid platform:

.. code-block:: shell

	<?xml version='1.0'?><!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd">
	<platform version="4.1">
		<AS id="AS0" routing="Floyd">

Then, we create our access point and station hosts:

.. code-block:: shell

			<host id="alice" speed="1Gf"/>
			<host id="bob"   speed="1Gf"/>

We must specify that alice will be our access point. To do that we
simply add the property ``wifi_link`` to the host ``alice``: 

.. code-block:: shell

			<host id="alice" speed="1Gf">
				<prop id="wifi_link" value="net0"/>
			</host>

			<host id="bob"   speed="1Gf"/>

The value ``net0`` of this property defines the name of the wifi network
generated. To generate this wifi network we create a wifi link:

.. code-block:: shell``

			<link id="net0" bandwidth="0" latency="0" sharing_policy="WIFI"/>

The important information here are:
	* The id of the link, ``net0``, must match the network name defined by the property ``wifi_link`` of the access point node
	* The sharing policy must be set to ``WIFI``

Note: bandwidth and latency are mandatory by simgrid to create a link but are NOT used to create a wifi network. Instead the
wifi network capabilities are defined by its MCS, NSS and distance from access point to station. Those properties are described in section :ref:`Optional access point node properties <optional_prop>`_

To connect the station node to the access point node, we
create a route between the hosts:

.. code-block:: shell

			<route src="alice" dst="bob">
				<link_ctn id="net0" />
			</route>

Finally, we end the xml file with the missing closing tags:

.. code-block:: shell

		</AS>
	</platform>

.. _optional_prop:

Optional access point node properties
--------------------------------------

The MCS (`Modulation and Coding Scheme <https://en.wikipedia.org/wiki/Link_adaptation>`_) can be set with the property ``wifi_mcs``:

.. code-block:: shell

			 <host id="alice" speed="1Gf">
				<prop id="wifi_link" value="net0"/>
				<prop id="wifi_mcs" value="5"/>
			</host>

Its default value is 3.

The NSS (Number of Spatial Streams, also known as the `number of antennas <https://en.wikipedia.org/wiki/IEEE_802.11n-2009#Number_of_antennas>`_) can be set with the property ``wifi_nss``:

.. code-block:: shell

			<host id="alice" speed="1Gf">
				<prop id="wifi_link" value="net0"/>
				<prop id="wifi_nss" value="2"/>
			</host>
			
Its default value is 1.

Note: not all value of MCS and NSS are valid nor compatible. Check `802.11n standard <https://en.wikipedia.org/wiki/IEEE_802.11n-2009#Data_rates>`_ for more information.

Optional station node properties
---------------------------------

The distance in meter at which the station is placed from the access point can
be set with the property ``wifi_distance``.

.. code-block:: shell

			<host id="alice" speed="100.0Mf,50.0Mf,20.0Mf" pstate="0">
				<prop id="wifi_distance" value="30" />
			</host>

Its default value is 10. 

Limitations
===========

A ns-3 platform is automatically created from the provided SimGrid
platform. However, there are some known caveats:

	* The default values (e.g., TCP parameters) are the ns-3 default values.
	* ns-3 networks are routed using the shortest path algorithm, using
		``ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables``.

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
