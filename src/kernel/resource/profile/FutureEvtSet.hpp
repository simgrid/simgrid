/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <queue>
#ifndef FUTUREEVTSET_HPP
#define FUTUREEVTSET_HPP

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
  typedef std::pair<double, Event*> Qelt;
  std::priority_queue<Qelt, std::vector<Qelt>, std::greater<Qelt>> heap_;
};

} // namespace profile
} // namespace kernel
} // namespace simgrid

#endif