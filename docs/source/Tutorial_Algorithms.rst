.. _usecase_simalgo:

Simulating Algorithms
=====================

SimGrid was conceived as a tool to study distributed algorithms. Its
modern :ref:`S4U interface <S4U_doc>` makes it easy to assess Cloud,
P2P, HPC, IoT, and similar settings.

A typical SimGrid simulation is composed of several |Actors|_, that
execute user-provided functions. The actors have to explicitly use the
S4U interface to express their computation, communication, disk usage,
and other |Activities|_ so that they get reflected within the
simulator. These activities take place on **Resources** (|Hosts|_,
|Links|_, |Disks|_). SimGrid predicts the time taken by each
activity and orchestrates accordingly the actors waiting for the
completion of these activities.

Each actor executes a user-provided function on a simulated |Host|_
with which it can interact. Communications are not directly sent to
actors, but posted onto a |Mailbox|_ that serves as a rendezvous point
between communicating actors.

.. |Actors| replace:: **Actors**
.. _Actors: app_s4u.html#s4u-actor

.. |Activities| replace:: **Activities**
.. _Activities: app_s4u.html#s4u-activity

.. |Hosts| replace:: **Hosts**
.. _Hosts: app_s4u.html#s4u-host

.. |Links| replace:: **Links**
.. _Links: app_s4u.html#s4u-link

.. |Disks| replace:: **Disks**
.. _Disks: app_s4u.html#s4u-disk

.. |VirtualMachines| replace:: **VirtualMachines**
.. _VirtualMachines: app_s4u.html#s4u-virtualmachine

.. |Host| replace:: **Host**
.. _Host: app_s4u.html#s4u-host

.. |Link| replace:: **Link**
.. _Link: app_s4u.html#s4u-link

.. |Mailbox| replace:: **Mailbox**
.. _Mailbox: app_s4u.html#s4u-mailbox

.. |Barrier| replace:: **Barrier**
.. _Barrier: app_s4u.html#s4u-barrier

.. |ConditionVariable| replace:: **ConditionVariable**
.. _ConditionVariable: app_s4u.html#s4u-conditionvariable

.. |Mutex| replace:: **Mutex**
.. _Mutex: app_s4u.html#s4u-mutex

**In the remainder of this tutorial**, you will discover a simple yet
fully-functioning example of SimGrid simulation: the Master/Workers
application. We will detail each part of the code and the necessary
configuration to make it work.  After this tour, several exercises
are proposed to let you discover some of the SimGrid features, hands
on the keyboard. This practical session will be given in C++, which you
are supposed to know beforehand.


Discover the Master/Workers
---------------------------

This section introduces an example of SimGrid simulation. This
simple application is composed of two kinds of actors: the **master**
is in charge of distributing some computational tasks to a set of
**workers** that execute them.

.. image:: /tuto_s4u/img/intro.svg
   :align: center

We first present a round-robin version of this application, where the
master dispatches the tasks to the workers, one after the other, until
all tasks are dispatched. You will improve this scheme later in this tutorial.

The Actors
..........

Let's start with the code of the master. It is represented by the
*master* function below. This simple function takes at least 3
parameters (the number of tasks to dispatch, their computational size
in flops to compute, and their communication size in bytes to
exchange). Every parameter after the third one must be the name of a
host on which a worker is waiting for something to compute.

Then, the tasks are sent one after the other, each on a mailbox named
after the worker's hosts. On the other side, a given worker (which
code is given below) waits for incoming tasks on its 
mailbox.



In the end, once all tasks are dispatched, the master dispatches
another task per worker, but this time with a negative amount of flops
to compute. Indeed, this application decided by convention, that the
workers should stop when encountering such a negative compute_size.

At the end of the day, the only SimGrid specific functions used in
this example are :cpp:func:`simgrid::s4u::Mailbox::by_name` and
:cpp:func:`simgrid::s4u::Mailbox::put`. Also, :c:macro:`XBT_INFO` is used
as a replacement to `printf()` or `std::cout` to ensure that the messages
are nicely logged along with the simulated time and actor name.


