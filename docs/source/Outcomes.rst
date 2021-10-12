.. _outcomes:

Simulation outcomes
###################

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

To be written. For now, see `this page <https://simgrid.org/contrib/R_visualization.html>`_.

Building your own logging
*************************

You can add callbacks to the existing signals to get informed of each and every even occurring in the simulator. These callbacks can be used to write logs on disk in the format that
you want. Some users did so to visualize the behavior of their simulators with `Jaeger <https://www.jaegertracing.io/>`_.
