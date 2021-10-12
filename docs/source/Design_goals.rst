SimGrid's Design Goals
######################

Any SimGrid simulation comes down to a set of **actors** using some
**resources** through **activities**. SimGrid provides several kinds of
resources (link, CPU, disk, and synchronization objects such as mutex
or semaphores) along with the corresponding activity kinds
(communication, execution, I/O, lock). SimGrid users provide a
platform instantiation (list of interconnected resources) and an
application (the code executed by actors) along with the actors'
placement on the platform.

The actors (ie, the user code) can only interact with the platform
through activities, that are somewhat similar to synchronizations.
Some are very natural (locking a mutex is a synchronization with the
other actors using the same mutex) while others activities constitute
more original synchronization: execution, communication, and I/O have a
quantitative component that depends on the resources. But still,
writing some data to disk is seen as a synchronization with the other
actors using the same disk. When you lock a mutex, you can proceed
only when that mutex gets unlocked by someone else. Similarly, when you
do an I/O, you can proceed once the disk delivered enough performance
to fulfill your demand (along with the concurrent demands of the other
actors occurring at the same time). Communication activities have both a
qualitative component (the actual communication starts only when both
the sender and receiver are ready to proceed) and a quantitative
component (consuming the communication power of the link resources).

The design of SimGrid is shaped by several design goals:

 - **reproducibility**: re-executing the same simulation must lead to
   the exact same outcome, even if it runs on another computer or
   operating system. When possible, this should also be true when you
   use another version of SimGrid.
 - **speed**: running a given simulation should be as fast as possible
 - **versatility**: ability to simulate many kinds of distributed systems
   and resource models. But the simulation should be parsimonious too,
   to not hinder the tool's usability. SimGrid tries to provide sane
   default settings along with the possibility to augment and modify
   the provided models and their default settings.
 - **scalability**: ability to deal with very large simulations. In the
   number of actors, in the size of the platform, in the number of
   events, or all together.

Actors and activities
*********************

The first crux of the SimGrid design lays in the interaction between
each actor and the activities. For the sake of reproducibility, the
actors cannot interact directly with their environment: every
modification request is serialized through a central component that
processes them in a reproducible order. For the sake of speed, SimGrid
is designed as an operating system: the actors issue **simcalls** to a
simulation kernel that is called **maestro** (because it decides which
actors can proceed and which ones must wait).

In practice, a SimGrid simulation is a suite of so-called **scheduling
rounds**, during which all actors that are not currently blocked on a
simcall get executed. For that, maestro passes the control flow to the
code of each actor, that are written in either C++, C, Fortran, Python,
or Java. The control flow then returns to the maestro when the actor
blocks on its next blocking simcall. Note that the time it takes to
execute the actor code has to be reported to the simulator using
execution activities. SMPI programs are automatically benchmarked
while these executions must be manually reported in S4U. The simulated
time is discrete in SimGrid and only progresses between scheduling
rounds, so all events occurring during a given scheduling round occur
at the exact same simulated timestamp, even if the actors are usually
executed sequentially on the real platform.

To modify their environment, the actors issue either **immediate
simcalls** that take no time in the simulation (e.g.: spawning another
actor), or **blocking simcalls** that must wait for future events (e.g.:
mutex locks require the mutex to be unlocked by its owner;
communications wait for the network to provide enough communication
performance to fulfill the demand). A given scheduling round is
usually composed of several sub-scheduling rounds during which
immediate simcalls are resolved. This ends when all actors are either
terminated or within a blocking simcall. The simulation models are
then used to compute the time at which the first next simcall
terminates. The time is advanced to that point, and a new scheduling
round begins with all actors that got unblocked at that timestamp.

