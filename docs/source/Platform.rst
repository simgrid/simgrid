.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("PlatformBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

.. _platform:

Describing your Simulated Platform
##################################

In SimGrid, platforms are usually described in XML. This formalism has
some drawbacks, but using a specific format ensures that the platform
is not mixed with the tested application. This separation of concern
:ref:`is a must <howto_science>` for your Modeling and Simulation (M&S)
work. When XML is too limiting, you may describe your platforms using
directly :ref:`C++ code <platform_cpp>`.

Any simulated platform must contain **basic elements**, such as
:ref:`pf_tag_host`, :ref:`pf_tag_link`, :ref:`pf_tag_disk`, and similar.
SimGrid makes no assumption about the **routing of your platform**, so you must declare
explicitly the network path taken between each pair of hosts.
This can be done through a flat list of :ref:`pf_tag_route` for each pair of hosts (routes
are symmetrical by default), or you may use the advanced concept of :ref:`networking zone <platform_routing>`
to efficiently express the routing of your platform.
Finally, you may also describe an **experimental scenario**, with qualitative (e.g., bandwidth variations representing
an external load) and qualitative (e.g., representing how some elements fail and restart over time) changes.

Here is a minimalistic platform example, describing a zone which routing is fully described, containing two hosts and a
link. You need to explicitly add a :ref:`pf_tag_route` between ``host0`` and ``host1`` for this link to get used during
communications.

.. code-block:: xml

  <zone id="AS5-4" routing="Full">
    <host id="host0" speed="1Gf"/>
    <host id="host1" speed="2Gf"/>
    <link id="link0" bandwidth="125MBps" latency="100us"/>
    <route src="host0" dst="host1"><link_ctn id="link0"/></route>
  </zone>

SimGrid only perform minimalistic verifications about the described platforms, to make things flexible and interesting.
It enables weird topologies, such as a single link used for all communications in a large platform, or a single used by
hosts from different zones, or even worse. It is also OK to not describe some routing paths. SimGrid won't complain
unless your application tries to actually use inexistant paths. In short, it is your responsibility to write proper
platform files, and SimGrid will not try to be smarter than you!

To learn further about platform descriptions, the easiest is to look at the
:ref:`many examples <platform_examples>` included in the archive and described
in the next section. This documentation also contains some :ref:`hints and
howtos <howto>`, as well as the full :ref:`XML reference guide
<platform_reference>`.

..  LocalWords:  SimGrid
