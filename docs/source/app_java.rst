.. _Java_doc:

The Java Bindings
#################

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" width="100%" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     //var elem=document.querySelector("#TOC").contentDocument.getElementById("S4UBox")
     //elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1";
   }
   </script>
   <br/>
   <br/>

   
This section describes jMSG, the Java API to Simgrid. This API mimicks 
:ref:`MSG <MSG_doc>`, which is a simple yet somehow realistic interface.
The full reference documentation is provided at the end of this page.

Most of the documentation of the :ref:`MSG API <MSG_doc>` in C applies
directly to the Java bindings (any divergence is seen as a bug that we
should fix). MSG structures are mapped to Java objects as expected,
and the MSG functions are methods in these objects.

Installing the Java bindings
****************************

The easiest is to use a :ref:`precompiled jarfile <install_java_precompiled>`,
but some people may prefer to :ref:`compile it from the sources <install_src>`.


Using the Java bindings
***********************

In most cases, you can use the SimGrid bindings as if it was a Java
library:

.. code-block:: shell

   $ javac -classpath .:path/to/simgrid.jar your/java/Code.java
   $ java -classpath .:path/to/simgrid.jar your.java.Code the/parameter/to/your/code

For example:

.. code-block:: shell

   $ cd examples/java
   $ java -classpath ../../simgrid.jar:. .:../../simgrid.jar app.pingpong.Main ../platforms/platform.xml 

Any SimGrid simulation (java or not) is usually constituted of several
kind of actors or processes (classes extending @c Msg.Process) that
are deployed over the hosts of the virtual platform. So, your code
should declare these actors, plus a Main class in charge of deploying
your actors on the platform. Please refer to the examples for details.

Troubleshooting
***************

Actually, these bindings are not only implemented in Java. They do use
the C implementation of SimGrid. This should be transparent as this
library is directly included in the ``simgrid.jar`` file but things can
still go wrong is several ways.

Error: library simgrid not found
================================

This means that the JVM fails to load the native library. If you use a
precompiled jarfile, please report this bug.

If you built it yourself, you can try to use an installed version of
the library instead of the one included in the jar. For that, add the
path to the native library into the ``LD_LIBRARY_PATH`` variable (or in
the ``DYLD_LIBRARY_PATH`` on Mac OSX).

pthread_create failed
=====================

You reached the amount of threads that can be run on your system. Try
increasing the thread limits of your operating system.

Other errors
============

When using jMSG, your program can crash for 3 main reasons:

- Your Java part is not good: you'll have a good old java exception thrown,
  and hence you should be able to correct it by yourself.
- Our java part is not good: you'll also have a java exception thrown, but
  we have real doubts this can happen, since the java part is only a JNI
  binding. The other option is that it crashed because you used incorrectly
  the MSG API, so this means also you should have an MSGException. It means
  you should read carefully MSG samples and/or documentation.
- Something has crashed in the C part. Okay, here comes the tricky
  thing. It happens mainly for 2 reasons:
  
  - When something goes wrong in your simulation, sometimes the C part stops
    because you used SimGrid incorrectly, and JNI bindings are not fond of that.
    It means that you'll have something that looks ugly, but you should be able
    to identify what's going wrong in your code by carefully reading the whole
    error message
  - It may happen that the problem comes directly from SimGrid: in this case,
    the error should be uglier. In that case, you may submit a bug directly to
    SimGrid.

API Reference
*************

Package org.simgrid.msg
=======================

.. java:package:: org.simgrid.msg

.. toctree::
   :maxdepth: 1

   Class org.simgrid.msg.As <java/org/simgrid/msg/As>
   Class org.simgrid.msg.Comm <java/org/simgrid/msg/Comm>
   Class org.simgrid.msg.File <java/org/simgrid/msg/File>
   Class org.simgrid.msg.Host <java/org/simgrid/msg/Host>
   Class org.simgrid.msg.HostFailureException <java/org/simgrid/msg/HostFailureException>
   Class org.simgrid.msg.HostNotFoundException <java/org/simgrid/msg/HostNotFoundException>
   Class org.simgrid.msg.JniException <java/org/simgrid/msg/JniException>
   Class org.simgrid.msg.Msg <java/org/simgrid/msg/Msg>
   Class org.simgrid.msg.MsgException <java/org/simgrid/msg/MsgException>
   Class org.simgrid.msg.Mutex <java/org/simgrid/msg/Mutex>
   Class org.simgrid.msg.Process <java/org/simgrid/msg/Process>
   Class org.simgrid.msg.ProcessKilledError <java/org/simgrid/msg/ProcessKilledError>
   Class org.simgrid.msg.ProcessNotFoundException <java/org/simgrid/msg/ProcessNotFoundException>
   Class org.simgrid.msg.RngStream <java/org/simgrid/msg/RngStream>
   Class org.simgrid.msg.Semaphore <java/org/simgrid/msg/Semaphore>
   Class org.simgrid.msg.Storage <java/org/simgrid/msg/Storage>
   Class org.simgrid.msg.StorageNotFoundException <java/org/simgrid/msg/StorageNotFoundException>
   Class org.simgrid.msg.Task <java/org/simgrid/msg/Task>
   Class org.simgrid.msg.TaskCancelledException <java/org/simgrid/msg/TaskCancelledException>
   Class org.simgrid.msg.TimeoutException <java/org/simgrid/msg/TimeoutException>
   Class org.simgrid.msg.TransferFailureException <java/org/simgrid/msg/TransferFailureException>
   Class org.simgrid.msg.VM <java/org/simgrid/msg/VM>