.. literalinclude:: ../../examples/cpp/app-masterworkers/s4u-app-masterworkers-fun.cpp
   :language: c++
   :start-after: master-begin
   :end-before: master-end

Here comes the code of the worker actors. This function expects no
parameter from its vector of strings. Its code is very simple: it
expects messages on the mailbox that is named after its host. As long as it gets valid
computation requests (whose compute_amount is positive), it computes
this task and waits for the next one.

The worker retrieves its own host with
:cpp:func:`simgrid::s4u::this_actor::get_host`. The
:ref:`simgrid::s4u::this_actor <API_s4u_this_actor>`
namespace contains many such helping functions.

.. literalinclude:: ../../examples/cpp/app-masterworkers/s4u-app-masterworkers-fun.cpp
   :language: c++
   :start-after: worker-begin
   :end-before: worker-end

Starting the Simulation
.......................

And this is it. In only a few lines, we defined the algorithm of our
master/workers examples.

That being said, an algorithm alone is not enough to define a
simulation: SimGrid is a library, not a program. So you need to define
your own ``main()`` function as follows. This function is in charge of
creating a SimGrid simulation engine (on line 3), register the actor
functions to the engine (on lines 7 and 8), load the simulated platform
from its description file (on line 11), map actors onto that platform
(on line 12) and run the simulation until its completion on line 15.

.. literalinclude:: ../../examples/cpp/app-masterworkers/s4u-app-masterworkers-fun.cpp
   :language: c++
   :start-after: main-begin
   :end-before: main-end
   :linenos:

As you can see, this also requires a platform file and a deployment
file.

Platform File
.............

Platform files define the simulated platform on which the provided
application will take place. It contains one or several **Network
Zone** |api_s4u_NetZone|_ that contains both |Host|_ and |Link|_
Resources, as well as routing information.

Such files can get rather long and boring, so the example below is
only an excerpt of the full ``examples/platforms/small_platform.xml``
file. For example, most routing information is missing, and only the
route between the hosts Tremblay and Fafard is given. This path
traverses 6 links (named 4, 3, 2, 0, 1, and 8). There are several
examples of platforms in the archive under ``examples/platforms``.

.. |api_s4u_NetZone| image:: /img/extlink.png
   :align: middle
   :width: 12
.. _api_s4u_NetZone: app_s4u.html#s4u-netzone

.. |api_s4u_Link| image:: /img/extlink.png
   :align: middle
   :width: 12
.. _api_s4u_Link: app_s4u.html#s4u-link

.. literalinclude:: ../../examples/platforms/small_platform.xml
   :language: xml
   :lines: 1-10,12-20,56-62,192-
   :caption: (excerpts of the small_platform.xml file)

Deployment File
...............

Deployment files specify the execution scenario: it lists the actors
that should be started, along with their parameters. In the following
example, we start 6 actors: one master and 5 workers.

.. literalinclude:: ../../examples/cpp/app-masterworkers/s4u-app-masterworkers_d.xml
   :language: xml

Execution Example
.................

This time, we have all parts: once the program is compiled, we can
execute it as follows. Note how the XBT_INFO() requests turned into
informative messages.

.. "WARNING: Over dedent has detected" is expected here as we remove the $ marker this way

.. literalinclude:: ../../examples/cpp/app-masterworkers/s4u-app-masterworkers.tesh
   :language: shell
   :start-after: s4u-app-masterworkers-fun
   :prepend: $$$ ./masterworkers platform.xml deploy.xml
   :append: $$$
   :dedent: 2

Each example included in the SimGrid distribution comes with a `tesh`
file that presents how to start the example once compiled, along with
the expected output. These files are used for the automatic testing of
the framework but can be used to see the examples' output without
compiling them. See e.g. the file
`examples/cpp/app-masterworkers/s4u-app-masterworkers.tesh <https://framagit.org/simgrid/simgrid/-/blob/master/examples/cpp/app-masterworkers/s4u-app-masterworkers.tesh>`_.
Lines starting with `$` are the commands to execute;
lines starting with `>` are the expected output of each command, while
lines starting with `!` are configuration items for the test runner.


