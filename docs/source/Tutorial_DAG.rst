.. _simdag:

Simulating DAG
==============

This tutorial presents the basics to understand how DAG are represented in Simgrid and how to simulate their workflow. 

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

.. image:: /tuto_dag/img/dag.svg
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
Dependencies between Activities are created using :cpp:func:`Activity::add_successor(ActivityPtr)`.

.. code-block:: cpp

   exec->add_successor(comm);

The Activity ``comm`` will not start until ``exec`` has been completed.

Lab 1: Basics
---------------

The goal of this lab is to describe the following DAG: 

.. image:: /tuto_dag/img/dag1.svg
   :align: center

In this DAG we want ``c1`` to compute 1e9 flops, ``c2`` to compute 5e9 flops and ``c3`` to compute 2e9 flops. 
There is also a data transfer of 5e8 bytes between ``c1`` and ``c3``.

First of all, include the Simgrid library and define the log category.

.. code-block:: cpp

   #include "simgrid/s4u.hpp"

   XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this s4u tutorial");

Inside the ``main`` function create an instance of :ref:`Engine <API_s4u_Engine>` and load the platform.

.. code-block:: cpp

    simgrid::s4u::Engine e(&argc, argv);
    e.load_platform(argv[1]);

Retrieve pointers to some hosts.

.. code-block:: cpp

    simgrid::s4u::Host* tremblay = e.host_by_name("Tremblay");
    simgrid::s4u::Host* jupiter  = e.host_by_name("Jupiter");

Initiate the activities.

.. code-block:: cpp

    simgrid::s4u::ExecPtr c1 = simgrid::s4u::Exec::init();
    simgrid::s4u::ExecPtr c2 = simgrid::s4u::Exec::init();
    simgrid::s4u::ExecPtr c3 = simgrid::s4u::Exec::init();
    simgrid::s4u::CommPtr t1 = simgrid::s4u::Comm::sendto_init();

Give names to thoses activities.

.. code-block:: cpp

    c1->set_name("c1");
    c2->set_name("c2");
    c3->set_name("c3");
    t1->set_name("t1");

Set the amount of work for each activity.

.. code-block:: cpp

    c1->set_flops_amount(1e9);
    c2->set_flops_amount(5e9);
    c3->set_flops_amount(2e9);
    t1->set_payload_size(5e8);

Define the dependencies between the activities.

.. code-block:: cpp

    c1->add_successor(t1);
    t1->add_successor(c3);
    c2->add_successor(c3);

Set the location of each Exec activity and source and destination for the Comm activity.

.. code-block:: cpp

    c1->set_host(tremblay);
    c2->set_host(jupiter);
    c3->set_host(jupiter);
    t1->set_source(tremblay);
    t1->set_destination(jupiter);

Start the executions of Activities without dependencies.

.. code-block:: cpp

    c1->start();
    c2->start();

Add a callback to monitor the activities.

.. code-block:: cpp

   Activity::on_completion_cb([](simgrid::s4u::Activity const& activity) {
      XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", activity.get_cname(), activity.get_start_time(),
               activity.get_finish_time());
      });

Finally, run the simulation.

.. code-block:: cpp

   e.run();

The execution of this code should give you the following output:

.. code-block:: bash

   [10.194200] [main/INFO] Activity 'c1' is complete (start time: 0.000000, finish time: 10.194200)
   [65.534235] [main/INFO] Activity 'c2' is complete (start time: 0.000000, finish time: 65.534235)
   [85.283378] [main/INFO] Activity 't1' is complete (start time: 10.194200, finish time: 85.283378)
   [111.497072] [main/INFO] Activity 'c3' is complete (start time: 85.283378, finish time: 111.497072)

Lab 2: Import a DAG from a file
---------------

In this lab we present how to import a DAG into you Simgrid simulation, either using a DOT file, a JSON file, or a DAX file. 

The files presented in this lab describe the following DAG:

.. image:: /tuto_dag/img/dag2.svg
   :align: center

From a DOT file
...............

A DOT file describes a workflow in accordance with the graphviz format.

The following DOT file describes the workflow presented at the beginning of this lab:

.. code-block:: xml

   digraph G {
      c1 [size="1e9"];
      c2 [size="5e9"];
      c3 [size="2e9"];

      root->c1 [size="2e8"];
      root->c2 [size="1e8"];
      c1->c3   [size="5e8"];
      c2->c3   [size="-1"];
      c3->end  [size="2e8"];
   }

