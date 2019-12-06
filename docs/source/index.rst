.. SimGrid documentation master file

The Modern Age of Computer Systems Simulation
=============================================

SimGrid is a framework to simulate distributed computer systems. It can be used
to either :ref:`assess abstract algorithms <usecase_simalgo>`, or to
:ref:`debug and profile real MPI applications <usecase_smpi>`.

SimGrid is routinely used in studies on (data-)Grids,
IaaS Clouds (:ref:`API <API_s4u_VirtualMachine>`, :ref:`examples <s4u_ex_clouds>`),
Clusters, High Performance Computing (`🖹 <http://hal.inria.fr/hal-01415484>`_),
Peer-to-Peer systems,
Volunteer Computing (`🖹 <http://www.sciencedirect.com/science/article/pii/S1569190X17301028>`__)
Fog Computing (`🖹 <http://ieeexplore.ieee.org/document/7946412/>`__),
MapReduce (`🖹 <http://www.sciencedirect.com/science/article/pii/S0167819113000215>`__) and
`much more <https://simgrid.org/Usages.html>`_.

The simulation models are **fast** (`🖹 <http://hal.inria.fr/hal-00650233>`__) and
**highly scalable** (`🖹 <http://hal.inria.fr/inria-00602216/>`__) while
**theoretically sound and experimentally assessed** (`🖹 <http://doi.acm.org/10.1145/2517448>`__).
Most of the time, SimGrid is used to predict the performance (time and energy) of a
given IT infrastructure, and it includes a prototype model checker to formally
assess these systems.

Technically speaking, SimGrid is a library. It is neither a graphical
interface nor a command-line simulator running user scripts. The
interaction with SimGrid is done by writing programs with the exposed
functions to build your own simulator. This can be done in C/C++, Python or Java,
on Linux, Mac OSX or Windows (using the WSL).

SimGrid is a Free Software distributed under the LGPL-2.1 license. You are
welcome to use, study, share and improve it, provided that your version are as
free as ours. SimGrid is developed since 20 years by a lively community of users
and researchers from several groups, initially in France and in the U.S.A. It
is steadily funded by several research bodies. We hope that you will
come and join us!

SimGrid is a powerful tool, and this documentation will help you taking the best
of it. Check its contents on the left. Each tutorial presents a classical use
case, in a fast and practical manner. The user manual contains more
thorough information. In each part, the important concepts are concisely
introduced, before the reference manual. SimGrid is also described in several
`scientific papers <https://simgrid.org/Publications.html>`_.

Please report any documentation issue, including typos or unclear elements. You
can even propose changes by clicking on the "Edit on FramaGit" button at the top
of every page. Bugs in the code should be reported
`on FramaGit <https://framagit.org/simgrid/simgrid/issues>`_


.. toctree::
   :hidden:
   :maxdepth: 1
   :caption: Tutorials:

	Simulating Algorithms <Tutorial_Algorithms.rst>
	Simulating MPI Applications <Tutorial_MPI_Applications.rst>

.. toctree::
   :hidden:
   :maxdepth: 2
   :caption: User Manual:

      Introduction <Introduction.rst>
         Installing SimGrid <Installing_SimGrid.rst>
         Start your Own Project <Start_Your_Own_Project.rst>
      Describing your Application <application.rst>
         The S4U Interface <app_s4u.rst>
         The SMPI Interface <app_smpi.rst>
         The MSG Interface <app_msg.rst>
      Describing the Simulated Platform <platform.rst>
         Examples <Platform_Examples.rst>
         Modeling Hints <platform_howtos.rst>
         XML Reference <XML_Reference.rst>
      Describing the Experimental Setup <Experimental_Setup.rst>
         Configuring SimGrid <Configuring_SimGrid.rst>
         Deploying your Application <Deploying_your_Application.rst>
      The SimGrid Models <models.rst>
         ns-3 as a SimGrid model <ns3.rst>
      SimGrid Plugins <Plugins.rst>
      Simulation Outcomes <outcomes.rst>
      The SimGrid Community <community.rst>
      Frequently Asked Questions <faq.rst>



.. Cheat Sheet on the sublevels
..
..   # with overline, for parts
..   * with overline, for chapters
..   =, for sections
..   -, for subsections
..   ^, for subsubsections
..   ", for paragraphs
