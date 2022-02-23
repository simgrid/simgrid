/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/lmm/bmf.hpp"
#include <eigen3/Eigen/LU>
#include <iostream>
#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_bmf, kernel, "Kernel BMF solver");

void simgrid::kernel::lmm::BmfSystem::set_matrix_A()
{
  A_.resize(active_constraint_set.size(), variable_set.size());
  A_.setZero();
  maxA_.resize(active_constraint_set.size(), variable_set.size());

  int var_idx = 0;
  for (Variable& var : variable_set) {
    if (var.sharing_penalty_ <= 0)
      continue;
    bool active = false;
    var.value_  = 1; // assign something by default for tasks with 0 consumption
    for (const Element& elem : var.cnsts_) {
      double consumption = elem.consumption_weight;
      if (consumption > 0) {
        int cnst_idx             = cnst2idx_[elem.constraint];
        A_(cnst_idx, var_idx)    = consumption;
        maxA_(cnst_idx, var_idx) = elem.max_consumption_weight;
        active                   = true;
      }
    }
    if (active) {
      idx2Var_[var_idx] = &var;
      var_idx++;
    }
  }
  // resize matrix to active variables only
  A_.conservativeResize(Eigen::NoChange_t::NoChange, var_idx);
  maxA_.conservativeResize(Eigen::NoChange_t::NoChange, var_idx);
}

void simgrid::kernel::lmm::BmfSystem::set_vector_C()
{
  C_.resize(active_constraint_set.size());
  cnst2idx_.clear();
  int cnst_idx = 0;
  for (const Constraint& cnst : active_constraint_set) {
    C_(cnst_idx)     = cnst.bound_;
    cnst2idx_[&cnst] = cnst_idx;
    cnst_idx++;
  }
}

std::unordered_map<int, std::vector<int>>
simgrid::kernel::lmm::BmfSystem::get_alloc(const Eigen::VectorXd& fair_sharing, bool initial) const
{
  std::unordered_map<int, std::vector<int>> alloc;
  for (int player_idx = 0; player_idx < A_.cols(); player_idx++) {
    int selected_resource = NO_RESOURCE;
    double bound          = idx2Var_.at(player_idx)->get_bound();
    double min_share      = (bound <= 0 || initial) ? -1 : bound;
    for (int cnst_idx = 0; cnst_idx < A_.rows(); cnst_idx++) {
      if (A_(cnst_idx, player_idx) <= 0.0)
        continue;

      double share = fair_sharing[cnst_idx] / A_(cnst_idx, player_idx);
      if (min_share == -1 || double_positive(min_share - share, sg_maxmin_precision)) {
        selected_resource = cnst_idx;
        min_share         = share;
      }
    }
    alloc[selected_resource].push_back(player_idx);
  }
  return alloc;
}

void simgrid::kernel::lmm::BmfSystem::set_fair_sharing(const std::unordered_map<int, std::vector<int>>& alloc,
                                                       const Eigen::VectorXd& rho, Eigen::VectorXd& fair_sharing) const
{
  for (int r = 0; r < fair_sharing.size(); r++) {
    auto it = alloc.find(r);
    if (it != alloc.end()) {      // resource selected by some player, fair share depends on rho
      int player = it->second[0]; // equilibrium assures that every player receives the same, use one of them to
                                  // calculate the fair sharing for resource r
      fair_sharing[r] = A_(r, player) * rho[player];
    } else { // nobody selects this resource, fair_sharing depends on resource saturation
      // resource r is saturated (A[r,*] * rho > C), divide it among players
      double consumption_r = A_.row(r) * rho;
      double_update(&consumption_r, C_[r], sg_maxmin_precision);
      if (consumption_r > 0.0) {
        int n_players   = std::count_if(A_.row(r).data(), A_.row(r).data() + A_.row(r).size(),
                                      [](double v) { return double_positive(v, sg_maxmin_precision); });
        fair_sharing[r] = C_[r] / n_players;
      } else {
        fair_sharing[r] = C_[r];
      }
    }
  }
}

template <typename T> std::string simgrid::kernel::lmm::BmfSystem::debug_eigen(const T& obj) const
{
  std::stringstream debug;
  debug << obj;
  return debug.str();
}

template <typename T> std::string simgrid::kernel::lmm::BmfSystem::debug_vector(const std::vector<T>& vector) const
{
  std::stringstream debug;
  std::copy(vector.begin(), vector.end(), std::ostream_iterator<T>(debug, " "));
  return debug.str();
}

std::string simgrid::kernel::lmm::BmfSystem::debug_alloc(const std::unordered_map<int, std::vector<int>>& alloc) const
{
  std::stringstream debug;
  for (const auto& e : alloc) {
    debug << "{" + std::to_string(e.first) + ": [" + debug_vector(e.second) + "]}, ";
  }
  return debug.str();
}

double simgrid::kernel::lmm::BmfSystem::get_resource_capacity(int resource,
                                                              const std::vector<int>& bounded_players) const
{
  double capacity = C_[resource];
  for (int p : bounded_players) {
    capacity -= A_(resource, p) * idx2Var_.at(p)->get_bound();
  }
  return capacity;
}

