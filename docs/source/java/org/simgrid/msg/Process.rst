class org.simgrid.msg... java:import:: java.util ArrayList

.. java:import:: java.util Arrays

Process
=============================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public abstract class Process implements Runnable

   A process may be defined as a code, with some private data, executing in a location (host). All the process used by your simulation must be declared in the deployment file (XML format). To create your own process you must inherit your own process from this class and override the method "main()". For example if you want to use a process named Slave proceed as it : (1) import the class Process of the package simgrid.msg import simgrid.msg.Process; public class Slave extends simgrid.msg.Process { (2) Override the method function \verbatim public void main(String[] args) { System.out.println("Hello MSG"); } \endverbatim } The name of your process must be declared in the deployment file of your simulation. For the example, for the previous process Slave this file must contains a line : <process host="Maxims" function="Slave"/>, where Maxims is the host of the process Slave. All the process of your simulation are automatically launched and managed by Msg. A process use tasks to simulate communications or computations with another process. For more information see Task. For more information on host concept see Host.

Fields
------
startTime
^^^^^^^^^

.. java:field:: protected double startTime
   :outertype: Process

   Time at which the process should be created

Constructors
------------
Process
^^^^^^^

.. java:constructor:: public Process(String hostname, String name) throws HostNotFoundException
   :outertype: Process

   Constructs a new process from the name of a host and his name. The method function of the process doesn't have argument.

   :param hostname: Where to create the process.
   :param name: The name of the process.
   :throws HostNotFoundException: if no host with this name exists.

Process
^^^^^^^

.. java:constructor:: public Process(String hostname, String name, String[] args) throws HostNotFoundException
   :outertype: Process

   Constructs a new process from the name of a host and his name. The arguments of the method function of the process are specified by the parameter args.

   :param hostname: Where to create the process.
   :param name: The name of the process.
   :param args: The arguments of the main function of the process.
   :throws HostNotFoundException: if no host with this name exists.

Process
^^^^^^^

.. java:constructor:: public Process(Host host, String name)
   :outertype: Process

   Constructs a new process from a host and his name. The method function of the process doesn't have argument.

   :param host: Where to create the process.
   :param name: The name of the process.

Process
^^^^^^^

.. java:constructor:: public Process(Host host, String name, String[] argsParam)
   :outertype: Process

   Constructs a new process from a host and his name, the arguments of here method function are specified by the parameter args.

   :param host: Where to create the process.
   :param name: The name of the process.
   :param argsParam: The arguments of main method of the process.

Process
^^^^^^^

.. java:constructor:: public Process(Host host, String name, String[] args, double startTime, double killTime)
   :outertype: Process

   Constructs a new process from a host and his name, the arguments of here method function are specified by the parameter args.

   :param host: Where to create the process.
   :param name: The name of the process.
   :param args: The arguments of main method of the process.
   :param startTime: Start time of the process
   :param killTime: Kill time of the process

Methods
-------
create
^^^^^^

.. java:method:: protected native void create(Host host)
   :outertype: Process

   The native method to create an MSG process.

   :param host: where to create the process.

debugAllThreads
^^^^^^^^^^^^^^^

.. java:method:: public static void debugAllThreads()
   :outertype: Process

exit
^^^^

.. java:method:: public void exit()
   :outertype: Process

   Stops the execution of the current actor

fromPID
^^^^^^^

.. java:method:: public static native Process fromPID(int pid)
   :outertype: Process

   This static method gets a process from a PID.

   :param pid: The process identifier of the process to get.
   :return: The process with the specified PID.

getCount
^^^^^^^^

.. java:method:: public static native int getCount()
   :outertype: Process

   This static method returns the current amount of processes running

   :return: The count of the running processes

getCurrentProcess
^^^^^^^^^^^^^^^^^

.. java:method:: public static native Process getCurrentProcess()
   :outertype: Process

   This static method returns the currently running process.

   :return: The current process.

getHost
^^^^^^^

.. java:method:: public Host getHost()
   :outertype: Process

   Returns the host of the process.

   :return: The host instance of the process.