Context switching between the actors and maestro is highly optimized
for the sake of simulation performance. SimGrid provides several
implementations of this mechanism, called **context factories**. These
implementations fall into two categories: Preemptive contexts are
based on full-fledged system threads such as pthread on Linux or Java
threads in the JVM. They are usually better supported by external
debuggers and profiling tools, but less efficient. The most efficient
factories use non-preemptive mechanisms, such as SysV's ucontexts,
boost's context, or our own hand-tuned implementation, that is written
in assembly language. This is possible because a given actor is never
interrupted between consecutive simcalls in SimGrid.

For the sake of performance, actors can be executed in parallel using
several system threads for non-preemptive contexts. But in our
experience, this rarely leads to any performance improvement because
most applications simulated on top of SimGrid are fine-grained: when
the simulation performance really matters, the users tend to abstract
away any large computations to efficiently simulate the control flow
of their application. In addition, parallel simulation puts unpleasant
restrictions on the user code, that must be correctly isolated. To be
honest, most of the existing SMPI implementation cannot be used in
parallel yet.

Parsimonious model versatility
******************************

Another orthogonal crux of the SimGrid design is the parsimonious
versatility in modeling. For that, we tend to unify all resource and
activity kinds. As you have seen, we parallel the classical notion of
**computational power** with the more original **communication power** and
**I/O power**. Asynchronous executions are less common than the
asynchronous communications that proliferate in MPI but they are still
provided for sake of symmetry: they even prove useful to efficiently
simulate thread pools. Note that asynchronous mutex locking is still to be
added to SimGrid atm. The notion of **pstate** was introduced to model
the stepwise variation of computational speed depending on the DVFS,
and was reused to model the bootup and shutdown phases of a CPU: the
computational speed is 0 at these specific pstates. This pstate notion
was extended to represent the fact that the bandwidth provided by a
wifi link to a given station depends on its signal-noise ratio (SNR).

Further on this line, all provided resource models are very comparable
internally. They rely on linear inequation systems, stating for
example that the sum of the computational power received by all
computation activities located on a given CPU cannot overpass the
computational power provided by this resource. This extends nicely to
multi-resources activities such as communications using several links,
and also to the so-called parallel tasks (abstract activities
representing a parallel execution kernel consuming both the
communication and computational power of a set of machines). Specific
coefficients are added to the linear system to reflect how the real
resources are shared between concurrent usages. The resulting system
is then solved using a max-min objective function that maximizes the
minimum of all shares allocated to activities. Our experience shows
that this approach can successfully be used for fast yet accurate
simulations of complex phenomena, provided that the model's
coefficients and constants are carefully tailored and instantiated to
that phenomenon.

Model-checking
**************

Even if it was not in the original goals of SimGrid, the framework now
integrates a full-featured model-checker (dubbed MC or Mc SimGrid)
that can exhaustively explore all execution paths that the application
could experience. Conceptually, Mc SimGrid is built upon the ideas
presented previously. Instead of using the resource models to compute
the order simcall terminations, it explores every order that is
causally possible. In a simulation entailing only three concurrent
events (i.e., simcalls) A, B, and C, it will first explore the
scenario where the activities order is ABC, then the ACB order, then
BAC, then BCA, then CAB and finally CBA. Of course, the number of
scenarios to explore grows exponentially with the number of simcalls
in the simulation. Mc SimGrid leverages reduction techniques to avoid
re-exploring equivalent traces.

In practice, Mc SimGrid can be used to verify classical `safety and
liveness properties
<https://en.wikipedia.org/wiki/Linear_time_property>`_, but also
`communication determinism
<https://hal.inria.fr/hal-01953167/document>`_, a property that allows
more efficient solutions toward fault-tolerance. It can alleviate the
state space explosion problem through `Dynamic Partial Ordering
Reduction (DPOR)
<https://en.wikipedia.org/wiki/Partial_order_reduction>`_ and `state
equality <https://hal.inria.fr/hal-01900120/document>`_.

Mc SimGrid is far more experimental than other parts of the framework,
such as SMPI that can now be used to run many full-featured MPI codes
out of the box.
