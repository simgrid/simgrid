.. _contrib_doc:

Contributor's Documentation
===========================

This page describes the software infrastructure for SimGrid potential contributors. It is particularly interesting if you plan to send some patches for
inclusion in the project code.

Enforcing the coding standards
------------------------------

If you plan to contribute code to the SimGrid project, you must ensure that your changes follow our coding standards. For that, simply install the relevant
tools and scripts.

.. code-block:: console

   # install clang-format
   sudo apt-get install clang-format # debian/ubuntu

   # tell git to call the script on each commit
   ln -s $(realpath tools/git-hooks/clang-format.pre-commit) .git/hooks/pre-commit

This will add an extra verification before integrating any commit that you could prepare. If your code does not respects our formatting code, git will say so,
and provide a ready to use patch that you can apply to improve your commit. Just read the error message you get to find how to apply the fix after review.

If you prefer not to apply the fix on a specific commit (because the formatter fails on this case), then add ``--no-verify`` to your git commit command line.

Interacting with cmake
----------------------

Configuring
^^^^^^^^^^^

The :ref:`default build configuration <install_src_config>` is intended for users, but contributors should change these settings for their comfort, and to
produce better code. In particular, the default is to build highly optimized binaries, at the price of high compilation time and somewhat harder debug sessions.
Instead, contributors want to disable the ``enable_compile_optimizations`` compile option. Likewise, ``enable_compile_warnings`` is off by default to not bother
the users, but the contributors should really enable it, as your code will not be integrated if it raises warnings from the compiler.

.. code-block:: console

   cmake -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON \
         -GNinja .

As you can see, we advise to use Ninja instead of the good old Makefiles. In our experience, Ninja is a bit faster because it does not fork as many external
commands as make for basic operations.

Adding a new file
^^^^^^^^^^^^^^^^^

The content of the archive is mostly built from the lists given in ``tools/cmake/DefinePackages.cmake``. The only exceptions are the files related to the
examples, that are listed in the relevant ``CMakeLists.txt`` file, e.g. ``examples/cpp/CMakeLists.txt``. This information is duplicated in ``MANIFEST.in`` for
the python building process, that does not use cmake to build the archive.

When you add a new file, you should use the relevant build target to check that every files followed by git are included in the archive or properly excluded (by
being listed in ``tools/internal/check_dist_archive.exclude``), and that all settings are consistent.

Interacting with the tests
--------------------------

Breaking tests once in a while is completely OK in SimGrid. We accept temporarily breakages because we want things to evolve, but if you break something, it is
your duty to fix it within 24 hours to not hinder the work of others.

Understanding the existing tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We use unit tests based on `Catch2 <https://github.com/catchorg/Catch2/>`_, but most of our testing is done through integration tests that launch a specific
simulation example and enforce that the output remains unchanged. Since SimGrid displays the timestamp of every logged event, such integration tests ensure that
every change of the models' prediction will be noticed. We designed a specific tool for these integration tests, called TESH (the TEsting SHell). ``ctest`` is
used to run both unit and integration tests. You can :ref:`run only some of the tests <install_src_test>` if you prefer. To launch the unit tests out of ctest,
build and execute the ``./unit-tests`` binary (it accepts a ``--help`` parameter). To rebuild and run all tests, use the ``./BuildSimGrid.sh`` script.

The SimGrid code base contains a lot of tests for a coverage well over 80%. We want the contributors to feel confident, and improve the code without fearing of
breaking something without noticing. Tests must be fast (because we run them very often), the should stress many aspects of the code (to actually catch problems) and
clear to read (understanding and debugging failures should be easy).

In fact, our integration tests fall in two categories. The ones located in ``examples/`` are part of the documentation, so readability is utterly important
here. The goal is that novice users could grab some of the provided examples and assemble them to constitute a first version of their simulator. These files can
only load public header files, and we tend to avoid advanced C++ constructs that would hinder the readability of these files. On the other hand, the integration
tests located in ``teshsuite/`` are usually torture tests trying to exhaustively cover all cases. They may be less readable and some even rely on a small DSL to
describe all test cases and the expected result (check for example ``teshsuite/models/cloud-sharing``), but they should still be rather easy to debug on need.
For this reason, we tend to avoid random testing that is difficult to reproduce. For example, the tests under ``teshsuite/models/`` try to be very verbose to
make the model intend clear, and the code easier to debug on need. The WiFi tests seem to be a good example of that trend.

