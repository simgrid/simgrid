class org.simgrid.msg.VM
========================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class VM extends Host

Constructors
------------
VM
^^

.. java:constructor:: public VM(Host host, String name)
   :outertype: VM

   Create a `basic` VM : 1 core and 1GB of RAM.

   :param host: Host node
   :param name: name of the machine

VM
^^

.. java:constructor:: public VM(Host host, String name, int coreAmount)
   :outertype: VM

   Create a VM without useless values (for humans)

   :param host: Host node
   :param name: name of the machine
   :param coreAmount: the amount of cores of the VM

VM
^^

.. java:constructor:: public VM(Host host, String name, int ramSize, int migNetSpeed, int dpIntensity)
   :outertype: VM

   Create a VM with 1 core

   :param host: Host node
   :param name: name of the machine
   :param ramSize: size of the RAM that should be allocated (in MBytes)
   :param migNetSpeed: (network bandwidth allocated for migrations in MB/s, if you don't know put zero ;))
   :param dpIntensity: (dirty page percentage according to migNetSpeed, [0-100], if you don't know put zero ;))

VM
^^

.. java:constructor:: public VM(Host host, String name, int coreAmount, int ramSize, int migNetSpeed, int dpIntensity)
   :outertype: VM

   Create a VM

   :param host: Host node
   :param name: name of the machine
   :param coreAmount: the amount of cores of the VM
   :param ramSize: size of the RAM that should be allocated (in MBytes)
   :param migNetSpeed: (network bandwidth allocated for migrations in MB/s, if you don't know put zero ;))
   :param dpIntensity: (dirty page percentage according to migNetSpeed, [0-100], if you don't know put zero ;))

Methods
-------
all
^^^

.. java:method:: public static native VM[] all()
   :outertype: VM

   Retrieve the list of all existing VMs

destroy
^^^^^^^

.. java:method:: public native void destroy()
   :outertype: VM

   Shutdown and unref the VM.

finalize
^^^^^^^^

.. java:method:: @Deprecated @Override protected void finalize() throws Throwable
   :outertype: VM

   Make sure that the GC also destroys the C object

getCoreAmount
^^^^^^^^^^^^^

.. java:method:: public int getCoreAmount()
   :outertype: VM

   Returns the amount of virtual CPUs provided

getVMByName
^^^^^^^^^^^

.. java:method:: public static native VM getVMByName(String name)
   :outertype: VM

   Retrieve a VM from its name

isCreated
^^^^^^^^^

.. java:method:: public native int isCreated()
   :outertype: VM

   Returns whether the given VM is currently suspended

isMigrating
^^^^^^^^^^^

.. java:method:: public native int isMigrating()
   :outertype: VM

   Returns whether the given VM is currently running

isRunning
^^^^^^^^^

.. java:method:: public native int isRunning()
   :outertype: VM

   Returns whether the given VM is currently running

isSuspended
^^^^^^^^^^^

.. java:method:: public native int isSuspended()
   :outertype: VM

   Returns whether the given VM is currently suspended

migrate
^^^^^^^

.. java:method:: public void migrate(Host destination) throws HostFailureException
   :outertype: VM

   Change the host on which all processes are running (pre-copy is implemented)

resume
^^^^^^

.. java:method:: public native void resume()
   :outertype: VM

   Immediately resumes the execution of all processes within the given VM No resume cost occurs. If you want to simulate this too, you want to use a @ref File.read() before or after, depending on the exact semantic of VM resume to you.

setBound
^^^^^^^^

.. java:method:: public native void setBound(double bound)
   :outertype: VM

   Set a CPU bound for a given VM.

   :param bound: in flops/s

shutdown
^^^^^^^^

.. java:method:: public native void shutdown()
   :outertype: VM

   Immediately kills all processes within the given VM. No extra delay occurs. If you want to simulate this too, you want to use a MSG_process_sleep()

start
^^^^^

.. java:method:: public native void start()
   :outertype: VM

   start the VM

suspend
^^^^^^^

.. java:method:: public native void suspend()
   :outertype: VM

   Immediately suspend the execution of all processes within the given VM No suspension cost occurs. If you want to simulate this too, you want to use a @ref File.write() before or after, depending on the exact semantic of VM suspend to you.

