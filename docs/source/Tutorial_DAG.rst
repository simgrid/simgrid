.. _simdag:

Simulating DAG
==============

This tutorial presents the basics to understand how DAG are represented in SimGrid and how to simulate their workflow.

Definition of a DAG
-------------------

Directed Acyclic Graph: 

.. math::

   \mathcal{G} = (\mathcal{V},\mathcal{E})

Set of vertices representing :ref:`Activities <API_s4u_Activity>`: 

.. math::

   \mathcal{V} = {v_i | i = 1, ..., V}

Set of edges representing precedence constraints between :ref:`Activities <API_s4u_Activity>`: 

.. math::

   \mathcal{E} = {e_i,j | (i,j) \in {1, ..., V} x {1, ..., V}}

.. image:: /img/dag.svg
   :align: center

Representing Vertices/Activities
................................

There is two types of :ref:`Activities <API_s4u_Activity>` that can represent Vertices: :ref:`Exec <API_s4u_Exec>` and :ref:`Comm <API_s4u_Comm>`.
Thoses activities must be initiated and configured to properly describe your worflow.

An Exec represents the execution of an amount of flop on a :ref:`Host <API_s4u_Host>` of your platform.

.. code-block:: cpp

   ExecPtr exec = Exec::init();
   exec->set_flops_amount(int);
   exec->set_host(Host*);
   exec->start();

A Comm represents a data transfer between two :ref:`Hosts <API_s4u_Host>` of your platform. 

.. code-block:: cpp

   CommPtr comm = Comm::sendto_init();
   comm->set_source(Host*);
   comm->set_destination(Host*);
   comm->start();

Representing Edges/Dependencies
...............................

An activity will not start until all of its dependencies have been completed.
Activities may have any number of successors.
Dependencies between Activities are created using :cpp:func:`simgrid::s4u::Activity::add_successor`.

.. code-block:: cpp

   exec->add_successor(comm);

The Activity ``comm`` will not start until ``exec`` has been completed.

Lab 1: Basics
---------------

The goal of this lab is to describe the following DAG: 

.. image:: /img/dag1.svg
   :align: center

In this DAG we want ``c1`` to compute 1e9 flops, ``c2`` to compute 5e9 flops and ``c3`` to compute 2e9 flops. 
There is also a data transfer of 5e8 bytes between ``c1`` and ``c3``.

First of all, include the SimGrid library and define the log category.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 6-8

Inside the ``main`` function create an instance of :ref:`Engine <API_s4u_Engine>` and load the platform.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 12-13

Retrieve pointers to some hosts.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 15-16

Initiate the activities.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 18-21

Give names to thoses activities.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 23-26

Set the amount of work for each activity.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 28-31

Define the dependencies between the activities.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 33-35

Set the location of each Exec activity and source and destination for the Comm activity.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 37-41

Start the executions of Activities without dependencies.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 43-44

Add a callback to monitor the activities.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 46-49

Finally, run the simulation.

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.cpp
   :language: cpp
   :lines: 51

The execution of this code should give you the following output:

.. literalinclude:: ../../examples/cpp/dag-tuto/s4u-dag-tuto.tesh
   :language: none
   :lines: 4-

Lab 2: Import a DAG from a file
-------------------------------

In this lab we present how to import a DAG into you SimGrid simulation, either using a DOT file, a JSON file, or a DAX file.

The files presented in this lab describe the following DAG:

.. image:: /img/dag2.svg
   :align: center

From a DOT file
...............

A DOT file describes a workflow in accordance with the graphviz format.

The following DOT file describes the workflow presented at the beginning of this lab:

.. literalinclude:: ../../examples/cpp/dag-from-dot-simple/dag.dot
   :language: dot

It can be imported as a vector of Activities into SimGrid using :cpp:func:`simgrid::s4u::create_DAG_from_DOT`. Then, you have to assign hosts to your Activities.

.. literalinclude:: ../../examples/cpp/dag-from-dot-simple/s4u-dag-from-dot-simple.cpp
   :language: cpp

The execution of this code should give you the following output:

.. literalinclude:: ../../examples/cpp/dag-from-dot-simple/s4u-dag-from-dot-simple.tesh
   :language: none
   :lines: 4-

From a JSON file
................

A JSON file describes a workflow in accordance with the `wfformat <https://github.com/wfcommons/wfformat>`_ .

The following JSON file describes the workflow presented at the beginning of this lab:

.. literalinclude:: ../../examples/cpp/dag-from-json-simple/dag.json
   :language: json

It can be imported as a vector of Activities into SimGrid using :cpp:func:`simgrid::s4u::create_DAG_from_json`.

.. literalinclude:: ../../examples/cpp/dag-from-json-simple/s4u-dag-from-json-simple.cpp
   :language: cpp

The execution of this code should give you the following output:

.. literalinclude:: ../../examples/cpp/dag-from-json-simple/s4u-dag-from-json-simple.tesh
   :language: none
   :lines: 4-

From a DAX file [deprecated]
............................

A DAX file describes a workflow in accordance with the `Pegasus <http://pegasus.isi.edu/>`_ format.

The following DAX file describes the workflow presented at the beginning of this lab:

.. literalinclude:: ../../examples/cpp/dag-from-dax-simple/dag.xml
   :language: xml

It can be imported as a vector of Activities into SimGrid using :cpp:func:`simgrid::s4u::create_DAG_from_DAX`.

.. literalinclude:: ../../examples/cpp/dag-from-dax-simple/s4u-dag-from-dax-simple.cpp
   :language: cpp

Lab 3: Scheduling with the Min-Min algorithm
--------------------------------------------

