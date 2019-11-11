class org.simgrid.msg.File
==========================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class File

Fields
------
SEEK_CUR
^^^^^^^^

.. java:field:: public static final int SEEK_CUR
   :outertype: File

SEEK_END
^^^^^^^^

.. java:field:: public static final int SEEK_END
   :outertype: File

SEEK_SET
^^^^^^^^

.. java:field:: public static final int SEEK_SET
   :outertype: File

Constructors
------------
File
^^^^

.. java:constructor:: public File(String path)
   :outertype: File

   Constructor, opens the file.

   :param path: is the file location on the storage

Methods
-------
close
^^^^^

.. java:method:: public native void close()
   :outertype: File

   Close the file.

nativeInit
^^^^^^^^^^

.. java:method:: public static native void nativeInit()
   :outertype: File

   Class initializer, to initialize various JNI stuff

open
^^^^

.. java:method:: protected native void open(String path)
   :outertype: File

   Opens the file whose name is the string pointed to by path.

   :param path: is the file location on the storage

read
^^^^

.. java:method:: public native long read(long size, long nMemb)
   :outertype: File

   Read elements of a file.

   :param size: of each element
   :param nMemb: is the number of elements of data to write
   :return: the actually read size

seek
^^^^

.. java:method:: public native void seek(long offset, long origin)
   :outertype: File

   Write elements into a file.

   :param offset: : number of bytes to offset from origin
   :param origin: : Position used as reference for the offset. It is specified by one of the following constants defined in <stdio.h> exclusively to be used as arguments for this function (SEEK_SET = beginning of file, SEEK_CUR = current position of the file pointer, SEEK_END = end of file)

write
^^^^^

.. java:method:: public native long write(long size, long nMemb)
   :outertype: File

   Write elements into a file.

   :param size: of each element
   :param nMemb: is the number of elements of data to write
   :return: the actually written size

