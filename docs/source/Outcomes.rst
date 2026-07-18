.. _outcomes:

Simulation outcomes
###################

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     //var elem=document.querySelector("#TOC").contentDocument.getElementById("DeployBox")
     //elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

.. _outcome_logs:

Textual logging
***************

Using ``printf`` or ``println`` to display information is possible, but quickly unpractical, as the logs of all processes get intermixed in your program's output. Instead, you
should use SimGrid's logging facilities, that are inspired from `Log4J <https://en.wikipedia.org/wiki/Log4j>`_. This way, you can filter the messages at runtime, based on their
severity and their topic. There  is four main concepts in SimGrid's logging mechanism:

The **category** of a message represents its topic. These categories are organized as a hierarchy, loosely corresponding to SimGrid's modules architecture. :ref:`Existing categories
<logging_categories>` are documented online, but some of them may be disabled depending on the compilation options. Use ``--help-log-categories`` on the command line to see
the categories actually provided a given simulator.

The message **priority** represents its severity. It can be one of ``trace``, ``debug``, ``verb``, ``info``, ``warn``, ``error`` and ``critical``. Every category has a configured
threshold, and only the messages with a higher severity are displayed (the others are not even evaluated). For example, you may want to see every debugging message of the Host
handling, while filtering out every other messages that are of a lesser priority than "error". For that, use the following command line argument:
``--log=root.thresh:error --log=s4u_host.thresh:debug``

You can also change the **layout** used to format the messages, using format directives that are similar to the *printf* ones: ``%r`` prints the time elapsed since the beginning of
the simulation; ``%a`` gives the actor name, etc. Many such directives :ref:`are available <log/fmt>`. You can have a specific layout per category, and it will be inherited by all
its sub-categories.

Finally, the **appender** actually displays the produced messages. SimGrid provides four appenders so far: the default one prints on *stderr*. ``file`` writes to a given file,
``rollfile`` does the same, but overwrites old messages when the file grows too large and ``splitfile`` creates new files when the maximum size is reached. Each category can have
its own appender.

For more information, please refer to the :ref:`programmer's interface <logging_prog>` to learn how to produce messages from your code, or to :ref:`logging_config` to see how to
change the settings at runtime.

.. _outcome_vizu:

Graphical and statistical logging
*********************************

SimGrid comes with an extensive support to trace and register what happens during the simulation, so that it can be
either visualized or statistically analysed afterward. This is widely used to observe and understand the behavior of
parallel applications and distributed algorithms: the user instruments the application, and the traces are analyzed
after the execution to highlight unexpected behaviors and bottlenecks, or even to help correcting distributed
algorithms.

The tracing system records the resource (hosts and links) utilization over time. Traces can optionally be organized in
user-defined *categories*, so that the utilization can be broken down per category (e.g. per algorithmic phase). SMPI
simulations can also be traced at the level of the MPI calls, to produce Gantt-like visualizations of the
communications.

The resulting trace files are written in the `Paje <http://github.com/schnorr/pajeng/>`_ format, and can be
explored with dedicated Paje visualization tools, or post-processed for statistical analysis. In addition to this
general description, `this page <https://simgrid.org/contrib/R_visualization.html>`_ gives a concrete, hands-on
example of importing and analyzing SimGrid traces with R.

The tracing subsystem is entirely configured through the ``--cfg=`` command line switch; see the :ref:`Tracing
configuration Options <tracing_tracing_options>` subsection of :ref:`options` for the full reference, or run your
simulator with ``--help-tracing`` for a detailed and up-to-date description of every tracing parameter.

.. note::
   Some content from the historical Doxygen documentation (``doc/doxygen/outcomes_vizu.doc``) have not been ported
   here yet. Most of it is probably obsolete and should be dropped, but he list of tools and tips to analyze the 
   resulting trace files (``pj_dump``, R/ggplot2, PajeNG, ...) is probably still relevant.

Building your own logging
*************************

You can add callbacks to the existing signals to get informed of each and every even occurring in the simulator. These callbacks can be used to write logs on disk in the format that
you want. Some users did so to visualize the behavior of their simulators with `Jaeger <https://www.jaegertracing.io/>`_.
