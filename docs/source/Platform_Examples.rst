.. raw:: html

   <object id="TOC" data="graphical-toc.svg" width="100%" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("ExamplesBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

.. _platform_examples:
   
Platform Examples
=================

Here is a very simple platform file, containing 3 resources (two hosts
and one link), and explicitly giving the route between the hosts.

.. literalinclude:: ../../examples/platforms/two_hosts.xml
   :language: xml   

The root tag must be ``<platform>``, and its ``version`` attribute
specifies the used DTD version. When the DTD evolutions introduce
backward-incompatible changes, this number gets updated. Use the
``simgrid_update_xml`` utility to upgrade your platform files on need.


Then, every platform element must be located within a given
**networking zone** introduced with the :ref:`pf_tag_zone` tag.  Zones
are in charge of the routing: an host wants to communicate with
another host of the same zone, it is the zone's duty to find the list
of links that are involved in the communication. Here, since we use
``routing="Full"``, all routes must be explicitly given using the
:ref:`pf_tag_route` and :ref:`pf_tag_linkctn` tags (this :ref:`routing
model <pf_rm>` is both simple and inefficient :) It is OK to not
specify each and every route between hosts, as long as you don't start
at runtime any communications on the missing routes.

Any zone may contain sub-zones itself, leading to a hierarchical
decomposition of the platform. This can be more efficient (as the
inter-zone routing gets factorized with :ref:`pf_tag_zoneroute`), and
allows to have more than one routing model in your platform. For
example, you could have a coordinate-based routing for the WAN parts
of your platforms, a full routing within each datacenter, and a highly
optimized routing within each cluster of the datacenter.  In this
case, determining the route between two given hosts gets @ref
routing_basics "somewhat more complex" but SimGrid still computes
these routes for you in a time- and space-efficient manner.
Here is an illustration of these concepts:

.. image:: img/zone_hierarchy.png

The zone "AS2" models the core of a national network interconnecting a
small flat cluster (AS4) and a larger hierarchical cluster (AS5), a
subset of a LAN (AS6), and a set of peers scattered around the world
(AS7).

.. todo:: Add more examples, such as the cloud example descibed in
	  previous paragraph
   
