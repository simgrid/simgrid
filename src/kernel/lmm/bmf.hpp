/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_BMF_HPP
#define SURF_BMF_HPP

#include "src/kernel/lmm/maxmin.hpp"
#include <Eigen/Dense>
#include <unordered_set>

namespace simgrid {
namespace kernel {
namespace lmm {

/** @brief Generate all combinations of valid allocation */
class XBT_PUBLIC AllocationGenerator {
public:
  explicit AllocationGenerator(Eigen::MatrixXd A);

  /**
   * @brief Get next valid allocation
   *
   * @param next_alloc Allocation (OUTPUT)
   * @return true if there's an allocation not tested yet, false otherwise
   */
  bool next(std::vector<int>& next_alloc);

private:
  Eigen::MatrixXd A_;
  std::vector<int> alloc_;
  bool first_ = true;
};

/**
 * @beginrst
 *
 * Despite the simplicity of BMF fairness definition, it's quite hard to
 * find a BMF allocation in the general case.
 *
 * This solver implements one possible algorithm to find a BMF, as proposed
 * at: https://hal.archives-ouvertes.fr/hal-01552739.
 *
 * The idea of this algorithm is that each player/flow "selects" a resource to
 * saturate. Then, we calculate the rate each flow would have with this allocation.
 * If the allocation is a valid BMF and no one needs to move, it's over. Otherwise,
 * each player selects a new resource to saturate based on the minimim rate possible
 * between all resources.
 *
 * The steps:
 * 1) Given an initial allocation B_i
 * 2) Build a matrix A'_ji and C'_ji which assures that the player receives the most
 * share at selected resources
 * 3) Solve: A'_ji * rho_i = C'_j
 * 4) Calculate the minimum fair rate for each resource j: f_j. The f_j represents
 * the maximum each flow can receive at the resource j.
 * 5) Builds a new vector B'_i = arg min(f_j/A_ji).
 * 6) Stop if B == B' (nobody needs to move), go to step 2 otherwise
 *
 * Despite the overall good performance of this algorithm, which converges in a few
 * iterations, we don't have any assurance about its convergence. In the worst case,
 * it may be needed to test all possible combination of allocations (which is exponential).
 *
 * @endrst
 */
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
   * @brief Auxiliary method to get list of bounded player from allocation
   *
   * @param alloc Current allocation
   * @return list of bounded players
   */
  std::vector<int> get_bounded_players(const allocation_map_t& alloc) const;

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

  std::set<std::vector<int>> allocations_; //!< set of already tested allocations, since last identified loop
  AllocationGenerator gen_;
  std::vector<int> allocations_age_;
  static constexpr int NO_RESOURCE = -1;                    //!< flag to indicate player has selected no resource
  int max_iteration_;                                       //!< number maximum of iterations of BMF algorithm
};

/**
 * @beginrst
 *
 * A BMF (bottleneck max fairness) solver to resolve inequation systems.
 *
 * Usually, SimGrid relies on a *max-min fairness* solver to share the resources.
 * Max-min is great when sharing homogenous resources, however it cannot be used with heterogeneous resources.
 *
 * BMF is a natural alternative to max-min, providing a fair-sharing of heterogeneous resources (CPU, network, disk).
 * It is specially relevant for the implementation of parallel tasks whose sharing involves different
 * kinds of resources.
 *
 * BMF assures that every flow receives the maximum share possible in at least 1 bottleneck (fully used) resource.
 *
 * The BMF is characterized by:
 * - A_ji: a matrix of requirement for flows/player. For each resource j, and flow i, A_ji represents the utilization
 * of resource j for 1 unit of the flow i.
 * - rho_i: the rate allocated for flow i (same among all resources)
 * - C_j: the capacity of each resource (can be bytes/s, flops/s, etc)
 *
 * Therefore, these conditions need to satisfied to an allocation be considered a BMF:
 * 1) All constraints are respected (flows cannot use more than the resource has available)
 *   - for all resource j and player i: A_ji * rho_i <= C_j
 * 2) At least 1 resource is fully used (bottleneck).
 *   - for some resource j: A_ji * rho_i = C_j
 * 3) Each flow (player) receives the maximum share in at least 1 bottleneck.
 *   - for all player i: exist a resource j: A_ji * rho_i >= A_jk * rho_k for all other player k
 *
 * Despite the prove of existence of a BMF allocation in the general case, it may not
 * be unique, which leads to possible different rate for the applications.
 *
 * More details about BMF can be found at: https://hal.inria.fr/hal-01243985/document
 *
 * @endrst
 */
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
  void get_flows_data(Eigen::Index number_cnsts, Eigen::MatrixXd& A, Eigen::MatrixXd& maxA, Eigen::VectorXd& phi);
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
