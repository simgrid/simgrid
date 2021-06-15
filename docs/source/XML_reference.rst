.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("ReferenceBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

.. _platform_reference:

Complete XML Reference
**********************

Your platform description should follow the specification presented in the
`simgrid.dtd <https://simgrid.org/simgrid.dtd>`_ DTD file. The same DTD is used for both platform and deployment files.

-------------------------------------------------------------------------------

.. _pf_tag_disk:

<disk>
------

SimGrid can simulate the time it takes to read or write data on disk, even if the stored data is not made persistent in
any way by SimGrid. This means that your application will correctly be slowed down when doing simulated I/O, but there
is no way to get the data stored this way. 

We decided to not model anything beyond raw access in SimGrid because we believe that there is not single way of doing so.
We provide an example model of file system as a plugin, (sparsely) documented in :ref:`plugin_filesystem`.

**Parent tags:** :ref:`pf_tag_host` |br|
**Children tags:** :ref:`pf_tag_prop` |br|
**Attributes:**

:``id``: A name of your choice (must be unique on this host).
:``read_bw``: Read bandwidth for this disk. You must specify a unit as follows.

   **Units in bytes and powers of 2** (1 KiBps = 1,024 Bps):
     Bps, KiBps, MiBps, GiBps, TiBps, PiBps, or EiBps. |br|
   **Units in bits  and powers of 2** (1 Bps = 8 bps):
     bps, Kibps, Mibps, Gibps, Tibps, Pibps, or Eibps. |br|
   **Units in bytes and powers of 10**  (1 KBps = 1,000 Bps):
     Bps, KBps, MBps, GBps, TBps, PBps, or EBps. |br|
   **Units in bits  and powers of 10:**
     bps, Kbps, Mbps, Gbps, Tbps, Pbps, or Ebps.

:``write_bw``: Write bandwidth for this disk. You must specify a unit as for the read bandwidth.

.. code-block:: xml

    <host id="alice" speed="1Gf">
      <disk id="Disk1" read_bw="200MBps" write_bw="80MBps">
        <!-- you can add properties for anything you want: they are not used by SimGrid -->
        <prop id="content" value="storage/content/small_content.txt"/>
      </disk>
      <prop id="ram" value="100B" />
    </host>

-------------------------------------------------------------------------------

.. _pf_tag_config:

<config>
--------

Adding configuration flags directly into the platform file becomes particularly
useful when the realism of the described platform depends on some specific
flags. For example, this could help you to finely tune SMPI. Almost all
:ref:`command-line configuration items <options_list>` can be configured this
way.

Each configuration flag is described as a :ref:`pf_tag_prop` whose ``id`` is the
name of the flag and ``value`` is what it has to be set to.

**Parent tags:** :ref:`pf_tag_platform` (must appear before any other tags) |br|
**Children tags:** :ref:`pf_tag_prop` |br|
**Attributes:** none

.. code-block:: xml

   <?xml version = '1.0'?>
   <!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
   <platform version = "4.1">
     <config>
       <prop id = "maxmin/precision" value = "0.000010" />
       <prop id = "cpu/optim" value = "TI" />
       <prop id = "network/model" value = "SMPI" />
       <prop id = "smpi/bw-factor" value = "65472:0.940694;15424:0.697866;9376:0.58729" />
     </config>

     <!-- The rest of your platform -->
   </platform>

-------------------------------------------------------------------------------

.. _pf_tag_host:

<host>
------

A host is the computing resource on which an actor can run. See :cpp:class:`simgrid::s4u::Host`.

**Parent tags:** :ref:`pf_tag_zone` (only leaf zones, i.e., zones containing neither inner zones nor clusters) |br|
**Children tags:** :ref:`pf_tag_mount`, :ref:`pf_tag_prop`, :ref:`pf_tag_disk` |br|
**Attributes:**

:``id``: Host name.
   Must be unique over the whole platform.
:``speed``: Computational power (per core, in flop/s).
   If you use DVFS, provide a comma-separated list of values for each pstate (see :ref:`howto_dvfs`).
:``core``: Amount of cores (default: 1).
   See :ref:`howto_multicore`.
:``availability_file``:
   File containing the availability profile.
   Almost every lines of such files describe timed events as ``date ratio``.
   Example:

   .. code-block:: python

      1 0.5
      2 0.2
      5 1
      LOOPAFTER 5

   - At time t = 1, half of the host computational power (0.5 means 50%) is used to process some background load, hence
     only 50% of this initial power remains available to your own simulation.
   - At time t = 2, the available power drops at 20% of the initial value.
   - At time t = 5, the host can compute at full speed again.
   - At time t = 10, the profile is reset (as we are 5 seconds after the last event). Then the available speed will drop
     again to 50% at time t = 11.

   If your profile does not contain any LOOPAFTER line, then it will
   be executed only once and not repeated.

   .. warning:: Don't get fooled: Bandwidth and Latency profiles of a :ref:`pf_tag_link` contain absolute values, while
      Availability profiles of a :ref:`pf_tag_host` contain ratios.
:``state_file``: File containing the state profile.
   Almost every lines of such files describe timed events as ``date boolean``.
   Example:

   .. code-block:: python

      1 0
      2 1
      LOOPAFTER 8

   - At time t = 1, the host is turned off (a zero value means OFF)
   - At time t = 2, the host is turned back on (any other value than zero means ON)
   - At time t = 10, the profile is reset (as we are 8 seconds after the last event). Then the host will be turned off 
     again at time t = 11.

   If your profile does not contain any LOOPAFTER line, then it will
   be executed only once and not repeated.

:``coordinates``: Vivaldi coordinates (meaningful for Vivaldi zones only).
   See :ref:`pf_tag_peer`.
:``pstate``: Initial pstate (default: 0, the first one).
   See :ref:`howto_dvfs`.

-------------------------------------------------------------------------------

.. _pf_tag_link:

<link>
------

SimGrid links usually represent one-hop network connections (see :cpp:class:`simgrid::s4u::Link`), i.e., a single wire. 
They can also be used to abstract a larger network interconnect, e.g., the entire transcontinental network, into a 
single element.

**Parent tags:** :ref:`pf_tag_zone` (both leaf zones and inner zones) |br|
**Children tags:** :ref:`pf_tag_prop` |br|
**Attributes:**

:``id``:  Link name. Must be unique over the whole platform.
:``bandwidth``: Maximum bandwidth for this link. You must specify a unit as follows.

   **Units in bytes and powers of 2** (1 KiBps = 1,024 Bps):
     Bps, KiBps, MiBps, GiBps, TiBps, PiBps, or EiBps. |br|
   **Units in bits  and powers of 2** (1 Bps = 8 bps):
     bps, Kibps, Mibps, Gibps, Tibps, Pibps, or Eibps. |br|
   **Units in bytes and powers of 10**  (1 KBps = 1,000 Bps):
     Bps, KBps, MBps, GBps, TBps, PBps, or EBps. |br|
   **Units in bits  and powers of 10:**
     bps, Kbps, Mbps, Gbps, Tbps, Pbps, or Ebps.

:``latency``: Latency for this link (default: 0.0). You must specify a unit as follows.

   ==== =========== ======================
   Unit Meaning     Duration in seconds
   ==== =========== ======================
   ps   picosecond  10⁻¹² = 0.000000000001
   ns   nanosecond  10⁻⁹ = 0.000000001
   us   microsecond 10⁻⁶ = 0.000001
   ms   millisecond 10⁻³ = 0.001
   s    second      1
   m    minute      60
   h    hour        60 * 60
   d    day         60 * 60 * 24
   w    week        60 * 60 * 24 * 7
   ==== =========== ======================

:``sharing_policy``: Sharing policy for the link. Possible values are ``SHARED``, ``FATPIPE`` or ``SPLITDUPLEX``
   (default: ``SPLITDUPLEX``).

   If set to ``SPLITDUPLEX``, the link models the full-duplex
   behavior, as meant in TCP or UDP. To that extend, the link is
   actually split in two links whose names are suffixed with "_UP" and
   "_DOWN". You should then specify the direction to use when
   referring to that link in a :ref:`pf_tag_link_ctn`.

   If set to ``FATPIPE``, flows have no impact on each other, hence
   each flow can exploit the full bandwidth. This models Internet
   backbones that cannot get saturated by your application. From your
   application point of view, there is no congestion on these
   backbones.

   If set to ``SHARED``, the available bandwidth is fairly shared
   among ALL flows traversing this link. The resulting link is not
   full-duplex (as UDP or TCP would be): communications in both
   directions share the same link. Prefer ``SPLITDUPLEX`` for TCP flows.

:``bandwidth_file``: File containing the bandwidth profile.
   Almost every lines of such files describe timed events as ``date
   bandwidth`` (in bytes per second).
   Example:

   .. code-block:: python

      4.0 40000000
      8.0 60000000
      LOOPAFTER 12.0

   - At time t = 4, the bandwidth is of 40 MBps.
   - At time t = 8, it raises to 60MBps.
   - At time t = 24, it drops at 40 MBps again.

   If your profile does not contain any LOOPAFTER line, then it will
   be executed only once and not repeated.

   .. warning:: Don't get fooled: Bandwidth and Latency profiles of a :ref:`pf_tag_link` contain absolute values, while
      Availability profiles of a :ref:`pf_tag_host` contain ratios.

:``latency_file``: File containing the latency profile.
   Almost every lines of such files describe timed events as ``date
   latency`` (in seconds).
   Example:

   .. code-block:: python

      1.0 0.001
      3.0 0.1
      LOOPAFTER 5.0

   - At time t = 1, the latency is of 1ms (0.001 second)
   - At time t = 3, the latency is of 100ms (0.1 second)
   - At time t = 8 (5 seconds after the last event), the profile loops.
   - At time t = 9 (1 second after the loop reset), the latency is back at 1ms.

   If your profile does not contain any LOOPAFTER line, then it will
   be executed only once and not repeated.

  .. warning:: Don't get fooled: Bandwidth and Latency profiles of a :ref:`pf_tag_link` contain absolute values, while
      Availability profiles of a :ref:`pf_tag_host` contain ratios.

:``state_file``: File containing the state profile. See :ref:`pf_tag_host`.

-------------------------------------------------------------------------------

.. _pf_tag_link_ctn:

<link_ctn>
----------

An element in a route, representing a previously defined link.

**Parent tags:** :ref:`pf_tag_route` |br|
**Children tags:** none |br|
**Attributes:**

:``id``: Link that is to be included in this route.
:``direction``: either ``UP`` (by default) or ``DOWN``, specifying whether to
                use the uplink or downlink component of the link (that must
                follow the ``SPLITDUPLEX`` sharing policy). |br|
                Please refer to the ``sharing_policy`` attribute in
                :ref:`pf_tag_link`.

-------------------------------------------------------------------------------

.. _pf_tag_peer:

<peer>
------

This tag represents a peer, as in Peer-to-Peer (P2P) networks. It is
handy to model situations where hosts have an asymmetric
connectivity. Computers connected through set-top-boxes usually have a
much better download rate than their upload rate.  To model this,
<peer> creates and connects several elements: a host, an upload link
and a download link.

**Parent tags:** :ref:`pf_tag_zone` (only with Vivaldi routing) |br|
**Children tags:** none |br|
**Attributes:**

:``id``: Name of the host. Must be unique on the whole platform.
:``speed``: Computational power (in flop/s).

   If you use DVFS, provide a comma-separated list of values for each pstate (see :ref:`howto_dvfs`). 
:``bw_in``: Bandwidth of the private downstream link, along with its
	    unit. See :ref:`pf_tag_link`.
:``bw_out``: Bandwidth of the private upstream link, along with its
	     unit. See :ref:`pf_tag_link`.
:``lat``: Latency of both private links. See :ref:`pf_tag_link`.
:``coordinates``: Coordinates of the gateway for this peer.

   The communication latency between a host A = (xA,yA,zA) and a host B = (xB,yB,zB) is computed as follows:

   latency = sqrt( (xA-xB)² + (yA-yB)² ) + zA + zB

   See the documentation of
   :cpp:class:`simgrid::kernel::routing::VivaldiZone` for details on
   how the latency is computed from the coordinates, and on how the up
   and down bandwidth are used.
:``availability_file``: File containing the availability profile.
   See the full description in :ref:`pf_tag_host`
:``state_file``: File containing the state profile.
   See the full description in :ref:`pf_tag_host`

-------------------------------------------------------------------------------

.. _pf_tag_platform:

<platform>
----------

**Parent tags:** none (this is the root tag of every file) |br|
**Children tags:** :ref:`pf_tag_config` (must come first),
:ref:`pf_tag_cluster`, :ref:`pf_tag_cabinet`, :ref:`pf_tag_peer`,
:ref:`pf_tag_zone`, :ref:`pf_tag_trace`, :ref:`pf_tag_trace_connect`, or
:ref:`pf_tag_actor` in :ref:`deployment <deploy>` files.|br|
**Attributes:**

:``version``: Version of the DTD, describing the whole XML format.
	      This versioning allow future evolutions, even if we
	      avoid backward-incompatible changes. The current version
	      is **4.1**. The ``simgrid_update_xml`` program can
	      upgrade most of the past platform files to the most recent
	      formalism.

-------------------------------------------------------------------------------

.. _pf_tag_prop:

<prop>
------

This tag can be used to attach user-defined properties to some
platform elements. Both the name and the value can be any string of
your wish. You can use this to pass extra parameters to your code and
the plugins.

From your code, you can interact with these properties using the
following functions:

- Actor: :cpp:func:`simgrid::s4u::Actor::get_property` or :cpp:func:`MSG_process_get_property_value`
- Cluster: this is a zone, see below.
- Host: :cpp:func:`simgrid::s4u::Host::get_property` or :cpp:func:`MSG_host_get_property_value`
- Link: :cpp:func:`simgrid::s4u::Link::get_property`
- Disk: :cpp:func:`simgrid::s4u::Disk::get_property`
- Storage :cpp:func:`MSG_storage_get_property_value` (deprecated)
- Zone: :cpp:func:`simgrid::s4u::Zone::get_property` of :cpp:func:`MSG_zone_get_property_value`

**Parent tags:** :ref:`pf_tag_actor`, :ref:`pf_tag_config`, :ref:`pf_tag_cluster`, :ref:`pf_tag_host`,
:ref:`pf_tag_link`, :ref:`pf_tag_disk`,:ref:`pf_tag_storage` (deprecated), :ref:`pf_tag_zone` |br|
**Children tags:** none |br|
**Attributes:**

:``id``: Name of the defined property.
:``value``: Value of the defined property.

-------------------------------------------------------------------------------

.. _pf_tag_route:

<route>
-------

A path between two network locations, composed of several occurrences
of :ref:`pf_tag_link` .

**Parent tags:** :ref:`pf_tag_zone` |br|
**Children tags:** :ref:`pf_tag_link_ctn` |br|
**Attributes:**

:``src``: Host from which this route starts. Must be an existing host.
:``dst``: Host to which this route leads. Must be an existing host.
:``symmetrical``: Whether this route is symmetrical, ie, whether we
		  are defining the route ``dst -> src`` at the same
		  time. Valid values: ``yes``, ``no``, ``YES``, ``NO``.

-------------------------------------------------------------------------------

.. _pf_tag_router:

<router>
------------------------------------------------------------------

A router is similar to a :ref:`pf_tag_host`, but it cannot contain
any actor. It is only useful to some routing algorithms. In
particular, they are useful when you want to use the NS3 bindings to
break the routes that are longer than 1 hop.

**Parent tags:** :ref:`pf_tag_zone` (only leaf zones, i.e., zones containing neither inner zones nor clusters) |br|
**Attributes:**

:``id``: Router name.
   No other host or router may have the same name over the whole platform.
:``coordinates``: Vivaldi coordinates. See :ref:`pf_tag_peer`.

-------------------------------------------------------------------------------

.. _pf_tag_zone:

<zone>
------

A networking zone is an area in which elements are located. See :cpp:class:`simgrid::s4u::Zone`.

**Parent tags:** :ref:`pf_tag_platform`, :ref:`pf_tag_zone` (only internal nodes, i.e., zones
containing only inner zones or clusters but no basic
elements such as host or peer) |br|
**Children tags (if internal zone):** :ref:`pf_tag_cluster`, :ref:`pf_tag_link`, :ref:`pf_tag_zone` |br|
**Children tags (if leaf zone):** :ref:`pf_tag_host`, :ref:`pf_tag_link`, :ref:`pf_tag_peer` |br|
**Attributes:**

:``id``: Zone name.
   No other zone may have the same name over the whole platform.
:``routing``: Routing algorithm to use.

-------------------------------------------------------------------------------

.. _pf_tag_zoneRoute:

<zoneRoute>
------

The purpose of this entity is to define a route between two zones.
Recall that all zones form a tree, so to connect two sibling zones,
you must give such a zoneRoute specifying the source and destination zones,
along with the gateway in each zone (i.e., the point to reach within that zone to reach the zone),
and the list of links to go from one zone to another.

**Parent tags:** :ref:`pf_tag_zone` |br|
**Children tags:** :ref:`pf_tag_link_ctn` |br|
**Attributes:**

:``src``: Zone from which this route starts. Must be an existing zone.
:``dst``: Zone to which this route leads. Must be an existing zone.
:``gw_src``: Netpoint (within src zone) from which this route starts. Must be an existing host/router.
:``gw_dst``: Netpoint (within dst zone) to which this route leads. Must be an existing host/router.
:``symmetrical``: Whether this route is symmetrical, ie, whether we
		  are defining the route ``dst -> src`` at the same
		  time. Valid values: ``yes``, ``no``, ``YES``, ``NO``.

.. |br| raw:: html

   <br />
