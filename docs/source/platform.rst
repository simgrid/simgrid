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

Describing your Simulated Platform
##################################

In SimGrid, platforms are usually described in XML. This formalism has
some drawbacks, but using a specific format ensures that the platform
is not mixed with the tested application. This separation of concern
:ref:`is a must <howto_science>` for your Modeling and Simulation (M&S)
work. When XML is too limiting, you may describe your platforms using
the :ref:`lua bindings <platform_lua>` (it is not yet possible to do so in
python or directly in C++).

We understand that writing a complex platform description can be tricky, we thus included 
:ref:`many examples <platform_examples>` in the archive. This
documentation also contains some :ref:`hints and howtos <howto>`, as well
as the full :ref:`XML reference guide <platform_reference>`.


Any simulated platform must contain **basic elements**, such as hosts, links, disks, etc. SimGrid gives you a great
liberty when defining the **routing of your platform**, i.e., the network path taken between each pair of hosts.
Finally, you may also describe an **experimental scenario**, with qualitative (e.g., bandwidth variations representing
an external load) and qualitative (e.g., representing how some elements fail and restart over time) changes.


First Example
*************

Imagine you want to describe a little platform with three hosts,
interconnected as follows:

.. image:: /tuto_smpi/3hosts.png
   :align: center

This can be done with the following platform file, that considers the
simulated platform as a graph of hosts and network links.
	   
.. literalinclude:: /tuto_smpi/3hosts.xml
   :language: xml

The most important elements are the basic ones: :ref:`pf_tag_host`,
:ref:`pf_tag_link`, and similar. Then come the routes between any pair
of hosts, that are given explicitly with :ref:`pf_tag_route` (routes
are symmetrical by default). Any host must be given a computational
speed (in flops) while links must be given a latency (in seconds) and
a bandwidth (in bytes per second). Note that you can write 1Gflops
instead of 1000000000flops, and similar.

Every platform element must be located within a given **networking
zone** .  Zones are in
charge of the routing, see below.

The last thing you must know on SimGrid platform files is that the
root tag must be :ref:`pf_tag_platform`. If the ``version`` attribute
does not match what SimGrid expects, you will be hinted to use to the
``simgrid_update_xml`` utility to update your file.


Defining a Routing
******************

Networking zones (:ref:`pf_tag_zone`) are used to factorize the description
to reduce the size of your platform on disk and in memory. Then, when
a host wants to communicate with another host belonging to the same
zone, it is the zone's duty to find the list of links that are
involved in the communication. In the above examples, since we use
``routing="Full"``, all routes must be explicitly given using the
:ref:`pf_tag_route` and :ref:`pf_tag_link_ctn` tags (this :ref:`routing
model <pf_rm>` is both simple and inefficient :) It is OK to not
specify each and every route between hosts, as long as you do not try
to start a communication on any of the missing routes during your
simulation.

Any zone may contain sub-zones, allowing for a hierarchical
decomposition of the platform. Routing can be made more efficient (as the
inter-zone routing gets factored with :ref:`pf_tag_zoneroute`), and
allows you to have more than one routing model in your platform. For
example, you can have a coordinate-based routing for the WAN parts
of your platforms, a full routing within each datacenter, and a highly
optimized routing within each cluster of the datacenter.  In this
case, determining the route between two given hosts gets :ref:`routing_basics`
"somewhat more complex" but SimGrid still computes
these routes for you in a time- and space-efficient manner.
Here is an illustration of these concepts:

.. image:: img/zone_hierarchy.png

Circles represent processing units and squares represent network
routers. Bold lines represent communication links. The zone "AS2" models the core of a national network interconnecting a
small flat cluster (AS4) and a larger hierarchical cluster (AS5), a
subset of a LAN (AS6), and a set of peers scattered around the world
(AS7).

.. todo:: Add more examples, such as the cloud example described in
          previous paragraph

Performance Profiles and Churn
******************************

..  LocalWords:  SimGrid
