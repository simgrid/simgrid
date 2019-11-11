class org.simgrid.msg.Task
==========================

.. java:package:: org.simgrid.msg
   :noindex:

.. java:type:: public class Task

   A task is either something to compute somewhere, or something to exchange between two hosts (or both). It is defined by a computing amount and a message size.

Fields
------
name
^^^^

.. java:field:: protected String name
   :outertype: Task

   Task name

Constructors
------------
Task
^^^^

.. java:constructor:: public Task()
   :outertype: Task

   Default constructor (all fields to 0 or null)

Task
^^^^

.. java:constructor:: public Task(String name, double flopsAmount, double bytesAmount)
   :outertype: Task

   Construct a new task with the specified processing amount and amount of data needed.

   :param name: Task's name
   :param flopsAmount: A value of the processing amount (in flop) needed to process the task. If 0, then it cannot be executed with the execute() method. This value has to be ≥ 0.
   :param bytesAmount: A value of amount of data (in bytes) needed to transfert this task. If 0, then it cannot be transferred with the get() and put() methods. This value has to be ≥ 0.

Task
^^^^

.. java:constructor:: public Task(String name, Host[] hosts, double[] flopsAmount, double[] bytesAmount)
   :outertype: Task

   Construct a new parallel task with the specified processing amount and amount for each host implied.

   :param name: The name of the parallel task.
   :param hosts: The list of hosts implied by the parallel task.
   :param flopsAmount: The amount of operations to be performed by each host of hosts. flopsAmount[i] is the total number of operations that have to be performed on hosts[i].
   :param bytesAmount: A matrix describing the amount of data to exchange between hosts. The length of this array must be hosts.length * hosts.length. It is actually used as a matrix with the lines being the source and the columns being the destination of the communications.

Methods
-------
cancel
^^^^^^

.. java:method:: public native void cancel()
   :outertype: Task

   Cancels a task.

dsend
^^^^^

.. java:method:: public native void dsend(String mailbox)
   :outertype: Task

   Send the task asynchronously on the specified mailbox, with no way to retrieve whether the communication succeeded or not

dsendBounded
^^^^^^^^^^^^

.. java:method:: public native void dsendBounded(String mailbox, double maxrate)
   :outertype: Task

   Send the task asynchronously on the specified mailbox, with no way to retrieve whether the communication succeeded or not

execute
^^^^^^^

.. java:method:: public native void execute() throws HostFailureException, TaskCancelledException
   :outertype: Task

   Executes a task on the location on which the current process is running.

   :throws HostFailureException:
   :throws TaskCancelledException:

finalize
^^^^^^^^

.. java:method:: @Deprecated @Override protected void finalize() throws Throwable
   :outertype: Task

   Deletes a task once the garbage collector reclaims it

getFlopsAmount
^^^^^^^^^^^^^^

.. java:method:: public native double getFlopsAmount()
   :outertype: Task

   Gets the remaining amount of flops to execute in this task If it's ongoing, you get the exact amount at the present time. If it's already done, it's 0.

getMessageSize
^^^^^^^^^^^^^^

.. java:method:: public double getMessageSize()
   :outertype: Task

getName
^^^^^^^

.. java:method:: public String getName()
   :outertype: Task

   Gets the name of the task

getSender
^^^^^^^^^

.. java:method:: public native Process getSender()
   :outertype: Task

   Gets the sender of the task (or null if not sent yet)

getSource
^^^^^^^^^

.. java:method:: public native Host getSource()
   :outertype: Task

   Gets the source of the task (or null if not sent yet).

irecv
^^^^^

.. java:method:: public static native Comm irecv(String mailbox)
   :outertype: Task

   Starts listening for receiving a task from an asynchronous communication

   :param mailbox:
   :return: a Comm handler

irecvBounded
^^^^^^^^^^^^

.. java:method:: public static native Comm irecvBounded(String mailbox, double rate)
   :outertype: Task

   Starts listening for receiving a task from an asynchronous communication with a capped rate

   :param mailbox:
   :return: a Comm handler

isend
^^^^^

.. java:method:: public native Comm isend(String mailbox)
   :outertype: Task

   Sends the task on the mailbox asynchronously

isendBounded
^^^^^^^^^^^^

.. java:method:: public native Comm isendBounded(String mailbox, double maxrate)
   :outertype: Task

   Sends the task on the mailbox asynchronously (capping the sending rate to \a maxrate)

