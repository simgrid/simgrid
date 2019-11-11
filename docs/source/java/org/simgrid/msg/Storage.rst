class org.simgrid.msg.Storage
=============================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class Storage

Fields
------
name
^^^^

.. java:field:: protected String name
   :outertype: Storage

   Storage name

Methods
-------
all
^^^

.. java:method:: public static native Storage[] all()
   :outertype: Storage

   This static method returns all of the storages of the installed platform.

   :return: An array containing all the storages installed.

getByName
^^^^^^^^^

.. java:method:: public static native Storage getByName(String name) throws HostNotFoundException, StorageNotFoundException
   :outertype: Storage

   This static method gets a storage instance associated with a native storage of your platform. This is the best way to get a java storage object.

   :param name: The name of the storage to get.
   :throws StorageNotFoundException: if the name of the storage is not valid.
   :return: The storage object with the given name.

getFreeSize
^^^^^^^^^^^

.. java:method:: public native long getFreeSize()
   :outertype: Storage

   This method returns the free size (in bytes) of a storage element.

   :return: The free size (in bytes) of the storage element.

getHost
^^^^^^^

.. java:method:: public native String getHost()
   :outertype: Storage

   Returns the host name the storage is attached to

   :return: the host name the storage is attached to

getName
^^^^^^^

.. java:method:: public String getName()
   :outertype: Storage

   This method returns the name of a storage.

   :return: The name of the storage.

getProperty
^^^^^^^^^^^

.. java:method:: public native String getProperty(String name)
   :outertype: Storage

   Returns the value of a given storage property.

getSize
^^^^^^^

.. java:method:: public native long getSize()
   :outertype: Storage

   This method returns the size (in bytes) of a storage element.

   :return: The size (in bytes) of the storage element.

getUsedSize
^^^^^^^^^^^

.. java:method:: public native long getUsedSize()
   :outertype: Storage

   This method returns the used size (in bytes) of a storage element.

   :return: The used size (in bytes) of the storage element.

nativeInit
^^^^^^^^^^

.. java:method:: public static native void nativeInit()
   :outertype: Storage

   Class initializer, to initialize various JNI stuff

setProperty
^^^^^^^^^^^

.. java:method:: public native void setProperty(String name, String value)
   :outertype: Storage

   Change the value of a given storage property.

toString
^^^^^^^^

.. java:method:: @Override public String toString()
   :outertype: Storage

