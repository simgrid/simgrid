.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("RoutingBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

.. _platform_routing:

Defining a Routing
##################

Networking zones (:ref:`pf_tag_zone`) are an advanced concept used to factorize the description
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

