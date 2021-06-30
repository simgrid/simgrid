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
human-readable, quick way to start your experiments. Although, XML format brings
several drawbacks as your platforms get larger and more complex (see :ref:`platform_cpp_beyond`).

In this case, it may be more interesting to write your platform directly
in C++ code. It allows you to programmatically describe your platform and
remove the intermediate XML parser during simulations. Take care to follow
the recommendations in :ref:`Modeling Hints <howto>` to keep a clear separation
of concerns between your platform and your application.

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

For more details about how to describe the platforms, please give a look at :ref:`examples<platform_cpp_example>`
or directly at the S4U API.

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

Using the C++ interface, you can use the specific function to create these 2 links. Note
that you need to define the direction in the add_route function when adding a route containing
a split-duplex link. Otherwise, SimGrid cannot know which link (UP/DOWN) to use.

.. code-block:: cpp

    auto* link = zone->create_split_duplex_link("1", "125MBps")->set_latency("24us")->seal();
    
    zone->add_route(S1, C1, nullptr, nullptr, {{link, LinkInRoute::Direction::UP}});

.. note::
    Do not use set_sharing_policy(SharingPolicy::SPLITDUPLEX).
    SimGrid will complain since set_sharing_policy should be used only with (SHARED and FATPIPE)


Loading the platform
====================

The C++ interface to build the platforms give you freedom to organize your code
as you wish, separating (or unifying) your application from your platform code.
However, we provide a small hack if you want to keep the same structure of the
old code with XML platforms. You can pass a library (.so) file to ``Engine::load_platform``
function, having a predefined function implemented. When loading the platform, the
Engine will look for a function with this signature: "**void load_platform(const sg4::Engine& e)**", and
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


.. _platform_cpp_beyond:

Beyond the XML: the power of C++ platforms
******************************************

This section describes one of the advantages of using C++ code to write your platforms.

Let's see an example of the description of a Fat-Tree in XML (:ref:`platform_examples_fattree`)

.. literalinclude:: ../../examples/platforms/cluster_fat_tree.xml
   :language: xml
   :lines: 1-3,10-

Our cluster *bob* is composed of 16 hosts with the same 1Gf CPU power.

Imagine now that you want to simulate the same **Fat-Tree topology with** more complex **hosts**,
composed of **1 CPU, 1 GPU and some interconnecting bus**.

Unfortunately, this is not possible with the XML description since its syntax obliges
that the leaves in your Fat-Tree to be single Hosts. However, with the C++ API, your
leaves can be composed of other zones, creating a **Fat-Tree of FullZones** for example.

Consequently, you can describe the desired platform as follows:

.. code-block:: c++

    sg4::Engine e(&argc, argv);
    sg4::create_fatTree_zone("bob", e.get_netzone_root(), {2, {4, 4}, {1, 2}, {1, 2}}, {create_hostzone, create_loopback, {}}, 125e6,
                           50e-6, sg4::Link::SharingPolicy::SPLITDUPLEX)->seal();

Note that the leaves and loopback links are defined through callbacks, as follows:

.. code-block:: c++

    /* create the loopback link for each leaf in the Fat-Tree */
    static sg4::Link* create_loopback(sg4::NetZone* zone, const std::vector<unsigned int>& /*coord*/, int id)
    {
        // note that you could set different loopback links for each leaf
        return zone->create_link("limiter-" + std::to_string(id), 1e6)->seal();
    }

    /* create each leaf in the Fat-Tree, return a pair composed of: <object (host, zone), gateway> */
    static std::pair<simgrid::kernel::routing::NetPoint*, simgrid::kernel::routing::NetPoint*>
    create_hostzone(const sg4::NetZone* zone, const std::vector<unsigned int>& /*coord*/, int id)
    {
      /* creating zone */
      std::string hostname = "host" + std::to_string(id);
      auto* host_zone = sg4::create_full_zone(hostname);
      /* setting my parent zone */
      host_zone->set_parent(zone);

      /* creating CPU */
      std::string cpu_name  = hostname + "-cpu" + std::to_string(i);
      const sg4::Host* cpu = host_zone->create_host(cpu_name, 1e9)->seal();
      /* creating GPU */
      std::string gpu_name  = hostname + "-gpu" + std::to_string(i);
      const sg4::Host* gpu = host_zone->create_host(gpu_name, 1e12)->seal();
      /* connecting them */
      sg4::Link* link   = host_zone->create_link("link-" + cpu_name, 10e9)->set_latency(10e-9)->seal();
      host_zone->add_route(cpu->get_netpoint(), gpu->get_netpoint(), nullptr, nullptr, std::vector<sg4::Link*>{link});

      host_zone->seal();
      /* cpu is the gateway for this host */
      return std::make_pair(host_zone->get_netpoint(), cpu->get_netpoint());
    }

The code is straightforward and can be easily adapted to more complex environments thanks to the flexibility
provided by the C++ API.

