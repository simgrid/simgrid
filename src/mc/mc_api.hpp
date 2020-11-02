#ifndef SIMGRID_MC_API_HPP
#define SIMGRID_MC_API_HPP

#include <map>
#include <memory>
#include <vector>

#include "simgrid/forward.h"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_state.hpp"

#include "xbt/base.h"

namespace simgrid {
namespace mc {

enum class PatternCommunicationType_Dev {
  none    = 0,
  send    = 1,
  receive = 2,
};

class PatternCommunication_Dev {
public:
  int num = 0;
  simgrid::kernel::activity::CommImpl* comm_addr;
  PatternCommunicationType_Dev type = PatternCommunicationType_Dev::send;
  unsigned long src_proc            = 0;
  unsigned long dst_proc            = 0;
  const char* src_host              = nullptr;
  const char* dst_host              = nullptr;
  std::string rdv;
  std::vector<char> data;
  int tag   = 0;
  int index = 0;

  PatternCommunication_Dev() { std::memset(&comm_addr, 0, sizeof(comm_addr)); }

  PatternCommunication_Dev dup() const
  {
    simgrid::mc::PatternCommunication_Dev res;
    // num?
    res.comm_addr = this->comm_addr;
    res.type      = this->type;
    // src_proc?
    // dst_proc?
    res.dst_proc = this->dst_proc;
    res.dst_host = this->dst_host;
    res.rdv      = this->rdv;
    res.data     = this->data;
    // tag?
    res.index = this->index;
    return res;
  }
};

class state_detail {
public:
  /* Can be used as a copy of the remote synchro object */
  simgrid::mc::Remote<simgrid::kernel::activity::CommImpl> internal_comm_;

  /** Snapshot of system state (if needed) */
  std::shared_ptr<simgrid::mc::Snapshot> system_state_;

  // For CommunicationDeterminismChecker
  std::vector<std::vector<simgrid::mc::PatternCommunication>> incomplete_comm_pattern_;
  std::vector<unsigned> communication_indices_;

  explicit state_detail(unsigned long state_number);
};

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
  std::map<unsigned long, std::unique_ptr<state_detail>> state_detail_;

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

  // ACTOR FUNCTIONS
  std::vector<simgrid::mc::ActorInformation>& get_actors() const;
  bool actor_is_enabled(aid_t pid) const;

  // MODEL_CHECKER FUNCTIONS
  ModelChecker* get_model_checker() const;
  void mc_inc_visited_states() const;
  void mc_inc_executed_trans() const;
  unsigned long mc_get_visited_states() const;
  unsigned long mc_get_executed_trans() const;
  bool mc_check_deadlock() const;
  void mc_show_deadlock() const;
  smx_actor_t mc_smx_simcall_get_issuer(s_smx_simcall const* req) const;
  bool mc_is_null() const;
  Checker* mc_get_checker() const;
  RemoteSimulation& mc_get_remote_simulation() const;
  void handle_simcall(Transition const& transition) const;
  void mc_wait_for_requests() const;
  void mc_exit(int status) const;
  std::string const& mc_get_host_name(std::string const& hostname) const;
  PageStore& mc_page_store() const;
  void mc_dump_record_path() const;
  smx_simcall_t mc_state_choose_request(simgrid::mc::State* state) const;

  // SIMCALL FUNCTIONS
  bool request_depend(smx_simcall_t req1, smx_simcall_t req2) const;
  std::string request_to_string(smx_simcall_t req, int value, RequestType request_type) const;
  std::string request_get_dot_output(smx_simcall_t req, int value) const;
  const char* simix_simcall_name(e_smx_simcall_t kind) const;

  // SNAPSHOT FUNCTIONS
  bool snapshot_equal(const Snapshot* s1, const Snapshot* s2) const;

  // STATE FUNCTIONS
  void create_state_detail(unsigned long state_number);

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