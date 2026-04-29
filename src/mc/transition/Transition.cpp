/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/Transition.hpp"
#include "src/mc/remote/Channel.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"
#include <atomic>
#include <simgrid/config.h>

#if SIMGRID_HAVE_MC
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/transition/TransitionActor.hpp"
#include "src/mc/transition/TransitionAny.hpp"
#include "src/mc/transition/TransitionComm.hpp"
#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "src/mc/transition/TransitionRandom.hpp"
#include "src/mc/transition/TransitionSynchro.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_transition, mc, "Logging specific to MC transitions");

namespace simgrid::mc {
std::atomic_ulong Transition::executed_transitions_ = 0;
std::atomic_ulong Transition::replayed_transitions_ = 0;

// Do not move this to the header, to ensure that we have a vtable for Transition
Transition::~Transition() = default;

std::string Transition::to_string(bool) const
{
  return "";
}
std::string Transition::dot_string() const
{
  static constexpr std::array<const char*, 13> colors{{"blue", "red", "green3", "goldenrod", "brown", "purple",
                                                       "magenta", "turquoise4", "gray25", "forestgreen", "hotpink",
                                                       "lightblue", "tan"}};
  const char* color = colors[(aid_ - 1) % colors.size()];

  return xbt::string_printf("label = \"[(%ld)] %s\", color = %s, fontcolor = %s", aid_, Transition::to_c_str(type_),
                            color, color);
}
void Transition::replay(RemoteApp& app) const
{
  replayed_transitions_++;
#if SIMGRID_HAVE_MC
  app.handle_simcall(aid_, times_considered_, false);
  app.wait_for_requests();
#endif
}

void Transition::deserialize_memory_tracker(mc::Channel& channel)
{
  memory_tracker_.deserialize(channel);

  XBT_DEBUG("Created a memory access tracker for the new transition");
}

Transition* deserialize_transition(aid_t issuer, int times_considered, mc::Channel& channel)
{
#if SIMGRID_HAVE_MC
  mc::Transition::Type simcall = channel.unpack<mc::Transition::Type>();

  XBT_DEBUG("Deserializing a %s transition", mc::Transition::to_c_str(simcall));
  switch (simcall) {
    case Transition::Type::BARRIER_ASYNC_LOCK:
    case Transition::Type::BARRIER_WAIT:
      return new BarrierTransition(issuer, times_considered, simcall, channel);

    case Transition::Type::COMM_ASYNC_RECV:
      return new CommRecvTransition(issuer, times_considered, channel);
    case Transition::Type::COMM_ASYNC_SEND:
      return new CommSendTransition(issuer, times_considered, channel);
    case Transition::Type::COMM_IPROBE:
      return new CommIprobeTransition(issuer, times_considered, channel);
    case Transition::Type::COMM_TEST:
      return new CommTestTransition(issuer, times_considered, channel);
    case Transition::Type::COMM_WAIT:
      return new CommWaitTransition(issuer, times_considered, channel);

    case Transition::Type::TESTANY:
      return new TestAnyTransition(issuer, times_considered, channel);
    case Transition::Type::WAITANY:
      return new WaitAnyTransition(issuer, times_considered, channel);

    case Transition::Type::RANDOM:
      return new RandomTransition(issuer, times_considered, channel);

    case Transition::Type::MUTEX_TRYLOCK:
    case Transition::Type::MUTEX_ASYNC_LOCK:
    case Transition::Type::MUTEX_TEST:
    case Transition::Type::MUTEX_WAIT:
    case Transition::Type::MUTEX_UNLOCK:
      return new MutexTransition(issuer, times_considered, simcall, channel);

    case Transition::Type::SEM_ASYNC_LOCK:
    case Transition::Type::SEM_UNLOCK:
    case Transition::Type::SEM_WAIT:
      return new SemaphoreTransition(issuer, times_considered, simcall, channel);

    case Transition::Type::CONDVAR_ASYNC_LOCK:
    case Transition::Type::CONDVAR_BROADCAST:
    case Transition::Type::CONDVAR_SIGNAL:
    case Transition::Type::CONDVAR_WAIT:
      return new CondvarTransition(issuer, times_considered, simcall, channel);

    case Transition::Type::ACTOR_CREATE:
      return new ActorCreateTransition(issuer, times_considered, channel);
    case Transition::Type::ACTOR_EXIT:
      return new ActorExitTransition(issuer, times_considered, channel);
    case Transition::Type::ACTOR_JOIN:
      return new ActorJoinTransition(issuer, times_considered, channel);
    case Transition::Type::ACTOR_SLEEP:
      return new ActorSleepTransition(issuer, times_considered, channel);

    case Transition::Type::OBJECT_ACCESS:
      return new ObjectAccessTransition(issuer, times_considered, channel);

    case Transition::Type::UNKNOWN:
      return new Transition(Transition::Type::UNKNOWN, issuer, times_considered);

    default:
      break;
  }
  xbt_die("Invalid transition type %d received. Did you implement a new observer in the app without implementing the "
          "corresponding transition in the checker?",
          (int)simcall);
#else
  xbt_die("Deserializing transitions is only interesting in MC mode.");
#endif
}

/* This function exists so that the compiler can inline all calls to the depends() function thanks of the static_cast<>.
 * There is no need for a dynamic dispatch with such a static_cast, so the function can be called directly. Since the
 * depends() functions are in the header files, the compiler goes further and inlines them on need. */
bool Transition::dispatch_depends(const Transition* other) const
{
#if SIMGRID_HAVE_MC
  // Actions executed by the same actor are always dependent
  if (other->aid_ == aid_)
    return true;

  // Enforce our comparison order
  if (other->type_ < type_)
    return other->dispatch_depends(this);

  switch (type_) {
    case Transition::Type::BARRIER_ASYNC_LOCK:
    case Transition::Type::BARRIER_WAIT:
      return static_cast<const BarrierTransition*>(this)->depends(other);

    case Transition::Type::COMM_ASYNC_RECV:
      return static_cast<const CommRecvTransition*>(this)->depends(other);
    case Transition::Type::COMM_ASYNC_SEND:
      return static_cast<const CommSendTransition*>(this)->depends(other);
    case Transition::Type::COMM_IPROBE:
      return static_cast<const CommIprobeTransition*>(this)->depends(other);
    case Transition::Type::COMM_TEST:
      return static_cast<const CommTestTransition*>(this)->depends(other);
    case Transition::Type::COMM_WAIT:
      return static_cast<const CommWaitTransition*>(this)->depends(other);

    case Transition::Type::TESTANY:
      return static_cast<const TestAnyTransition*>(this)->depends(other);
    case Transition::Type::WAITANY:
      return static_cast<const WaitAnyTransition*>(this)->depends(other);

    case Transition::Type::RANDOM:
      return static_cast<const RandomTransition*>(this)->depends(other);

    case Transition::Type::MUTEX_TRYLOCK:
    case Transition::Type::MUTEX_ASYNC_LOCK:
    case Transition::Type::MUTEX_TEST:
    case Transition::Type::MUTEX_WAIT:
    case Transition::Type::MUTEX_UNLOCK:
      return static_cast<const MutexTransition*>(this)->depends(other);

    case Transition::Type::SEM_ASYNC_LOCK:
    case Transition::Type::SEM_UNLOCK:
    case Transition::Type::SEM_WAIT:
      return static_cast<const SemaphoreTransition*>(this)->depends(other);

    case Transition::Type::CONDVAR_ASYNC_LOCK:
    case Transition::Type::CONDVAR_BROADCAST:
    case Transition::Type::CONDVAR_SIGNAL:
    case Transition::Type::CONDVAR_WAIT:
      return static_cast<const CondvarTransition*>(this)->depends(other);

    case Transition::Type::ACTOR_CREATE:
      return static_cast<const ActorCreateTransition*>(this)->depends(other);
    case Transition::Type::ACTOR_EXIT:
      return static_cast<const ActorExitTransition*>(this)->depends(other);
    case Transition::Type::ACTOR_JOIN:
      return static_cast<const ActorJoinTransition*>(this)->depends(other);
    case Transition::Type::ACTOR_SLEEP:
      return static_cast<const ActorSleepTransition*>(this)->depends(other);

    case Transition::Type::OBJECT_ACCESS:
      return static_cast<const ObjectAccessTransition*>(this)->depends(other);

    case Transition::Type::UNKNOWN:
      return depends(other); // used for the tests

    case Transition::Type::MUTEX_LOCK_NOMC:
    case Transition::Type::SEM_LOCK_NOMC:
    case Transition::Type::CONDVAR_NOMC:
      xbt_die("We are not supposed to compute the dependency of the NOMC transitions (type is %d)", (int)type_);
    default:
      break;
  }
  xbt_die("Invalid transition type %d while computing the depends. Did you implement a new transition kind without "
          "properly extending the checker?",
          (int)type_);
#else
  xbt_die("Deserializing transitions is only interesting in MC mode.");
#endif
}

// boost::intrusive_ptr<Transition> support:
void intrusive_ptr_add_ref(Transition* transition)
{
  transition->refcount_.fetch_add(1, std::memory_order_acq_rel);
}

void intrusive_ptr_release(Transition* transition)
{
  if (transition->refcount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
    delete transition;
}

} // namespace simgrid::mc
