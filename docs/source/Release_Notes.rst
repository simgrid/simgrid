.. release_notes:

SimGrid 4 Release Notes
=======================

This page gathers some release notes, to shed light on the recent and current development of SimGrid.
Version 3.12 was the last pure release of SimGrid 3 while all versions starting with v3.13 can be seen as usable pre-versions of SimGrid 4.
Ancient versions are not documented here (the project started in 1998), but in the ChangeLog file only.

Version 3.13 (Apr 27. 2016)
---------------------------

The Half Release, a.k.a. the Zealous Easter Trim:

.. rst-class:: compact-list

  * Remove half of the lines of code: v3.12 was 286k lines while v3.13 is only 142k lines.  |br|
    Experimental untested unused "features" removed, and several parts were rewritten from C to C++.

  * Introducing v4 of the XML platform format (many long-due cleanups)
  * MSG examples fully reorganized (in C and Java)
  * The S4U interface is rising, toward SimGrid 4:|br| All host manipulations now done in S4U, 
    SimDag was mostly rewritten on top of S4U but MSG & SimDag interfaces mostly unchanged.

Version 3.14 (Dec 25. 2016)
---------------------------

This version (aka, the Christmas Pi) mainly introduces internal reorganization on the way to the future SimGrid 4 version.
These changes should be transparent to the users of the MSG, SMPI and SimDag interfaces, even if some buggy features were reworked
while some new features were added.

Users are expected to switch to this new version at their own pace, as we do not have the manpower to do any bugfixing on the old releases.
This version was tested on several Linux distributions, Mac OSX, Windows (restricted to the Java bindings when not using the Ubuntu
subsystem of Win10), FreeBSD and NetBSD.

Main changes:

.. rst-class:: compact-list

  * Documentation reorganized and improved
  * S4U interface further rising, toward SimGrid 4: |br|
    Routing code rewritten for readability; Virtual Machines almost turned into a plugin; MSG, SimDag, MPI interfaces mostly unchanged.
  * The model-checker now works on NetBSD too.

Version 3.14.159 (Dec 29. 2016)
-------------------------------

The "Christmas Pi (better approximation)" release fixes various small glitches from the 3.14 version.

Version 3.15 (March 22 2017)
----------------------------

The Spring Release: continuous integration servers become green.

.. rst-class:: compact-list

  * S4U: progress, integrating more parts of SimDag; New examples.
  * SMPI: Support MPI 2.2; Convert internals to C++ (TBC).
  * Java: Massive memleaks and performance issues fixed.
  * Plus the usual bug fixes, cleanups and documentation improvements.

Users are expected to switch to this new version at their own pace, as we do not have the manpower to do any bugfixing on the old releases.
This version was tested on several Linux distributions, Mac OSX, Windows (restricted to our Java bindings), FreeBSD and NetBSD.
None of our 600+ integration tests is known to fail on any of these.

Version 3.16 (June 22 2017)
---------------------------

The Blooming Spring Release: developments are budding.

.. rst-class:: compact-list

  * S4U: Progress; Activity refcounting is now automatic.
  * XML: <AS> can now be named <zone> as they should.
  * SMPI: Further performance improvements; RMA support.
  * Cloud: Multi-core VMs (do not overcommit them yet)
  * (+ bug fixes, cleanups and documentation improvements)

