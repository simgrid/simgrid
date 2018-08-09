.. _usecase_simalgo:

Simulating Algorithms
=====================

SimGrid was conceived as a tool to study distributed algorithms. Its
modern S4U interface makes it easy to assess Cloud, P2P, HPC, IoT and
similar settings.

A typical SimGrid simulation is composed of several **Actors**
|api_s4u_Actor|_ , that execute user-provided functions. The actors
have to explicitly use the S4U interface to express their computation,
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


Discover the Master/Workers
---------------------------

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
incoming tasks on its own mailbox. Notice how this mailbox mechanism
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
nicely logged along with the simulated time and actor name.
 
     
.. literalinclude:: ../../examples/s4u/app-masterworkers/s4u-app-masterworkers-fun.cpp
   :language: c++
   :start-after: master-begin
   :end-before: master-end

Here comes the code of the worker actors. This function expects only one
parameter from its vector of strings: its identifier so that it knows
on which mailbox its incoming tasks will arrive. Its code is very
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
	      

Improve it Yourself
-------------------

In this section, you will modify the example presented earlier to
explore the quality of the proposed algorithm. For now, it works and
the simulation prints things, but the truth is that we have no idea of
whether this is a good algorithm to dispatch tasks to the workers.
This very simple setting raises many interesting questions:

.. image:: /images/tuto-masterworkers-question.svg
   :align: center

- Which algorithm should the master use? Or should the worker decide
  by themselves?

    Round Robin is not an efficient algorithm when all tasks are not
    processed at the same speed.  It would probably be more efficient
    if the workers were asking for tasks when ready.

- Should tasks be grouped in batches or sent separately?

    The workers will starve if they don't get the tasks fast
    enough. One possibility to reduce latency would be to send tasks
    in pools instead of one by one. But if the pools are too big, the
    load balancing will likely get uneven, in particular when
    distributing the last tasks.

- How does the quality of such algorithm dependent on the platform
  characteristics and on the task characteristics?

    Whenever the input communication time is very small compared to
    processing time and workers are homogeneous, it is likely that the
    round-robin algorithm performs very well. Would it still hold true
    when transfer time is not negligible? What if some tasks are
    performed faster on some specific nodes?
	   
- The network topology interconnecting the master and the workers
  may be quite complicated. How does such a topology impact the
  previous result?

    When data transfers are the bottleneck, it is likely that a good
    modeling of the platform becomes essential. The SimGrid platform
    models are particularly handy to account for complex platform
    topologies.

- What is the best applicative topology?

    Is a flat master worker deployment sufficient? Should we go for a
    hierarchical algorithm, with some forwarders taking large pools of
    tasks from the master, each of them distributing their tasks to a
    sub-pool of workers? Or should we introduce super-peers,
    dupplicating the master's role in a peer-to-peer manner?  Do the
    algorithms require a perfect knowledge of the network?

- How is such an algorithm sensitive to external workload variation?

    What if bandwidth, latency and computing speed can vary with no
    warning?  Shouldn't you study whether your algorithm is sensitive
    to such load variations?

- Although an algorithm may be more efficient than another, how does
  it interfere with unrelated applications executing on the same
  facilities?

**SimGrid was invented to answer such questions.** Do not believe the
fools saying that all you need to study such settings is a simple
discrete event simulator. Do you really want to reinvent the wheel,
debug your own tool, optimize it and validate its models against real
settings for ages, or do you prefer to sit on the shoulders of a
giant? With SimGrid, you can focus on your algorithm. The whole
simulation mechanism is already working.

Here is the visualization of a SimGrid simulation of two master worker
applications (one in light gray and the other in dark gray) running in
concurrence and showing resource usage over a long period of time. It
was obtained with the Triva software.

.. image:: /images/tuto-masterworkers-result.png
   :align: center

Prerequisite
............

Before your proceed, you need to :ref:`install SimGrid <install>`, a
C++ compiler and also ``pajeng`` to visualize the traces. The provided
code template requires cmake to compile. On Debian and Ubuntu for
example, you can get them as follows:

.. code-block:: shell

   sudo apt install simgrid pajeng cmake g++

An initial version of the source code is provided on framagit. This
template compiles with cmake. If SimGrid is correctly installed, you
should be able to clone the repository and recompile everything as
follows:

.. code-block:: shell

   git clone git@framagit.org:simgrid/simgrid-template-s4u.git
   cd simgrid-template-s4u/
   cmake .
   make

If you struggle with the compilation, then you should double check
your :ref:`SimGrid installation <install>`.  On need, please refer to
the :ref:`Troubleshooting your Project Setup
<install_yours_troubleshooting>` section.

Discovering the Provided Code
.............................

Please compile and execute the provided simulator as follows:


.. code-block:: shell

   make master-workers
   ./master-workers small_platform.xml master-workers_d.xml

For a more "fancy" output, you can use simgrid-colorizer. 

.. code-block:: shell

   ./master-workers small_platform.xml master-workers_d.xml 2>&1 | simgrid-colorizer

If you installed SimGrid to a non-standard path, you may have to
specify the full path to simgrid-colorizer on the above line, such as
``/opt/simgrid/bin/simgrid-colorizer``. If you did not install it at all,
you can find it in <simgrid_root_directory>/bin/colorize.

.. todo::

   Explain how to generate a Gantt-Chart with S4U and pajeng.



..  LocalWords:  SimGrid
