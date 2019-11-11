class org.simgrid.msg.Host
==========================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class Host

   A host object represents a location (any possible place) where a process may run. Thus it is represented as a physical resource with computing capabilities, some mailboxes to enable running process to communicate with remote ones, and some private data that can be only accessed by local process. An instance of this class is always bound with the corresponding native host. All the native hosts are automatically created during the call of the method Msg.createEnvironment(). This method take as parameter a platform file which describes all elements of the platform (host, link, root..). You cannot create a host yourself. The best way to get an host instance is to call the static method Host.getByName(). For example to get the instance of the host. If your platform file description contains an host named "Jacquelin" : \verbatim Host jacquelin; try { jacquelin = Host.getByName("Jacquelin"); } catch(HostNotFoundException e) { System.err.println(e.toString()); } ... \endverbatim

Fields
------
name
^^^^

.. java:field:: protected String name
   :outertype: Host

Constructors
------------
Host
^^^^

.. java:constructor:: protected Host()
   :outertype: Host

Methods
-------
all
^^^

.. java:method:: public static native Host[] all()
   :outertype: Host

   Returns all hosts of the installed platform.

currentHost
^^^^^^^^^^^

.. java:method:: public static native Host currentHost()
   :outertype: Host

   Returns the host of the current process.

getAttachedStorage
^^^^^^^^^^^^^^^^^^

.. java:method:: public native String[] getAttachedStorage()
   :outertype: Host

   This methods returns the list of storages (names) attached to an host

getAvgLoad
^^^^^^^^^^

.. java:method:: public native double getAvgLoad()
   :outertype: Host

   Returns the average load of the host as a ratio since the beginning of the simulation

getByName
^^^^^^^^^

.. java:method:: public static native Host getByName(String name) throws HostNotFoundException
   :outertype: Host

   This static method gets an host instance associated with a native host of your platform. This is the best way to get a java host object.

   :param name: The name of the host to get.
   :throws HostNotFoundException: if the name of the host is not valid.
   :return: The host object with the given name.

getComputedFlops
^^^^^^^^^^^^^^^^

.. java:method:: public native double getComputedFlops()
   :outertype: Host

   Returns the number of flops computed of the host since the beginning of the simulation

getConsumedEnergy
^^^^^^^^^^^^^^^^^

.. java:method:: public native double getConsumedEnergy()
   :outertype: Host

   Returns the amount of Joules consumed by that host so far Please note that since the consumption is lazily updated, it may require a simcall to update it. The result is that the actor requesting this value will be interrupted, the value will be updated in kernel mode before returning the control to the requesting actor.

getCoreNumber
^^^^^^^^^^^^^

.. java:method:: public native double getCoreNumber()
   :outertype: Host

getCount
^^^^^^^^

.. java:method:: public static native int getCount()
   :outertype: Host

   Counts the installed hosts.

getCurrentLoad
^^^^^^^^^^^^^^

.. java:method:: public native double getCurrentLoad()
   :outertype: Host

   Returns the current load of the host, as a ratio = achieved_flops / (core_current_speed * core_amount) See simgrid::plugin::HostLoad::get_current_load() for the full documentation.

getCurrentPowerPeak
^^^^^^^^^^^^^^^^^^^

.. java:method:: public native double getCurrentPowerPeak()
   :outertype: Host

   Returns the speed of the processor (in flop/s) at the current pstate. See also @ref plugin_energy.

getData
^^^^^^^

.. java:method:: public Object getData()
   :outertype: Host

getLoad
^^^^^^^

.. java:method:: public native double getLoad()
   :outertype: Host

   Returns the current computation load (in flops per second)

getMountedStorage
^^^^^^^^^^^^^^^^^

.. java:method:: public native Storage[] getMountedStorage()
   :outertype: Host

   Returns the list of mount point names on an host

getName
^^^^^^^

.. java:method:: public String getName()
   :outertype: Host

getPowerPeakAt
^^^^^^^^^^^^^^

.. java:method:: public native double getPowerPeakAt(int pstate)
   :outertype: Host

   Returns the speed of the processor (in flop/s) at a given pstate. See also @ref plugin_energy.

getProperty
^^^^^^^^^^^

.. java:method:: public native String getProperty(String name)
   :outertype: Host

getPstate
^^^^^^^^^

.. java:method:: public native int getPstate()
   :outertype: Host

   Returns the current pstate

getPstatesCount
^^^^^^^^^^^^^^^

.. java:method:: public native int getPstatesCount()
   :outertype: Host

getSpeed
^^^^^^^^

.. java:method:: public native double getSpeed()
   :outertype: Host

   This method returns the speed of the processor of a host (in flops), regardless of the current load of the machine.

hasData
^^^^^^^

.. java:method:: public boolean hasData()
   :outertype: Host

   Returns true if the host has an associated data object.

isOn
^^^^

.. java:method:: public native boolean isOn()
   :outertype: Host

   Tests if an host is up and running.

off
^^^

.. java:method:: public native void off() throws ProcessKilledError
   :outertype: Host

   Stops the host if it is on

on
^^

.. java:method:: public native void on()
   :outertype: Host

   Starts the host if it is off

setAsyncMailbox
^^^^^^^^^^^^^^^

.. java:method:: public static native void setAsyncMailbox(String mailboxName)
   :outertype: Host

   This static method sets a mailbox to receive in asynchronous mode. All messages sent to this mailbox will be transferred to the receiver without waiting for the receive call. The receive call will still be necessary to use the received data. If there is a need to receive some messages asynchronously, and some not, two different mailboxes should be used.

   :param mailboxName: The name of the mailbox

setData
^^^^^^^

.. java:method:: public void setData(Object data)
   :outertype: Host

setProperty
^^^^^^^^^^^

.. java:method:: public native void setProperty(String name, String value)
   :outertype: Host

setPstate
^^^^^^^^^

.. java:method:: public native void setPstate(int pstate)
   :outertype: Host

   Changes the current pstate

toString
^^^^^^^^

.. java:method:: @Override public String toString()
   :outertype: Host

updateAllEnergyConsumptions
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. java:method:: public static native void updateAllEnergyConsumptions()
   :outertype: Host

   After this call, sg_host_get_consumed_energy() will not interrupt your process (until after the next clock update).