SMPI performance was further improved, using various OS-level magic to speed up our virtualization of the MPI code to be run. This allowed
Tom to run a simulation of HPL involving 10^6 processes... Wow. The dlopen privatization schema should even allows to run your ranks
in parallel (even if it's not well tested yet).

On the Cloud side, we implemented multi-core VMs, which naturally acts as containers of processes;
S4U, the future interface of SimGrid 4.0 also progressed quite a bit.

The Storage is currently cleaned up by Fred, and some API changes are to be expected. We are sorry but the exisiting API is so crappy that
nobody ever used it, I guess. If you need it, please speak up soon!

We renamed AS into NetZone in the XML files (but the old one still work so no rush to update your platform). Since our platforms are
hierarchical, it makes no sense to name our zones "Autonomous Systems". Some other documentation pages got updated and modified. If
you see a particularly old, misleading or otherwise ugly doc page, please tell us and we'll fix it. Even typo reports are welcome.

But most of the work on S4U is not directly visible from the user. We rewamped the whole code of activities (comms, execs, mutex, etc) to
not mix the applicative logic (calling test() or wait()) with the object refcounting. Now, you can test your comm object as many time as
you want. This change was really intrusive in our internals, and we're not done with restabilizing every bits, but we're on it.

Still on the S4U front, we managed to remove a few more XBT modules. We prefer to use the std or boost libraries nowadays, and switching
away from the XBT module enable to reduce our maintainance burden. Be warned that XBT will not always remain included in SimGrid.

On the infrastructure side, we are trying to setup a regular build task for the main projects using SimGrid, to check that our changes
don't break them. The one of StarPU is close to be working (even if not completely). If you want to have your own code tested regularly
against the SimGrid git to warn us about breakage that we introduce, please come to us. We can grant you the right to do the needed config
in our Jenkins instance.

v3.16 also contains the usual bug fixes, such as the jarfile that should now work on Mac OSX (this time for real :) or the Java bindings
that should now be clear of any memory leak.

In addition, the future already started. We have ongoing changesets that were not ready for 3.16 but should be part of 3.17:

.. rst-class:: compact-list

  - Energy modeling for the network too
  - New reduction algorithm for the model-checker, based on event folding structures
  - Multi-model simulations, to specify a differing networking model for each netzone.

Version 3.17 (Oct 8. 2017)
--------------------------

This version is dubbed the "The Drained Leaks release", because almost no known memleak remains, despite testing.

.. rst-class:: compact-list

  * Many many internal cleanups (almost 700 commits since 3.16).
  * The coverage of our tests is above 80%.
  * All memleaks but one plugged; A dozen of bugs fixed.
  * XBT: Further replace XBT with std::* constructs.

Version 3.18 (Dec. 24 2017)
---------------------------

This is an important version for SimGrid: MSG is now deprecated, and new projects should use S4U instead.
There is still some work to do before SimGrid 4: S4U is not ready for SimDag users yet unfortunately. This will come for sure.

Main changes in the "Ho Ho Ho! SimGrid 4 beta is coming to town" release:

.. rst-class:: compact-list

  * Convert almost all interesting MSG examples to S4U.
  * New model: energy consumption due to the network.
  * Major cleanups in the disk and storage subsystems.
  * (+ further deprecate XBT, bug fixes and doc improvement)

SimGrid 4 *may* be there by the next solstice.

Version 3.19 (March 21. 2018)
-----------------------------

In total, this "Moscovitly-cold Spring" release brings more than 500 commits made by 7 individuals over the last 3 months.

.. rst-class:: compact-list

  * SMPI: Allow to start new actors and ranks after simulation start.
  * SMPI: Support ICC, better testing on classical proxy apps.
  * Some kernel headers are now installed, allowing external plugins.
  * (+ the classical bug fixes and doc improvement)

Version 3.19.1 (March 22. 2018)
-------------------------------

As you may know, we are currently refactoring SimGrid in deep.
Upcoming SimGrid4 will be really different from SimGrid3: modular, standard and extensible vs. layered, homegrown and rigid. C++ vs. C.

Our goal is to smooth this transition, with backward compatibility and automatic update paths, while still progressing toward SimGrid4.

SimGrid remains open during works: The last pure SimGrid3 release was v3.12 while all subsequent versions are usable alpha versions of
SimGrid4: Existing interfaces remain unchanged, but the new S4U interface is budding and the internals are deeply reorganized.

Since 2015, we work hard to reduce the changes to public APIs. When we need to rename a public library symbol in S4U, we let your compiler
issue an explicative warning when you use the deprecated function. These messages remain for four releases, i.e. for one full year,
before turning into an error. Starting with v3.15, your can also adapt to API changes with the SIMGRID_VERSION macro, that is defined to
31500 for v3.15, to 31901 for v3.19.1 and so on.

Starting with this v3.19.1, our commitment to reduce the changes to the public interfaces is extended from the API to the ABI: a program
using only MSG or SimDag and compiled against a given version of simgrid can probably be used with a later version of SimGrid without
recompilation. We will do our best... but don't expect too much of it, that's a really difficult goal during such profund refactoring.

The difference between v3.19 and v3.19.1 is that the former was accidentally breaking the ABI of MSG, while the later is restoring the
previous ABI.

S4U and kernel APIs will still evolve until SimGrid4, with one-year deprecation warnings as currently. In fact, cleaning up these
interfaces and converting them to snake_case() is one release goal of v3.20. But don't worry, we are working to smooth this upgrade path.

Once the S4U interface stabilizes, we will provide C bindings on top of it, along with Java and Python ones. Maybe in 3.21 or 3.22.

All this is not contradictory with the fact that MSG as a whole is deprecated, because this deprecation only means that new projects
should go for S4U instead of MSG to benefit of the future. Despite this deprecation, old MSG projects should still be usable with no 
change, if we manage to. This is a matter of scientific reproducibility to us.

Version 3.20 (June 25 2018)
---------------------------

We were rather productive this season, with a total of 837 commits made by 8 individuals over the last 3 months.

The most visible change is the S4U API sanitizing. We were using an awful mix of snake_case and CamelCase, and we now use snake_case
everywhere. We apologize for the inconvenience, but it's for sake of sanity. Plus, we put portability wrappers in place: you don't have to
change your code until v3.24 if you can live with warnings. The MSG API was not changed, of course.

The robustness of SMPI continues to improve. It was rock stable, and you can now use it to move the world (if your lever is long enough).
We now use several full-scale projects as nightly integration tests: StarPU, BigDFT and also 45 Proxy Apps from various collections.
https://framagit.org/simgrid/SMPI-proxy-apps

Main changes in the "proxy snake_case()" release are:

.. rst-class:: compact-list

  * Sanitize the public API. Compatibility wrappers in place for one year.
  * More CI: ~45 Proxy Apps + BigDFT + StarPU now tested nightly
  * MPI: Port the trace replay engine to C++, fix visualization
  * (+ the classical bug fixes and doc improvement)

Version 3.21 (October 5. 2018)
------------------------------

This release introduces a few nice features, but the most visible is certainly the new documentation. We started to completely overhaul it.
The result is still somewhat in progress, but we feel that it's much better already. We added a complete tutorial on S4U, we started a
tutorial on SMPI (still incomplete), we slightly improved the MSG and Java doc, and greatly improved the S4U doc. The section on writing
platform files is not converted in the new doc and you'll have to refer to the 3.20 documentation for that (sorry -- time went out).

Please give us feedback on this new doc. We want to make it as useful to you as possible, but it's very hard without (constructive) feedback
from your side ;)

