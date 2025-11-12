/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_EVENTSETCALCULATOR_HPP
#define SIMGRID_MC_UDPOR_EVENTSETCALCULATOR_HPP

#include "simgrid/forward.h"
#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/transition/TransitionActor.hpp"
#include "src/mc/transition/TransitionAny.hpp"
#include "src/mc/transition/TransitionComm.hpp"
#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "src/mc/transition/TransitionRandom.hpp"
#include "src/mc/transition/TransitionSynchro.hpp"

#include <memory>
#include <optional>

namespace simgrid::mc::udpor {

/**
 * @brief Computes incrementally the portion of the extension set for a new configuration `C`
 */
struct ExtensionSetCalculator final {
private:
  static EventSet partially_extend_CommSend(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_CommRecv(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_CommWait(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_CommTest(const Configuration&, Unfolding*, std::shared_ptr<Transition>);

  // Helper methods that are used for the mutex extension computation
  static std::pair<aid_t, aid_t> firstTwoOwners(uintptr_t mutex_id, EventSet history);
  static bool is_mutex_available_before(const UnfoldingEvent* e, std::shared_ptr<MutexTransition> mutex);

  static EventSet partially_extend_MutexAsyncLock(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_MutexWait(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_MutexTest(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_MutexUnlock(const Configuration&, Unfolding*, std::shared_ptr<Transition>);

  // Helper methods that are used for the mutex extension computation
  static aid_t first_waiting_before(const EventSet history, unsigned sem_id);
  static aid_t first_waiting_before(const UnfoldingEvent*, unsigned sem_id);
  static int available_token_after(const UnfoldingEvent*, unsigned sem_id);

  static EventSet partially_extend_SemAsyncLock(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_SemWait(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_SemUnlock(const Configuration&, Unfolding*, std::shared_ptr<Transition>);

  // Helper method that find the event that created the actor given in parameter if any
  static std::optional<const UnfoldingEvent*> find_ActorCreate_Event(const EventSet history, aid_t actor);

  static EventSet partially_extend_ActorJoin(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_ActorExit(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_ActorSleep(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_ActorCreate(const Configuration&, Unfolding*, std::shared_ptr<Transition>);

public:
  static EventSet partially_extend(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
};

} // namespace simgrid::mc::udpor
#endif