getName
^^^^^^^

.. java:method:: public String getName()
   :outertype: Process

   Returns the name of the process

getPID
^^^^^^

.. java:method:: public int getPID()
   :outertype: Process

   This method returns the PID of the process.

   :return: The PID of the process.

getPPID
^^^^^^^

.. java:method:: public int getPPID()
   :outertype: Process

   This method returns the PID of the parent of a process.

   :return: The PID of the parent of the process.

getProperty
^^^^^^^^^^^

.. java:method:: public native String getProperty(String name)
   :outertype: Process

   Returns the value of a given process property.

isSuspended
^^^^^^^^^^^

.. java:method:: public native boolean isSuspended()
   :outertype: Process

   Tests if a process is suspended.

   **See also:** :java:ref:`.suspend()`, :java:ref:`.resume()`

kill
^^^^

.. java:method:: public native void kill()
   :outertype: Process

   Simply kills the receiving process. SimGrid sometimes have issues when you kill processes that are currently communicating and such. We are working on it to fix the issues.

kill
^^^^

.. java:method:: public static void kill(Process p)
   :outertype: Process

killAll
^^^^^^^

.. java:method:: public static native void killAll()
   :outertype: Process

   This method kills all running process of the simulation.

main
^^^^

.. java:method:: public abstract void main(String[] args) throws MsgException
   :outertype: Process

   The main function of the process (to implement by the user).

   :param args:
   :throws MsgException:

migrate
^^^^^^^

.. java:method:: public native void migrate(Host host)
   :outertype: Process

   Migrates a process to another host.

   :param host: The host where to migrate the process.

restart
^^^^^^^

.. java:method:: public native void restart()
   :outertype: Process

   Restarts the process from the beginning

resume
^^^^^^

.. java:method:: public native void resume()
   :outertype: Process

   Resume a process that was suspended by \ :java:ref:`suspend()`\ .

run
^^^

.. java:method:: @Override public void run()
   :outertype: Process

   This method runs the process. It calls the method function that you must overwrite.

setAutoRestart
^^^^^^^^^^^^^^

.. java:method:: public native void setAutoRestart(boolean autoRestart)
   :outertype: Process

   Specify whether the process should restart when its host restarts after a failure A process naturally stops when its host stops. It starts again only if autoRestart is set to true. Otherwise, it just disappears when the host stops.

setKillTime
^^^^^^^^^^^

.. java:method:: public native void setKillTime(double killTime)
   :outertype: Process

   Set the kill time of the process

   :param killTime: the time when the process is killed

sleep
^^^^^

.. java:method:: public static void sleep(long millis) throws HostFailureException
   :outertype: Process

   Makes the current process sleep until millis milliseconds have elapsed. You should note that unlike "waitFor" which takes seconds (as usual in SimGrid), this method takes milliseconds (as usual for sleep() in Java).

   :param millis: the length of time to sleep in milliseconds.

sleep
^^^^^

.. java:method:: public static native void sleep(long millis, int nanos) throws HostFailureException
   :outertype: Process

   Makes the current process sleep until millis milliseconds and nanos nanoseconds have elapsed. Unlike \ :java:ref:`waitFor(double)`\  which takes seconds, this method takes milliseconds and nanoseconds. Overloads Thread.sleep.

   :param millis: the length of time to sleep in milliseconds.
   :param nanos: additional nanoseconds to sleep.

start
^^^^^

.. java:method:: public final void start()
   :outertype: Process

   This method actually creates and run the process. It is a noop if the process is already launched.

suspend
^^^^^^^

.. java:method:: public native void suspend()
   :outertype: Process

   Suspends the process. See \ :java:ref:`resume()`\  to resume it afterward

waitFor
^^^^^^^

.. java:method:: public native void waitFor(double seconds) throws HostFailureException
   :outertype: Process

   Makes the current process sleep until time seconds have elapsed.

   :param seconds: The time the current process must sleep.

yield
^^^^^

.. java:method:: public static native void yield()
   :outertype: Process

   Yield the current process. All other processes that are ready at the same timestamp will be executed first

