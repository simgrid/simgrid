.. _options:

Configuring SimGrid
===================

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("ConfigBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

A number of options can be given at runtime to change the default
SimGrid behavior. For a complete list of all configuration options
accepted by the SimGrid version used in your simulator, simply pass
the --help configuration flag to your program. If some of the options
are not documented on this page, this is a bug that you should please
report so that we can fix it. Note that some of the options presented
here may not be available in your simulators, depending on the
:ref:`compile-time options <install_src_config>` that you used.

Setting Configuration Items
---------------------------

There is several way to pass configuration options to the simulators.
The most common way is to use the ``--cfg`` command line argument. For
example, to set the item ``Item`` to the value ``Value``, simply
type the following on the command-line:

.. code-block:: shell

   my_simulator --cfg=Item:Value (other arguments)

Several ``--cfg`` command line arguments can naturally be used. If you
need to include spaces in the argument, don't forget to quote the
argument. You can even escape the included quotes (write ``@'`` for ``'`` if
you have your argument between simple quotes).

Another solution is to use the ``<config>`` tag in the platform file. The
only restriction is that this tag must occur before the first
platform element (be it ``<zone>``, ``<cluster>``, ``<peer>`` or whatever).
The ``<config>`` tag takes an ``id`` attribute, but it is currently
ignored so you don't really need to pass it. The important part is that
within that tag, you can pass one or several ``<prop>`` tags to specify
the configuration to use. For example, setting ``Item`` to ``Value``
can be done by adding the following to the beginning of your platform
file:

.. code-block:: xml

  <config>
    <prop id="Item" value="Value"/>
  </config>

A last solution is to pass your configuration directly in your program
with :cpp:func:`simgrid::s4u::Engine::set_config` or :cpp:func:`MSG_config`.

.. code-block:: cpp

   #include <simgrid/s4u.hpp>

   int main(int argc, char *argv[]) {
     simgrid::s4u::Engine e(&argc, argv);

     simgrid::s4u::Engine::set_config("Item:Value");

     // Rest of your code
   }

.. _options_list:

Existing Configuration Items
----------------------------

.. note::
  The full list can be retrieved by passing ``--help`` and
  ``--help-cfg`` to an executable that uses SimGrid. Try passing
  ``help`` as a value to get the list of values accepted by a given
  option. For example, ``--cfg=plugin:help`` will give you the list
  of plugins available in your installation of SimGrid.

- **contexts/factory:** :ref:`cfg=contexts/factory`
- **contexts/guard-size:** :ref:`cfg=contexts/guard-size`
- **contexts/nthreads:** :ref:`cfg=contexts/nthreads`
- **contexts/stack-size:** :ref:`cfg=contexts/stack-size`
- **contexts/synchro:** :ref:`cfg=contexts/synchro`

- **cpu/maxmin-selective-update:** :ref:`Cpu Optimization Level <options_model_optim>`
- **cpu/model:** :ref:`options_model_select`
- **cpu/optim:** :ref:`Cpu Optimization Level <options_model_optim>`

- **debug/breakpoint:** :ref:`cfg=debug/breakpoint`
- **debug/clean-atexit:** :ref:`cfg=debug/clean-atexit`
- **debug/verbose-exit:** :ref:`cfg=debug/verbose-exit`

- **exception/cutpath:** :ref:`cfg=exception/cutpath`

- **host/model:** :ref:`options_model_select`

- **maxmin/precision:** :ref:`cfg=maxmin/precision`
- **maxmin/concurrency-limit:** :ref:`cfg=maxmin/concurrency-limit`

- **msg/debug-multiple-use:** :ref:`cfg=msg/debug-multiple-use`

- **model-check:** :ref:`options_modelchecking`
- **model-check/checkpoint:** :ref:`cfg=model-check/checkpoint`
- **model-check/communications-determinism:** :ref:`cfg=model-check/communications-determinism`
- **model-check/dot-output:** :ref:`cfg=model-check/dot-output`
- **model-check/max-depth:** :ref:`cfg=model-check/max-depth`
- **model-check/property:** :ref:`cfg=model-check/property`
- **model-check/reduction:** :ref:`cfg=model-check/reduction`
- **model-check/replay:** :ref:`cfg=model-check/replay`
- **model-check/send-determinism:** :ref:`cfg=model-check/send-determinism`
- **model-check/termination:** :ref:`cfg=model-check/termination`
- **model-check/timeout:** :ref:`cfg=model-check/timeout`
- **model-check/visited:** :ref:`cfg=model-check/visited`

- **network/bandwidth-factor:** :ref:`cfg=network/bandwidth-factor`
- **network/crosstraffic:** :ref:`cfg=network/crosstraffic`
- **network/latency-factor:** :ref:`cfg=network/latency-factor`
- **network/loopback-lat:** :ref:`cfg=network/loopback`
- **network/loopback-bw:** :ref:`cfg=network/loopback`
- **network/maxmin-selective-update:** :ref:`Network Optimization Level <options_model_optim>`
- **network/model:** :ref:`options_model_select`
- **network/optim:** :ref:`Network Optimization Level <options_model_optim>`
- **network/TCP-gamma:** :ref:`cfg=network/TCP-gamma`
- **network/weight-S:** :ref:`cfg=network/weight-S`

- **ns3/TcpModel:** :ref:`options_pls`
- **ns3/seed:** :ref:`options_pls`
- **path:** :ref:`cfg=path`
- **plugin:** :ref:`cfg=plugin`

- **storage/max_file_descriptors:** :ref:`cfg=storage/max_file_descriptors`

- **surf/precision:** :ref:`cfg=surf/precision`

- **For collective operations of SMPI,** please refer to Section :ref:`cfg=smpi/coll-selector`
- **smpi/auto-shared-malloc-thresh:** :ref:`cfg=smpi/auto-shared-malloc-thresh`
- **smpi/async-small-thresh:** :ref:`cfg=smpi/async-small-thresh`
- **smpi/buffering:** :ref:`cfg=smpi/buffering`
- **smpi/bw-factor:** :ref:`cfg=smpi/bw-factor`
- **smpi/coll-selector:** :ref:`cfg=smpi/coll-selector`
- **smpi/comp-adjustment-file:** :ref:`cfg=smpi/comp-adjustment-file`
- **smpi/cpu-threshold:** :ref:`cfg=smpi/cpu-threshold`
- **smpi/display-allocs:** :ref:`cfg=smpi/display-allocs`
- **smpi/display-timing:** :ref:`cfg=smpi/display-timing`
- **smpi/errors-are-fatal:** :ref:`cfg=smpi/errors-are-fatal`
- **smpi/finalization-barrier:** :ref:`cfg=smpi/finalization-barrier`
- **smpi/grow-injected-times:** :ref:`cfg=smpi/grow-injected-times`
- **smpi/host-speed:** :ref:`cfg=smpi/host-speed`
- **smpi/IB-penalty-factors:** :ref:`cfg=smpi/IB-penalty-factors`
- **smpi/iprobe:** :ref:`cfg=smpi/iprobe`
- **smpi/iprobe-cpu-usage:** :ref:`cfg=smpi/iprobe-cpu-usage`
- **smpi/init:** :ref:`cfg=smpi/init`
- **smpi/keep-temps:** :ref:`cfg=smpi/keep-temps`
- **smpi/lat-factor:** :ref:`cfg=smpi/lat-factor`
- **smpi/ois:** :ref:`cfg=smpi/ois`
- **smpi/or:** :ref:`cfg=smpi/or`
- **smpi/os:** :ref:`cfg=smpi/os`
- **smpi/papi-events:** :ref:`cfg=smpi/papi-events`
- **smpi/privatization:** :ref:`cfg=smpi/privatization`
- **smpi/privatize-libs:** :ref:`cfg=smpi/privatize-libs`
- **smpi/send-is-detached-thresh:** :ref:`cfg=smpi/send-is-detached-thresh`
- **smpi/shared-malloc:** :ref:`cfg=smpi/shared-malloc`
- **smpi/shared-malloc-hugepage:** :ref:`cfg=smpi/shared-malloc-hugepage`
- **smpi/simulate-computation:** :ref:`cfg=smpi/simulate-computation`
- **smpi/test:** :ref:`cfg=smpi/test`
- **smpi/wtime:** :ref:`cfg=smpi/wtime`
- **smpi/list-leaks** :ref:`cfg=smpi/list-leaks`

- **Tracing configuration options** can be found in Section :ref:`tracing_tracing_options`

- **storage/model:** :ref:`options_model_select`

- **vm/model:** :ref:`options_model_select`

.. _options_model:

Configuring the Platform Models
-------------------------------

.. _options_model_select:

Choosing the Platform Models
............................

SimGrid comes with several network, CPU and disk models built in,
and you can change the used model at runtime by changing the passed
configuration. The three main configuration items are given below.
For each of these items, passing the special ``help`` value gives you
a short description of all possible values (for example,
``--cfg=network/model:help`` will present all provided network
models). Also, ``--help-models`` should provide information about all
models for all existing resources.

- ``network/model``: specify the used network model. Possible values:

  - **LV08 (default one):** Realistic network analytic model
    (slow-start modeled by multiplying latency by 13.01, bandwidth by
    .97; bottleneck sharing uses a payload of S=20537 for evaluating
    RTT). Described in `Accuracy Study and Improvement of Network
    Simulation in the SimGrid Framework
    <http://mescal.imag.fr/membres/arnaud.legrand/articles/simutools09.pdf>`_.
  - **Constant:** Simplistic network model where all communication
    take a constant time (one second). This model provides the lowest
    realism, but is (marginally) faster.
  - **SMPI:** Realistic network model specifically tailored for HPC
    settings (accurate modeling of slow start with correction factors on
    three intervals: < 1KiB, < 64 KiB, >= 64 KiB). This model can be
    :ref:`further configured <options_model_network>`.
  - **IB:** Realistic network model specifically tailored for HPC
    settings with InfiniBand networks (accurate modeling contention
    behavior, based on the model explained in `this PhD work
    <http://mescal.imag.fr/membres/jean-marc.vincent/index.html/PhD/Vienne.pdf>`_.
    This model can be :ref:`further configured <options_model_network>`.
  - **CM02:** Legacy network analytic model. Very similar to LV08, but
    without corrective factors. The timings of small messages are thus
    poorly modeled. This model is described in `A Network Model for
    Simulation of Grid Application
    <https://hal.inria.fr/inria-00071989/document>`_.
  - **ns-3** (only available if you compiled SimGrid accordingly):
    Use the packet-level network
    simulators as network models (see :ref:`model_ns3`).
    This model can be :ref:`further configured <options_pls>`.

- ``cpu/model``: specify the used CPU model.  We have only one model
  for now:

  - **Cas01:** Simplistic CPU model (time=size/speed)

- ``host/model``: The host concept is the aggregation of a CPU with a
  network card. Three models exists, but actually, only 2 of them are
  interesting. The "compound" one is simply due to the way our
  internal code is organized, and can easily be ignored. So at the
  end, you have two host models: The default one allows aggregation of
  an existing CPU model with an existing network model, but does not
  allow parallel tasks because these beasts need some collaboration
  between the network and CPU model. That is why, ptask_07 is used by
  default when using SimDag.

  - **default:** Default host model. Currently, CPU:Cas01 and
    network:LV08 (with cross traffic enabled)
  - **compound:** Host model that is automatically chosen if
    you change the network and CPU models
  - **ptask_L07:** Host model somehow similar to Cas01+CM02 but
    allowing "parallel tasks", that are intended to model the moldable
    tasks of the grid scheduling literature.

- ``storage/model``: specify the used storage model. Only one model is
  provided so far.
- ``vm/model``: specify the model for virtual machines. Only one model
  is provided so far.

.. todo: make 'compound' the default host model.

.. _options_model_optim:

Optimization Level
..................

The network and CPU models that are based on lmm_solve (that
is, all our analytical models) accept specific optimization
configurations.

  - items ``network/optim`` and ``cpu/optim`` (both default to 'Lazy'):

    - **Lazy:** Lazy action management (partial invalidation in lmm +
      heap in action remaining).
    - **TI:** Trace integration. Highly optimized mode when using
      availability traces (only available for the Cas01 CPU model for
      now).
    - **Full:** Full update of remaining and variables. Slow but may be
      useful when debugging.

  - items ``network/maxmin-selective-update`` and
    ``cpu/maxmin-selective-update``: configure whether the underlying
    should be lazily updated or not. It should have no impact on the
    computed timings, but should speed up the computation. |br| It is
    still possible to disable this feature because it can reveal
    counter-productive in very specific scenarios where the
    interaction level is high. In particular, if all your
    communication share a given backbone link, you should disable it:
    without it, a simple regular loop is used to update each
    communication. With it, each of them is still updated (because of
    the dependency induced by the backbone), but through a complicated
    and slow pattern that follows the actual dependencies.

.. _cfg=maxmin/precision:
.. _cfg=surf/precision:

Numerical Precision
...................

**Option** ``maxmin/precision`` **Default:** 0.00001 (in flops or bytes) |br|
**Option** ``surf/precision`` **Default:** 0.00001 (in seconds)

The analytical models handle a lot of floating point values. It is
possible to change the epsilon used to update and compare them through
this configuration item. Changing it may speedup the simulation by
discarding very small actions, at the price of a reduced numerical
precision. You can modify separately the precision used to manipulate
timings (in seconds) and the one used to manipulate amounts of work
(in flops or bytes).

.. _cfg=maxmin/concurrency-limit:

Concurrency Limit
.................

**Option** ``maxmin/concurrency-limit`` **Default:** -1 (no limit)

The maximum number of variables per resource can be tuned through this
option. You can have as many simultaneous actions per resources as you
want. If your simulation presents a very high level of concurrency, it
may help to use e.g. 100 as a value here. It means that at most 100
actions can consume a resource at a given time. The extraneous actions
are queued and wait until the amount of concurrency of the considered
resource lowers under the given boundary.

Such limitations help both to the simulation speed and simulation accuracy
on highly constrained scenarios, but the simulation speed suffers of this
setting on regular (less constrained) scenarios so it is off by default.

.. _options_model_network:

Configuring the Network Model
.............................

.. _cfg=network/TCP-gamma:

Maximal TCP Window Size
^^^^^^^^^^^^^^^^^^^^^^^

**Option** ``network/TCP-gamma`` **Default:** 4194304

The analytical models need to know the maximal TCP window size to take
the TCP congestion mechanism into account.  On Linux, this value can
be retrieved using the following commands. Both give a set of values,
and you should use the last one, which is the maximal size.

.. code-block:: shell

   cat /proc/sys/net/ipv4/tcp_rmem # gives the sender window
   cat /proc/sys/net/ipv4/tcp_wmem # gives the receiver window

.. _cfg=network/bandwidth-factor:
.. _cfg=network/latency-factor:
.. _cfg=network/weight-S:

Correcting Important Network Parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

SimGrid can take network irregularities such as a slow startup or
changing behavior depending on the message size into account.  You
should not change these values unless you really know what you're
doing.  The corresponding values were computed through data fitting
one the timings of packet-level simulators, as described in `Accuracy
Study and Improvement of Network Simulation in the SimGrid Framework
<http://mescal.imag.fr/membres/arnaud.legrand/articles/simutools09.pdf>`_.


If you are using the SMPI model, these correction coefficients are
themselves corrected by constant values depending on the size of the
exchange.  By default SMPI uses factors computed on the Stampede
Supercomputer at TACC, with optimal deployment of processes on
nodes. Again, only hardcore experts should bother about this fact.


.. todo:: This section should be rewritten, and actually explain the
	  options network/bandwidth-factor, network/latency-factor,
	  network/weight-S.

.. _cfg=smpi/IB-penalty-factors:

Infiniband model
^^^^^^^^^^^^^^^^

InfiniBand network behavior can be modeled through 3 parameters
``smpi/IB-penalty-factors:"βe;βs;γs"``, as explained in `this PhD
thesis
<http://mescal.imag.fr/membres/jean-marc.vincent/index.html/PhD/Vienne.pdf>`_ (in French)
or more concisely in `this paper <https://hal.inria.fr/hal-00953618/document>`_,
even if that paper does only describe models for myrinet and ethernet.
You can see in Fig 2 some results for Infiniband, for example. This model
may be outdated by now for modern infiniband, anyway, so a new
validation would be good. 

The three paramaters are defined as follows:

- βs: penalty factor for outgoing messages, computed by running a simple send to
  two nodes and checking slowdown compared to a single send to one node,
  dividing by 2
- βe: penalty factor for ingoing messages, same computation method but with one
  node receiving several messages
- γr: slowdown factor when communication buffer memory is saturated. It needs a
  more complicated pattern to run in order to be computed (5.3 in the thesis,
  page 107), and formula in the end is γr = time(c)/(3×βe×time(ref)), where
  time(ref) is the time of a single comm with no contention).

Once these values are computed, a penalty is assessed for each message (this is
the part implemented in the simulator) as shown page 106 of the thesis. Here is
a simple translation of this text. First, some notations:

- ∆e(e) which corresponds to the incoming degree of node e, that is to say the number of communications having as destination node e.
- ∆s (s) which corresponds to the degree outgoing from node s, that is to say the number of communications sent by node s.
- Φ (e) which corresponds to the number of communications destined for the node e but coming from a different node.
- Ω (s, e) which corresponds to the number of messages coming from node s to node e. If node e only receives communications from different nodes then Φ (e) = ∆e (e). On the other hand if, for example, there are three messages coming from node s and going from node e then Φ (e) 6 = ∆e (e) and Ω (s, e) = 3

To determine the penalty for a communication, two values need to be calculated. First, the penalty caused by the conflict in transmission, noted ps.


- if ∆s (i) = 1 then ps = 1. 
- if ∆s (i) ≥ 2 and ∆e (i) ≥ 3 then ps = ∆s (i) × βs × γr
- else, ps = ∆s (i) × βs 


Then,  the penalty caused by the conflict in reception (noted pe) should be computed as follows:

- if ∆e (i) = 1 then pe = 1
- else, pe = Φ (e) × βe × Ω (s, e) 

Finally, the penalty associated with the communication is:
p = max (ps ∈ s, pe)

.. _cfg=network/crosstraffic:

Simulating Cross-Traffic
^^^^^^^^^^^^^^^^^^^^^^^^

Since SimGrid v3.7, cross-traffic effects can be taken into account in
analytical simulations. It means that ongoing and incoming
communication flows are treated independently. In addition, the LV08
model adds 0.05 of usage on the opposite direction for each new
created flow. This can be useful to simulate some important TCP
phenomena such as ack compression.

For that to work, your platform must have two links for each
pair of interconnected hosts. An example of usable platform is
available in ``examples/platforms/crosstraffic.xml``.

This is activated through the ``network/crosstraffic`` item, that
can be set to 0 (disable this feature) or 1 (enable it).

Note that with the default host model this option is activated by default.

.. _cfg=network/loopback:

Configuring loopback link
^^^^^^^^^^^^^^^^^^^^^^^^^

Several network model provide an implicit loopback link to account for local 
communication on a host. By default it has a 10GBps bandwidth and a null latency.
This can be changed with ``network/loopback-lat`` and ``network/loopback-bw`` 
items.

.. _cfg=smpi/async-small-thresh:

Simulating Asynchronous Send
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

(this configuration item is experimental and may change or disappear)

It is possible to specify that messages below a certain size (in bytes) will be
sent as soon as the call to MPI_Send is issued, without waiting for
the correspondent receive. This threshold can be configured through
the ``smpi/async-small-thresh`` item. The default value is 0. This
behavior can also be manually set for mailboxes, by setting the
receiving mode of the mailbox with a call to
:cpp:func:`MSG_mailbox_set_async`. After this, all messages sent to
this mailbox will have this behavior regardless of the message size.

This value needs to be smaller than or equals to the threshold set at
:ref:`cfg=smpi/send-is-detached-thresh`, because asynchronous messages
are meant to be detached as well.

.. _options_pls:

Configuring ns-3
^^^^^^^^^^^^^^^^

**Option** ``ns3/TcpModel`` **Default:** "default" (ns-3 default)

When using ns-3, there is an extra item ``ns3/TcpModel``, corresponding
to the ``ns3::TcpL4Protocol::SocketType`` configuration item in
ns-3. The only valid values (enforced on the SimGrid side) are
'default' (no change to the ns-3 configuration), 'NewReno' or 'Reno' or
'Tahoe'.

**Option** ``ns3/seed`` **Default:** "" (don't set the seed in ns-3)

This option is the random seed to provide to ns-3 with
``ns3::RngSeedManager::SetSeed`` and ``ns3::RngSeedManager::SetRun``.

If left blank, no seed is set in ns-3. If the value 'time' is
provided, the current amount of seconds since epoch is used as a seed.
Otherwise, the provided value must be a number to use as a seed.

Configuring the Storage model
.............................

.. _cfg=storage/max_file_descriptors:

File Descriptor Count per Host
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Option** ``storage/max_file_descriptors`` **Default:** 1024

Each host maintains a fixed-size array of its file descriptors. You
can change its size through this item to either enlarge it if your
application requires it or to reduce it to save memory space.

.. _cfg=plugin:

Activating Plugins
------------------

SimGrid plugins allow one to extend the framework without changing its
source code directly. Read the source code of the existing plugins to
learn how to do so (in ``src/plugins``), and ask your questions to the
usual channels (Stack Overflow, Mailing list, IRC). The basic idea is
that plugins usually register callbacks to some signals of interest.
If they need to store some information about a given object (Link, CPU
or Actor), they do so through the use of a dedicated object extension.

Some of the existing plugins can be activated from the command line,
meaning that you can activate them from the command line without any
modification to your simulation code. For example, you can activate
the host energy plugin by adding ``--cfg=plugin:host_energy`` to your
command line.

Here is a partial list of plugins that can be activated this way. You can get
the full list by passing ``--cfg=plugin:help`` to your simulator.

  - :ref:`Host Energy <plugin_host_energy>`: models the energy dissipation of the compute units.
  - :ref:`Link Energy <plugin_link_energy>`: models the energy dissipation of the network.
  - :ref:`Host Load <plugin_host_load>`: monitors the load of the compute units.

.. _options_modelchecking:

Configuring the Model-Checking
------------------------------

To enable SimGrid's model-checking support, the program should
be executed using the simgrid-mc wrapper:

.. code-block:: shell

   simgrid-mc ./my_program

Safety properties are expressed as assertions using the function
:cpp:func:`void MC_assert(int prop)`.

.. _cfg=smpi/buffering:

Specifying the MPI buffering behavior
.....................................

**Option** ``smpi/buffering`` **Default:** infty

Buffering in MPI has a huge impact on the communication semantic. For example,
standard blocking sends are synchronous calls when the system buffers are full
while these calls can complete immediately without even requiring a matching
receive call for small messages sent when the system buffers are empty.

In SMPI, this depends on the message size, that is compared against two thresholds:

- if (size < :ref:`smpi/async-small-thresh <cfg=smpi/async-small-thresh>`) then
  MPI_Send returns immediately, even if the corresponding receive has not be issued yet.
- if (:ref:`smpi/async-small-thresh <cfg=smpi/async-small-thresh>` < size < :ref:`smpi/send-is-detached-thresh <cfg=smpi/send-is-detached-thresh>`) then
  MPI_Send returns as soon as the corresponding receive has been issued. This is known as the eager mode.
- if (:ref:`smpi/send-is-detached-thresh <cfg=smpi/send-is-detached-thresh>` < size) then
  MPI_Send returns only when the message has actually been sent over the network. This is known as the rendez-vous mode.

The ``smpi/buffering`` (only valid with MC) option gives an easier interface to choose between these semantics. It can take two values:

- **zero:** means that buffering should be disabled. All communications are actually blocking.
- **infty:** means that buffering should be made infinite. All communications are non-blocking.

.. _cfg=model-check/property:

Specifying a liveness property
..............................

**Option** ``model-check/property`` **Default:** unset

If you want to specify liveness properties, you have to pass them on
the command line, specifying the name of the file containing the
property, as formatted by the `ltl2ba <https://github.com/utwente-fmt/ltl2ba>`_ program.
Note that ltl2ba is not part of SimGrid and must be installed separately.

.. code-block:: shell

   simgrid-mc ./my_program --cfg=model-check/property:<filename>

.. _cfg=model-check/checkpoint:

Going for Stateful Verification
...............................

By default, the system is backtracked to its initial state to explore
another path, instead of backtracking to the exact step before the fork
that we want to explore (this is called stateless verification). This
is done this way because saving intermediate states can rapidly
exhaust the available memory. If you want, you can change the value of
the ``model-check/checkpoint`` item. For example,
``--cfg=model-check/checkpoint:1`` asks to take a checkpoint every
step.  Beware, this will certainly explode your memory. Larger values
are probably better, make sure to experiment a bit to find the right
setting for your specific system.

.. _cfg=model-check/reduction:

Specifying the kind of reduction
................................

The main issue when using the model-checking is the state space
explosion. You can activate some reduction technique with
``--cfg=model-check/reduction:<technique>``. For now, this
configuration variable can take 2 values:

 - **none:** Do not apply any kind of reduction (mandatory for
   liveness properties, as our current DPOR algorithm breaks cycles)
 - **dpor:** Apply Dynamic Partial Ordering Reduction. Only valid if
   you verify local safety properties (default value for safety
   checks).

Another way to mitigate the state space explosion is to search for
cycles in the exploration with the :ref:`cfg=model-check/visited`
configuration. Note that DPOR and state-equality reduction may not
play well together. You should choose between them.

Our current DPOR implementation could be improved in may ways. We are
currently improving its efficiency (both in term of reduction ability
and computational speed), and future work could make it compatible
with liveness properties.

.. _cfg=model-check/visited:

Size of Cycle Detection Set (state equality reduction)
......................................................

Mc SimGrid can be asked to search for cycles during the exploration,
i.e. situations where a new explored state is in fact the same state
than a previous one.. This can prove useful to mitigate the state
space explosion with safety properties, and this is the crux when
searching for counter-examples to the liveness properties.

Note that this feature may break the current implementation of the
DPOR reduction technique.

The ``model-check/visited`` item is the maximum number of states, which
are stored in memory. If the maximum number of snapshotted state is
reached, some states will be removed from the memory and some cycles
might be missed. Small values can lead to incorrect verifications, but
large values can exhaust your memory and be CPU intensive as each new
state must be compared to that amount of older saved states.

The default settings depend on the kind of exploration. With safety
checking, no state is snapshotted and cycles cannot be detected. With
liveness checking, all states are snapshotted because missing a cycle
could hinder the exploration soundness.

.. _cfg=model-check/termination:

Non-Termination Detection
.........................

The ``model-check/termination`` configuration item can be used to
report if a non-termination execution path has been found. This is a
path with a cycle, which means that the program might never terminate.

This only works in safety mode, not in liveness mode.

This options is disabled by default.

.. _cfg=model-check/dot-output:

Dot Output
..........

If set, the ``model-check/dot-output`` configuration item is the name
of a file in which to write a dot file of the path leading to the
property violation discovered (safety or liveness violation), as well
as the cycle for liveness properties. This dot file can then be fed to the
graphviz dot tool to generate a corresponding graphical representation.

.. _cfg=model-check/max-depth:

Exploration Depth Limit
.......................

The ``model-check/max-depth`` can set the maximum depth of the
exploration graph of the model checker. If this limit is reached, a
logging message is sent and the results might not be exact.

By default, the exploration is limited to the depth of 1000.

.. _cfg=model-check/timeout:

Handling of Timeouts
....................

By default, the model checker does not handle timeout conditions: the `wait`
operations never time out. With the ``model-check/timeout`` configuration item
set to **yes**, the model checker will explore timeouts of `wait` operations.

.. _cfg=model-check/communications-determinism:
.. _cfg=model-check/send-determinism:

Communication Determinism
.........................

The ``model-check/communications-determinism`` and
``model-check/send-determinism`` items can be used to select the
communication determinism mode of the model checker, which checks
determinism properties of the communications of an application.

.. _options_mc_perf:

Verification Performance Considerations
.......................................

The size of the stacks can have a huge impact on the memory
consumption when using model-checking. By default, each snapshot will
save a copy of the whole stacks and not only of the part that is
really meaningful: you should expect the contribution of the memory
consumption of the snapshots to be:
:math:`\text{number of processes} \times \text{stack size} \times \text{number of states}`.

When compiled against the model checker, the stacks are not
protected with guards: if the stack size is too small for your
application, the stack will silently overflow into other parts of the
memory (see :ref:`contexts/guard-size <cfg=contexts/guard-size>`).

.. _cfg=model-check/replay:

Replaying buggy execution paths from the model checker
......................................................

Debugging the problems reported by the model checker is challenging:
First, the application under verification cannot be debugged with gdb
because the model checker already traces it. Then, the model checker may
explore several execution paths before encountering the issue, making it
very difficult to understand the output. Fortunately, SimGrid provides
the execution path leading to any reported issue so that you can replay
this path reported by the model checker, enabling the usage of classical
debugging tools.

When the model checker finds an interesting path in the application
execution graph (where a safety or liveness property is violated), it
generates an identifier for this path. Here is an example of the output:

.. code-block:: shell

   [  0.000000] (0:@) Check a safety property
   [  0.000000] (0:@) **************************
   [  0.000000] (0:@) *** PROPERTY NOT VALID ***
   [  0.000000] (0:@) **************************
   [  0.000000] (0:@) Counter-example execution trace:
   [  0.000000] (0:@)   [(1)Tremblay (app)] MC_RANDOM(3)
   [  0.000000] (0:@)   [(1)Tremblay (app)] MC_RANDOM(4)
   [  0.000000] (0:@) Path = 1/3;1/4
   [  0.000000] (0:@) Expanded states = 27
   [  0.000000] (0:@) Visited states = 68
   [  0.000000] (0:@) Executed transitions = 46

The interesting line is ``Path = 1/3;1/4``, which means that you should use
``--cfg=model-check/replay:1/3;1/4`` to replay your application on the buggy
execution path. All options (but the model checker related ones) must
remain the same. In particular, if you ran your application with
``smpirun -wrapper simgrid-mc``, then do it again. Remove all
MC-related options, keep non-MC-related ones and add
``--cfg=model-check/replay:???``.

Currently, if the path is of the form ``X;Y;Z``, each number denotes
the actor's pid that is selected at each indecision point. If it's of
the form ``X/a;Y/b``, the X and Y are the selected pids while the a
and b are the return values of their simcalls. In the previous
example, ``1/3;1/4``, you can see from the full output that the actor
1 is doing MC_RANDOM simcalls, so the 3 and 4 simply denote the values
that these simcall return on the execution branch leading to the
violation.

Configuring the User Code Virtualization
----------------------------------------

.. _cfg=contexts/factory:

Selecting the Virtualization Factory
....................................

**Option** contexts/factory **Default:** "raw"

In SimGrid, the user code is virtualized in a specific mechanism that
allows the simulation kernel to control its execution: when a user
process requires a blocking action (such as sending a message), it is
interrupted, and only gets released when the simulated clock reaches
the point where the blocking operation is done. This is explained
graphically in the `relevant tutorial, available online
<https://simgrid.org/tutorials/simgrid-simix-101.pdf>`_.

In SimGrid, the containers in which user processes are virtualized are
called contexts. Several context factory are provided, and you can
select the one you want to use with the ``contexts/factory``
configuration item. Some of the following may not exist on your
machine because of portability issues. In any case, the default one
should be the most effcient one (please report bugs if the
auto-detection fails for you). They are approximately sorted here from
the slowest to the most efficient:

 - **thread:** very slow factory using full featured threads (either
   pthreads or windows native threads). They are slow but very
   standard. Some debuggers or profilers only work with this factory.
 - **java:** Java applications are virtualized onto java threads (that
   are regular pthreads registered to the JVM)
 - **ucontext:** fast factory using System V contexts (Linux and FreeBSD only)
 - **boost:** This uses the `context
   implementation <http://www.boost.org/doc/libs/1_59_0/libs/context/doc/html/index.html>`_
   of the boost library for a performance that is comparable to our
   raw implementation.
   |br| Install the relevant library (e.g. with the
   libboost-contexts-dev package on Debian/Ubuntu) and recompile
   SimGrid.
 - **raw:** amazingly fast factory using a context switching mechanism
   of our own, directly implemented in assembly (only available for x86
   and amd64 platforms for now) and without any unneeded system call.

The main reason to change this setting is when the debugging tools become
fooled by the optimized context factories. Threads are the most
debugging-friendly contexts, as they allow one to set breakpoints
anywhere with gdb and visualize backtraces for all processes, in order
to debug concurrency issues. Valgrind is also more comfortable with
threads, but it should be usable with all factories (Exception: the
callgrind tool really dislikes raw and ucontext factories).

.. _cfg=contexts/stack-size:

Adapting the Stack Size
.......................

**Option** ``contexts/stack-size`` **Default:** 8192 KiB

Each virtualized used process is executed using a specific system
stack. The size of this stack has a huge impact on the simulation
scalability, but its default value is rather large. This is because
the error messages that you get when the stack size is too small are
rather disturbing: this leads to stack overflow (overwriting other
stacks), leading to segfaults with corrupted stack traces.

If you want to push the scalability limits of your code, you might
want to reduce the ``contexts/stack-size`` item. Its default value is
8192 (in KiB), while our Chord simulation works with stacks as small
as 16 KiB, for example. You can ensure that some actors have a specific
size by simply changing the value of this configuration item before
creating these actors. The :cpp:func:`simgrid::s4u::Engine::set_config` 
functions are handy for that.

This *setting is ignored* when using the thread factory (because there
is no way to modify the stack size with C++ system threads). Instead,
you should compile SimGrid and your application with
``-fsplit-stack``. Note that this compilation flag is not compatible
with the model checker right now.

The operating system should only allocate memory for the pages of the
stack which are actually used and you might not need to use this in
most cases. However, this setting is very important when using the
model checker (see :ref:`options_mc_perf`).

.. _cfg=contexts/guard-size:

Disabling Stack Guard Pages
...........................

**Option** ``contexts/guard-size`` **Default** 1 page in most case (0 pages on Windows or with MC)

Unless you use the threads context factory (see
:ref:`cfg=contexts/factory`), a stack guard page is usually used
which prevents the stack of a given actor from overflowing on another
stack. But the performance impact may become prohibitive when the
amount of actors increases.  The option ``contexts/guard-size`` is the
number of stack guard pages used.  By setting it to 0, no guard pages
will be used: in this case, you should avoid using small stacks (with
:ref:`contexts/stack-size <cfg=contexts/stack-size>`) as the stack
will silently overflow on other parts of the memory.

When no stack guard page is created, stacks may then silently overflow
on other parts of the memory if their size is too small for the
application.

.. _cfg=contexts/nthreads:
.. _cfg=contexts/synchro:

Running User Code in Parallel
.............................

Parallel execution of the user code is only considered stable in
SimGrid v3.7 and higher, and mostly for MSG simulations. SMPI
simulations may well fail in parallel mode. It is described in
`INRIA RR-7653 <http://hal.inria.fr/inria-00602216/>`_.

If you are using the **ucontext** or **raw** context factories, you can
request to execute the user code in parallel. Several threads are
launched, each of them handling the same number of user contexts at each
run. To activate this, set the ``contexts/nthreads`` item to the amount
of cores that you have in your computer (or lower than 1 to have the
amount of cores auto-detected).

When parallel execution is activated, you can choose the
synchronization schema used with the ``contexts/synchro`` item,
which value is either:

 - **futex:** ultra optimized synchronisation schema, based on futexes
   (fast user-mode mutexes), and thus only available on Linux systems.
   This is the default mode when available.
 - **posix:** slow but portable synchronisation using only POSIX
   primitives.
 - **busy_wait:** not really a synchronisation: the worker threads
   constantly request new contexts to execute. It should be the most
   efficient synchronisation schema, but it loads all the cores of
   your machine for no good reason. You probably prefer the other less
   eager schemas.

Configuring the Tracing
-----------------------

The :ref:`tracing subsystem <outcomes_vizu>` can be configured in
several different ways depending on the used interface (S4U, SMPI, SimDag)
and the kind of traces that needs to be obtained. See the
:ref:`Tracing Configuration Options subsection
<tracing_tracing_options>` for a full description of each
configuration option.

We detail here a simple way to get the traces working for you, even if
you never used the tracing API.


- Any SimGrid-based simulator (MSG, SimDag, SMPI, ...) and raw traces:

  .. code-block:: shell

     --cfg=tracing:yes --cfg=tracing/uncategorized:yes

  The first parameter activates the tracing subsystem, and the second
  tells it to trace host and link utilization (without any
  categorization).

- MSG or SimDag-based simulator and categorized traces (you need to
  declare categories and classify your tasks according to them) 

  .. code-block:: shell

     --cfg=tracing:yes --cfg=tracing/categorized:yes

  The first parameter activates the tracing subsystem, and the second
  tells it to trace host and link categorized utilization.

- SMPI simulator and traces for a space/time view:

  .. code-block:: shell

     smpirun -trace ...

  The `-trace` parameter for the smpirun script runs the simulation
  with ``--cfg=tracing:yes --cfg=tracing/smpi:yes``. Check the
  smpirun's `-help` parameter for additional tracing options.

Sometimes you might want to put additional information on the trace to
correctly identify them later, or to provide data that can be used to
reproduce an experiment. You have two ways to do that:

- Add a string on top of the trace file as comment:

  .. code-block:: shell

     --cfg=tracing/comment:my_simulation_identifier

- Add the contents of a textual file on top of the trace file as comment:

  .. code-block:: shell

     --cfg=tracing/comment-file:my_file_with_additional_information.txt

Please, use these two parameters (for comments) to make reproducible
simulations. For additional details about this and all tracing
options, check See the :ref:`tracing_tracing_options`.

Configuring MSG
---------------

.. _cfg=msg/debug-multiple-use:

Debugging MSG Code
..................

**Option** ``msg/debug-multiple-use`` **Default:** off

Sometimes your application may try to send a task that is still being
executed somewhere else, making it impossible to send this task. However,
for debugging purposes, one may want to know what the other host is/was
doing. This option shows a backtrace of the other process.

Configuring SMPI
----------------

The SMPI interface provides several specific configuration items.
These are not easy to see, since the code is usually launched through the
``smiprun`` script directly.

.. _cfg=smpi/host-speed:
.. _cfg=smpi/cpu-threshold:
.. _cfg=smpi/simulate-computation:

Automatic Benchmarking of SMPI Code
...................................

In SMPI, the sequential code is automatically benchmarked, and these
computations are automatically reported to the simulator. That is to
say that if you have a large computation between a ``MPI_Recv()`` and
a ``MPI_Send()``, SMPI will automatically benchmark the duration of
this code, and create an execution task within the simulator to take
this into account. For that, the actual duration is measured on the
host machine and then scaled to the power of the corresponding
simulated machine. The variable ``smpi/host-speed`` allows one to
specify the computational speed of the host machine (in flop/s by
default) to use when scaling the execution times.

The default value is ``smpi/host-speed=20kf`` (= 20,000 flop/s). This
is probably underestimated for most machines, leading SimGrid to
overestimate the amount of flops in the execution blocks that are
automatically injected in the simulator. As a result, the execution
time of the whole application will probably be overestimated until you
use a realistic value.

When the code consists of numerous consecutive MPI calls, the
previous mechanism feeds the simulation kernel with numerous tiny
computations. The ``smpi/cpu-threshold`` item becomes handy when this
impacts badly on the simulation performance. It specifies a threshold (in
seconds) below which the execution chunks are not reported to the
simulation kernel (default value: 1e-6).

.. note:: The option ``smpi/cpu-threshold`` ignores any computation
   time spent below this threshold. SMPI does not consider the
   `amount of time` of these computations; there is no offset for
   this. Hence, a value that is too small, may lead to unreliable
   simulation results.

In some cases, however, one may wish to disable simulation of
the computation of an application. This is the case when SMPI is used not to
simulate an MPI application, but instead an MPI code that performs
"live replay" of another MPI app (e.g., ScalaTrace's replay tool, or
various on-line simulators that run an app at scale). In this case the
computation of the replay/simulation logic should not be simulated by
SMPI. Instead, the replay tool or on-line simulator will issue
"computation events", which correspond to the actual MPI simulation
being replayed/simulated. At the moment, these computation events can
be simulated using SMPI by calling internal smpi_execute*() functions.

To disable the benchmarking/simulation of a computation in the simulated
application, the variable ``smpi/simulate-computation`` should be set
to **no**.  This option just ignores the timings in your simulation; it
still executes the computations itself. If you want to stop SMPI from
doing that, you should check the SMPI_SAMPLE macros, documented in
Section :ref:`SMPI_use_faster`.

+------------------------------------+-------------------------+-----------------------------+
|  Solution                          | Computations executed?  | Computations simulated?     |
+====================================+=========================+=============================+
| --cfg=smpi/simulate-computation:no | Yes                     | Never                       |
+------------------------------------+-------------------------+-----------------------------+
| --cfg=smpi/cpu-threshold:42        | Yes, in all cases       | If it lasts over 42 seconds |
+------------------------------------+-------------------------+-----------------------------+
| SMPI_SAMPLE() macro                | Only once per loop nest | Always                      |
+------------------------------------+-------------------------+-----------------------------+

.. _cfg=smpi/comp-adjustment-file:

Slow-down or speed-up parts of your code
........................................

**Option** ``smpi/comp-adjustment-file:`` **Default:** unset

This option allows you to pass a file that contains two columns: The
first column defines the section that will be subject to a speedup;
the second column is the speedup. For instance:

.. code-block:: shell

  "start:stop","ratio"
  "exchange_1.f:30:exchange_1.f:130",1.18244559422142

The first line is the header - you must include it.  The following
line means that the code between two consecutive MPI calls on line 30
in exchange_1.f and line 130 in exchange_1.f should receive a speedup
of 1.18244559422142. The value for the second column is therefore a
speedup, if it is larger than 1 and a slowdown if it is smaller
than 1. Nothing will be changed if it is equal to 1.

Of course, you can set any arbitrary filenames you want (so the start
and end don't have to be in the same file), but be aware that this
mechanism only supports `consecutive calls!`

Please note that you must pass the ``-trace-call-location`` flag to
smpicc or smpiff, respectively. This flag activates some internal
macro definitions that help with obtaining the call location.

.. _cfg=smpi/bw-factor:

Bandwidth Factors
.................

**Option** ``smpi/bw-factor``
|br| **Default:** 65472:0.940694;15424:0.697866;9376:0.58729;5776:1.08739;3484:0.77493;1426:0.608902;732:0.341987;257:0.338112;0:0.812084

The possible throughput of network links is often dependent on the
message sizes, as protocols may adapt to different message sizes. With
this option, a series of message sizes and factors are given, helping
the simulation to be more realistic. For instance, the current default
value means that messages with size 65472 bytes and more will get a total of
MAX_BANDWIDTH*0.940694, messages of size 15424 to 65471 will get
MAX_BANDWIDTH*0.697866, and so on (where MAX_BANDWIDTH denotes the
bandwidth of the link).

An experimental script to compute these factors is available online. See
https://framagit.org/simgrid/platform-calibration/
https://simgrid.org/contrib/smpi-saturation-doc.html

.. _cfg=smpi/display-timing:

Reporting Simulation Time
.........................

**Option** ``smpi/display-timing`` **Default:** 0 (false)

Most of the time, you run MPI code with SMPI to compute the time it
would take to run it on a platform. But since the code is run through
the ``smpirun`` script, you don't have any control on the launcher
code, making it difficult to report the simulated time when the
simulation ends. If you enable the ``smpi/display-timing`` item,
``smpirun`` will display this information when the simulation
ends.
SMPI will also display information about the amout of real time spent
in application code and in SMPI internals, to provide hints about the
need to use sampling to reduce simulation time.

.. _cfg=smpi/display-allocs:

Reporting memory allocations
............................

**Option** ``smpi/display-allocs`` **Default:** 0 (false)

SMPI intercepts malloc and calloc calls performed inside the running
application, if it wasn't compiled with SMPI_NO_OVERRIDE_MALLOC.
With this option, SMPI will show at the end of execution the amount of
memory allocated through these calls, and locate the most expensive one.
This helps finding the targets for manual memory sharing, or the threshold
to use for smpi/auto-shared-malloc-thresh option (see :ref:`cfg=smpi/auto-shared-malloc-thresh`).

.. _cfg=smpi/keep-temps:

Keeping temporary files after simulation
........................................

**Option** ``smpi/keep-temps`` **default:** 0 (false)

SMPI usually generates a lot of temporary files that are cleaned after
use. This option requests to preserve them, for example to debug or
profile your code. Indeed, the binary files are removed very early
under the dlopen privatization schema, which tends to fool the
debuggers.

.. _cfg=smpi/lat-factor:

Latency factors
...............

**Option** ``smpi/lat-factor`` |br|
**default:** 65472:11.6436;15424:3.48845;9376:2.59299;5776:2.18796;3484:1.88101;1426:1.61075;732:1.9503;257:1.95341;0:2.01467

The motivation and syntax for this option is identical to the motivation/syntax
of :ref:`cfg=smpi/bw-factor`.

There is an important difference, though: While smpi/bw-factor `reduces` the
actual bandwidth (i.e., values between 0 and 1 are valid), latency factors
increase the latency, i.e., values larger than or equal to 1 are valid here.

.. _cfg=smpi/papi-events:

Trace hardware counters with PAPI
.................................

**Option** ``smpi/papi-events`` **default:** unset

When the PAPI support is compiled into SimGrid, this option takes the
names of PAPI counters and adds their respective values to the trace
files (See Section :ref:`tracing_tracing_options`).

.. warning::

   This feature currently requires superuser privileges, as registers
   are queried.  Only use this feature with code you trust! Call
   smpirun for instance via ``smpirun -wrapper "sudo "
   <your-parameters>`` or run ``sudo sh -c "echo 0 >
   /proc/sys/kernel/perf_event_paranoid"`` In the later case, sudo
   will not be required.

It is planned to make this feature available on a per-process (or per-thread?) basis.
The first draft, however, just implements a "global" (i.e., for all processes) set
of counters, the "default" set.

.. code-block:: shell

   --cfg=smpi/papi-events:"default:PAPI_L3_LDM:PAPI_L2_LDM"

.. _cfg=smpi/privatization:

Automatic Privatization of Global Variables
...........................................

**Option** ``smpi/privatization`` **default:** "dlopen" (when using smpirun)

MPI executables are usually meant to be executed in separate
processes, but SMPI is executed in only one process. Global variables
from executables will be placed in the same memory region and shared
between processes, causing intricate bugs.  Several options are
possible to avoid this, as described in the main `SMPI publication
<https://hal.inria.fr/hal-01415484>`_ and in the :ref:`SMPI
documentation <SMPI_what_globals>`. SimGrid provides two ways of
automatically privatizing the globals, and this option allows one to
choose between them.

  - **no** (default when not using smpirun): Do not automatically
    privatize variables.  Pass ``-no-privatize`` to smpirun to disable
    this feature.
  - **dlopen** or **yes** (default when using smpirun): Link multiple
    times against the binary.
  - **mmap** (slower, but maybe somewhat more stable):
    Runtime automatic switching of the data segments.

.. warning::
   This configuration option cannot be set in your platform file. You can only
   pass it as an argument to smpirun.

.. _cfg=smpi/privatize-libs:

Automatic privatization of global variables inside external libraries
.....................................................................

**Option** ``smpi/privatize-libs`` **default:** unset

**Linux/BSD only:** When using dlopen (default) privatization,
privatize specific shared libraries with internal global variables, if
they can't be linked statically.  For example libgfortran is usually
used for Fortran I/O and indexes in files can be mixed up.

Multiple libraries can be given, semicolon separated.

This configuration option can only use either full paths to libraries,
or full names.  Check with ldd the name of the library you want to
use.  For example:

.. code-block:: shell

   ldd allpairf90
      ...
      libgfortran.so.3 => /usr/lib/x86_64-linux-gnu/libgfortran.so.3 (0x00007fbb4d91b000)
      ...

Then you can use ``--cfg=smpi/privatize-libs:libgfortran.so.3``
or ``--cfg=smpi/privatize-libs:/usr/lib/x86_64-linux-gnu/libgfortran.so.3``,
but not ``libgfortran`` nor ``libgfortran.so``.

.. _cfg=smpi/send-is-detached-thresh:

Simulating MPI detached send
............................

**Option** ``smpi/send-is-detached-thresh`` **default:** 65536

This threshold specifies the size in bytes under which the send will
return immediately. This is different from the threshold detailed in
:ref:`cfg=smpi/async-small-thresh` because the message is not
really sent when the send is posted. SMPI still waits for the
corresponding receive to be posted, in order to perform the communication
operation.

.. _cfg=smpi/coll-selector:

Simulating MPI collective algorithms
....................................

**Option** ``smpi/coll-selector`` **Possible values:** naive (default), ompi, mpich

SMPI implements more than 100 different algorithms for MPI collective
communication, to accurately simulate the behavior of most of the
existing MPI libraries. The ``smpi/coll-selector`` item can be used to
select the decision logic either of the OpenMPI or the MPICH libraries. (By
default SMPI uses naive version of collective operations.)

Each collective operation can be manually selected with a
``smpi/collective_name:algo_name``. Available algorithms are listed in
:ref:`SMPI_use_colls`.

.. TODO:: All available collective algorithms will be made available
          via the ``smpirun --help-coll`` command.

Add a barrier in MPI_Finalize
.............................

.. _cfg=smpi/finalization-barrier:

**Option** ``smpi/finalization-barrier`` **default:** off

By default, SMPI processes are destroyed as soon as soon as their code ends,
so after a successful MPI_Finalize call returns. In some rare cases, some data
might have been attached to MPI objects still active in the remaining processes,
and can be destroyed eagerly by the finished process.
If your code shows issues at finalization, such as segmentation fault, triggering
this option will add an explicit MPI_Barrier(MPI_COMM_WORLD) call inside the
MPI_Finalize, so that all processes will terminate at almost the same point.
It might affect the total timing by the cost of a barrier.

.. _cfg=smpi/errors-are-fatal:

**Option** ``smpi/errors-are-fatal`` **default:** on

By default, SMPI processes will crash if a MPI error code is returned. MPI allows
to explicitely set MPI_ERRORS_RETURN errhandler to avoid this behaviour. This flag
will turn on this behaviour by default (for all concerned types and errhandlers).
This can ease debugging by going after the first reported error.

.. _cfg=smpi/iprobe:

Inject constant times for MPI_Iprobe
....................................

**Option** ``smpi/iprobe`` **default:** 0.0001

The behavior and motivation for this configuration option is identical
with :ref:`smpi/test <cfg=smpi/test>`, but for the function
``MPI_Iprobe()``

.. _cfg=smpi/iprobe-cpu-usage:

Reduce speed for iprobe calls
.............................

**Option** ``smpi/iprobe-cpu-usage`` **default:** 1 (no change)

MPI_Iprobe calls can be heavily used in applications. To account
correctly for the energy that cores spend probing, it is necessary to
reduce the load that these calls cause inside SimGrid.

For instance, we measured a maximum power consumption of 220 W for a
particular application but only 180 W while this application was
probing. Hence, the correct factor that should be passed to this
option would be 180/220 = 0.81.

.. _cfg=smpi/init:

Inject constant times for MPI_Init
..................................

**Option** ``smpi/init`` **default:** 0

The behavior and motivation for this configuration option is identical
with :ref:`smpi/test <cfg=smpi/test>`, but for the function ``MPI_Init()``.

.. _cfg=smpi/ois:

Inject constant times for MPI_Isend()
.....................................

**Option** ``smpi/ois``

The behavior and motivation for this configuration option is identical
with :ref:`smpi/os <cfg=smpi/os>`, but for the function ``MPI_Isend()``.

.. _cfg=smpi/os:

Inject constant times for MPI_send()
....................................

**Option** ``smpi/os``

In several network models such as LogP, send (MPI_Send, MPI_Isend) and
receive (MPI_Recv) operations incur costs (i.e., they consume CPU
time). SMPI can factor these costs in as well, but the user has to
configure SMPI accordingly as these values may vary by machine.  This
can be done by using ``smpi/os`` for MPI_Send operations; for MPI_Isend
and MPI_Recv, use ``smpi/ois`` and ``smpi/or``, respectively. These work
exactly as ``smpi/ois``.

This item can consist of multiple sections; each section takes three
values, for example ``1:3:2;10:5:1``.  The sections are divided by ";"
so this example contains two sections.  Furthermore, each section
consists of three values.

1. The first value denotes the minimum size in bytes for this section to take effect;
   read it as "if message size is greater than this value (and other section has a larger
   first value that is also smaller than the message size), use this".
   In the first section above, this value is "1".

2. The second value is the startup time; this is a constant value that will always
   be charged, no matter what the size of the message. In the first section above,
   this value is "3".

3. The third value is the `per-byte` cost. That is, it is charged for every
   byte of the message (incurring cost messageSize*cost_per_byte)
   and hence accounts also for larger messages. In the first
   section of the example above, this value is "2".

Now, SMPI always checks which section it should use for a given
message; that is, if a message of size 11 is sent with the
configuration of the example above, only the second section will be
used, not the first, as the first value of the second section is
closer to the message size. Hence, when ``smpi/os=1:3:2;10:5:1``, a
message of size 11 incurs the following cost inside MPI_Send:
``5+11*1`` because 5 is the startup cost and 1 is the cost per byte.

Note that the order of sections can be arbitrary; they will be ordered internally.

.. _cfg=smpi/or:

Inject constant times for MPI_Recv()
....................................

**Option** ``smpi/or``

The behavior and motivation for this configuration option is identical
with :ref:`smpi/os <cfg=smpi/os>`, but for the function ``MPI_Recv()``.

.. _cfg=smpi/test:
.. _cfg=smpi/grow-injected-times:

Inject constant times for MPI_Test
..................................

**Option** ``smpi/test`` **default:** 0.0001

By setting this option, you can control the amount of time a process
sleeps when MPI_Test() is called; this is important, because SimGrid
normally only advances the time while communication is happening and
thus, MPI_Test will not add to the time, resulting in deadlock if it is
used as a break-condition as in the following example:

.. code-block:: cpp

   while(!flag) {
       MPI_Test(request, flag, status);
       ...
   }

To speed up execution, we use a counter to keep track of how often we
checked if the handle is now valid or not. Hence, we actually
use counter*SLEEP_TIME, that is, the time MPI_Test() causes the
process to sleep increases linearly with the number of previously
failed tests. This behavior can be disabled by setting
``smpi/grow-injected-times`` to **no**. This will also disable this
behavior for MPI_Iprobe.

.. _cfg=smpi/shared-malloc:
.. _cfg=smpi/shared-malloc-hugepage:

Factorize malloc()s
...................

**Option** ``smpi/shared-malloc`` **Possible values:** global (default), local

If your simulation consumes too much memory, you may want to modify
your code so that the working areas are shared by all MPI ranks. For
example, in a block-cyclic matrix multiplication, you will only
allocate one set of blocks, and all processes will share them.
Naturally, this will lead to very wrong results, but this will save a
lot of memory. So this is still desirable for some studies. For more on
the motivation for that feature, please refer to the `relevant section
<https://simgrid.github.io/SMPI_CourseWare/topic_understanding_performance/matrixmultiplication>`_
of the SMPI CourseWare (see Activity #2.2 of the pointed
assignment). In practice, change the calls for malloc() and free() into
SMPI_SHARED_MALLOC() and SMPI_SHARED_FREE().

SMPI provides two algorithms for this feature. The first one, called 
``local``, allocates one block per call to SMPI_SHARED_MALLOC()
(each call site gets its own block) ,and this block is shared
among all MPI ranks.  This is implemented with the shm_* functions
to create a new POSIX shared memory object (kept in RAM, in /dev/shm)
for each shared block.

With the ``global`` algorithm, each call to SMPI_SHARED_MALLOC()
returns a new address, but it only points to a shadow block: its memory
area is mapped on a 1 MiB file on disk. If the returned block is of size
N MiB, then the same file is mapped N times to cover the whole block.
At the end, no matter how many times you call SMPI_SHARED_MALLOC, this will
only consume 1 MiB in memory.

You can disable this behavior and come back to regular mallocs (for
example for debugging purposes) using ``no`` as a value.

If you want to keep private some parts of the buffer, for instance if these
parts are used by the application logic and should not be corrupted, you
can use SMPI_PARTIAL_SHARED_MALLOC(size, offsets, offsets_count). For example:

.. code-block:: cpp

   mem = SMPI_PARTIAL_SHARED_MALLOC(500, {27,42 , 100,200}, 2);

This will allocate 500 bytes to mem, such that mem[27..41] and
mem[100..199] are shared while other area remain private.

Then, it can be deallocated by calling SMPI_SHARED_FREE(mem).

When smpi/shared-malloc:global is used, the memory consumption problem
is solved, but it may induce too much load on the kernel's pages table.
In this case, you should use huge pages so that the kernel creates only one
entry per MB of malloced data instead of one entry per 4 kB.
To activate this, you must mount a hugetlbfs on your system and allocate
at least one huge page:

.. code-block:: shell

    mkdir /home/huge
    sudo mount none /home/huge -t hugetlbfs -o rw,mode=0777
    sudo sh -c 'echo 1 > /proc/sys/vm/nr_hugepages' # echo more if you need more

Then, you can pass the option
``--cfg=smpi/shared-malloc-hugepage:/home/huge`` to smpirun to
actually activate the huge page support in shared mallocs.

.. _cfg=smpi/auto-shared-malloc-thresh:

Automatically share allocations
...............................

**Option** ``smpi/auto-shared-malloc-thresh:`` **Default:** 0 (false)
   This value in bytes represents the size above which all allocations
   will be "shared" by default (as if they were performed through
   SMPI_SHARED_MALLOC macros). Default = 0 = disabled feature.
   The value must be carefully chosen to only select data buffers which
   will not modify execution path or cause crash if their content is false.
   Option :ref:`cfg=smpi/display-allocs` can be used to locate the largest
   allocation detected in a run, and provide a good starting threshold.
   Note : malloc, calloc and free are overridden by smpicc/cxx by default.
   This can cause some troubles if codes are already overriding these. If this
   is the case, defining SMPI_NO_OVERRIDE_MALLOC in the compilation flags can
   help, but will make this feature unusable.

.. _cfg=smpi/wtime:

Inject constant times for MPI_Wtime, gettimeofday and clock_gettime
...................................................................

**Option** ``smpi/wtime`` **default:** 10 ns

This option controls the amount of (simulated) time spent in calls to
MPI_Wtime(), gettimeofday() and clock_gettime(). If you set this value
to 0, the simulated clock is not advanced in these calls, which leads
to issues if your application contains such a loop:

.. code-block:: cpp

   while(MPI_Wtime() < some_time_bound) {
        /* some tests, with no communication nor computation */
   }

When the option smpi/wtime is set to 0, the time advances only on
communications and computations. So the previous code results in an
infinite loop: the current [simulated] time will never reach
``some_time_bound``.  This infinite loop is avoided when that option
is set to a small value, as it is by default since SimGrid v3.21.

Note that if your application does not contain any loop depending on
the current time only, then setting this option to a non-zero value
will slow down your simulations by a tiny bit: the simulation loop has
to be broken out of and reset each time your code asks for the current time.
If the simulation speed really matters to you, you can avoid this
extra delay by setting smpi/wtime to 0.

.. _cfg=smpi/list-leaks:

Report leaked MPI objects
.........................

**Option** ``smpi/list-leaks`` **default:** 0

This option controls whether to report leaked MPI objects.
The parameter is the number of leaks to report.

Other Configurations
--------------------

.. _cfg=debug/clean-atexit:

Cleanup at Termination
......................

**Option** ``debug/clean-atexit`` **default:** on

If your code is segfaulting during its finalization, it may help to
disable this option to request that SimGrid not attempt any cleanups at
the end of the simulation. Since the Unix process is ending anyway,
the operating system will wipe it all.

.. _cfg=path:

Search Path
...........

**Option** ``path`` **default:** . (current dir)

It is possible to specify a list of directories to search in for the
trace files (see :ref:`pf_trace`) by using this configuration
item. To add several directory to the path, set the configuration
item several times, as in ``--cfg=path:toto --cfg=path:tutu``

.. _cfg=debug/breakpoint:

Set a Breakpoint
................

**Option** ``debug/breakpoint`` **default:** unset

This configuration option sets a breakpoint: when the simulated clock
reaches the given time, a SIGTRAP is raised.  This can be used to stop
the execution and get a backtrace with a debugger.

It is also possible to set the breakpoint from inside the debugger, by
writing in global variable simgrid::simix::breakpoint. For example,
with gdb:

.. code-block:: shell

   set variable simgrid::simix::breakpoint = 3.1416

.. _cfg=debug/verbose-exit:

Behavior on Ctrl-C
..................

**Option** ``debug/verbose-exit`` **default:** on

By default, when Ctrl-C is pressed, the status of all existing actors
is displayed before exiting the simulation. This is very useful to
debug your code, but it can become troublesome if you have many
actors. Set this configuration item to **off** to disable this
feature.

.. _cfg=exception/cutpath:

Truncate local path from exception backtrace
............................................

**Option** ``exception/cutpath`` **default:** off

This configuration option is used to remove the path from the
backtrace shown when an exception is thrown. This is mainly useful for
the tests: the full file path would makes the tests non-reproducible because
the paths of source files depend of the build settings. That would
break most of the tests since their output is continually compared.

.. _logging_config:

Logging configuration
---------------------

As introduced in :ref:`outcome_logs`, the SimGrid logging mechanism allows to configure at runtime the messages that should be displayed and those that should be omitted. Each
message produced in the code is given a category (denoting its topic) and a priority. Then at runtime, each category is given a threshold (only messages of priority higher than
that threshold are displayed), a layout (deciding how the messages in this category are formatted), and an appender (deciding what to do with the message: either print on stderr or
to a file).

This section explains how to configure this logging features. You can also refer to the documentation of the :ref:`programmer's interface <logging_prog>`, that allows to produce
messages from your code.

Most of the time, the logging mechanism is configured at runtime using the ``--log`` command-line argument, even if you can also use :c:func:`xbt_log_control_set()` to control it from
your program. To pass configure more than one setting, you can either pass several ``--log`` arguments, or separate your settings with spaces, that must be quoted accordingly. In
practice, the following is equivalent to the above settings: ``--log=root.thresh:error --log=s4u_host.thresh:debug``.

If you want to specify more than one setting, you can either pass several ``--log`` argument to your program as above, or separate them with spaces. In this case, you want to quote
your settings, as in ``--log="root.thresh:error s4u_host.thresh:debug"``. The parameters are interpreted in order, from left to right.


Threshold configuration
.......................

The keyword ``threshold`` controls which logging event will get displayed in a given category. For example, ``--log=root.threshold:debug`` displays *every* message produced in the
``root`` category and its subcategories (i.e., every message produced -- this is *extremely* verbose), while ``--log=root.thres:critical`` turns almost everything off. As you can
see, ``threshold`` can be abbreviated here.

Existing thresholds:

 - ``trace`` some functions display a message at this level when entering or returning
 - ``debug`` output that is mostly useful when debugging the corresponding module.
 - ``verbose`` verbose output that is only mildly interesting and can easily be ignored
 - ``info`` usual output (this is the default threshold of all categories)
 - ``warning`` minor issue encountered
 - ``error`` issue encountered
 - ``critical`` major issue encountered, such as assertions failures

.. _log/fmt:

Format configuration
....................

The keyword ``fmt`` controls the layout (the format) of a logging category. For example, ``--log=root.fmt:%m`` reduces the output to the user-message only, removing any decoration such
as the date, or the actor ID, everything. Existing format directives:

 - %%: the % char
 - %n: line separator (LOG4J compatible)
 - %e: plain old space (SimGrid extension)

 - %m: user-provided message

 - %c: Category name (LOG4J compatible)
 - %p: Priority name (LOG4J compatible)

 - %h: Hostname (SimGrid extension)
 - %a: Actor name (SimGrid extension -- note that with SMPI this is the integer value of the process rank)
 - %i: Actor PID (SimGrid extension -- this is a 'i' as in 'i'dea)
 - %t: Thread "name" (LOG4J compatible -- actually the address of the thread in memory)

 - %F: file name where the log event was raised (LOG4J compatible)
 - %l: location where the log event was raised (LOG4J compatible, like '%%F:%%L' -- this is a l as in 'l'etter)
 - %L: line number where the log event was raised (LOG4J compatible)
 - %M: function name (LOG4J compatible -- called method name here of course).

 - %d: date (UNIX-like epoch)
 - %r: application age (time elapsed since the beginning of the application)


``--log=root.fmt:'[%h:%a:(%i) %r] %l: %m%n'`` gives you the default layout used for info messages while ``--log=root.fmt:'[%h:%a:(%i) %r] %l: [%c/%p] %m%n'`` gives you the default
layout for the other priorities (it adds the source code location). Also, the actor identification is omitted by the default layout for the messages coming directly from the
SimGrid kernel, so info messages are formatted with ``[%r] [%c/%p] %m%n`` in this case. When specifying the layout manually, such distinctions are currently impossible, and the
provided layout is used for every messages.

As with printf, you can specify the precision and width of the fields. For example, ``%.4r`` limits the date precision to four digits while ``%15h`` limits the host name to at most
15 chars.


If you want to have spaces in your log format, you should protect it. Otherwise, SimGrid will consider that this is a space-separated list of several parameters. But you should
also protect it from the shell that also splits command line arguments on spaces. At the end, you should use something such as ``--log="'root.fmt:%l: [%p/%c]: %m%n'"``. 
Another option is to use the ``%e`` directive for spaces, as in ``--log=root.fmt:%l:%e[%p/%c]:%e%m%n``. 

Category appender
.................

The keyword ``app`` controls the appended of a logging category. For example ``--log=root.app:file:mylogfile`` redirects every output to the file ``mylogfile``.

With the ``splitfile`` appender, a new file is created when the size of the output reaches the specified size. The format is ``--log=root.app:splitfile:<size>:<file name>``. For
example, ``--log=root.app:splitfile:500:mylog_%`` creates log files of at most 500 bytes, using the names ``mylog_0``, ``mylog_1``, ``mylog_2``, etc.

The ``rollfile`` appender uses one file only, but the file is emptied and recreated when its size reaches the specified maximum. For example, ``--log=root.app:rollfile:500:mylog``
ensures that the log file ``mylog`` will never overpass 500 bytes in size.

Any appender setup this way have its own layout format, that you may change afterward. When specifying a new appender, its additivity is set to false to prevent log event displayed
by this appender to "leak" to any other appender higher in the hierarchy. You can naturally change that if you want your messages to be displayed twice.

Category additivity
...................

The keyword ``add`` controls the additivity of a logging category. By default, the messages are only passed one appender only: the more specific, i.e. the first one found when
climbing the tree from the category in which they were produced. In Log4J parlance, it is said that the default additivity of appenders is false. If you change this setting to
``on`` (or ``yes`` or ``1``), the produced messages will also be passed to the upper appender.

Let's consider a more complex example: ``--log="root.app:file:all.log s4u.app:file:iface.log xbt.app:file:xbt.log xbt.add:yes``. Here, the logging of s4u will be sent to the
``iface.log`` file; the logging of the xbt toolbox will be sent to both the ``xbt.log`` file and the ``all.log`` file (because xbt additivity was enabled); and every other loggings
will only be sent to ``all.log``.

Other options
.............

``--help-logs`` displays a complete help message about logging in SimGrid.

``--help-log-categories`` displays the actual hierarchy of log categories for this binary.

``--log=no_loc`` hides the source locations (file names and line numbers) from the messages. This is useful to make tests reproducible.


.. |br| raw:: html

   <br />