Improve it Yourself
-------------------

In this section, you will modify the example presented earlier to
explore the quality of the proposed algorithm. It already works, and
the simulation prints things, but the truth is that we have no idea of
whether this is a good algorithm to dispatch tasks to the workers.
This very simple setting raises many interesting questions:

.. image:: /tuto_s4u/img/question.svg
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

- How does the quality of such an algorithm dependent on the platform
  characteristics and on the task characteristics?

    Whenever the input communication time is very small compared to
    processing time and workers are homogeneous, it is likely that the
    round-robin algorithm performs very well. Would it still hold true
    when transfer time is not negligible? What if some tasks are
    performed faster on some specific nodes?

- The network topology interconnecting the master and the workers
  may be quite complicated. How does such a topology impact the
  previous result?

    When data transfers are the bottleneck, it is likely that good
    modeling of the platform becomes essential. The SimGrid platform
    models are particularly handy to account for complex platform
    topologies.

- What is the best applicative topology?

    Is a flat master-worker deployment sufficient? Should we go for a
    hierarchical algorithm, with some forwarders taking large pools of
    tasks from the master, each of them distributing their tasks to a
    sub-pool of workers? Or should we introduce super-peers,
    duplicating the master's role in a peer-to-peer manner?  Do the
    algorithms require a perfect knowledge of the network?

- How is such an algorithm sensitive to external workload variation?

    What if bandwidth, latency, and computing speed can vary with no
    warning?  Shouldn't you study whether your algorithm is sensitive
    to such load variations?

- Although an algorithm may be more efficient than another, how does
  it interfere with unrelated applications executing on the same
  facilities?

**SimGrid was invented to answer such questions.** Do not believe the
fools saying that all you need to study such settings is a simple
discrete event simulator. Do you really want to reinvent the wheel,
debug and optimize your own tool, and validate its models against real
settings for ages, or do you prefer to sit on the shoulders of a
giant? With SimGrid, you can focus on your algorithm. The whole
simulation mechanism is already working.

Here is the visualization of a SimGrid simulation of two master-worker
applications (one in light gray and the other in dark gray) running in
concurrence and showing resource usage over a long period of time. It
was obtained with the Triva software.

.. image:: /tuto_s4u/img/result.png
   :align: center

Using Docker
............

The easiest way to take the tutorial is to use the dedicated Docker
image. Once you `installed Docker itself
<https://docs.docker.com/install/>`_, simply do the following:

.. code-block:: shell

   docker pull simgrid/tuto-s4u
   docker run -it --rm --name simgrid --volume ~/simgrid-tutorial:/source/tutorial simgrid/tuto-s4u bash

This will start a new container with all you need to take this
tutorial, and create a ``simgrid-tutorial`` directory in your home on
your host machine that will be visible as ``/source/tutorial`` within the
container.  You can then edit the files you want with your favorite
editor in ``~/simgrid-tutorial``, and compile them within the
container to enjoy the provided dependencies.

.. warning::

   Any change to the container out of ``/source/tutorial`` will be lost
   when you log out of the container, so don't edit the other files!

All needed dependencies are already installed in this container
(SimGrid, a C++ compiler, CMake, pajeng, and R). Vite being only
optional in this tutorial, it is not installed to reduce the image
size.

The code template is available under ``/source/simgrid-template-s4u.git``
in the image. You should copy it to your working directory and
recompile it when you first log in:

