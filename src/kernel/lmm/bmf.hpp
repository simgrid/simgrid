/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_BMF_HPP
#define SURF_BMF_HPP

#include "src/kernel/lmm/maxmin.hpp"
#include <boost/container_hash/hash.hpp>
#include <eigen3/Eigen/Dense>
#include <unordered_set>

namespace simgrid {
namespace kernel {
namespace lmm {

class XBT_PUBLIC AllocationGenerator {
public:
  explicit AllocationGenerator(Eigen::MatrixXd A);

  bool next(std::vector<int>& next_alloc);

private:
  Eigen::MatrixXd A_; //!< A_ji: resource usage matrix, each row j represents a resource and col i a flow/player
  std::vector<int> alloc_;
  bool first_ = true;
};

class XBT_PUBLIC BmfSolver {
public:
  /**
   * @brief Instantiate the BMF solver
   *
   * @param A A_ji: consumption of player i on resource j
   * @param maxA maxA_ji: consumption of larger player i on resource j
   * @param C Resource capacity
   * @param shared Is resource shared between player or each player receives the full capacity (FATPIPE links)
   * @param phi Bound for each player
   */
  BmfSolver(Eigen::MatrixXd A, Eigen::MatrixXd maxA, Eigen::VectorXd C, std::vector<bool> shared, Eigen::VectorXd phi);
  /** @brief Solve equation system to find a fair-sharing of resources */
  Eigen::VectorXd solve();

private:
  using allocation_map_t = std::unordered_map<int, std::unordered_set<int>>;
  /**
   * @brief Get actual resource capacity considering bounded players
   *
   * Calculates the resource capacity considering that some players on it may be bounded by user,
   * i.e. an explicit limit in speed was configured
   *
   * @param resource Internal index of resource in C_ vector
   * @param bounded_players List of players that are externally bounded
   * @return Actual resource capacity
   */
  double get_resource_capacity(int resource, const std::vector<int>& bounded_players) const;

  /**
   * @brief Given an allocation calculates the speed/rho for each player
   *
   * Do the magic!!
   * Builds 2 auxiliares matrices A' and C' and solves the system: rho_i = inv(A'_ji) * C'_j
   *
   * All resources in A' and C' are saturated, i.e., sum(A'_j * rho_i) = C'_j.
   *
   * The matrix A' is built as follows:
   * - For each resource j in alloc: copy row A_j to A'
   * - If 2 players (i, k) share a same resource, assure fairness by adding a row in A' such as:
   *   -  A_ji*rho_i - Ajk*rho_j = 0
   *
   * @param alloc for each resource, players that chose to saturate it
   * @return Vector rho with "players' speed"
   */
  Eigen::VectorXd equilibrium(const allocation_map_t& alloc) const;

  /**
   * @brief Given a fair_sharing vector, gets the allocation
   *
   * The allocation for player i is given by: min(bound, f_j/A_ji).
   * The minimum between all fair-sharing and the external bound (if any)
   *
   * The algorithm dictates a random initial allocation. For simplicity, we opt to use the same
   * logic with the fair_sharing vector.
   *
   * @param fair_sharing Fair sharing vector
   * @param initial Is this the initial allocation?
   * @return allocation vector
   */
  bool get_alloc(const Eigen::VectorXd& fair_sharing, const allocation_map_t& last_alloc, allocation_map_t& alloc,
                 bool initial);

  bool disturb_allocation(allocation_map_t& alloc, std::vector<int>& alloc_by_player);
  /**
   * @brief Calculates the fair sharing for each resource
   *
   * Basically 3 options:
   * 1) resource in allocation: A_ji*rho_i since all players who selected this resource have the same share
   * 2) resource not selected by saturated (fully used): divide it by the number of players C_/n_players
   * 3) resource not selected and not-saturated: no limitation
   *
   * @param alloc Allocation map (resource-> players)
   * @param rho Speed for each player i
   * @param fair_sharing Output vector, fair sharing for each resource j
   */
  void set_fair_sharing(const allocation_map_t& alloc, const Eigen::VectorXd& rho, Eigen::VectorXd& fair_sharing) const;

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
  std::vector<int> alloc_map_to_vector(const allocation_map_t& alloc) const;

  /**
   * @brief Set of debug functions to print the different objects
   */
  template <typename T> std::string debug_eigen(const T& obj) const;
  template <typename C> std::string debug_vector(const C& container) const;
  std::string debug_alloc(const allocation_map_t& alloc) const;

  Eigen::MatrixXd A_;    //!< A_ji: resource usage matrix, each row j represents a resource and col i a flow/player
  Eigen::MatrixXd maxA_; //!< maxA_ji,  similar as A_, but containing the maximum consumption of player i (if player a
                         //!< single flow it's equal to A_)
  Eigen::VectorXd C_;    //!< C_j Capacity of each resource
  std::vector<bool> C_shared_; //!< shared_j Resource j is shared or not
  Eigen::VectorXd phi_;        //!< phi_i bound for each player

  std::unordered_set<std::vector<int>, boost::hash<std::vector<int>>> allocations_;
  AllocationGenerator gen_;
  std::vector<int> allocations_age_;
  static constexpr int NO_RESOURCE = -1;                    //!< flag to indicate player has selected no resource
  int max_iteration_               = sg_bmf_max_iterations; //!< number maximum of iterations of BMF algorithm
};

/**
 * @brief Bottleneck max-fair system
 */
class XBT_PUBLIC BmfSystem : public System {
public:
  using System::System;
  /** @brief Implements the solve method to calculate a BMF allocation */
  void solve() final;

private:
  using allocation_map_t = std::unordered_map<int, std::unordered_set<int>>;
  /**
   * @brief Solve equation system to find a fair-sharing of resources
   *
   * @param cnst_list Constraint list (modified for selective update or active)
   */
  template <class CnstList> void bmf_solve(const CnstList& cnst_list);
  /**
   * @brief Iterates over system and build the consumption matrix A_ji and maxA_ji
   *
   * Each row j represents a resource and each col i a player/flow
   *
   * Considers only active variables to build the matrix.
   *
   * @param number_cnsts Number of constraints in the system
   * @param A Consumption matrix (OUTPUT)
   * @param maxA Max subflow consumption matrix (OUTPUT)
   * @param phi Bounds for variables
   */
  void get_flows_data(int number_cnsts, Eigen::MatrixXd& A, Eigen::MatrixXd& maxA, Eigen::VectorXd& phi);
  /**
   * @brief Builds the vector C_ with resource's capacity
   *
   * @param cnst_list Constraint list (modified for selective update or active)
   * @param C Resource capacity vector
   * @param shared Resource is shared or not (fatpipe links)
   */
  template <class CnstList>
  void get_constraint_data(const CnstList& cnst_list, Eigen::VectorXd& C, std::vector<bool>& shared);

  std::unordered_map<int, Variable*> idx2Var_; //!< Map player index (and position in matrices) to system's variable
  std::unordered_map<const Constraint*, int> cnst2idx_; //!< Conversely map constraint to index
  bool warned_nonlinear_ = false;
};

} // namespace lmm
} // namespace kernel
} // namespace simgrid

#endif