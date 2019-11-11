class org.simgrid.msg.Msg
=========================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public final class Msg

Methods
-------
createEnvironment
^^^^^^^^^^^^^^^^^

.. java:method:: public static final native void createEnvironment(String platformFile)
   :outertype: Msg

   Create the simulation environment by parsing a platform file.

critical
^^^^^^^^

.. java:method:: public static final native void critical(String s)
   :outertype: Msg

   Issue a critical logging message.

debug
^^^^^

.. java:method:: public static final native void debug(String msg)
   :outertype: Msg

   Issue a debug logging message.

deployApplication
^^^^^^^^^^^^^^^^^

.. java:method:: public static final native void deployApplication(String deploymentFile)
   :outertype: Msg

   Starts your processes by parsing a deployment file.

energyInit
^^^^^^^^^^

.. java:method:: public static final native void energyInit()
   :outertype: Msg

   Tell the kernel that you want to use the energy plugin

environmentGetRoutingRoot
^^^^^^^^^^^^^^^^^^^^^^^^^

.. java:method:: public static final native As environmentGetRoutingRoot()
   :outertype: Msg

error
^^^^^

.. java:method:: public static final native void error(String msg)
   :outertype: Msg

   Issue an error logging message.

fileSystemInit
^^^^^^^^^^^^^^

.. java:method:: public static final native void fileSystemInit()
   :outertype: Msg

   Tell the kernel that you want to use the filesystem plugin.

getClock
^^^^^^^^

.. java:method:: public static final native double getClock()
   :outertype: Msg

   Retrieves the simulation time

info
^^^^

.. java:method:: public static final native void info(String msg)
   :outertype: Msg

   Issue an information logging message

init
^^^^

.. java:method:: public static final native void init(String[] args)
   :outertype: Msg

   Initialize a MSG simulation.

   :param args: The arguments of the command line of the simulation.

loadInit
^^^^^^^^

.. java:method:: public static final native void loadInit()
   :outertype: Msg

   Initializes the HostLoad plugin. The HostLoad plugin provides an API to get the current load of each host.

main
^^^^

.. java:method:: public static void main(String[] args)
   :outertype: Msg

   Example launcher. You can use it or provide your own launcher, as you wish

   :param args:

run
^^^

.. java:method:: public static final native void run()
   :outertype: Msg

   Run the MSG simulation. After the simulation, you can freely retrieve the information that you want.. In particular, retrieving the status of a process or the current date is perfectly ok.

verb
^^^^

.. java:method:: public static final native void verb(String msg)
   :outertype: Msg

   Issue a verbose logging message.

warn
^^^^

.. java:method:: public static final native void warn(String msg)
   :outertype: Msg

   Issue a warning logging message.