.. code-block:: shell

   cp -r /source/simgrid-template-s4u.git/* /source/tutorial
   cd /source/tutorial
   cmake .
   make

Using your Computer Natively
............................

To take the tutorial on your machine, you first need to :ref:`install 
a recent version of SimGrid <install>`, a C++ compiler, and also
``pajeng`` to visualize the traces. You may want to install `Vite
<http://vite.gforge.inria.fr/>`_ to get a first glance at the traces.
The provided code template requires CMake to compile. On Debian and
Ubuntu for example, you can get them as follows:

.. code-block:: shell

   sudo apt install simgrid pajeng cmake g++ vite

For R analysis of the produced traces, you may want to install R
and the `pajengr <https://github.com/schnorr/pajengr#installation/>`_ package.

.. code-block:: shell

   # install R and necessary packages
   sudo apt install r-base r-cran-devtools r-cran-tidyverse
   # install pajengr dependencies
   sudo apt install git cmake flex bison
   # install the pajengr R package
   Rscript -e "library(devtools); install_github('schnorr/pajengr');"

An initial version of the source code is provided on framagit. This
template compiles with CMake. If SimGrid is correctly installed, you
should be able to clone the `repository
<https://framagit.org/simgrid/simgrid-template-s4u>`_ and recompile
everything as follows:

.. code-block:: shell

   # (exporting SimGrid_PATH is only needed if SimGrid is installed in a non-standard path)
   export SimGrid_PATH=/where/to/simgrid

   git clone https://framagit.org/simgrid/simgrid-template-s4u.git
   cd simgrid-template-s4u/
   cmake .
   make

If you struggle with the compilation, then you should double-check
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

For a classical Gantt-Chart visualization, you can use `Vite
<http://vite.gforge.inria.fr/>`_ if you have it installed, as
follows. But do not spend too much time installing Vite, because there
is a better way to visualize SimGrid traces (see below).

.. code-block:: shell

   ./master-workers small_platform.xml master-workers_d.xml --cfg=tracing:yes --cfg=tracing/actor:yes
   vite simgrid.trace

.. image:: /tuto_s4u/img/vite-screenshot.png
   :align: center

.. note::

   If you use an older version of SimGrid (before v3.26), you should use
   ``--cfg=tracing/msg/process:yes`` instead of ``--cfg=tracing/actor:yes``.

If you want the full power to visualize SimGrid traces, you need
to use R. As a start, you can download this `starter script
<https://framagit.org/simgrid/simgrid/raw/master/docs/source/tuto_s4u/draw_gantt.R>`_
and use it as follows:

.. code-block:: shell

   ./master-workers small_platform.xml master-workers_d.xml --cfg=tracing:yes --cfg=tracing/actor:yes
   Rscript draw_gantt.R simgrid.trace

It produces a ``Rplots.pdf`` with the following content:

.. image:: /tuto_s4u/img/Rscript-screenshot.png
   :align: center


Lab 1: Simpler Deployments
--------------------------

In the provided example, adding more workers quickly becomes a pain:
You need to start them (at the bottom of the file) and inform the
master of its availability with an extra parameter. This is mandatory
if you want to inform the master of where the workers are running. But
actually, the master does not need to have this information.

We could leverage the mailbox mechanism flexibility, and use a sort of
yellow page system: Instead of sending data to the worker running on
Fafard, the master could send data to the third worker. Ie, instead of
using the worker location (which should be filled in two locations),
we could use their ID (which should be filled in one location
only).

This could be done with the following deployment file. It's 
not shorter than the previous one, but it's still simpler because the
information is only written once. It thus follows the `DRY
<https://en.wikipedia.org/wiki/Don't_repeat_yourself>`_ `SPOT
<http://wiki.c2.com/?SinglePointOfTruth>`_ design principle.

.. literalinclude:: tuto_s4u/deployment1.xml
   :language: xml


Copy your ``master-workers.cpp`` into ``master-workers-lab1.cpp`` and
add a new executable into ``CMakeLists.txt``. Then modify your worker
function so that it gets its mailbox name not from the name of its
host, but from the string passed as ``args[1]``. The master will send
messages to all workers based on their number, for example as follows:

.. code-block:: cpp

   for (int i = 0; i < tasks_count; i++) {
     std::string worker_rank          = std::to_string(i % workers_count);
     std::string mailbox_name         = std::string("worker-") + worker_rank;
     simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(mailbox_name);

     mailbox->put(...);

     ...
   }


Wrap up
.......

The mailboxes are a very powerful mechanism in SimGrid, allowing many
interesting application settings. They may feel unusual if you are
used to BSD sockets or other classical systems, but you will soon
appreciate their power. They are only used to match 
communications but have no impact on the communication
timing. ``put()`` and ``get()`` are matched regardless of their
initiators' location and then the real communication occurs between
the involved parties.

Please refer to the full `Mailboxes' documentation
<app_s4u.html#s4u-mailbox>`_ for more details.


Lab 2: Using the Whole Platform
-------------------------------

It is now easier to add a new worker, but you still have to do it
manually. It would be much easier if the master could start the
workers on its own, one per available host in the platform. The new
deployment file should be as simple as:

.. literalinclude:: tuto_s4u/deployment2.xml
   :language: xml


Creating the workers from the master
....................................

For that, the master needs to retrieve the list of hosts declared in
the platform with :cpp:func:`simgrid::s4u::Engine::get_all_hosts`.
Then, the master should start the worker actors with
:cpp:func:`simgrid::s4u::Actor::create`.

``Actor::create(name, host, func, params...)`` is a very flexible
function. Its third parameter is the function that the actor should
execute. This function can take any kind of parameter, provided that
you pass similar parameters to ``Actor::create()``. For example, you
could have something like this:

.. code-block:: cpp

  void my_actor(int param1, double param2, std::string param3) {
    ...
  }
  int main(int argc, char argv**) {
     ...
     simgrid::s4u::ActorPtr actor;
     actor = simgrid::s4u::Actor::create("name", simgrid::s4u::Host::by_name("the_host"),
                                         &my_actor, 42, 3.14, "thevalue");
     ...
  }


Master-Workers Communication
............................

Previously, the workers got from their parameter the name of the
mailbox they should use. We can still do so: the master should build
such a parameter before using it in the ``Actor::create()`` call. The
master could even pass directly the mailbox as a parameter to the
workers.

Since we want later to study concurrent applications, it is advised to
use a mailbox name that is unique over the simulation even if there is
more than one master.

One possibility for that is to use the actor ID (aid) of each worker
as a mailbox name. The master can retrieve the aid of the newly
created actor with ``actor->get_pid()`` while the actor itself can
retrieve its own aid with ``simgrid::s4u::this_actor::get_pid()``.
The retrieved value is an ``aid_t``, which is an alias for ``long``.

Instead of having one mailbox per worker, you could also reorganize
completely your application to have only one mailbox per master. All
the workers of a given master would pull their work from the same
mailbox, which should be passed as a parameter to the workers.
This requires fewer mailboxes but prevents the master from taking
any scheduling decision. It depends on how you want to organize
your application and what you want to study with your simulator. In
this tutorial, that's probably not a good idea.

Wrap up
.......

In this exercise, we reduced the amount of configuration that our
simulator requests. This is both a good idea and a dangerous
trend. This simplification is another application of the good old DRY/SPOT
programming principle (`Don't Repeat Yourself / Single Point Of Truth
<https://en.wikipedia.org/wiki/Don%27t_repeat_yourself>`_), and you
really want your programming artifacts to follow these software
engineering principles.

But at the same time, you should be careful in separating your
scientific contribution (the master/workers algorithm) and the
artifacts used to test it (platform, deployment, and workload). This is
why SimGrid forces you to express your platform and deployment files
in XML instead of using a programming interface: it forces a clear
separation of concerns between things of different nature.

Lab 3: Fixed Experiment Duration
--------------------------------

In the current version, the number of tasks is defined through the
worker arguments. Hence, tasks are created at the very beginning of
the simulation. Instead, have the master dispatching tasks for a
predetermined amount of time.  The tasks must now be created on need
instead of beforehand.

Of course, usual time functions like ``gettimeofday`` will give you the
time on your real machine, which is pretty useless in the
simulation. Instead, retrieve the time in the simulated world with
:cpp:func:`simgrid::s4u::Engine::get_clock`.

You can still stop your workers with a specific task as previously,
or you may kill them forcefully with :cpp:func:`simgrid::s4u::Actor::kill`.

Anyway, the new deployment `deployment3.xml` file should thus look
like this:

.. literalinclude:: tuto_s4u/deployment3.xml
   :language: xml

Controlling the message verbosity
.................................

Not all messages are equally informative, so you probably want to
change some of the ``XBT_INFO`` into ``XBT_DEBUG`` so that they are
hidden by default. For example, you may want to use ``XBT_INFO`` once
every 100 tasks and ``XBT_DEBUG`` when sending all the other tasks. Or
you could show only the total number of tasks processed by
default. You can still see the debug messages as follows:

.. code-block:: shell

   ./master-workers-lab3 small_platform.xml deployment3.xml --log=s4u_app_masterworker.thres:debug


Lab 4: Competing Applications
-----------------------------

It is now time to start several applications at once, with the following ``deployment4.xml`` file.

.. literalinclude:: tuto_s4u/deployment4.xml
   :language: xml

Things happen when you do so, but it remains utterly difficult to
understand what's happening exactly. Even Gantt visualizations
contain too much information to be useful: it is impossible to
understand which task belongs to which application. To fix this, we
will categorize the tasks.

Instead of starting the execution in one function call only with
``this_actor::execute(cost)``, you need to
create the execution activity, set its tracing category, and then start
it and wait for its completion, as follows:

.. code-block:: cpp

   simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(compute_cost);
   exec->set_tracing_category(category);
   // exec->start() is optional here as wait() starts the activity on need
   exec->wait();

You can shorten this code as follows:

.. code-block:: cpp

   simgrid::s4u::this_actor::exec_init(compute_cost)->set_tracing_category(category)->wait();

Visualizing the result
.......................

vite is not enough to understand the situation, because it does not
deal with categorization. This time, you absolutely must switch to R,
as explained on `this page
<https://simgrid.org/contrib/R_visualization.html>`_.

.. todo::

   Include here the minimal setting to view something in R.


Lab 5: Better Scheduling
------------------------

You don't need a very advanced visualization solution to notice that
round-robin is completely suboptimal: most of the workers keep waiting
for more work. We will move to a First-Come First-Served mechanism
instead.

For that, your workers should explicitly request  work with a
message sent to a channel that is specific to their master. The name
of that private channel can be the one used to categorize the
executions, as it is already specific to each master.

The master should serve in a round-robin manner the requests it
receives until the time is up. Changing the communication schema can
be a bit hairy, but once it works, you will see that such as simple
FCFS schema allows one to double the number of tasks handled over time
here. Things may be different with another platform file.

Further Improvements
....................

From this, many things can easily be added. For example, you could:

- Allow workers to have several pending requests  to overlap
  communication and computations as much as possible. Non-blocking
  communication will probably become handy here.
- Add a performance measurement mechanism, enabling the master to make smart scheduling choices.
- Test your code on other platforms, from the ``examples/platforms``
  directory in your archive.

  What is the largest number of tasks requiring 50e6 flops and 1e5
  bytes that you manage to distribute and process in one hour on
  ``g5k.xml`` ?
- Optimize not only for the number of tasks handled but also for the
  total energy dissipated.
- And so on. If you come up with a nice extension, please share
  it with us so that we can extend this tutorial.

After this Tutorial
-------------------

This tutorial is now terminated. You could keep reading the online documentation and
tutorials, or you could head up to the :ref:`example section <s4u_examples>` to read some code.

.. todo::

   Things to improve in the future:

   - Propose equivalent exercises and skeleton in java (and Python once we have a python binding).

..  LocalWords:  SimGrid