Another big change is that we are currently moving our development from github to framagit. We thought that framagit is a better place to
develop an Open Source project as ours. Head now to https://simgrid.org You can still use github if you prefer to use closed source code ;)

Main changes of The Restarting Documentation (TRD) release:

.. rst-class:: compact-list

  * Start to overhaul the documentation, and move to Sphinx + RTD.
  * Allow dynamic replay of MPI apps, controlled by S4U actors
  * Rewrite the support for auto-restarted actors (was utterly broken)
  * (+ the classical bug fixes and doc improvement)

Version 3.22 (April 2. 2019)
----------------------------

The Easter Christmas Release. It was expected from Chrismas, but I was so late that I even managed to miss the spring deadline.
This started to be a running joke, so I decided to release it for April 1. But I'm even late for this... Sorry :)

.. rst-class:: compact-list

  * Introducing the Python bindings (still beta)
  * Doc: SMPI tutorial and platform description ported to RTD
  * Many internal cleanups leading to some user-level speedups
  * (+ the classical bug fixes and internal refactorings)

The most visible change is certainly the new Python bindings. They are rather experimental yet, and their API may change a bit in future
release, but you are already welcome to test them. Many examples are now also available in Python, and the missing ones are on their way.

This new bindings further deprecates the old MSG and Java interfaces, which are still provided (and will remain so for a few years at least
for the existing users). Their examples are now hidden in deprecated/ Please switch to S4U if you like C++ or to Python if not.

This new version also introduce a heavy load of internal cleanups. Fred converted more internals to real C++, with more classes and less
procedural cruft. Henri and the other Wrench developpers reported many bugs around activity canceling and resource failures, and we fixed
quite a bit of them, but many dark snakes remain in that lake. Fred and Martin converted more doc to the new system (the platform chapter
is not finished, but it's not worse than the old one either) while Augustin completed the tutorial for MPI applications. Augustin also
added several non-blocking collectives to SMPI, even if Martin finally decided to release right before he could complete the last ones
(sorry). We continued cutting on XBT, replacing many functions and modules by their standard counterparts in C++11 or in Boost. We are
now using Catch2 for our unit testing. These cleanups may speedup your simulations by something like 10%.

Version 3.23 (June 25. 2019)
----------------------------

Main change in the "Exotic Solstice" Release:

.. rst-class:: compact-list

  * Support for Solaris and Haiku OSes. Just for fun :)
  * SMPI: more of MPI3.1; some MPI/IO and async collectives.
  * Python bindings can now be installed from pip.
  * (+ many many bug fixes and internal refactorings)

