class org.simgrid.msg.Mutex
===========================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class Mutex

   A mutex implemented on top of SimGrid synchronization mechanisms. You can use it exactly the same way that you use the mutexes, but to handle the interactions between the processes within the simulation. Don't mix simgrid synchronization with Java native one, or it will deadlock!

Constructors
------------
Mutex
^^^^^

.. java:constructor:: public Mutex()
   :outertype: Mutex

Methods
-------
acquire
^^^^^^^

.. java:method:: public native void acquire()
   :outertype: Mutex

finalize
^^^^^^^^

.. java:method:: @Deprecated @Override protected void finalize() throws Throwable
   :outertype: Mutex

nativeInit
^^^^^^^^^^

.. java:method:: public static native void nativeInit()
   :outertype: Mutex

   Class initializer, to initialize various JNI stuff

release
^^^^^^^

.. java:method:: public native void release()
   :outertype: Mutex

