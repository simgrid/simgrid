/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FUTURE_HPP
#define XBT_FUTURE_HPP

#include <future>
#include <utility>
#include <exception>

namespace simgrid {
namespace xbt {

/** Fulfill a promise by executing a given code */
template<class R, class F>
void fulfillPromise(std::promise<R>& promise, F code)
{
  try {
    promise.set_value(code());
  }
  catch(...) {
    promise.set_exception(std::current_exception());
  }
}

/** Fulfill a promise by executing a given code
 *
 *  This is a special version for `std::promise<void>` because the default
 *  version does not compile in this case.
 */
template<class F>
void fulfillPromise(std::promise<void>& promise, F code)
{
  try {
    (code)();
    promise.set_value();
  }
  catch(...) {
    promise.set_exception(std::current_exception());
  }
}

}
}

#endif