listen
^^^^^^

.. java:method:: public static native boolean listen(String mailbox)
   :outertype: Task

   Listen whether there is a task waiting (either for a send or a recv) on the mailbox identified by the specified alias

listenFrom
^^^^^^^^^^

.. java:method:: public static native int listenFrom(String mailbox)
   :outertype: Task

   Tests whether there is a pending communication on the mailbox identified by the specified alias, and who sent it

nativeFinalize
^^^^^^^^^^^^^^

.. java:method:: protected native void nativeFinalize()
   :outertype: Task

nativeInit
^^^^^^^^^^

.. java:method:: public static native void nativeInit()
   :outertype: Task

   Class initializer, to initialize various JNI stuff

receive
^^^^^^^

.. java:method:: public static Task receive(String mailbox) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Task

   Retrieves next task on the mailbox identified by the specified alias

   :param mailbox:
   :return: a Task

receive
^^^^^^^

.. java:method:: public static native Task receive(String mailbox, double timeout) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Task

   Retrieves next task on the mailbox identified by the specified alias (wait at most \a timeout seconds)

   :param mailbox:
   :param timeout:
   :return: a Task

receiveBounded
^^^^^^^^^^^^^^

.. java:method:: public static Task receiveBounded(String mailbox, double rate) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Task

   Retrieves next task from the mailbox identified by the specified name with a capped rate

   :param mailbox:
   :return: a Task

receiveBounded
^^^^^^^^^^^^^^

.. java:method:: public static native Task receiveBounded(String mailbox, double timeout, double rate) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Task

   Retrieves next task on the mailbox identified by the specified name (wait at most \a timeout seconds) with a capped rate

   :param mailbox:
   :param timeout:
   :return: a Task

send
^^^^

.. java:method:: public void send(String mailbox) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Task

   Sends the task on the specified mailbox

   :param mailbox: where to send the message
   :throws TimeoutException:
   :throws HostFailureException:
   :throws TransferFailureException:

send
^^^^

.. java:method:: public void send(String mailbox, double timeout) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Task

   Sends the task on the specified mailbox (wait at most \a timeout seconds)

   :param mailbox: where to send the message
   :param timeout:
   :throws TimeoutException:
   :throws HostFailureException:
   :throws TransferFailureException:

sendBounded
^^^^^^^^^^^

.. java:method:: public void sendBounded(String mailbox, double maxrate) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Task

   Sends the task on the specified mailbox (capping the sending rate to \a maxrate)

   :param mailbox: where to send the message
   :param maxrate:
   :throws TransferFailureException:
   :throws HostFailureException:
   :throws TimeoutException:

sendBounded
^^^^^^^^^^^

.. java:method:: public native void sendBounded(String mailbox, double timeout, double maxrate) throws TransferFailureException, HostFailureException, TimeoutException
   :outertype: Task

   Sends the task on the specified mailbox (capping the sending rate to \a maxrate) with a timeout

   :param mailbox: where to send the message
   :param timeout:
   :param maxrate:
   :throws TransferFailureException:
   :throws HostFailureException:
   :throws TimeoutException:

setBound
^^^^^^^^

.. java:method:: public native void setBound(double bound)
   :outertype: Task

   Changes the maximum CPU utilization of a computation task. Unit is flops/s.

setBytesAmount
^^^^^^^^^^^^^^

.. java:method:: public native void setBytesAmount(double bytesAmount)
   :outertype: Task

   Set the amount of bytes to exchange the task Warning if the communication is already started and ongoing, this call does nothing.

   :param bytesAmount: the size of the task

setFlopsAmount
^^^^^^^^^^^^^^

.. java:method:: public native void setFlopsAmount(double flopsAmount)
   :outertype: Task

   Set the computation amount needed to process the task Warning if the execution is already started and ongoing, this call does nothing.

   :param flopsAmount: the amount of computation needed to process the task

setName
^^^^^^^

.. java:method:: public native void setName(String name)
   :outertype: Task

   Sets the name of the task

   :param name: the new task name

setPriority
^^^^^^^^^^^

.. java:method:: public native void setPriority(double priority)
   :outertype: Task

   This method sets the priority of the computation of the task. The priority doesn't affect the transfer rate. For example a priority of 2 will make the task receive two times more cpu than the other ones.

   :param priority: The new priority of the task.