Version 3.24 (October 10. 2019)
-------------------------------

This is the Clean Disk Release:

.. rst-class:: compact-list

  * Introduce an experimental Wifi network model.
  * Introduce <disk> (cleaner logic than <storage>).
  * SMPI: Implement Errhandlers and some more MPI3.1 calls.
  * (+ many bug fixes and internal refactorings)

Since June, we continued our new release schema: v3.23.2 got released at some point as an interim release for people wanting something
between stable releases (tested on many systems but coming at most once per quarter) and git version (almost always working but you never
know). We plan to do so more often in the future, maybe with one interim version per month. Between interim versions, we use an odd
version number: v3.23.1 then 3.23.3 until yesterday, and soon 3.24.1.

As a user, there is no urgency to upgrade, even if you should not wait more than 9 months to upgrade to another stable version: our policy is
to keep backward compatibility and nice upgrading pathes for 3 stable versions.  v3.24 removes symbols that got deprecated in v3.20, last
year. It deprecates things that will continue to work until v3.27.

Speaking of deprecation, we would like to hear from you if you are using the Java bindings under Windows without the WSL installed.
Maintaining these native bindings are rather tedious, and we are wondering whether having Java+WSL would be sufficient.

In any case, please remember that we like to hear success stories, i.e. reports of the nice things you did with SimGrid. Not only bug
reports are welcome :)

Version 3.25 (Feb 2. 2020)
--------------------------

This is the "Palindrom Day" release (today is 02 02 2020).

.. rst-class:: compact-list

   * Improve the Python usability (stability and documentation). |br|
     A nasty synchronization bug (due to a bad handling of the GIL) was ironed out, so that no known bug remains in Python examples.
     The Python documentation is now integrated with the C++ one, also along with the C bindings that were previously not documented.
     The API documentation is now split by theme in the hope to keep it readable.

   * Further deprecate MSG: you now have to pass -Denable_msg=ON to cmake. |br|
     This is OFF by default (also disabling the Java API that is still based on MSG).
     The plan is to completely remove MSG by 2020Q4 or 2021Q1.

   * SimDAG++: Automatic dependencies on S4U activities (experimental). |br|
     This implements some features of SimDAG within S4U, but not all of them: you cannot block an activity until it's scheduled on a resource 
     and there is no heterogeneous wait_any() that would mix Exec/Comm/Io activities. See ``examples/s4u/{io,exec,comm}-dependent`` for what's already there.

Since last fall, we continued to push toward the future SimGrid4 release. This requires to remove MSG and SimDAG once all users have
migrated to S4U. The two old interfaces are still here, but this release gives another gentle incitative toward the migration. You now
have to explicitly ask for MSG to be compiled in (and it may be removed by Q42020 or Q12021 along with the current Java bindings), and
this release proposes a nice S4U replacement for some parts of SimDAG.

Since last release also, we had no answer of potential users of the Java bindings on Windows without the WSL installed. We will probably
drop this architecture in the near future, then. Simplifying our settings is mandatory to continue to push SimGrid forward.

Version 3.26 (Dec 16. 2020)
---------------------------

To celebrate the ease of the lockdown in France, we decided to bring another version of SimGrid to the world.
This is the "Release" release. Indeed a small contribution to the event, but this release was long overdue anyway.

.. rst-class:: compact-list

  * SMPI: improved support of the proxy apps (including those using petsc)
  * WiFi: easier description in XML; energy plugin; more examples.
  * ns-3: Many bug fixes, can use the wifi models too.
  * (+ many bug fixes, documentation improvement and internal refactoring)

Version 3.27 (March 29. 2021)
-----------------------------

To celebrate the 1176th anniversary of the siege of Paris by Vikings in 845, we just released another version of SimGrid, the Ragnar Release.
Yeah, that's a stupid release name, but we already had 4 "spring release" in the past, so we needed another name.

.. rst-class:: compact-list

  * SMPI: can now report leaks and hint about the mallocs and kernels hindering simulation scalability.
  * Doc: Several new sections in the user manual, and start documenting the internals.
  * S4U: Direct comms from host to host, without mailboxes.

