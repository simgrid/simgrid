/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_BMF_HPP
#define SURF_BMF_HPP

#include "src/kernel/lmm/maxmin.hpp"
#include <eigen3/Eigen/Dense>

namespace simgrid {
namespace kernel {
namespace lmm {

class XBT_PUBLIC BmfSystem : public System {
public:
  using System::System;
  void solve() final { bottleneck_solve(); }

private:
  void bottleneck_solve();
  void set_matrix_A();
  void set_vector_C();
  std::unordered_map<int, std::vector<int>> get_alloc(const Eigen::VectorXd& fair_sharing, bool initial) const;
  double get_resource_capacity(int resource, const std::vector<int>& bounded_players) const;
  Eigen::VectorXd equilibrium(const std::unordered_map<int, std::vector<int>>& alloc) const;

  void set_fair_sharing(const std::unordered_map<int, std::vector<int>>& alloc, const Eigen::VectorXd& rho,
                        Eigen::VectorXd& fair_sharing) const;

  template <typename T> std::string debug_eigen(const T& obj) const;
  template <typename T> std::string debug_vector(const std::vector<T>& vector) const;
  std::string debug_alloc(const std::unordered_map<int, std::vector<int>>& alloc) const;
  /**
   * @brief Check if allocation is BMF
   *
   * To be a bmf allocation it must:
   * - respect the capacity of all resources
   * - saturate at least 1 resource
   * - every player receives maximum share in at least 1 saturated resource
   * @param rho Allocation
   * @return true if BMF false otherwise
   */
  bool is_bmf(const Eigen::VectorXd& rho) const;

  int max_iteration_ = 10;
  Eigen::MatrixXd A_;
  Eigen::MatrixXd maxA_;
  std::unordered_map<int, Variable*> idx2Var_;
  Eigen::VectorXd C_;
  std::unordered_map<const Constraint*, int> cnst2idx_;
  static constexpr int NO_RESOURCE = -1; //!< flag to indicate player has selected no resource
};

} // namespace lmm
} // namespace kernel
} // namespace simgrid

#endif