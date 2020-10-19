.. _community:

The SimGrid Community
=====================

SimGrid is a free software, written by a community of people. It
started as a little software to help ourselves in our own research,
and as more people put their input into the pot, it turned into
something that we hope to be valuable to many people. So yes. We hope
that SimGrid is helping you doing what you want, and that you will
join our community of happy simgriders.

Contacting the community
------------------------

There are several locations where you can connect and discuss about
SimGrid. If you have a question, please have a look at the
documentation and examples first, but if some remain don't hesitate to
ask the community for help. If you do not have a question, just come
to us and say hello! We love earing about how people use SimGrid.

 - For questions or remarks, drop us an email on the `user mailing
   list <mailto:simgrid-user@lists.gforge.inria.fr>`_ (to subscribe,
   visit the `web interface
   <http://lists.gforge.inria.fr/mailman/listinfo/simgrid-user>`__);
   you can also check out `our archives
   <http://lists.gforge.inria.fr/pipermail/simgrid-user/>`_.  We
   prefer you to **not use private emails**. SimGrid is an open
   framework, and you never know who have the time and knowledge to
   answer your question, so please keep messages on the public mailing
   list.
 - Join us on IRC and ask your question directly on the channel \#simgrid at
   ``irc.debian.org``
   (or use the ugly `web interface <https://webchat.oftc.net/?channels=%23simgrid>`__
   if you don't have a
   `real client <https://en.wikipedia.org/wiki/Comparison_of_Internet_Relay_Chat_clients>`_
   installed). When no non-french speaker are connected, we usually
   chat in french on this channel, but we do switch back to english
   when we have a guest.
   
   Be warned that even if many people are connected to
   the channel, they may not be staring at their IRC windows.
   So don't be surprised if you don't get an answer in the 
   second, and turn to the mailing lists if nobody seems to be there.
   The logs of this channel are publicly
   `available online <http://colabti.org/irclogger/irclogger_logs/simgrid>`_,
   so may also want to check in a few hours if someone answered after
   you left. 
   
 - Asking your question on
   `StackOverflow <http://stackoverflow.com/questions/tagged/simgrid>`_
   is also a good idea, as this
   site is very well indexed. We answer questions there too (don't
   forget to use the SimGrid tag in your question so that we can see
   it), and they remain usable for the next users. 

Giving back to SimGrid
----------------------

We are sometimes asked by users how to give back to the project. Here
are some ideas, but if you have new ones, feel free to share them with us.

Spread the word
^^^^^^^^^^^^^^^

There are many ways to help the SimGrid project. The first and most
natural one is to **use SimGrid for your research, and say so**. Cite
the SimGrid framework in your papers and discuss of its advantages with
your colleagues to spread the word. When we ask for new funding to
sustain the project, the number of publications enabled by SimGrid is
always the first question we get. The more you use the framework,
the better for us. 

Make sure that your scientific publications using SimGrid actually
cite the `right paper <https://simgrid.org/publications.html>`_.
Also make sure that these citations are correctly listed on 
`our list <https://simgrid.org/usages.html>`_.

You can also **help us constituting an active and welcoming user
community**. Subscribe to the mailing lists, and answer the
questions that newscomers have if you can. Point them (gently ;) to
the relevant part of the documentation on need, and help them becoming
part of our community too. 

Another easy way to help the project is to add a link to the `SimGrid
homepage <https://simgrid.org>`_ on your site to improve SimGrid's ranking in
search engines.

Finally, if you organize a scientific event where you expect many
potential users, you can invite us to give a tutorial on SimGrid. We
found that 45 minutes to one hour is very sharp, but
`doable <http://people.irisa.fr/Martin.Quinson/blog/2012/1120/Simgrid_at_Louvain/>`_.
It is enough to explain the main motivations and outcomes of the
project in order to motivate the attendees get more information on
SimGrid, and eventually improve their scientific habits by using a
sound simulation framework. 

Report (and fix) issues
^^^^^^^^^^^^^^^^^^^^^^^

Because of its size and complexity, SimGrid far from perfect and
contains a large amount of glitches and issues. When you find one,
don't assume that it's here because we don't care. It survived only
because nobody told us. We unfortunately cannot endlessly review our
large code and documentation base. So please, **report any issue you
find**, be it a typo in the documentation, a paragraph that needs to
be reworded, a bug in the code, or any other problem. The best way to
do so is to open an issue on our
`Bug Tracker <https://github.com/simgrid/simgrid/issues>`_ so
that we don't forget about it. 

The worst way to report such issues is to go through private emails.
These are unreliable, and we are trying to develop SimGrid openly, so
private discussions are to be avoided if possible. 

If you can provide a patch fixing the issue you report, that's even
better. If you cannot, then you need to give us a minimal working
example (MWE), that is a ready to use solution that reproduces the
problem you face. Your bug will take much more time
for us to reproduce and fix if you don't give us the MWE, so you want
to help us helping you to get things efficient.

Of course, a very good way to give back to the SimGrid community is to
triage and fix the bugs in the Bug Tracking Systems. If the bug report
has no MWE, we'd love you to contribute one. If you can come up with a
patch, we will be more than happy to apply your changes so that the
whole community enjoys them.

Extending SimGrid and its Ecosystem
-----------------------------------

