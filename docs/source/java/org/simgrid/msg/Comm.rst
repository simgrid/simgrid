class org.simgrid.msg.Comm
==========================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class Comm

   Communication action, representing an ongoing communication between processes.

Fields
------
finished
^^^^^^^^

.. java:field:: protected boolean finished
   :outertype: Comm

   Indicates if the communication is finished

receiving
^^^^^^^^^

.. java:field:: protected boolean receiving
   :outertype: Comm

   Indicates if the communication is a receiving communication

task
^^^^

.. java:field:: protected Task task
   :outertype: Comm

   Task associated with the comm. Beware, it can be null

Constructors
------------
Comm
^^^^

.. java:constructor:: protected Comm()
   :outertype: Comm

   Protected constructor, used by Comm factories in Task.

Methods
-------
finalize
^^^^^^^^

.. java:method:: @Deprecated @Override protected void finalize() throws Throwable
   :outertype: Comm

   Destroy the C communication object, when the GC reclaims the java part.

getTask
^^^^^^^

.. java:method:: public Task getTask()
   :outertype: Comm

   Returns the task associated with the communication. if the communication isn't finished yet, will return null.

nativeFinalize
^^^^^^^^^^^^^^

.. java:method:: protected native void nativeFinalize()
   :outertype: Comm

nativeInit
^^^^^^^^^^

.. java:method:: public static native void nativeInit()
   :outertype: Comm

   Class initializer, to initialize various JNI stuff

test
^^^^

.. java:method:: public native boolean test() throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Comm

   Returns if the communication is finished or not. If the communication has finished and there was an error, raise an exception.

waitAll
^^^^^^^

.. java:method:: public static native void waitAll(Comm[] comms, double timeout) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Comm

   Wait all of the communications

waitAll
^^^^^^^

.. java:method:: public static void waitAll(Comm[] comms) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Comm

   Wait all of the communications, with no maximal delay

waitAny
^^^^^^^

.. java:method:: public static native int waitAny(Comm[] comms) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Comm

   Wait any of the communications, and return the rank of the terminating comm

waitCompletion
^^^^^^^^^^^^^^

.. java:method:: public void waitCompletion() throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Comm

   Wait infinitely for the completion of the communication (infinite timeout)

waitCompletion
^^^^^^^^^^^^^^

.. java:method:: public native void waitCompletion(double timeout) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Comm

   Wait for the completion of the communication. Throws an exception if there were an error in the communication.

   :param timeout: Time before giving up (infinite time if negative)