Adding a test
^^^^^^^^^^^^^

We often say that a feature that is not tested is a feature that could soon disappear. So you want to write tests for the features you add. To add new unit
tests, please refer to the end of ``tools/cmake/Tests.cmake``) for some examples. Catch2 comes with a good documentation and many examples online. If you add a
new feature, you should add an integration test in ``examples/``. Your code should be instructive, documented, reusable, and its output must be perfectly
reproducible. Some debugging information can be hidden when run from TESH with the :c:macro:`XBT_LOG_ISENABLED` macro. You then need to write a tesh file,
following the syntax described in this man page: ``man -l manpages/tesh.1`` Finally, you should add your source code and tesh file to the cmake infrastructure
by modifying for example the ``examples/cpp/CMakeLists.txt`` file. The test name shall allow the filtering of tests with ``ctest -R``. Do not forget to run
``make distcheck`` once you're done to check that you did not forget to add your new files to the distribution.

Noteworthy tests
^^^^^^^^^^^^^^^^

Here is a short list of some typical tests in SimGrid, in the hope that they can give you some inspiration for your own tests.
Note that good tests rarely involve randomness, which makes it difficult to understand and fix problems when they occur.

* Several **model tests** display the full computation and expected results for some simple cases. Check for example the tesh files
  of `teshsuite/models/cm02-tcpgamma/ <https://framagit.org/simgrid/simgrid/-/tree/master/teshsuite/models/cm02-tcpgamma>`_
  (about the impact of the :ref:`cfg=network/TCP-gamma` parameter), `teshsuite/models/wifi_usage/
  <https://framagit.org/simgrid/simgrid/-/tree/master/teshsuite/models/wifi_usage>`_ (on the wifi network model),
  `teshsuite/models/wifi_usage_decay/ <https://framagit.org/simgrid/simgrid/-/tree/master/teshsuite/models/wifi_usage_decay>`_
  (on the decay component of the wifi model, for highly congestioned APs), or `teshsuite/models/ptask_L07_usage/
  <https://framagit.org/simgrid/simgrid/-/tree/master/teshsuite/models/ptask_L07_usage>`_ (on parallel tasks). The goal of these tests
  is to be easy to understand just by reading their output.

  `teshsuite/models/cloud-sharing/ <https://framagit.org/simgrid/simgrid/-/tree/master/teshsuite/models/cloud-sharing>`_ is a
  bit special here, because it introduces a mini-DSL. Its purpose is to test the CPU sharing between competing tasks with and
  without VMs. The model is not very complex, but the corresponding code is somewhat intricated, thus the need for such an
  extensive testing. In this test, ``( [o]2 )4`` denotes a test with only one task (only one ``o``) on a VM with two cores (``[
  ]2``), which is itself on a PM with four cores (``( )4``). Likewise, ``( [oo]1 [ ]1 )2`` tests 2 VMs with one core each, one of
  them being empty and the other containing 2 active tasks. Both VMs are located on a PM with two cores.  |br|
  For each of these tests, we ensure that the performance delivered to each task matches our expectations, and that the reported
  amount of used cores is also correct to ensure correct energy predictions. 

* We also have several **torture tests**, such as `teshsuite/s4u/comm-pt2pt/comm-pt2pt.cpp
  <https://framagit.org/simgrid/simgrid/-/tree/master/teshsuite/s4u/comm-pt2pt/comm-pt2pt.cpp>`_ which tests exhaustively each
  case of a communication matching when no error is involved. Regular, asynchronous, detached communications, and communications
  with a permanent receiver declared are tests, both with the send first and with the receive first. Again, we have a tiny DSL
  to specify the tests to do (see the ``usage()`` function). 
  
  `teshsuite/s4u/activity-lifecycle/ <https://framagit.org/simgrid/simgrid/-/tree/master/teshsuite/s4u/activity-lifecycle>`_
  tortures all kind of activities (sleep, exec, comm and direct-comm). It kills their ressource at several instants, kills the
  host hosting the running actor, test the activity after death, etc. This time also, we have a little DSL but this time in the
  code: the source is heavily loaded with templates to make it easier to write many such tests, at the expense of readability.

  SimGrid also features a `chaos monkey <https://en.wikipedia.org/wiki/Chaos_engineering>`_ to apply torture testing to complex
  applications. It is composed of a plugin (embeeded in any SimGrid simulation) that can be used to get info about the resources
  and the timestamps at which actions happens, or to kill a given resource at a given time. It also features `a script
  <https://framagit.org/simgrid/simgrid/-/blob/master/tools/simgrid-monkey>`_ that uses this information to launch a given
  simulation many times, killing each resource at each timestamp to see how SimGrid and your code react to these failures. The
  monkey can even run your simulation in Valgrind to detect problems that would be silent otherwise. Try passing
  ``--cfg=plugin:cmonkey`` to your simulations for more information. We currently have a master-workers example (in both C++ and
  Python) and a semaphore example (in C++ only) in ``teshsuite/s4u``.