Contributing Code
^^^^^^^^^^^^^^^^^

If you deeply miss a feature in the framework, you should consider
implementing it yourself. SimGrid is free software, meaning that you are
free to help yourself. Of course, we'll do our best to assist you in
this task, so don't hesitate to contact us with your idea.

You could write a new plugin extending SimGrid in some way, or a
routing model for another kind of network. But even if you write your own
platform file, this is probably interesting to other users too, and
could be included to SimGrid. Modeling accurately a given platform is
a difficult work, which outcome is very precious to us.

Or maybe you developed an independent tool on top of SimGrid. We'd
love helping you gaining visibility by listing it in our 
`Contrib <https://simgrid.org/contrib.html>`_. 

Possible Enhancements
^^^^^^^^^^^^^^^^^^^^^

If you want to start working on the SimGrid codebase, here are a few
ideas of things that could be done to improve the current code (not all of them
are difficult, do trust yourself ;)

Time and duration
"""""""""""""""""

We should avoir using "-1" to mean "forever" at least in S4U and in
the internal code.  We should probably always use separate functions
(`wait` vs `wait_for`).

Futures and Promises
""""""""""""""""""""

 - Some features are missing in the Maestro future implementation
   (`simgrid::kernel::Future`, `simgrid::kernel::Promise`)
   could be extended to support additional features:
   `when_any`, `shared_future`, etc.

 - The corresponding feature might then be implemented in the user process
   futures (`simgrid::simix::Future`).

 - Currently `.then()` is not available for user futures. We would need to add
   a basic user event loop in order to queue the pending continuations.

 - We might need to provide an option to cancel a pending operation. This
   might be achieved by defining some `Action` or `Operation` class with an
   API compatible with `Future` (and convertible to it) but with an
   additional `.cancel()` method.

MC: Overhaul the state comparison code
""""""""""""""""""""""""""""""""""""""

The state comparison code is quite complicated. It has very long functions and
is programmed mostly using C idioms and is difficult to understand and debug.
It is in need of an overhaul:

  - cleanup, refactoring, usage of C++ features.

  - The state comparison code works by inferring types of blocks allocated on the
    heap by following pointers from known roots (global variables, local
    variables). Usually the first type found for a given block is used even if
    a better one could be found later. By using a first pass of type inference,
    on each snapshot before comparing the states, we might use a better type
    information on the different blocks.

  - We might benefit from adding logic for handling some known types. For
    example, both `std::string` and `std::vector` have a capacity which might
    be larger than the current size of the container. We should ignore
    the corresponding elements when comparing the states and inferring the types.

  - Another difficulty in the state comparison code is the detection of
    dangling pointers. We cannot easily know if a pointer is dangling and
    dangling pointers might lead us to choose the wrong type when inferring
    heap blocks. We might mitigate this problem by delaying the reallocation of
    a freed block until there is no blocks pointing to it anymore using some
    sort of basic garbage-collector.

MC: Hashing the states
""""""""""""""""""""""

In order to speed up the state comparison an idea was to create a hash of the
state. Only states with the same hash would need to be compared using the
state comparison algorithm. Some information should not be included in the
hash in order to avoid considering different states which would otherwise
would have been considered equal.

The states could be indexed by their hash. Currently they are indexed
by the number of processes and the amount of heap currently allocated
(see `DerefAndCompareByNbProcessesAndUsedHeap`).

Good candidate information for the state hashing:

 - number of processes;

 - their backtraces (instruction addresses);

 - their current simcall numbers;

 - some simcall arguments (eg. number of elements in a waitany);

 - number of pending communications;

 - etc.

Some basic infrastructure for this is already in the code (see `mc_hash.cpp`)
but it is currently disabled.

Interface with the model-checked processes
""""""""""""""""""""""""""""""""""""""""""

The model checker reads many information about the model-checked process by
`process_vm_readv()`-ing brutally the data structure of the model-checked
process leading to some inefficient code such as maintaining copies of complex
C++ structures in XBT dynars. We need a sane way to expose the relevant
information to the model checker.

Generic simcalls
""""""""""""""""

We have introduced some generic simcalls which can be used to execute a
callback in a SimGrid Maestro context. It makes it a lot easier to interface
the simulated process with the maestro. However, the callbacks for the
model checker which cannot decide how it should handle them. We would need a
solution for this if we want to be able to replace the simcalls the
model checker cares about by generic simcalls.

Defining an API for writing Model-Checking algorithms
"""""""""""""""""""""""""""""""""""""""""""""""""""""

Currently, writing a new model-checking algorithms in SimGridMC is quite
difficult: the logic of the model-checking algorithm is mixed with a lot of
low-level concerns about the way the model checker is implemented. This makes it
difficult to write new algorithms and difficult to understand, debug, and modify
the existing ones. We need a clean API to express the model-checking algorithms
in a form which is closer to the text-book/paper description. This API must
be exposed in a language which is more adequate to this task.

Tasks:

  1. Design and implement a clean API to express model-checking algorithms.
     A `Session` class currently exists for this but is not feature complete
     and should probably be rewritten. It should be easy to create bindings
     for different languages on top of this API.

  2. Create a binding to some better suited, dynamic, scripting language
     (e.g., Lua).

  3. Rewrite the existing model-checking algorithms in this language using the
     new API.