In some sense, these changes are just the tip of the iceberg, as we had many refactoring and internal cleanups in this release cycle too. Actually, we have 3
main ongoing refactoring that should bring us closer to SimGrid4, that will eventually occur.

The first change is dubbed SimDAG++. We want to make it possible to use S4U in the same spirit as SimDAG: centralized scheduling of tasks with dependencies. We
need to allow the maestro thread (the one that currently only call engine->run() in the main) to create asynchronous activities, chain them by declaring
dependencies, and run the simulation until some event of interest occurs.

Previous release introduced inter-activity dependency in s4u, this release introduces direct host-to-host communications (bypassing the mailboxes), but we
are not there yet: maestro cannot create asynchronous activities, and there is no way to run the simulation up to a certain point only.

The second ongoing refactoring is about the platform creation. Our goal is to provide a C++ API to create your platform from your code, without relying on
XML. There is a real possibility that this one will be part of the 3.28 release, in three months. Will see.

And the third front is about modernizing the model checker implementation. The current state is very difficult to work with, and we hope that once it's
simplified, we will be able to implement more efficient state space reduction techniques, and also allow more synchronization mechanism to be used in the
model checker (for now, our dpor algorithm cannot cope with mutexes).

In parallel to these refactoring, the work on SMPI stability and robustness peacefully continued. The list of MPI applications that can now work with
absolutely no change on top of SMPI really gets impressive... Check it yourself: https://framagit.org/simgrid/SMPI-proxy-apps

If you want to speak about it (or other SimGrid-related matter), please join us on Mattermost: https://framateam.org/simgrid/channels/town-square
Come! You don't even have to accept the cookies for that!

Version 3.28 (July 14. 2021)
----------------------------

To celebrate the birthday of Crown Princess Victoria, we just released another version of SimGrid, the "Victoriadagarna" release.

.. rst-class:: compact-list

  * Programmatic platform description (only C++ for now).
  * New plugin to simplify producer/consumer applications.
  * MC: new tutorial and associated docker image.
  * SMPI: improve error handling for incorrect advanced usages.
  * Many internal cleanups and refactoring to prepare the future.

As usual, even the full changelog is only the tip of the iceberg, given the amount of changes in the backstage.

This release is the big one for the programmatic platform descriptions, that are now fully usable from C++. XML will not
disappear anytime soon, but it is unlikely that we continue developing it in the future. When starting a new project, you should
probably go for the programmatic platforms. Or you could wait for the next release, where we hope to introduce the Python bindings of the
programmatic platforms. A platform in Python and an application in C++ may provide a better separation of concern (when it will be possible).

On the Model-Checking front, the code base did not evolve a lot, but we now provide a brand new tutorial and docker image for those wanting
to start using this feature. We are still not done with the refactoring required to unlock the future of Mc SimGrid and still
consider that it's less robust than the rest of SimGrid. We're working on it, and you may even find it useful as is anyway.

On the SimDag++ front (integrating the SimDAG interface to S4U), some work occurred in the backstage, but we were too busy with the
programmatic platforms to make this happen in this release. Maybe next season?

On the SMPI front, the work was on improving the usability. SMPI is now better at hinting the problem in buggy and almost-correct
applications, and it can assist the user in abstracting parts of the application to improve the simulation performance. Check the SMPI
tutorial for details.

Finally, we pursued our quest for a better codebase by following the hints of SonarCloud and other static analyzers. This is what it takes
to fight the technical debt and ensure that you'll still enjoy SimGrid in a decade. Along the same line, we removed the symbols that were
deprecated since 3 releases, as usual.

Version 3.29 (October 7. 2021)
------------------------------

To celebrate the "Ask a stupid question" release, we wish that every user ask one question about SimGrid.
On `Mattermost <https://framateam.org/simgrid/channels/town-square>`_,
`Stack Overflow <https://stackoverflow.com/questions/tagged/simgrid>`_,
or using the `issues tracker <https://framagit.org/simgrid/simgrid/-/issues>`_.

.. rst-class:: compact-list

   * Python bindings for the platform creation API
   * Introduce non-linear resource sharing, allowing decay models
   * New documentation section on realistic I/O modeling
   * (+ many bug fixes and internal refactoring)

This release finishes the work on programmatic platforms, that was ongoing since 3.27. It is now possible to define a complete platform in either C++
or python, and the XML approach is now deprecated. It will probably remain around for a long time, but no evolution is planned. New features will not
be ported to the XML parser (unless you provide a patch, of course).

