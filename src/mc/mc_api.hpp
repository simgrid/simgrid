#ifndef SIMGRID_MC_API_HPP
#define SIMGRID_MC_API_HPP

#include <memory>
#include <vector>

#include "simgrid/forward.h"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_request.hpp"
#include "src/mc/mc_state.hpp"
#include "src/mc/mc_record.hpp"
#include "xbt/automaton.hpp"
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

  struct DerefAndCompareByActorsCountAndUsedHeap {
    template <class X, class Y> bool operator()(X const& a, Y const& b) const
    {
      return std::make_pair(a->actors_count, a->heap_bytes_used) < std::make_pair(b->actors_count, b->heap_bytes_used);
    }
  };


public:
  // No copy:
  mc_api(mc_api const&) = delete;
  void operator=(mc_api const&) = delete;

  static mc_api& get()
  {
    static mc_api mcapi;
    return mcapi;
  }

  void initialize(char** argv) const;

  // ACTOR APIs  
  std::vector<simgrid::mc::ActorInformation>& get_actors() const;
  bool actor_is_enabled(aid_t pid) const;
  unsigned long get_maxpid() const;
  int get_actors_size() const;

  // COMMUNICATION APIs
  bool comm_addr_equal(const kernel::activity::CommImpl* comm_addr1, const kernel::activity::CommImpl* comm_addr2) const;
  kernel::activity::CommImpl* get_comm_isend_raw_addr(smx_simcall_t request) const;
  kernel::activity::CommImpl* get_comm_wait_raw_addr(smx_simcall_t request) const;
  kernel::activity::CommImpl* get_comm_waitany_raw_addr(smx_simcall_t request, int value) const;
  std::string get_pattern_comm_rdv(void* addr) const;
  unsigned long get_pattern_comm_src_proc(void* addr) const;
  unsigned long get_pattern_comm_dst_proc(void* addr) const;
  std::vector<char> get_pattern_comm_data(void* addr) const;
  std::vector<char> get_pattern_comm_data(const kernel::activity::CommImpl* comm_addr) const;
  const char* get_actor_host_name(smx_actor_t actor) const;
#if HAVE_SMPI
  bool check_send_request_detached(smx_simcall_t const& simcall) const;
#endif
  smx_actor_t get_src_actor(const kernel::activity::CommImpl* comm_addr) const;
  smx_actor_t get_dst_actor(const kernel::activity::CommImpl* comm_addr) const;

  // REMOTE APIs
  std::size_t get_remote_heap_bytes() const;

  // MODEL CHECKER APIs
  void mc_inc_visited_states() const;
  void mc_inc_executed_trans() const;
  unsigned long mc_get_visited_states() const;
  unsigned long mc_get_executed_trans() const;
  bool mc_check_deadlock() const;
  void mc_show_deadlock() const;
  bool mc_is_null() const;
  Checker* mc_get_checker() const;
  void set_checker(Checker* const checker) const;
  RemoteSimulation& mc_get_remote_simulation() const;
  void handle_simcall(Transition const& transition) const;
  void mc_wait_for_requests() const;
  XBT_ATTRIB_NORETURN void mc_exit(int status) const;
  std::string const& mc_get_host_name(std::string const& hostname) const;
  void dump_record_path() const;
  smx_simcall_t mc_state_choose_request(simgrid::mc::State* state) const;

  // SIMCALL APIs
  bool request_depend(smx_simcall_t req1, smx_simcall_t req2) const;
  std::string request_to_string(smx_simcall_t req, int value, RequestType request_type) const;
  std::string request_get_dot_output(smx_simcall_t req, int value) const;
  const char *simcall_get_name(simgrid::simix::Simcall kind) const;
  smx_actor_t simcall_get_issuer(s_smx_simcall const* req) const;
  long simcall_get_actor_id(s_smx_simcall const* req) const;
  smx_mailbox_t simcall_get_mbox(smx_simcall_t const req) const;

#if HAVE_SMPI
  int get_smpi_request_tag(smx_simcall_t const& simcall, simgrid::simix::Simcall type) const;
#endif

  // STATE APIs
  void restore_state(std::shared_ptr<simgrid::mc::Snapshot> system_state) const;
  void log_state() const;
  void restore_initial_state() const;

  // SNAPSHOT APIs
  bool snapshot_equal(const Snapshot* s1, const Snapshot* s2) const;
  simgrid::mc::Snapshot* take_snapshot(int num_state) const;

  // SESSION APIs
  void session_initialize() const;
  void s_close() const;
  void execute(Transition const& transition) const;

  // AUTOMATION APIs
  #if SIMGRID_HAVE_MC
  void automaton_load(const char *file) const;
  #endif
  std::vector<int> automaton_propositional_symbol_evaluate() const;
  std::vector<xbt_automaton_state_t> get_automaton_state() const;
  int compare_automaton_exp_label(const xbt_automaton_exp_label* l) const;
  void set_property_automaton(xbt_automaton_state_t const& automaton_state) const;
  inline DerefAndCompareByActorsCountAndUsedHeap compare_pair() const {
    return DerefAndCompareByActorsCountAndUsedHeap();
  }
  inline int automaton_state_compare(const_xbt_automaton_state_t const& s1, const_xbt_automaton_state_t const& s2) const {
    return xbt_automaton_state_compare(s1, s2);
  }
  xbt_automaton_exp_label_t get_automaton_transition_label(xbt_dynar_t const& dynar, int index) const;
  xbt_automaton_state_t get_automaton_transition_dst(xbt_dynar_t const& dynar, int index) const;

  // DYNAR APIs
  inline unsigned long get_dynar_length(const_xbt_dynar_t const& dynar) const {
    return xbt_dynar_length(dynar);
  }
};

} // namespace mc
} // namespace simgrid

#endif
