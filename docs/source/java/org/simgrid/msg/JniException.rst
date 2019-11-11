class org.simgrid.msg.JniException
==================================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class JniException extends RuntimeException

   Exception raised when there is a problem within the bindings (in JNI). That's a RuntimeException: I guess nobody wants to survive a JNI error in SimGrid

Constructors
------------
JniException
^^^^^^^^^^^^

.. java:constructor:: public JniException()
   :outertype: JniException

   Constructs an \ ``JniException``\  without a detail message.

JniException
^^^^^^^^^^^^

.. java:constructor:: public JniException(String s)
   :outertype: JniException

   Constructs an \ ``JniException``\  with a detail message.

JniException
^^^^^^^^^^^^

.. java:constructor:: public JniException(String string, Exception e)
   :outertype: JniException

