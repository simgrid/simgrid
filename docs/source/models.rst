.. _models:

The SimGrid Models
==================

.. _understanding_lv08:

Understanding the default TCP model
***********************************
When simulating a data transfer between two hosts, you may be surprised
by the obtained simulation time. Lets consider the following platform:

.. code-block:: xml

   <host id="A" speed="1Gf" />
   <host id="B" speed="1Gf" />

   <link id="link1" latency="10ms" bandwidth="1Mbps" />

   <route src="A" dst="B">
     <link_ctn id="link1" />
   </route>

If host `A` sends `100kB` (a hundred kilobytes) to host `B`, one could expect
that this communication would take `0.81` seconds to complete according to a
simple latency-plus-size-divided-by-bandwidth model (0.01 + 8e5/1e6 = 0.81).
However, the default TCP model of SimGrid is a bit more complex than that. It
accounts for three phenomena that directly impact the simulation time even
on such a simple example:

  - The size of a message at the application level (i.e., 100kB in this
    example) is not the size that will actually be transferred over the
    network. To mimic the fact that TCP and IP headers are added to each packet of
    the original payload, the TCP model of SimGrid empirically considers that
    `only 97% of the nominal bandwidth` are available. In other words, the
    size of your message is increased by a few percents, whatever this size be.

  - In the real world, the TCP protocol is not able to fully exploit the
    bandwidth of a link from the emission of the first packet. To reflect this
    `slow start` phenomenon, the latency declared in the platform file is
    multiplied by `a factor of 13.01`. Here again, this is an empirically
    determined value that may not correspond to every TCP implementations on
    every networks. It can be tuned when more realistic simulated times for
    short messages are needed though.

  - When data is transferred from A to B, some TCP ACK messages travel in the
    opposite direction. To reflect the impact of this `cross-traffic`, SimGrid
    simulates a flow from B to A that represents an additional bandwidth
    consumption of `0.05`. The route from B to A is implicitly declared in the
    platform file and uses the same link `link1` as if the two hosts were
    connected through a communication bus. The bandwidth share allocated to the
    flow from A to B is then the available bandwidth of `link1` (i.e., 97% of
    the nominal bandwidth of 1Mb/s) divided by 1.05 (i.e., the total consumption).
    This feature, activated by default, can be disabled by adding the
    `--cfg=network/crosstraffic:0` flag to command line.

As a consequence, the time to transfer 100kB from A to B as simulated by the
default TCP model of SimGrid is not 0.81 seconds but

.. code-block:: python

    0.01 * 13.01 + 800000 / ((0.97 * 1e6) / 1.05) =  0.996079 seconds.

.. todo::

   - Main existing models (contention, cste, LM07)
   - Main concepts (Routing, LMM) + link to the papers
   - How to switch on the command line

