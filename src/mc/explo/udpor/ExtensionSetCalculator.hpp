/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_EVENT_SET_HPP
#define SIMGRID_MC_UDPOR_EVENT_SET_HPP

#include "src/mc/explo/udpor/udpor_forward.hpp"
#include "src/mc/transition/Transition.hpp"
#include "src/mc/transition/TransitionActorJoin.hpp"
#include "src/mc/transition/TransitionAny.hpp"
#include "src/mc/transition/TransitionComm.hpp"
#include "src/mc/transition/TransitionObjectAccess.hpp"
#include "src/mc/transition/TransitionRandom.hpp"
#include "src/mc/transition/TransitionSynchro.hpp"

namespace simgrid::mc::udpor {

/**
 * @brief Computes incrementally the portion of the extension set for a new configuration `C`
 */
struct ExtensionSetCalculator final {
private:
  static EventSet partially_extend_CommSend(const Configuration&, std::shared_ptr<CommSendTransition>);
  static EventSet partially_extend_CommReceive(const Configuration&, std::shared_ptr<CommReceiveTransition>);
  static EventSet partially_extend_CommWait(const Configuration&, std::shared_ptr<CommWaitTransition>);
  static EventSet partially_extend_CommTest(const Configuration&, std::shared_ptr<CommTestTransition>);

public:
  static EventSet partially_extend(const Configuration&, const Unfolding&, const std::shared_ptr<Transition> action);
};

} // namespace simgrid::mc::udpor
#endif
