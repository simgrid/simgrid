/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_COMB_HPP
#define SIMGRID_MC_UDPOR_COMB_HPP

#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"
#include "src/xbt/utils/iter/variable_for_loop.hpp"

#include <boost/iterator/function_input_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <functional>
#include <vector>

namespace simgrid::mc::udpor {

/**
 * @brief  An individual element of a `Comb`, i.e. a
 * sequence of `const UnfoldingEvent*`s
 */
using Spike = std::vector<const UnfoldingEvent*>;

/**
 * @brief A two-dimensional data structure that is used
 * to compute partial alternatives in UDPOR
 *
 * The comb data structure is described in the paper
 * "Quasi-Optimal DPOR" by Nguyen et al. Formally,
 * if `A` is a set:
 *
 * """
 * An **A-Comb C of size `n` (`n` a natural number)**
 * is an *ordered* collection of spikes <s_1, s_2, ..., s_n>,
 * where each spike is a sequence of elements over A.
 *
 * Furthermore, a **combination over C** is any tuple
 * <a_1, a_2, ..., a_n> where a_i is a member of s_i
 * """
 *
 * The name probably comes from what the structure looks
 * like if you draw it out. For example, if `A = {1, 2, 3, ..., 10}`,
 * then a possible `A-comb` of size 8 might look like
 *
 * 1 2 3 4 5 6
 * 2 4 5 9 8
 * 10 9 2 1 1
 * 8 9 10 5
 * 1
 * 3 4 5
 * 1 4 4 5 1 6
 * 8 8 9
 *
 * which, if you squint a bit, looks like a comb (albeit with some
 * broken bristles [or spikes]). A combination is any selection of
 * one element within each spike; e.g. (where _x_ denotes element `x`
 * is selected)
 *
 * 1 2 _3_ 4 5 6
 * 2 _4_ 5 9 8
 * 10 9 2 _1_ 1
 * 8 _9_ 10 5
 * _1_
 * 3 4 _5_
 * 1 _4_ 4 5 1 6
 * _8_ 8 9
 *
 * A `Comb` as described by this C++ class is a `U-comb`, where
 * `U` is the set of events that UDPOR has explored. That is,
 * the comb is over a set of events
 */
class Comb : public std::vector<Spike> {
public:
  explicit Comb(unsigned k) : std::vector<Spike>(k) {}
  Comb(const Comb& other)            = default;
  Comb(Comb&& other)                 = default;
  Comb& operator=(const Comb& other) = default;
  Comb& operator=(Comb&& other)      = default;

  auto combinations_begin() const
  {
    std::vector<std::reference_wrapper<const Spike>> references;
    std::transform(begin(), end(), std::back_inserter(references), [](const Spike& spike) { return std::cref(spike); });
    return simgrid::xbt::variable_for_loop<const Spike>(std::move(references));
  }
  auto combinations_end() const { return simgrid::xbt::variable_for_loop<const Spike>(); }
};

} // namespace simgrid::mc::udpor
#endif