It can be imported as a vector of Activities into Simgrid using :cpp:func:`create_DAG_from_DOT(const std::string& filename)`. Then, you have to assign hosts to your Activities.

.. code-block:: cpp

   #include "simgrid/s4u.hpp"

   XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this s4u example");

   int main(int argc, char* argv[]) {
      simgrid::s4u::Engine e(&argc, argv);
      e.load_platform(argv[1]);

      std::vector<simgrid::s4u::ActivityPtr> dag = simgrid::s4u::create_DAG_from_dot(argv[2]);

      simgrid::s4u::Host* tremblay = e.host_by_name("Tremblay");
      simgrid::s4u::Host* jupiter  = e.host_by_name("Jupiter");
      simgrid::s4u::Host* fafard  = e.host_by_name("Fafard");

      dynamic_cast<simgrid::s4u::Exec*>(dag[0].get())->set_host(fafard);
      dynamic_cast<simgrid::s4u::Exec*>(dag[1].get())->set_host(tremblay);
      dynamic_cast<simgrid::s4u::Exec*>(dag[2].get())->set_host(jupiter);
      dynamic_cast<simgrid::s4u::Exec*>(dag[3].get())->set_host(jupiter);
      dynamic_cast<simgrid::s4u::Exec*>(dag[8].get())->set_host(jupiter);
    
      for (const auto& a : dag) {
         if (auto* comm = dynamic_cast<simgrid::s4u::Comm*>(a.get())) {
               auto pred = dynamic_cast<simgrid::s4u::Exec*>((*comm->get_dependencies().begin()).get());
               auto succ = dynamic_cast<simgrid::s4u::Exec*>(comm->get_successors().front().get());
               comm->set_source(pred->get_host())->set_destination(succ->get_host());
         }
      }

      simgrid::s4u::Activity::on_completion_cb([](simgrid::s4u::Activity const& activity) {
      XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", activity.get_cname(), activity.get_start_time(),
             activity.get_finish_time());
      });

      e.run();
      return 0;
   }

The execution of this code should give you the following output:

.. code-block:: bash

   [0.000000] [main/INFO] Activity 'root' is complete (start time: 0.000000, finish time: 0.000000)
   [33.394394] [main/INFO] Activity 'root->c2' is complete (start time: 0.000000, finish time: 33.394394)
   [39.832311] [main/INFO] Activity 'root->c1' is complete (start time: 0.000000, finish time: 39.832311)
   [50.026511] [main/INFO] Activity 'c1' is complete (start time: 39.832311, finish time: 50.026511)
   [98.928629] [main/INFO] Activity 'c2' is complete (start time: 33.394394, finish time: 98.928629)
   [125.115689] [main/INFO] Activity 'c1->c3' is complete (start time: 50.026511, finish time: 125.115689)
   [151.329383] [main/INFO] Activity 'c3' is complete (start time: 125.115689, finish time: 151.329383)
   [151.743605] [main/INFO] Activity 'c3->end' is complete (start time: 151.329383, finish time: 151.743605)
   [151.743605] [main/INFO] Activity 'end' is complete (start time: 151.743605, finish time: 151.743605)

From a JSON file
................

A JSON file describes a workflow in accordance with the `wfformat <https://github.com/wfcommons/wfformat>`_ .

The following JSON file describes the workflow presented at the beginning of this lab:

.. code-block:: JSON

   {
      "name": "simple_json",
      "schemaVersion": "1.0",
      "workflow": {
         "makespan": 0,
         "executedAt": "2023-03-09T00:00:00-00:00",
         "tasks": [
         {
            "name": "c1",
            "type": "compute",
            "parents": [],
            "runtime": 1e9,
            "machine": "Tremblay"
         },
         {
            "name": "t1",
            "type": "transfer",
            "parents": ["c1"],
            "bytesWritten": 5e8,
            "machine": "Jupiter"
         },
         {
            "name": "c2",
            "type": "compute",
            "parents": [],
            "runtime": 5e9,
            "machine": "Jupiter"
         },
         {
            "name": "c3",
            "type": "compute",
            "parents": ["t1","c2"],
         "runtime": 2e9,
         "machine": "Jupiter"
         }
         ],
         "machines": [
            {"nodeName": "Tremblay"},
            {"nodeName": "Jupiter"}
         ]
      }
   }

It can be imported as a vector of Activities into Simgrid using :cpp:func:`create_DAG_from_json(const std::string& filename)`. 

