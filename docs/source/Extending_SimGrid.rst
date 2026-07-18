.. _extending:

Extending SimGrid
##################

The :ref:`models <models>` and :ref:`interfaces <S4U_doc>` presented in the previous section are sensible defaults, but they
will not fit every study. Rather than forking SimGrid, you can extend or replace parts of its behavior from your own simulator's
code. This page is a map of the available mechanisms, from the lightest to the heaviest, so that you can pick the one that
matches your needs.

Discrete resource states (pstates)
**********************************

Any :cpp:class:`simgrid::s4u::Host` can be given a list of **pstates**, i.e., a set of numbered, discrete performance levels
(see :ref:`API_s4u_Host_dvfs`). This was designed for CPU frequency scaling (DVFS), but nothing prevents you from (ab)using it
for other purposes, such as giving a host a fictional "0 flop/s" pstate to model a boot or shutdown period, as shown in
:ref:`this modeling hint <howto_multicore>`. See the :ref:`DVFS example <s4u_ex_dvfs>` to get started.

Dynamic external variation (profiles)
*************************************

If a resource's characteristics change over simulated time independently of the activities using it (e.g., background load,
churn, hardware failures), attach it a **profile** instead of driving it by hand:

- A **speed profile** (:cpp:func:`simgrid::s4u::Host::set_speed_profile`, and the equivalent on links) makes the peak
  performance of a resource vary over time. See the :ref:`speed profile example <s4u_ex_speed_profile>`.
- A **state profile** (:cpp:func:`simgrid::s4u::Host::set_state_profile`, :cpp:func:`simgrid::s4u::Link::set_state_profile`)
  turns a resource on and off over time, which is convenient to model churn or failures. See :ref:`howto_churn` and the
  :ref:`state profile example <s4u_ex_platform_state_profile>`.

Both accept either a trace file (a list of timed events, see :ref:`pf_tag_host`) or a profile generated programmatically.

Correction factors
******************

A :cpp:class:`~simgrid::s4u::Disk` accepts a callback (:cpp:func:`simgrid::s4u::Disk::set_factor_cb`) that adjusts the disk's
effective speed based on the I/O operation being performed (e.g., its size or type), which is useful to reproduce non-trivial,
measured performance behaviors of real devices. See the :cpp:class:`Disk API reference <simgrid::s4u::Disk>` for the callback's
exact signature.

Non-linear sharing and concurrency limits
*****************************************

By default, when several activities compete for the same :cpp:class:`~simgrid::s4u::Disk`, :cpp:class:`~simgrid::s4u::Host`
or :cpp:class:`~simgrid::s4u::Link`, that resource's capacity is shared linearly between them. Two knobs let you deviate from
this default, on any of these three resource types:

- ``set_sharing_policy()`` (e.g. :cpp:func:`simgrid::s4u::Link::set_sharing_policy`) can take a callback that computes the
  resource's current speed as an arbitrary function of the number of concurrent activities, instead of the default linear
  sharing. SimGrid's own :ref:`Wi-Fi zones <models_wifi>` are implemented this way, and are a good reference to study.
- ``set_concurrency_limit()`` (e.g. :cpp:func:`simgrid::s4u::Link::set_concurrency_limit`) caps how many activities may use the
  resource at the same time; further activities are queued. See the :ref:`serialized communications example
  <s4u_ex_comm_serialize>`.

Plugins
*******

For anything that does not fit the above (e.g., collecting statistics, modeling energy consumption, reacting to arbitrary
simulation events), write a **plugin**: a piece of code, external to SimGrid's core, that attaches callbacks to signals fired by
the simulation kernel and can store its own state on any actor, activity, or resource. This is how most of SimGrid's own
optional features (energy, host load, batteries...) are implemented. See the dedicated :ref:`plugins` page.

Replacing a model entirely
**************************

When the mechanisms above are not enough, a resource model can be swapped for another simulator:

- The network model can be entirely replaced by the packet-level `ns-3 <https://www.nsnam.org/>`_ simulator; see
  :ref:`models_ns3`.
- Any model can be co-simulated with an external `FMI <https://fmi-standard.org/>`_ model (e.g. to couple SimGrid with a
  power-grid or building simulator); see :ref:`models_fmi`.
- Writing your own model is currently not possible without modifying the kernel directly. Please talk to us about your needs so
  that we help you modifying the codebase if possible.