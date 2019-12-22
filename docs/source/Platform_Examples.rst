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

.. _platform_examples:
   
Platform Examples
=================

Simple Example with 3 hosts
---------------------------

Imagine you want to describe a little platform with three hosts,
interconnected as follows:

.. image:: /tuto_smpi/3hosts.png
   :align: center

This can be done with the following platform file, that considers the
simulated platform as a graph of hosts and network links.

.. literalinclude:: /tuto_smpi/3hosts.xml
   :language: xml

The elements basic elements (with :ref:`pf_tag_host` and
:ref:`pf_tag_link`) are described first, and then the routes between
any pair of hosts are explicitly given with :ref:`pf_tag_route`. 

Any host must be given a computational speed in flops while links must
be given a latency and a bandwidth. You can write 1Gf for
1,000,000,000 flops (full list of units in the reference guide of 
:ref:`pf_tag_host` and :ref:`pf_tag_link`). 

Routes defined with :ref:`pf_tag_route` are symmetrical by default,
meaning that the list of traversed links from A to B is the same as
from B to A. Explicitly define non-symmetrical routes if you prefer.

Cluster with a Crossbar
-----------------------

A very common parallel computing platform is a homogeneous cluster in
which hosts are interconnected via a crossbar switch with as many
ports as hosts, so that any disjoint pairs of hosts can communicate
concurrently at full speed. For instance:

.. literalinclude:: ../../examples/platforms/cluster_crossbar.xml
   :language: xml
   :lines: 1-3,18-

One specifies a name prefix and suffix for each host, and then give an
integer range. In the example the cluster contains 65535 hosts (!),
named ``node-0.simgrid.org`` to ``node-65534.simgrid.org``. All hosts
have the same power (1 Gflop/sec) and are connected to the switch via
links with same bandwidth (125 MBytes/sec) and latency (50
microseconds).

.. todo::

   Add the picture.

Cluster with a Shared Backbone
------------------------------

Another popular model for a parallel platform is that of a set of
homogeneous hosts connected to a shared communication medium, a
backbone, with some finite bandwidth capacity and on which
communicating host pairs can experience contention. For instance:


.. literalinclude:: ../../examples/platforms/cluster_backbone.xml
   :language: xml
   :lines: 1-3,18-

The only differences with the crossbar cluster above are the ``bb_bw``
and ``bb_lat`` attributes that specify the backbone characteristics
(here, a 500 microseconds latency and a 2.25 GByte/sec
bandwidth). This link is used for every communication within the
cluster. The route from ``node-0.simgrid.org`` to ``node-1.simgrid.org``
counts 3 links: the private link of ``node-0.simgrid.org``, the backbone
and the private link of ``node-1.simgrid.org``.

.. todo::

   Add the picture.

Torus Cluster
-------------

Many HPC facilities use torus clusters to reduce sharing and
performance loss on concurrent internal communications. Modeling this
in SimGrid is very easy. Simply add a ``topology="TORUS"`` attribute
to your cluster. Configure it with the ``topo_parameters="X,Y,Z"``
attribute, where ``X``, ``Y`` and ``Z`` are the dimension of your
torus.

.. image:: ../../examples/platforms/cluster_torus.svg
   :align: center

.. literalinclude:: ../../examples/platforms/cluster_torus.xml
   :language: xml

Note that in this example, we used ``loopback_bw`` and
``loopback_lat`` to specify the characteristics of the loopback link
of each node (i.e., the link allowing each node to communicate with
itself). We could have done so in previous example too. When no
loopback is given, the communication from a node to itself is handled
as if it were two distinct nodes: it goes twice through the private
link and through the backbone (if any).

Fat-Tree Cluster
----------------

This topology was introduced to reduce the amount of links in the
cluster (and thus reduce its price) while maintaining a high bisection
bandwidth and a relatively low diameter. To model this in SimGrid,
pass a ``topology="FAT_TREE"`` attribute to your cluster. The
``topo_parameters=#levels;#downlinks;#uplinks;link count`` follows the
semantic introduced in the `Figure 1B of this article
<http://webee.eedev.technion.ac.il/wp-content/uploads/2014/08/publication_574.pdf>`_.

Here is the meaning of this example: ``2 ; 4,4 ; 1,2 ; 1,2``

- That's a two-level cluster (thus the initial ``2``).
- Routers are connected to 4 elements below them, regardless of its
  level. Thus the ``4,4`` component that is used as
  ``#downlinks``. This means that the hosts are grouped by 4 on a
  given router, and that there is 4 level-1 routers (in the middle of
  the figure).
- Hosts are connected to only 1 router above them, while these routers
  are connected to 2 routers above them (thus the ``1,2`` used as
  ``#uplink``).
- Hosts have only one link to their router while every path between a
  level-1 routers and level-2 routers use 2 parallel links. Thus the
  ``1,2`` that is used as ``link count``.

.. image:: ../../examples/platforms/cluster_fat_tree.svg
   :align: center

.. literalinclude:: ../../examples/platforms/cluster_fat_tree.xml
   :language: xml
   :lines: 1-3,10-


Dragonfly Cluster
-----------------

This topology was introduced to further reduce the amount of links
while maintaining a high bandwidth for local communications. To model
this in SimGrid, pass a ``topology="DRAGONFLY"`` attribute to your
cluster. It's based on the implementation of the topology used on
Cray XC systems, described in paper
`Cray Cascade: A scalable HPC system based on a Dragonfly network <https://dl.acm.org/citation.cfm?id=2389136>`_.

System description follows the format ``topo_parameters=#groups;#chassis;#routers;#nodes``
For example, ``3,4 ; 3,2 ; 3,1 ; 2``:

- ``3,4``: There are 3 groups with 4 links between each (blue level).
  Links to nth group are attached to the nth router of the group
  on our implementation.
- ``3,2``: In each group, there are 3 chassis with 2 links between each nth router
  of each group (black level)
- ``3,1``: In each chassis, 3 routers are connected together with a single link
  (green level)
- ``2``: Each router has two nodes attached (single link)

.. image:: ../../examples/platforms/cluster_dragonfly.svg
   :align: center

.. literalinclude:: ../../examples/platforms/cluster_dragonfly.xml
   :language: xml


.. todo:: Complete this page of the manual.

   SimGrid comes with an extensive set of platforms in the
   `examples/platforms <https://framagit.org/simgrid/simgrid/tree/master/examples/platforms>`_
   directory that should be described here.
   
