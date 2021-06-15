.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("ExamplesBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

.. _platform_cpp:
   
C++ Platforms
#############

Using XML to describe the platforms is very convenient. It provides a
human-readable, quick way to start your experiments. Although, as the
platform grows in size, XML format brings some drawbacks which may
make your life harder, specially for big and regular platforms such as
homogeneous clusters.

In this case, it may be more interesting to write your platform directly
in C++ code. It allows you to programmatically describe your platform and
remove the intermediate XML parsing during simulations. Take care to follows
the recommendations in :ref:`Modeling Hints <howto>` to keep a clear separation
of concerns between your platforms and your applications.

Describing Resources
********************

A platform in SimGrid is composed of several resources organized in different
Netzones. The different resources, such as hosts, disks and links, follow the same
idiom: create()->set()->set()->seal().

.. code-block:: c++

    NetZone* zone      = s4u::create_star_zone("zone0");
    Link* l_up   = zone->create_link("link_up", "125MBps")->set_latency("24us")->seal();
    Host* host   = zone->create_host("host0", "1Gf")->seal();
    zone->seal();

The first NetZone created will be the root zone of your platform. You're allowed to modified
an object as long as you did not seal it.

For more details about how to describe the platforms, please give a look at the :ref:`examples<platform_cpp_example>`
or directly the S4U API.

Links
=====

In the XML, you are allowed to do the following description:

.. code-block:: xml

    <link id="1" bandwidth="10kBps" latency="10ms" sharing_policy="SPLITDUPLEX"/>

    <route src="S1" dst="C1" symmetrical="NO">
      <link_ctn id="1" direction="DOWN"/>
    </route>

It is important to notice that this is a syntactic sugar provided by the XML to ease
the link utilization. A split-duplex link means that upgoing communications do not
share the bandwidth with downgoing communications. To emulate this behavior,
under the hood, SimGrid creates 2 links in this case: the *1_UP*
link and the *1_DOWN* link. As you can see, the selection of link to use
in the <route> tag is done by the ``direction=`` parameter.

Using the C++ interface, you should describe both links separately and use them
in the route description.

.. code-block:: cpp

    Link* l_up   = zone->create_link("1_UP", "125MBps")->set_latency("24us")->seal();
    Link* l_down = zone->create_link("1_DOWN", "125MBps")->set_latency("24us")->seal();
    
    zone->add_route(S1, C1, nullptr, nullptr, {link_down});


Loading the platform
====================

The C++ interface to build the platforms give you freedom to organize your code
as you wish, separating (or unifying) your application from your platform code.
However, we provide a small hack if you want to keep the same structure of the
old code with XML platforms. You can pass a library (.so) file to ``Engine::load_platform``
function, having a predefined function implemented. When loading the platform, the
Engine will look for a function called "**void load_platform(const sg4::Engine& e)**" and
execute it. It could be an easy way to make the transition between XML and C++ if necessary.

For more details, please refer to the cpp and CMakeLists.txt files in 
`examples/platform <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms>`_.


.. _platform_cpp_example:

Example
*******

The best way to build your C++ platform is starting from some examples.
Give a look in the examples folder in `examples/ <https://framagit.org/simgrid/simgrid/tree/master/examples/>`_.
For instance, the file `examples/cpp/clusters-multicpu/s4u-clusters-multicpu.cpp <https://framagit.org/simgrid/simgrid/-/blob/master/examples/cpp/clusters-multicpu/s4u-clusters-multicpu.cpp>`_ shows how to build complex platforms composed of
clusters of clusters.

Here, we present a complete example showing how to create 3 regulars clusters
connected through a shared link.

.. literalinclude:: ../../examples/platforms/griffon.cpp
   :language: cpp
