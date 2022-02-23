/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"
#include <simgrid/config.h>

#if SIMGRID_HAVE_MC
#include "src/mc/ModelChecker.hpp"
#include "src/mc/transition/TransitionAny.hpp"
#include "src/mc/transition/TransitionComm.hpp"
#include "src/mc/transition/TransitionRandom.hpp"
#include "src/mc/transition/TransitionSynchro.hpp"
#endif

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_transition, mc, "Logging specific to MC transitions");

namespace simgrid {
namespace mc {
unsigned long Transition::executed_transitions_ = 0;
unsigned long Transition::replayed_transitions_ = 0;

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
void Transition::replay() const
{
  replayed_transitions_++;

#if SIMGRID_HAVE_MC
  mc_model_checker->handle_simcall(aid_, times_considered_, false);
  mc_model_checker->wait_for_requests();
#endif
}

Transition* deserialize_transition(aid_t issuer, int times_considered, std::stringstream& stream)
{
#if SIMGRID_HAVE_MC
  short type;
  xbt_assert(stream >> type);
  xbt_assert(type >= 0 && type <= static_cast<short>(Transition::Type::UNKNOWN), "Invalid transition type %d received",
             type);

  auto simcall = static_cast<Transition::Type>(type);

  switch (simcall) {
    case Transition::Type::COMM_RECV:
      return new CommRecvTransition(issuer, times_considered, stream);
    case Transition::Type::COMM_SEND:
      return new CommSendTransition(issuer, times_considered, stream);
    case Transition::Type::COMM_TEST:
      return new CommTestTransition(issuer, times_considered, stream);
    case Transition::Type::COMM_WAIT:
      return new CommWaitTransition(issuer, times_considered, stream);

    case Transition::Type::TESTANY:
      return new TestAnyTransition(issuer, times_considered, stream);
    case Transition::Type::WAITANY:
      return new WaitAnyTransition(issuer, times_considered, stream);

    case Transition::Type::RANDOM:
      return new RandomTransition(issuer, times_considered, stream);

    case Transition::Type::MUTEX_TRYLOCK:
    case Transition::Type::MUTEX_LOCK:
    case Transition::Type::MUTEX_TEST:
    case Transition::Type::MUTEX_WAIT:
    case Transition::Type::MUTEX_UNLOCK:
      return new MutexTransition(issuer, times_considered, simcall, stream);

    case Transition::Type::UNKNOWN:
      return new Transition(Transition::Type::UNKNOWN, issuer, times_considered);
  }
  THROW_IMPOSSIBLE; // Some compilers don't detect that each branch of the above switch has a return
#else
  xbt_die("Deserializing transitions is only interesting in MC mode.");
#endif
}

} // namespace mc
} // namespace simgrid
