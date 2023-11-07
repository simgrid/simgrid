/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_EVENTSETCALCULATOR_HPP
#define SIMGRID_MC_UDPOR_EVENTSETCALCULATOR_HPP

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

  static EventSet partially_extend_MutexAsyncLock(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_MutexWait(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_MutexTest(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
  static EventSet partially_extend_MutexUnlock(const Configuration&, Unfolding*, std::shared_ptr<Transition>);

  static EventSet partially_extend_ActorJoin(const Configuration&, Unfolding*, std::shared_ptr<Transition>);

public:
  static EventSet partially_extend(const Configuration&, Unfolding*, std::shared_ptr<Transition>);
};

} // namespace simgrid::mc::udpor
#endif