In this lab we present how to schedule activities imported from a DAX file using the 
`Min-Min algorithm <https://www.researchgate.net/figure/The-Traditional-Min-Min-Scheduling-Algorithm_fig5_236346423>`_.

The source code for this lab can be found `here <https://framagit.org/simgrid/simgrid/-/blob/stable/examples/cpp/dag-scheduling/s4u-dag-scheduling.cpp>`_.

For code readability we first create the `sg4` namespace.

.. code-block:: cpp

   namespace sg4 = simgrid::s4u;

The core mechanism of the algorithm lies in three functions. 
They respectively serve the purpose of finding tasks to schedule, 
finding the best host to execute them and properly scheduling them.

Find Tasks to Schedule
......................

The role of this function is to retrieve tasks that are ready to be scheduled, i.e, that have their dependencies solved.

.. literalinclude:: ../../examples/cpp/dag-scheduling/s4u-dag-scheduling.cpp
   :language: cpp
   :lines: 15-38

Find the Best Placement
.......................

Once we have a task ready to be scheduled, we need to find the best placement for it.
This is done by evaluating the earliest finish time among all hosts.
It depends on the duration of the data transfers of the parents of this task to this host.

.. literalinclude:: ../../examples/cpp/dag-scheduling/s4u-dag-scheduling.cpp
   :language: cpp
   :lines: 40-91

Schedule a Task
...............

When the best host has been found, the task is scheduled on it:

* it sets the host of the task to schedule
* it stores the finish time of this task on the host
* it sets the destination of parents communication
* it sets the source of any child communication.

.. literalinclude:: ../../examples/cpp/dag-scheduling/s4u-dag-scheduling.cpp
   :language: cpp
   :lines: 93-113

Mixing it all Together
......................

Now that we have the key components of the algorithm let's merge them inside the main function.

.. code-block:: cpp

   int main(int argc, char** argv)
   {
   ...

First, we initialize the SimGrid Engine.

.. code-block:: cpp

   sg4::Engine e(&argc, argv);

The Min-Min algorithm schedules unscheduled tasks. 
To keep track of them we make use of the method :cpp:func:`simgrid::s4u::Engine::track_vetoed_activities`.

.. code-block:: cpp

   std::set<sg4::Activity*> vetoed;
   e.track_vetoed_activities(&vetoed);

We add the following callback that will be triggered at the end of execution activities.
This callback stores the finish time of the execution, 
to use it as a start time for any subsequent communications.

.. code-block:: cpp

  sg4::Activity::on_completion_cb([](sg4::Activity const& activity) {
    // when an Exec completes, we need to set the potential start time of all its ouput comms
    const auto* exec = dynamic_cast<sg4::Exec const*>(&activity);
    if (exec == nullptr) // Only Execs are concerned here
      return;
    for (const auto& succ : exec->get_successors()) {
      auto* comm = dynamic_cast<sg4::Comm*>(succ.get());
      if (comm != nullptr) {
        auto* finish_time = new double(exec->get_finish_time());
        // We use the user data field to store the finish time of the predecessor of the comm, i.e., its potential start
        // time
        comm->set_data(finish_time);
      }
    }
  });

We load the platform and force sequential execution on hosts.

.. code-block:: cpp

   e.load_platform(argv[1]);

  /* Mark all hosts as sequential, as it ought to be in such a scheduling example.
   *
   * It means that the hosts can only compute one thing at a given time. If an execution already takes place on a given
   * host, any subsequently started execution will be queued until after the first execution terminates */
  for (auto const& host : e.get_all_hosts()) {
    host->set_concurrency_limit(1);
    host->set_data(new double(0.0));
  }

The tasks are imported from a DAX file.

.. code-block:: cpp

  /* load the DAX file */
  auto dax = sg4::create_DAG_from_DAX(argv[2]);

We look for the best host for the root task and schedule it. 
We then advance the simulation to unlock next schedulable tasks.

.. code-block:: cpp

  /* Schedule the root first */
  double finish_time;
  auto* root = static_cast<sg4::Exec*>(dax.front().get());
  auto host  = get_best_host(root, &finish_time);
  schedule_on(root, host);
  e.run();

Then, we get to the major loop of the algorithm.
This loop goes on until all tasks have been scheduled and executed.
It starts by finding ready tasks using `get_ready_tasks`. 
It iteratively looks for the task that will finish first among ready tasks using `get_best_host`, and place it using `schedule_on`. 
When no more tasks can be placed, we advance the simulation.

.. code-block:: cpp

  while (not vetoed.empty()) {
    XBT_DEBUG("Start new scheduling round");
    /* Get the set of ready tasks */
    auto ready_tasks = get_ready_tasks(dax);
    vetoed.clear();

    if (ready_tasks.empty()) {
      /* there is no ready exec, let advance the simulation */
      e.run();
      continue;
    }
    /* For each ready exec:
     * get the host that minimizes the completion time.
     * select the exec that has the minimum completion time on its best host.
     */
    double min_finish_time   = std::numeric_limits<double>::max();
    sg4::Exec* selected_task = nullptr;
    sg4::Host* selected_host = nullptr;

    for (auto exec : ready_tasks) {
      XBT_DEBUG("%s is ready", exec->get_cname());
      double finish_time;
      host = get_best_host(exec, &finish_time);
      if (finish_time < min_finish_time) {
        min_finish_time = finish_time;
        selected_task   = exec;
        selected_host   = host;
      }
    }

    XBT_INFO("Schedule %s on %s", selected_task->get_cname(), selected_host->get_cname());
    schedule_on(selected_task, selected_host, min_finish_time);

    ready_tasks.clear();
    e.run();
  }

Finally, we clean up the memory.

.. code-block:: cpp

   /* Cleanup memory */
   for (auto const& h : e.get_all_hosts())
     delete h->get_data<double>();







