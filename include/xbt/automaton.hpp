/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_AUTOMATON_HPP
#define _XBT_AUTOMATON_HPP

#include <utility>

#include <xbt/automaton.h>

namespace simgrid {
namespace xbt {

/** Add a proposition to an automaton (the C++ way)
 *
 *  This API hides all the callback and dynamic allocation hell from
 *  the used which can use C++ style functors and lambda expressions.
 */
template<class F>
xbt_automaton_propositional_symbol_t add_proposition(
  xbt_automaton_t a, const char* id, F f)
{
  F* callback = new F(std::move(f));
  return xbt_automaton_propositional_symbol_new_callback(
    a, id,
    [](void* callback) -> int { return (*(F*)callback)(); },
    callback,
    [](void* callback) -> void { delete (F*)callback; }
  );
}

}
}
#endif