This release also paves the way for new models, with the introduction of two new features to the model solver:

.. rst-class:: compact-list

   * Non-linear resource sharing was introduced, allowing to model resource whose performance heavily degrades with contention. This may be used in the
     future for Wi-Fi links, where the total amount of data exchanged in a cell drops when the amount of stations reaches a threshold.
   * Dynamic factors model variability in the speed of activities. This can be used to model an overhead (e.g., there is a 20 bytes header in a 480
     bytes TCP packet so the factor 0.9583) but the novelty is this factor can now easily be adjusted depending on activity's and resources
     characteristics. |br|
     This existed for network (e.g., the effective bandwidth depends on the message in SMPI piecewise-linear network model) but it is now more general
     (the factor may depend on the source and destination and thus account to different behaviors for intra-node communications and extra-node
     communications) and is available for CPUs (e.g., if you want to model an affinity as in the "Unrelated Machines" problem in scheduling) and disks
     (e.g., if you want to model a stochastic capacity) too. |br|
     The same mechanism is also available for the latency, which allows to easily introduce complex variability patterns.

These new features are not used yet in the provided models, but this will probably change in future releases.

Version 3.30 (January 30. 2022)
-------------------------------

The Sunday Bloody Sunday release.

In may 2016, the future organization of the S4U activities was drafted on a Hawaiian whiteboard. We defined the life cycle of activities, their types,
and the way to combine them. All of this had been implemented since, but one piece was still missing: the capacity to express dependencies and vetoes
that can prevent an activity to start. The underlying idea was to be able to manage application DAGs, a la SimDag, through the S4U API, and have
maestro to handle the execution of such DAGs.

This release finishes this work, which is presented in a new set of examples (``examples/cpp/dag-*``). The direct consequences on the code base of this
new feature are:

 * The SimDag API for the simulation of the scheduling of Directed Acyclic Graphs has been finally dropped. It was marked as deprecated for a couple
   of years.
 * The removal of SimDag led us to also remove the export to Jedule files that was tightly coupled to SimDag. The instrumentation of DAG simulation
   is still possible through the regular instrumentation API based on the Paje format.
 
On the bindings front, we dropped the Lua bindings to create new platforms, as the C++ and Python interfaces are much better to that extend. 
Also, the algorithm tutorial can now be taken in Python, for those of you alergic to C++.

Finally, on the SMPI front, we introduced a new documentation section on calibrating the SMPI models from your measurements and fixed some issues
with the replay mechanism.

Version 3.31 (expected spring 2021)
-----------------------------------

On the model checking front, the long awaited big bang finally occurred, greatly simplifying future evolution. 

A formal verification with Mc SimGrid implies two processes: a verified application that is an almost regular SimGrid simulation, and a checker that
is an external process guiding the verified application to ensure that it explores every possible execution scenario. When formal verification was
initially introduced in SimGrid 15 years ago, both processes were intertwined in the same system process but the mandated system tricks made it
impossible to use classical debugging tools such as gdb or valgrind on that Frankenstein process. Having more than one heap in your process is not 
usually supported.

The design was simplified in v3.12 (2015) by splitting the application and the checker in separate system process. But both processes remained tightly
coupled: when the checker needed some information (for example the mailbox implied in a send operation to compute whether this operation `commutes
with another one <https://en.wikipedia.org/wiki/Partial_order_reduction>`_), the checker was directly reading the memory of the other system process.
This was efficient and nice in C, but it prevented us from using C++ features such as opaque ``std::function`` data types. As such, it hindered the
ongoing SimDAG++ code reorganization toward SimGrid4, where all activity classes should be homogeneously written in modern C++.

This release introduces a new design, where the simcalls are given object-oriented Observers that can serialize the relevant information over the wire. 
This information is used on the checker side to build Transition objects, representing the application simcalls. This explanation may not be crystal 
clear, but the checker code is now much easier to work with as the formal logic is not spoiled with system-level tricks to retrieve the needed information.

Future work on the model checker include: support for mutexes (that are still not handled), implementation of another exploration algorithm based on UDPOR
(`The Anh Pham's thesis <https://tel.archives-ouvertes.fr/tel-02462074/document>`_ was defended in 2019), and robustness improvement using the `MPI Bug 
Initiative <https://hal.archives-ouvertes.fr/hal-03474762>`_ tests. Many things that were long dreamed of now become technically possible in this code base.

.. |br| raw:: html

   <br />
