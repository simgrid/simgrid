/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef FUTUREEVTSET_HPP
#define FUTUREEVTSET_HPP

#include "simgrid/forward.h"
#include <queue>

namespace simgrid {
namespace kernel {
namespace profile {

/** @brief Future Event Set (collection of iterators over the traces)
 * That's useful to quickly know which is the next occurring event in a set of traces. */
class XBT_PUBLIC FutureEvtSet {
public:
  FutureEvtSet();
  FutureEvtSet(const FutureEvtSet&) = delete;
  FutureEvtSet& operator=(const FutureEvtSet&) = delete;
  virtual ~FutureEvtSet();
  double next_date() const;
  Event* pop_leq(double date, double* value, resource::Resource** resource);
  void add_event(double date, Event* evt);

private:
  using Qelt = std::pair<double, Event*>;
  std::priority_queue<Qelt, std::vector<Qelt>, std::greater<>> heap_;
};

// FIXME: kill that singleton
extern XBT_PRIVATE simgrid::kernel::profile::FutureEvtSet future_evt_set;

} // namespace profile
} // namespace kernel
} // namespace simgrid

#endif
