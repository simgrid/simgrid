class org.simgrid.msg.ProcessKilledError
========================================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class ProcessKilledError extends Error

   Used internally to interrupt the user code when the process gets killed. \rst You can catch it for cleanups or to debug, but DO NOT BLOCK IT, or your simulation will segfault! .. code-block:: java try { getHost().off(); } catch (ProcessKilledError e) { e.printStackTrace(); throw e; } \endrst

Constructors
------------
ProcessKilledError
^^^^^^^^^^^^^^^^^^

.. java:constructor:: public ProcessKilledError(String s)
   :outertype: ProcessKilledError

