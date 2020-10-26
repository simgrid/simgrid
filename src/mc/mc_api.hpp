#ifndef SIMGRID_MC_API_HPP
#define SIMGRID_MC_API_HPP

#include <memory>
#include <vector>

#include "simgrid/forward.h"
#include "src/mc/mc_forward.hpp"
#include "xbt/base.h"

namespace simgrid {
namespace mc {

/*
** This class aimes to implement FACADE APIs for simgrid. The FACADE layer sits between the CheckerSide
** (Unfolding_Checker, DPOR, ...) layer and the
** AppSide layer. The goal is to drill down into the entagled details in the CheckerSide layer and break down the
** detailes in a way that the CheckerSide eventually
** be capable to acquire the required information through the FACADE layer rather than the direct access to the AppSide.
*/

class mc_api {
private:
  mc_api() = default;

public:
  // No copy:
  mc_api(mc_api const&) = delete;
  void operator=(mc_api const&) = delete;

  static mc_api& get()
  {
    static mc_api mcapi;
    return mcapi;
  }

  void initialize(char** argv);

  // MODEL_CHECKER FUNCTIONS
  void create_model_checker(std::unique_ptr<RemoteSimulation> remote_simulation, int sockfd);
  ModelChecker* get_model_checker() const;
  void mc_inc_visited_states() const;
  void mc_inc_executed_trans() const;
  unsigned long mc_get_visited_states() const;
  unsigned long mc_get_executed_trans() const;
  bool mc_check_deadlock() const;
  std::vector<simgrid::mc::ActorInformation>& get_actors() const;
  bool actor_is_enabled(aid_t pid) const;
  void mc_assert(bool notNull, const char* message = "") const;
  bool mc_is_null() const;
  Checker* mc_get_checker() const;
  RemoteSimulation& mc_get_remote_simulation() const;
  void handle_simcall(Transition const& transition) const;
  void mc_wait_for_requests() const;
  void mc_exit(int status) const;
  std::string const& mc_get_host_name(std::string const& hostname) const;
  PageStore& mc_page_store() const;
  void mc_cleanup();

  // SESSION FUNCTIONS
  void s_initialize() const;
  void s_close() const;
  void s_restore_initial_state() const;
  void execute(Transition const& transition);
  void s_log_state() const;
};

} // namespace mc
} // namespace simgrid

#endif