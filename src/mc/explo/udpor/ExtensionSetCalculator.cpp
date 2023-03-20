/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/ExtensionSetCalculator.hpp"

#include <functional>
#include <unordered_map>
#include <xbt/asserts.h>
#include <xbt/ex.h>

using namespace simgrid::mc;

namespace simgrid::mc::udpor {

EventSet ExtensionSetCalculator::partially_extend(const Configuration& C, const Unfolding& U,
                                                  const std::shared_ptr<Transition> action)
{
  switch (action->type_) {
    case Transition::Type::COMM_WAIT: {
      return partially_extend_CommWait(C, U, std::static_pointer_cast<CommWaitTransition>(std::move(action)));
    }
    case Transition::Type::COMM_ASYNC_SEND: {
      return partially_extend_CommSend(C, U, std::static_pointer_cast<CommSendTransition>(std::move(action)));
    }
    case Transition::Type::COMM_ASYNC_RECV: {
      return partially_extend_CommRecv(C, U, std::static_pointer_cast<CommRecvTransition>(std::move(action)));
    }
    default: {
      xbt_assert(false,
                 "There is currently no specialized computation for the transition "
                 "'%s' for computing extension sets in UDPOR, so the model checker cannot "
                 "determine how to proceed. Please submit a bug report requesting "
                 "that the transition be supported in SimGrid using UDPOR and consider "
                 "using the other model-checking algorithms supported by SimGrid instead "
                 "in the meantime",
                 action->to_string().c_str());
      DIE_IMPOSSIBLE;
    }
  }
}

EventSet ExtensionSetCalculator::partially_extend_CommSend(const Configuration& C, const Unfolding& U,
                                                           const std::shared_ptr<CommSendTransition> action)
{
  return EventSet();
}

EventSet ExtensionSetCalculator::partially_extend_CommRecv(const Configuration& C, const Unfolding& U,
                                                           const std::shared_ptr<CommRecvTransition> action)
{
  return EventSet();
}

EventSet ExtensionSetCalculator::partially_extend_CommWait(const Configuration&, const Unfolding& U,
                                                           std::shared_ptr<CommWaitTransition>)
{
  return EventSet();
}

EventSet ExtensionSetCalculator::partially_extend_CommTest(const Configuration&, const Unfolding& U,
                                                           std::shared_ptr<CommTestTransition>)
{
  return EventSet();
}

} // namespace simgrid::mc::udpor