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

.. _platform_reference:
   
DTD Reference
*************

Your platform description should follow the specification presented in
the `simgrid.dtd <https://simgrid.org/simgrid.dtd>`_
DTD file. The same DTD is used for both the platform and deployment
files. 

.. _pf_tag_config:

------------------------------------------------------------------
<config>
------------------------------------------------------------------

Adding configuration flags into the platform file is particularly
useful when the described platform is best used with specific
flags. For example, you could finely tune SMPI in your platform file
directly.  Almost all :ref:`command-line configuration items <options_list>`
can be configured this way.

**Parent tags:** :ref:`pf_tag_platform` (must appear before any other tags) |br|
**Children tags:** :ref:`pf_tag_prop` |br|
**Attributes:** none

.. code-block:: xml

   <?xml version='1.0'?>
   <!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
   <platform version="4.1">
   <config>
	<prop id="maxmin/precision" value="0.000010" />
	<prop id="cpu/optim" value="TI" />
	<prop id="network/model" value="SMPI" />
	<prop id="smpi/bw-factor" value="65472:0.940694;15424:0.697866;9376:0.58729" />
   </config>

   <!-- The rest of your platform -->
   </platform>

|hr|
   
.. _pf_tag_host:

------------------------------------------------------------------
<host>
------------------------------------------------------------------

An host is the computing resource on which an actor can execute. See :cpp:class:`simgrid::s4u::Host`.

**Parent tags:** :ref:`pf_tag_zone` (only leaf zones, i.e. zones containing no inner zones nor clusters) |br|
**Children tags:** :ref:`pf_tag_mount`, :ref:`pf_tag_prop`, :ref:`pf_tag_storage` |br|
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
      LOOPAFTER 8

   - At time t=1, half of its power is taken by some background
     computations, so only 50% of its initial power remains available
     (0.5 means 50%). 
   - At time t=2, the available power drops at 20% of the total.
   - At time t=5, the host computes back at full speed.
   - At time t=10, the history is reset (because that's 5 seconds after
     the last event). So the available speed will drop at t=11.

   If your trace does not contain a LOOPAFTER line, then your profile
   is only executed once and not repetitively.

   .. warning:: Don't get fooled: Bandwidth and Latency profiles of a
      :ref:`pf_tag_link` are absolute values, but Availability
      profiles of :ref:`pf_tag_host` are ratio.
:``state_file``: File containing the state profile.
   Almost every lines of such files describe timed events as ``date boolean``.
   Example:

   .. code-block:: python
		   
      1 0
      2 1
      LOOPAFTER 8

   - At time t=1, the host is turned off (value 0 means OFF)
   - At time t=2, it is turned back on (other values means ON)
   - At time t=10, the history is reset (because that's 8 seconds after
     the last event). So the host will be turned off again at t=11.

   If your trace does not contain a LOOPAFTER line, then your profile
   is only executed once and not repetitively.

:``coordinates``: Vivaldi coordinates (Vivaldi zones only).
   See :ref:`pf_tag_peer`.
:``pstate``: Initial pstate (default: 0, the first one).
   See :ref:`howto_dvfs`.

|hr|
   
.. _pf_tag_link:

------------------------------------------------------------------
<link>
------------------------------------------------------------------

Network links can represent one-hop network connections (see
:cpp:class:`simgrid::s4u::Link`). SimGrid links can be used to
represent either a single wire, or to abstract a larger network
interconnect in a single element. A single link can for example be
used to model the transcontinental network.

**Parent tags:** :ref:`pf_tag_zone` (both leaf zones and inner zones) |br|
**Children tags:** :ref:`pf_tag_prop` |br|
**Attributes:**

:``id``:  Link name. Must be unique over the whole platform.
:``bandwidth``: Maximum bandwidth for this link. You must specify the
   unit as follows.

   **Units in bytes and powers of 2** (1 KiBps = 1024 Bps):
      Bps, KiBps, MiBps, GiBps, TiBps, PiBps, EiBps |br|
   **Units in bits  and powers of 2** (1 Bps = 8 bps):
      bps, Kibps, Mibps, Gibps, Tibps, Pibps, Eibps |br|
   **Units in bytes and powers of 10:**  (1 KBps = 1000 Bps)
      Bps, KBps, MBps, GBps, TBps, PBps, EBps |br|
   **Units in bits  and powers of 10:**
      'Ebps', 'Pbps', 'Tbps', 'Gbps', 'Mbps', 'kbps', 'bps'

:``latency``: Latency for this link (default: 0.0). You must specify
   the unit as follows.

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
   
		      
:``sharing_policy``: Sharing policy for the link. 
   Either ``SHARED``, ``FATPIPE`` or ``SPLITDUPLEX`` (default: ``SHARED``).

   If set to ``SHARED``, the available bandwidth is shared fairly
   between all flows traversing this link. This tend to model the
   sharing behavior of UDP or TCP.

   If set to ``FATPIPE``, the flows have no mutual impact, and each
   flow can obtain the full bandwidth of this link. This is intended
   to model the internet backbones that cannot get saturated by your
   application: you mostly experience their latency.

   If set to ``SPLITDUPLEX``, the link models cross-traffic
   effects. Under the ``SHARED`` policy, two flows of reverse
   direction share the same resource, and can only get half of the
   bandwidth each. But TCP connections are full duplex, meaning that
   all both directions can get the full bandwidth. To model this, any
   link under the ``SPLITDUPLEX`` policy is split in two links (their
   names are suffixed with "_UP" and "_DOWN"). You must then specify
   which direction gets actually used when referring to that link in a
   :ref:`pf_tag_link_ctn`.
	