Eigen::VectorXd
simgrid::kernel::lmm::BmfSystem::equilibrium(const std::unordered_map<int, std::vector<int>>& alloc) const
{
  int n_players       = A_.cols();
  Eigen::MatrixXd A_p = Eigen::MatrixXd::Zero(n_players, n_players); // square matrix with number of players
  Eigen::VectorXd C_p = Eigen::VectorXd::Zero(n_players);

  // iterate over alloc to verify if 2 players have chosen the same resource
  // if so, they must have a fair sharing of this resource, adjust A_p and C_p accordingly
  int last_row = n_players - 1;
  int first_row = 0;
  std::vector<int> bounded_players;
  for (const auto& e : alloc) {
    // add one row for the resource with A[r,]
    int cur_resource   = e.first;
    if (cur_resource == NO_RESOURCE) {
      bounded_players.insert(bounded_players.end(), e.second.begin(), e.second.end());
      continue;
    }
    A_p.row(first_row) = A_.row(cur_resource);
    C_p[first_row]     = get_resource_capacity(cur_resource, bounded_players);
    first_row++;
    if (e.second.size() > 1) {
      int i = e.second[0];                                 // first player
      for (size_t idx = 1; idx < e.second.size(); idx++) { // for each other player sharing this resource
        /* player i and k on this resource j: so maxA_ji*rho_i - maxA_jk*rho_k = 0 */
        int k            = e.second[idx];
        C_p[last_row]    = 0;
        A_p(last_row, i) = maxA_(cur_resource, i);
        A_p(last_row, k) = -maxA_(cur_resource, k);
        last_row--;
      }
    }
  }
  /* clear players which are externally bounded */
  for (int p : bounded_players) {
    A_p.col(p).setZero();
  }

  XBT_DEBUG("A':\n%s", debug_eigen(A_p).c_str());

  XBT_DEBUG("C':\n%s", debug_eigen(C_p).c_str());
  Eigen::VectorXd rho = Eigen::FullPivLU<Eigen::MatrixXd>(A_p).solve(C_p);
  for (int p : bounded_players) {
    rho[p] = idx2Var_.at(p)->get_bound();
  }
  return rho;
}

bool simgrid::kernel::lmm::BmfSystem::is_bmf(const Eigen::VectorXd& rho) const
{
  bool bmf = true;

  // 1) the capacity of all resources is respected
  Eigen::VectorXd remaining = (A_ * rho) - C_;
  bmf                       = bmf && (not std::any_of(remaining.data(), remaining.data() + remaining.size(),
                                [](double v) { return double_positive(v, sg_maxmin_precision); }));

  // 3) every player receives maximum share in at least 1 saturated resource
  // due to subflows, compare with the maximum consumption and not the A matrix
  Eigen::MatrixXd usage =
      maxA_.array().rowwise() * rho.transpose().array(); // usage_ji: indicates the usage of player i on resource j

  XBT_DEBUG("Usage_ji considering max consumption:\n%s", debug_eigen(usage).c_str());
  auto max_share = usage.rowwise().maxCoeff(); // max share for each resource j

  // matrix_ji: boolean indicating player p has the maximum share at resource j
  Eigen::MatrixXi player_max_share =
      ((usage.array().colwise() - max_share.array()).abs() <= sg_maxmin_precision).cast<int>();
  // but only saturated resources must be considered
  Eigen::VectorXi saturated = ((remaining.array().abs() <= sg_maxmin_precision)).cast<int>();
  XBT_DEBUG("Saturated_j resources:\n%s", debug_eigen(saturated).c_str());
  player_max_share.array().colwise() *= saturated.array();

  // just check if it has received at least it's bound
  for (int p = 0; p < rho.size(); p++) {
    if (double_equals(rho[p], idx2Var_.at(p)->get_bound(), sg_maxmin_precision)) {
      player_max_share(0, p) = 1; // it doesn't really matter, just to say that it's a bmf
      saturated[0]           = 1;
    }
  }

  // 2) at least 1 resource is saturated
  bmf = bmf && (saturated.array() == 1).any();

  XBT_DEBUG("Player_ji usage of saturated resources:\n%s", debug_eigen(player_max_share).c_str());
  // for all columns(players) it has to be the max at least in 1
  bmf = bmf && (player_max_share.colwise().sum().all() >= 1);
  return bmf;
}

void simgrid::kernel::lmm::BmfSystem::bottleneck_solve()
{
  if (not modified_)
    return;

  XBT_DEBUG("");
  XBT_DEBUG("Starting BMF solver");
  /* initialize players' weight and constraint matrices */
  set_vector_C();
  set_matrix_A();
  XBT_DEBUG("A:\n%s", debug_eigen(A_).c_str());
  XBT_DEBUG("C:\n%s", debug_eigen(C_).c_str());

  /* no flows to share, just returns */
  if (A_.cols() == 0)
    return;

  int it            = 0;
  auto fair_sharing = C_;

  /* BMF allocation for each player (current and last one) stop when are equal */
  std::unordered_map<int, std::vector<int>> last_alloc;
  auto cur_alloc = get_alloc(fair_sharing, true);
  Eigen::VectorXd rho;
  while (it < max_iteration_ && last_alloc != cur_alloc) {
    last_alloc = cur_alloc;
    XBT_DEBUG("BMF: iteration %d", it);
    XBT_DEBUG("B (current allocation): %s", debug_alloc(cur_alloc).c_str());

    // solve inv(A)*rho = C
    rho = equilibrium(cur_alloc);
    XBT_DEBUG("rho:\n%s", debug_eigen(rho).c_str());

    // get fair sharing for each resource
    set_fair_sharing(cur_alloc, rho, fair_sharing);
    XBT_DEBUG("Fair sharing vector (per resource):\n%s", debug_eigen(fair_sharing).c_str());

    // get new allocation for players
    cur_alloc = get_alloc(fair_sharing, false);
    XBT_DEBUG("B (new allocation): %s", debug_alloc(cur_alloc).c_str());
    it++;
  }

  xbt_assert(is_bmf(rho), "Not a BMF allocation");

  /* setting rhos */
  for (int i = 0; i < rho.size(); i++) {
    idx2Var_[i]->value_ = rho[i];
  }

  XBT_DEBUG("BMF done after %d iterations", it);
  print();
}
