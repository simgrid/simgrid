.. _model_ns3:

ns-3 as a SimGrid model
#######################

You can use the well-known `ns-3 <http://www.nsnam.org>`_ packet-level network
simulator as a SimGrid model, for example to investigate the validity of your
simulation. Just install ns-3 and recompile SimGrid accordingly.

Installing ns-3
***************

The easiest is to install it with the package manager. Under Debian/Ubuntu,
simply type as root:

.. code-block:: shell

  apt-get install libns3-dev ns3

You can also install it from scratch with the following commands:

.. code-block:: shell

  # Download the source
  wget http://www.nsnam.org/release/ns-allinone-3.26.tar.bz2
  tar -xf ns-allinone-3.26.tar.bz2
  cd ns-allinone-3.26/ns-3.26/
  # Configure, build and install
  ./waf configure --prefix="/opt/ns3" # or give another path if you prefer
  ./waf
  ./waf install

For more information, please refer to the ns-3 documentation
(`official website <http://www.nsnam.org>`_).

Enabling SimGrid's support for ns-3
***********************************

Normally, you just have to enable ns-3 in ccmake or cmake as
follows. If you installed ns-3 in a regular path, just drop the
NS3_HINT configuration item.

.. code-block:: shell

   cmake . -Denable_ns3=ON -DNS3_HINT=/opt/ns3 # or change the path if needed

By the end of the configuration, cmake reports whether ns-3 was found,
and this information is also available in ``include/simgrid/config.h``
If your local copy defines the variable ``SIMGRID_HAVE_NS3`` to 1, then ns-3
was correctly detected. If it's defined to 0, then something went
wrong. Explore ``CMakeFiles/CMakeOutput.log`` and
``CMakeFiles/CMakeError.log`` to diagnose the problem.

Afterward, you can test your installation as follows:

.. code-block:: shell

   ctest -R ns3

.. _ns3_use:

Using ns-3 from SimGrid
***********************

The SimGrid-ns3 binding only contains features that are common to both
systems: ns-3 wireless models are not available, while SimGrid routes
cannot be longer than 1. Also, the platform built in ns-3 from the
SimGrid description is very basic.

Any route longer than one will be ignored when using ns-3. They are
harmless, but you still need to connect your hosts using one-hop routes.
The best solution is to add routers to split your route. Here is an
example of invalid platform:

.. code-block:: shell

   <platform>
     <host id="alice" speed="1Gf" />
     <host id="bob"   speed="1Gf" />
  
     <link id="l1" bw="1Mbps" />
     <link id="l2" bw="1Mbps" />

     <route src="alice" dst="bob">
       <link_ctn id="l1"/> <!-- INVALID WITH NS-3 -->
       <link_ctn id="l2"/> <!-- length=2 IS TOO MUCH -->
     </route>
   </platform>
  
Here is the same platform expressed in a way that ns-3 will understand.
There is no direct connexion from alice to bob, but that's OK because
ns-3 will find the path from point to point.

.. code-block:: shell

   <platform>
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
   </platform>

Once your platform is OK, just change the :ref:`network/model
<options_model_select>` configuration option to "NS3" as follows. The rest
is unchanged.

.. code-block:: shell

   ./network-ns3 --cfg=network/model:NS3 (other parameters)

Many other files from the ``examples/platform directory`` are usable with the
ns-3 model, such as `examples/platforms/dogbone.xml <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms/dogbone.xml>`_.
Check the file  `examples/deprecated/msg/network-ns3/network-ns3.tesh <https://framagit.org/simgrid/simgrid/tree/master/examples/deprecated/msg/network-ns3/network-ns3.tesh>`_
to see which ones are used in our regression tests.

Shortcomings of the ns-3 bindings in SimGrid
--------------------------------------------

A ns-3 platform is automatically created from the provided SimGrid
platform. However, there are some known caveats:

  * The default values (e.g., TCP parameters) are the ns3 default values.
  * ns-3 networks are routed using the shortest path algorithm, using
    ``ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables``.
  * End hosts cannot have more than one interface card. So, your
    SimGrid hosts should be connected to the platform through only
    one link. Otherwise, your SimGrid host will be considered as a
    router.

Our goal is to keep the ns-3 plugin of SimGrid as easy (and hopefully
readable) as possible. If the current state does not fit your needs,
you should modify this plugin, and/or create your own plugin from the
existing one.

Troubleshooting with ns-3 and SimGrid
*************************************

I fail to compile ns-3 within SimGrid
-------------------------------------

If you have a ns-3 version that is not known to SimGrid yet, edit 
``tools/cmake/Modules/FindNS3.cmake`` in your SimGrid tree, according to
the comments on top of this file.

If the compilation fails on Debian/Ubuntu when linking the library
because of some .a file that cannot be used dynamically, then you are
probably using a very old (and buggy) ``libns3-dev``
package. Update it, or install ``libns3-3`` manually.

The simulation hangs at some point
----------------------------------

If your simulation hangs in a communication, this is probably because
one host is sending data that is not routable in your platform. Make
sure that you only use routes of length 1, and that any host is
connected to the platform.

Arguably, SimGrid could detect this situation and report it, but
unfortunately, this is still to be done.

I get a warning that some routes are ignored
--------------------------------------------

Any routes longer than one hop are ignored in ns-3. Please refer to
:ref:`ns3_use` for details.