:``bandwidth_file``: File containing the bandwidth profile.
   Almost every lines of such files describe timed events as ``date
   bandwidth`` (in bytes per second).
   Example:

   .. code-block:: python

      4.0 40000000
      8.0 60000000
      LOOPAFTER 12.0

   - At time t=4, the bandwidth is of 40 MBps.
   - At time t=8, it raises to 60MBps.
   - At time t=24, it drops at 40 MBps again.

   .. warning:: Don't get fooled: Bandwidth and Latency profiles of a
      :ref:`pf_tag_link` are absolute values, but Availability
      profiles of :ref:`pf_tag_host` are ratio.
:``latency_file``: File containing the latency profile.
   Almost every lines of such files describe timed events as ``date
   latency`` (in seconds).
   Example:
		   
   .. code-block:: python
		   
      1.0 0.001
      3.0 0.1
      LOOPAFTER 5.0

   - At time t=1, the latency is of 1ms (0.001 second)
   - At time t=3, the latency is of 100ms (0.1 second)
   - At time t=8 (5 seconds after the last event), the profile loops.
   - At time t=9 (1 second after the loop reset), the latency is back at 1ms.
      
   If your trace does not contain a LOOPAFTER line, then your profile
   is only executed once and not repetitively.
  
   .. warning:: Don't get fooled: Bandwidth and Latency profiles of a
      :ref:`pf_tag_link` are absolute values, but Availability
      profiles of :ref:`pf_tag_host` are ratio.
:``state_file``: File containing the state profile. See :ref:`pf_tag_host`.

|hr|

.. _pf_tag_peer:

------------------------------------------------------------------
<peer>
------------------------------------------------------------------

This tag represents a peer, as in Peer-to-Peer (P2P) networks. It is
handy to model situations where hosts have an asymmetric
connectivity. Computers connected through set-to-boxes usually have a
much better download rate than their upload rate.  To model this,
<peer> creates and connects several elements: an host, an upload link
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

   The communication latency between an host A=(xA,yA,zA) and an host
   B=(xB,yB,zB) is computed as follows:
 
   latency = sqrt( (xA-xB)² + (yA-yB)² ) + zA + zB

   See the documentation of
   :cpp:class:`simgrid::kernel::routing::VivaldiZone` for details on
   how the latency is computed from the coordinate, and on the the up
   and down bandwidth are used.
:``availability_file``: File containing the availability profile.
   See the full description in :ref:`pf_tag_host`
:``state_file``: File containing the state profile.
   See the full description in :ref:`pf_tag_host`

|hr|

.. _pf_tag_platform:

------------------------------------------------------------------
<platform>
------------------------------------------------------------------

**Parent tags:** none (this is the root tag of every file) |br|
**Children tags:** :ref:`pf_tag_config` (must come first),
:ref:`pf_tag_cluster`, :ref:`pf_tag_cabinet`, :ref:`pf_tag_peer`,
:ref:`pf_tag_zone`, :ref:`pf_tag_trace`, :ref:`pf_tag_trace_connect` |br|
**Attributes:** 

:``version``: Version of the DTD, describing the whole XML format.
	      This versionning allow future evolutions, even if we
	      avoid backward-incompatible changes. The current version
	      is **4.1**. The ``simgrid_update_xml`` program can
	      upgrade most of the past platform files to the recent
	      formalism.

|hr|
	      
.. _pf_tag_prop:

------------------------------------------------------------------
<prop>
------------------------------------------------------------------

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
- Storage: :cpp:func:`simgrid::s4u::Storage::get_property` or :cpp:func:`MSG_storage_get_property_value`
- Zone: :cpp:func:`simgrid::s4u::Zone::get_property` of :cpp:func:`MSG_zone_get_property_value`

**Parent tags:** :ref:`pf_tag_actor`, :ref:`pf_tag_config`, :ref:`pf_tag_cluster`, :ref:`pf_tag_host`,
:ref:`pf_tag_link`, :ref:`pf_tag_storage`, :ref:`pf_tag_zone` |br|
**Children tags:** none |br|
**Attributes:**

:``id``: Name of the defined property.
:``value``: Value of the defined property.

|hr|

.. _pf_tag_router:

------------------------------------------------------------------
<router>
------------------------------------------------------------------

A router is similar to an :ref:`pf_tag_host`, but it cannot contain
any actor. It is only useful to some routing algorithms. In
particular, they are useful when you want to use the NS3 bindings to
break the routes that are longer than 1 hop.

**Parent tags:** :ref:`pf_tag_zone` (only leaf zones, i.e. zones containing no inner zones nor clusters) |br|
**Children tags:** :ref:`pf_tag_prop`, :ref:`pf_tag_storage` |br|
**Attributes:**

:``id``: Router name.
   No other host or router may have the same name over the whole platform.
:``coordinates``: Vivaldi coordinates. See :ref:`pf_tag_peer`.	    

.. |br| raw:: html

   <br />

.. |hr| raw:: html

   <hr />
