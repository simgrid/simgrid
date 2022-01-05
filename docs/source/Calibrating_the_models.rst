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

.. _models_calibration:

Calibrating the models
######################

The simulation models in SimGrid have been object of thorough validation/invalidation campaigns, but the default values may
not be appropriate for simulating a particular system.

They are configured
through parameters from the :ref:`XML platform description file <platform>` and parameters passed via
:ref:`--cfg=Item:Value command-line arguments <options>`. A simulator may also include any number of custom model
parameters that are used to instantiate particular simulated activities (e.g., a simulator developed with the S4U API
typically defines volumes of computation, communication, and time to pass to methods such as :cpp:func:`execute()
<simgrid::s4u::this_actor::execute>`, :cpp:func:`put() <simgrid::s4u::Mailbox::put>`, or :cpp:func:`sleep_for()
<simgrid::s4u::this_actor::sleep_for>`). Regardless of the potential accuracy of the simulation models, if they are
instantiated with unrealistic parameter values, then the simulation will be inaccurate. 

Given the above, an integral and crucial part of simulation-driven research is **simulation calibration**: the process by
which one picks simulation parameter values based on observed real-world executions so that simulated executions have high
accuracy. We then say that a simulator is "calibrated". Once a simulator is calibrated for a real-world system, it can be
used to simulate that system accurately. But it can also be used to simulate different but structurally similar systems
(e.g., different scales, different basic hardware characteristics, different application workloads) with high confidence.

Research conclusions derived from simulation results obtained with an uncalibrated simulator are questionable in terms of
their relevance for real-world systems. Unfortunately, because simulation calibration is often a painstaking process, is it
often not performed sufficiently thoroughly (or at all!). We strongly urge SimGrid users to perform simulation calibration.
Here is an example of a research publication in which the authors have calibrated their (SimGrid) simulators:
https://hal.inria.fr/hal-01523608

.. include:: tuto_network_calibration/network_calibration_tutorial.rst

.. include:: tuto_disk/analysis.irst