Continuous integrations
^^^^^^^^^^^^^^^^^^^^^^^

We have many online build bots that launch the tests on various configurations and operating systems. The results are centralized on two Jenkins jobs: `the main
one <https://ci.inria.fr/simgrid/job/SimGrid/>`_ runs all tests on a variety of systems for each commit, while `Nightly
<https://ci.inria.fr/simgrid/job/SimGrid-Nightly/>`_ runs some additional code analysis every night. Several sister projects built on top of SimGrid regularly
test our git too. The FramaGit project gathers some additional `build badges <https://framagit.org/simgrid/simgrid>`_, and results are posted on the `bot-office
channel <https://framateam.org/simgrid/channels/bot-office>`_ on Mattermost. Our code quality is tested every night using `SonarQube
<https://sonarcloud.io/dashboard?id=simgrid_simgrid>`_ , and the `Debian build daemon <https://buildd.debian.org/status/package.php?p=simgrid>`_ test each
release on several CPU architectures. We maintain some scripts to interact with these tools under ``tools/cmake``.

Interacting with git
--------------------

The SimGrid community is relatively small and active. We are not used to maintain long-standing branches but instead, we strive to do our changes in an
incremental way, commiting all intermediary steps to the main branch. The only thing is to ensure that you don't break things on the path, i.e. that all tests
pass for all commits. If you prefer to do a branch to accumulate some commits in a branch, you should strive to make so for a short period of time to reduce the
burden of the branch maintenance. Large changes happen from time to time to the SimGrid source code, and your branch may become hard to rebase.

It is nice if your commit message could follow the git habits, explained in this `blog post
<http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html>`_, or in the `style guide of Atom
<https://github.com/atom/atom/blob/master/CONTRIBUTING.md#git-commit-messages>`_.

Type naming standard
--------------------

* Filenames shall be unique in the whole project (because of a bug in Sonar coverage computation).

In C++:

* Fields, methods and variables are in snake_case();
* Classes and Enum names are in UpperCamelCase;
* Enum values are in UPPER_SNAKE_CASE, just as constants.

* Files of public modules are usually named ``api_Class.cpp`` and ``api/Class.hpp`` (e.g. ``src/s4u/s4u_ConditionVariable.cpp`` and
  ``include/simgrid/s4u/ConditionVariable.hpp``).
* Files of internal modules are usually named ``Class.cpp`` and ``Class.hpp`` (e.g. ``src/kernel/activity/Activity.cpp`` and
  ``src/kernel/activity/Activity.hpp``) unless it raises name conflicts.

In C:

* Getters and setters are named ``sg_object_get_field()`` and ``sg_object_field()`` (e.g. ``sg_link_get_name()`` and ``sg_link_set_data()``);
* Variables and functions are snake_case();
* Typedefs do not hide the pointers, i.e. the * must be explicit. ``char* sg_host_get_name(sg_host_t* host)``.

Unsorted hints
--------------

* To thoroughly test your changes before pushing your commits, use several cmake configurations under sub-trees of ``build/``
  (that is ignored by git) as explained in :ref:`install_cmake_outsrc`. For example, I have the following directories:
  build/clang build/full build/mc (but YMMV).

* If you add or remove a file, you can check that everything is correctly setup with ``make distcheck``.

* If everything gets crazy, as if it were not using the code that you actually compile, well, maybe it's using another version
  of SimGrid on your disk. Use ``ldd`` on any simulator to check which library gets used. If you installed a SimGrid package,
  you probably need to uninstall it.

* If you break the logs, you want to define XBT_LOG_MAYDAY at the beginning of log.h. It deactivates the whole logging
  mechanism, switching to printfs instead. SimGrid then becomes incredibly verbose, but it you let you fixing things.

.. |br| raw:: html

   <br />