.. code-block:: cpp

   #include "simgrid/s4u.hpp"

   XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this s4u example");

   int main(int argc, char* argv[]) {
      simgrid::s4u::Engine e(&argc, argv);
      e.load_platform(argv[1]);

      std::vector<simgrid::s4u::ActivityPtr> dag = simgrid::s4u::create_DAG_from_json(argv[2]);

      simgrid::s4u::Activity::on_completion_cb([](simgrid::s4u::Activity const& activity) {
      XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", activity.get_cname(), activity.get_start_time(),
             activity.get_finish_time());
      });

      e.run();
      return 0;
   }

The execution of this code should give you the following output:

.. code-block:: bash

   [10.194200] [main/INFO] Activity 'c1' is complete (start time: 0.000000, finish time: 10.194200)
   [65.534235] [main/INFO] Activity 'c2' is complete (start time: 0.000000, finish time: 65.534235)
   [85.283378] [main/INFO] Activity 't1' is complete (start time: 10.194200, finish time: 85.283378)
   [111.497072] [main/INFO] Activity 'c3' is complete (start time: 85.283378, finish time: 111.497072)

From a DAX file [deprecated]
............................

A DAX file describes a workflow in accordance with the `Pegasus <http://pegasus.isi.edu/>`_ format.

The following DAX file describes the workflow presented at the beginning of this lab:

.. code-block:: xml

   <?xml version="1.0" encoding="UTF-8"?>
   <adag xmlns="http://pegasus.isi.edu/schema/DAX" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:schemaLocation="http://pegasus.isi.edu/schema/DAX http://pegasus.isi.edu/schema/dax-2.1.xsd"
      version="2.1">
      <job id="1" name="c1" runtime="10">
         <uses file="i1" link="input" register="true" transfer="true" optional="false" type="data" size="2e8"/>
         <uses file="o1" link="output" register="true" transfer="true" optional="false" type="data" size="5e8"/>
      </job>
      <job id="2" name="c2" runtime="50">
         <uses file="i2" link="input" register="true" transfer="true" optional="false" type="data" size="1e8"/>
      </job>
      <job id="3" name="c3" runtime="20">
         <uses file="o1" link="input" register="true" transfer="true" optional="false" type="data" size="5e8"/>
         <uses file="o3" link="output" register="true" transfer="true" optional="false" type="data" size="2e8"/>
      </job>
      <child ref="3">
         <parent ref="1"/>
         <parent ref="2"/>
      </child>
   </adag>

It can be imported as a vector of Activities into Simgrid using :cpp:func:`create_DAG_from_DAX(std::string)`.

.. code-block:: cpp

   #include "simgrid/s4u.hpp"

   XBT_LOG_NEW_DEFAULT_CATEGORY(main, "Messages specific for this s4u example");

   int main(int argc, char* argv[]) {
      simgrid::s4u::Engine e(&argc, argv);
      e.load_platform(argv[1]);

      std::vector<simgrid::s4u::ActivityPtr> dag = simgrid::s4u::create_DAG_from_DAX(argv[2]);

      simgrid::s4u::Host* tremblay = e.host_by_name("Tremblay");
      simgrid::s4u::Host* jupiter  = e.host_by_name("Jupiter");
      simgrid::s4u::Host* fafard  = e.host_by_name("Fafard");

      dynamic_cast<simgrid::s4u::Exec*>(dag[0].get())->set_host(fafard);
      dynamic_cast<simgrid::s4u::Exec*>(dag[1].get())->set_host(tremblay);
      dynamic_cast<simgrid::s4u::Exec*>(dag[2].get())->set_host(jupiter);
      dynamic_cast<simgrid::s4u::Exec*>(dag[3].get())->set_host(jupiter);
      dynamic_cast<simgrid::s4u::Exec*>(dag[8].get())->set_host(jupiter);
    
      for (const auto& a : dag) {
         if (auto* comm = dynamic_cast<simgrid::s4u::Comm*>(a.get())) {
            auto pred = dynamic_cast<simgrid::s4u::Exec*>((*comm->get_dependencies().begin()).get());
            auto succ = dynamic_cast<simgrid::s4u::Exec*>(comm->get_successors().front().get());
            comm->set_source(pred->get_host())->set_destination(succ->get_host());
         }
      }

      simgrid::s4u::Activity::on_completion_cb([](simgrid::s4u::Activity const& activity) {
      XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", activity.get_cname(), activity.get_start_time(),
         activity.get_finish_time());
      });

      e.run();
      return 0;
   }

