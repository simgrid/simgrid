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

First of all, include the Simgrid library and define the log category.

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

In this lab we present how to import a DAG into you Simgrid simulation, either using a DOT file, a JSON file, or a DAX file. 

The files presented in this lab describe the following DAG:

.. image:: /img/dag2.svg
   :align: center

From a DOT file
...............

A DOT file describes a workflow in accordance with the graphviz format.

The following DOT file describes the workflow presented at the beginning of this lab:

.. literalinclude:: ../../examples/cpp/dag-from-dot-simple/dag.dot
   :language: dot

It can be imported as a vector of Activities into Simgrid using :cpp:func:`simgrid::s4u::create_DAG_from_DOT`. Then, you have to assign hosts to your Activities.

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

It can be imported as a vector of Activities into Simgrid using :cpp:func:`simgrid::s4u::create_DAG_from_json`. 

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

It can be imported as a vector of Activities into Simgrid using :cpp:func:`simgrid::s4u::create_DAG_from_DAX`.

.. literalinclude:: ../../examples/cpp/dag-from-dax-simple/s4u-dag-from-dax-simple.cpp
   :language: cpp
