class org.simgrid.msg.Semaphore
===============================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class Semaphore

   A semaphore implemented on top of SimGrid synchronization mechanisms. You can use it exactly the same way that you use classical semaphores but to handle the interactions between the processes within the simulation.

Fields
------
capacity
^^^^^^^^

.. java:field:: protected final int capacity
   :outertype: Semaphore

   Semaphore capacity, defined when the semaphore is created. At most capacity process can acquire this semaphore at the same time.

Constructors
------------
Semaphore
^^^^^^^^^

.. java:constructor:: public Semaphore(int capacity)
   :outertype: Semaphore

   Creates a new semaphore with the given capacity. At most capacity process can acquire this semaphore at the same time.

Methods
-------
acquire
^^^^^^^

.. java:method:: public native void acquire(double timeout) throws TimeoutException
   :outertype: Semaphore

   Locks on the semaphore object until the provided timeout expires

   :param timeout: the duration of the lock
   :throws TimeoutException: if the timeout expired before the semaphore could be acquired.

acquire
^^^^^^^

.. java:method:: public void acquire()
   :outertype: Semaphore

   Locks on the semaphore object with no timeout

finalize
^^^^^^^^

.. java:method:: @Deprecated @Override protected void finalize() throws Throwable
   :outertype: Semaphore

   Deletes this semaphore when the GC reclaims it

getCapacity
^^^^^^^^^^^

.. java:method:: public int getCapacity()
   :outertype: Semaphore

   Returns the semaphore capacity

nativeInit
^^^^^^^^^^

.. java:method:: public static native void nativeInit()
   :outertype: Semaphore

   Class initializer, to initialize various JNI stuff

release
^^^^^^^

.. java:method:: public native void release()
   :outertype: Semaphore

   Releases the semaphore object

wouldBlock
^^^^^^^^^^

.. java:method:: public native boolean wouldBlock()
   :outertype: Semaphore

   returns a boolean indicating it this semaphore would block at this very specific time Note that the returned value may be wrong right after the function call, when you try to use it... But that's a classical semaphore issue, and SimGrid's semaphores are not different to usual ones here.

