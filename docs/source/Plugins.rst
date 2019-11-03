.. _plugins:

SimGrid Plugins
###############

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("PluginsBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

You can extend SimGrid without modifying it, thanks to our plugin
mechanism. This page describes how to write your own plugin, and
documents some of the plugins distributed with SimGrid:

  - :ref:`Host Energy <plugin_host_energy>`: models the energy dissipation of the compute units.
  - :ref:`Link Energy <plugin_link_energy>`: models the energy dissipation of the network.
  - :ref:`Host Load <plugin_host_load>`: monitors the load of the compute units.

You can activate these plugins with the :ref:`--cfg=plugin <cfg=plugin>` command
line option, for example with ``--cfg=plugin:host_energy``. You can get the full
list of existing plugins with ``--cfg=plugin:help``.

Defining a Plugin
*****************

A plugin can get some additional code executed within the SimGrid
kernel, and attach the data needed by that code to the SimGrid
objects. 

The host load plugin in 
`src/plugins/host_load.cpp <https://framagit.org/simgrid/simgrid/tree/master/src/plugins/host_load.cpp>`_
constitutes a good introductory example. It defines a class
```HostLoad``` that is meant to be attached to each host. This class
contains a ```EXTENSION_ID``` field that is mandatory to our extension
mechanism. Then, the function ```sg_host_load_plugin_init```
initializes the plugin. It first calls
:cpp:func:`simgrid::s4u::Host::extension_create()` to register its
extension to the ```s4u::Host``` objects, and then attaches some
callbacks to signals.

You can attach your own extension to most kinds of s4u object:
:cpp:class:`Actors <simgrid::s4u::Actor>`,
:cpp:class:`Disks <simgrid::s4u::Disk>`,
:cpp:class:`Hosts <simgrid::s4u::Host>` and
:cpp:class:`Links <simgrid::s4u::Link>`. If you need to extend another
kind of objects, please let us now.

Partial list of existing signals in s4u:

- :cpp:member:`Actor::on_creation <simgrid::s4u::Actor::on_creation>`
  :cpp:member:`Actor::on_suspend <simgrid::s4u::Actor::on_suspend>`
  :cpp:member:`Actor::on_resume <simgrid::s4u::Actor::on_resume>`
  :cpp:member:`Actor::on_sleep <simgrid::s4u::Actor::on_sleep>`
  :cpp:member:`Actor::on_wake_up <simgrid::s4u::Actor::on_wake_up>`
  :cpp:member:`Actor::on_migration_start <simgrid::s4u::Actor::on_migration_start>`
  :cpp:member:`Actor::on_migration_end <simgrid::s4u::Actor::on_migration_end>`
  :cpp:member:`Actor::on_termination <simgrid::s4u::Actor::on_termination>`
  :cpp:member:`Actor::on_destruction <simgrid::s4u::Actor::on_destruction>`
- :cpp:member:`Comm::on_sender_start <simgrid::s4u::Comm::on_sender_start>`
  :cpp:member:`Comm::on_receiver_start <simgrid::s4u::Comm::on_receiver_start>`
  :cpp:member:`Comm::on_completion <simgrid::s4u::Comm::on_completion>`
- :cpp:member:`Engine::on_platform_creation <simgrid::s4u::Engine::on_platform_creation>`
  :cpp:member:`Engine::on_platform_created <simgrid::s4u::Engine::on_platform_created>`
  :cpp:member:`Engine::on_time_advance <simgrid::s4u::Engine::on_time_advance>`
  :cpp:member:`Engine::on_simulation_end <simgrid::s4u::Engine::on_simulation_end>`
  :cpp:member:`Engine::on_deadlock <simgrid::s4u::Engine::on_deadlock>`
- :cpp:member:`Exec::on_start <simgrid::s4u::Exec::on_start>`
  :cpp:member:`Exec::on_completion <simgrid::s4u::Exec::on_completion>`
- :cpp:member:`Host::on_creation <simgrid::s4u::Host::on_creation>`
  :cpp:member:`Host::on_destruction <simgrid::s4u::Host::on_destruction>`
  :cpp:member:`Host::on_state_change <simgrid::s4u::Host::on_state_change>`
  :cpp:member:`Host::on_speed_change <simgrid::s4u::Host::on_speed_change>`
- :cpp:member:`Link::on_creation <simgrid::s4u::Link::on_creation>`
  :cpp:member:`Link::on_destruction <simgrid::s4u::Link::on_destruction>`
  :cpp:member:`Link::on_state_change <simgrid::s4u::Link::on_state_change>`
  :cpp:member:`Link::on_speed_change <simgrid::s4u::Link::on_bandwidth_change>`
  :cpp:member:`Link::on_communicate <simgrid::s4u::Link::on_communicate>`
  :cpp:member:`Link::on_communication_state_change <simgrid::s4u::Link::on_communication_state_change>`
- :cpp:member:`Netzone::on_creation <simgrid::s4u::Netzone::on_creation>`
  :cpp:member:`Netzone::on_seal <simgrid::s4u::Netzone::on_seal>`
  :cpp:member:`Netzone::on_route_creation <simgrid::s4u::Netzone::on_route_creation>`
- :cpp:member:`VirtualMachine::on_start <simgrid::s4u::VirtualMachine::on_start>`
  :cpp:member:`VirtualMachine::on_started <simgrid::s4u::VirtualMachine::on_started>`
  :cpp:member:`VirtualMachine::on_suspend <simgrid::s4u::VirtualMachine::on_suspend>`
  :cpp:member:`VirtualMachine::on_resume <simgrid::s4u::VirtualMachine::on_resume>`
  :cpp:member:`VirtualMachine::on_migration_start <simgrid::s4u::VirtualMachine::on_migration_start>`
  :cpp:member:`VirtualMachine::on_migration_end <simgrid::s4u::VirtualMachine::on_migration_end>`

Existing Plugins
****************

Only the major plugins are described here. Please check in src/plugins
to explore the other ones.

.. _plugin_host_energy:

Host Energy Plugin
==================

.. doxygengroup:: Plugin_host_energy

.. _plugin_link_energy:

Link Energy Plugin
==================

.. doxygengroup:: Plugin_link_energy

.. _plugin_host_load:

Host Load Plugin
================

.. doxygengroup:: Plugin_host_load

..  LocalWords:  SimGrid
