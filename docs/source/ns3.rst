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
install ns-3 from scratch (see the `ns-3 documentation <http://www.nsnam.org>`_).

Enabling ns-3 in SimGrid
========================

SimGrid must be recompiled with the ``enable_ns3`` option activated in cmake.
Optionally, use ``NS3_HINT`` to hint cmake about the path on disk
where ns-3 is installed.

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
systems: ns-3 wireless models are not available, while SimGrid routes
cannot be longer than 1. Also, the platform built in ns-3 from the
SimGrid description is very basic.

Platform files compatibility
============================

Any route longer than one will be ignored when using ns-3. They are
harmless, but you still need to connect your hosts using one-hop routes.
The best solution is to add routers to split your route. Here is an
example of an invalid platform:

.. code-block:: shell

   <platform>
     <zone id="zone0" routing="Full">
       <host id="alice" speed="1Gf" />
       <host id="bob"   speed="1Gf" />
  
       <link id="l1" bw="1Mbps" />
       <link id="l2" bw="1Mbps" />

       <route src="alice" dst="bob">
         <link_ctn id="l1"/>            <!-- !!!! INVALID WITH ns-3    !!!! -->
         <link_ctn id="l2"/>            <!-- !!!! length=2 IS TOO MUCH !!!! -->
       </route>
     </zone>
   </platform>
  
This can be reformulated as follows to make it usable with the ns-3 binding.
There is no direct connection from alice to bob, but that's OK because
ns-3 automatically routes from point to point.

.. code-block:: shell

   <platform>
     <zone id="zone0" routing="Full">
       <host id="alice" speed="1Gf" />
       <host id="bob"   speed="1Gf" />

       <router id="r1" /> <!-- routers are compute-less hosts -->

       <link id="l1" bw="1Mbps" />
       <link id="l2" bw="1Mbps" />

       <route src="alice" dst="r1">
         <link_ctn id="l1"/> 
       </route>
  
       <route src="r1" dst="bob">
         <link_ctn id="l2"/> 
       </route>
     </zone>
   </platform>

Once your platform is OK, just change the :ref:`network/model
<options_model_select>` configuration option to "ns-3" as follows. The rest
is unchanged.

.. code-block:: shell

   ./network-ns3 --cfg=network/model:ns-3 (other parameters)

Many other files from the ``examples/platform directory`` are usable with the
ns-3 model, such as `examples/platforms/dogbone.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/dogbone.xml>`_.
Check the file  `examples/s4u/network-ns3/network-ns3.tesh <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/network-ns3/network-ns3.tesh>`_
to see which ones are used in our regression tests.

Limitations
===========

A ns-3 platform is automatically created from the provided SimGrid
platform. However, there are some known caveats:

  * The default values (e.g., TCP parameters) are the ns-3 default values.
  * ns-3 networks are routed using the shortest path algorithm, using
    ``ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables``.
  * End hosts cannot have more than one interface card. So, your
    SimGrid hosts should be connected to the platform through only
    one link. Otherwise, your SimGrid host will be considered as a
    router.

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
