.. _usecase_simalgo:

Simulating Algorithms
=====================

SimGrid was conceived as a tool to study distributed algorithms. Its
modern S4U interface makes it easy to assess Cloud, P2P, HPC, IoT and
similar settings.

A typical SimGrid simulation is composed of several **Actors**
|api_s4u_Actor|_ , that execute user-provided functions. The actors
have to explicitly use the S4U inteface to express their computation,
communication, disk usage and other **Activities** |api_s4u_Activity|_
, so that they get reflected within the simulator. These activities
take place on **Resources** (CPUs, links, disks). SimGrid predicts the
time taken by each activity and orchestrates accordingly the actors
waiting for the completion of these activities.

.. |api_s4u_Actor| image:: /images/extlink.png
   :align: middle
   :width: 12
.. _api_s4u_Actor: api/classsimgrid_1_1s4u_1_1Actor.html#class-documentation

.. |api_s4u_Activity| image:: /images/extlink.png
   :align: middle
   :width: 12
.. _api_s4u_Activity: api/classsimgrid_1_1s4u_1_1Activity.html#class-documentation


Each actor executes a user-provided function on a simulated **Host**
|api_s4u_Host|_ with which it can interact. Communications are not
directly sent to actors, but posted onto **Mailboxes**
|api_s4u_Mailbox|_ that serve as rendez-vous points between
communicating processes. 

.. |api_s4u_Host| image:: /images/extlink.png
   :align: middle
   :width: 12
.. _api_s4u_Host: api/classsimgrid_1_1s4u_1_1Host.html#class-documentation

.. |api_s4u_Mailbox| image:: /images/extlink.png
   :align: middle
   :width: 12
.. _api_s4u_Mailbox: api/classsimgrid_1_1s4u_1_1Mailbox.html#class-documentation


The Master/Workers Example
--------------------------

This section introduces a first example of SimGrid simulation. This
simple application is composed of two kind of actors: the **master**
is in charge of distributing some computational tasks to a set of
**workers** that execute them. 

.. image:: /images/tuto-masterworkers-intro.svg
   :align: center

We first present a round-robin version of this application, where the
master dispatches the tasks to the workers, one after the other, until
all tasks are dispatched. Later in this tutorial, you will be given
the opportunity to improve this scheme.

The Actors
..........

Let's start with the code of the worker. It is represented by the
*master* function below. This simple function takes 4 parameters,
given as a vector of strings:

   - the number of workers managed by the master.
   - the number of tasks to dispatch
   - the computational size (in flops to compute) of each task 
   - the communication size (in bytes to exchange) of each task

Then, the tasks are sent one after the other, each on a mailbox named
"worker-XXX" where XXX is the number of an existing worker. On the
other side, a given worker (which code is given below) wait for
incomming tasks on its own mailbox. Notice how this mailbox mechanism
allow the actors to find each other without having all information:
the master don't have to know the actors nor even where they are, it
simply pushes the messages on mailbox which name is predetermined. 

At the end, once all tasks are dispatched, the master dispatches
another task per worker, but this time with a negative amount of flops
to compute. Indeed, this application decided by convention, that the
workers should stop when encountering such a negative compute_size.

At the end of the day, the only SimGrid specific functions used in
this example are :func:`simgrid::s4u::Mailbox::by_name` and
:func:`simgrid::s4u::Mailbox::put`. Also, XBT_INFO() is used as a
replacement to printf() or to cout to ensure that the messages are
nicely loggued along with the simulated time and actor name.
 
     
.. literalinclude:: ../../examples/s4u/app-masterworkers/s4u-app-masterworkers-fun.cpp
   :language: c++
   :start-after: master-begin
   :end-before: master-end

Here comes the code of the worker actors. This function expects only one
parameter from its vector of strings: its identifier so that it knows
on which mailbox its incomming tasks will arrive. Its code is very
simple: as long as it gets valid computation requests (whose
compute_amount is positive), it compute this task and waits for the
next one.	

.. literalinclude:: ../../examples/s4u/app-masterworkers/s4u-app-masterworkers-fun.cpp
   :language: c++
   :start-after: worker-begin
   :end-before: worker-end

Starting the Simulation
.......................
		
And this is it. In only a few lines, we defined the algorithm of our
master/workers examples. Well, this is true, but an algorithm alone is
not enough to define a simulation.

First, SimGrid is a library, not a program. So you need to define your
own `main()` function, as follows. This function is in charge of
creating a SimGrid simulation engine (on line 3), register the actor
functions to the engine (on lines 7 and 8), load the virtual platform
from its description file (on line 11), map actors onto that platform
(on line 12) and run the simulation until its completion on line 15.

.. literalinclude:: ../../examples/s4u/app-masterworkers/s4u-app-masterworkers-fun.cpp
   :language: c++
   :start-after: main-begin
   :end-before: main-end
   :linenos:

After that, the missing pieces are the platform and deployment
files.

Platform File
.............

Platform files define the virtual platform on which the provided
application will take place. In contains one or several **Network
Zone** |api_s4u_NetZone|_ that contain both **Host-** |api_s4u_Host|_
and **Link-** |api_s4u_Link|_ Resources, as well as routing
information.

Such files can get rather long and boring, so the example below is
only an excerpts of the full ``examples/platforms/small_platform.xml``
file. For example, most routing information are missing, and only the
route between the hosts Tremblay and Fafard is given. This path
traverses 6 links (4, 3, 2, 0, 1 and 8). The full file, along with
other examples, can be found in the archive under
``examples/platforms``.
		
.. |api_s4u_NetZone| image:: /images/extlink.png
   :align: middle
   :width: 12
.. _api_s4u_NetZone: api/classsimgrid_1_1s4u_1_1NetZone.html#class-documentation

.. |api_s4u_Link| image:: /images/extlink.png
   :align: middle
   :width: 12
.. _api_s4u_Link: api/classsimgrid_1_1s4u_1_1Link.html#class-documentation

.. literalinclude:: ../../examples/platforms/small_platform.xml
   :language: xml
   :lines: 1-10,12-20,56-63,192-
   :caption: (excerpts of the small_platform.xml file)

Deployment File
...............

Deployment files specify the execution scenario: it lists the actors
that should be started, along with their parameter. In the following
example, we start 6 actors: one master and 5 workers.

.. literalinclude:: ../../examples/s4u/app-masterworkers/s4u-app-masterworkers_d.xml
   :language: xml

Execution Example
.................

This time, we have all parts: once the program is compiled, we can
execute it as follows. Note how the XBT_INFO() requests turned into
informative messages.
	      
.. literalinclude:: ../../examples/s4u/app-masterworkers/s4u-app-masterworkers.tesh
   :language: shell
   :start-after: s4u-app-masterworkers-fun
   :prepend: $$$ ./masterworkers platform.xml deploy.xml
   :append: $$$
   :dedent: 2
	      

..  LocalWords:  SimGrid
