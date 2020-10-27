#include "mc_api.hpp"

#include "src/mc/Session.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/remote/RemoteSimulation.hpp"
#include "src/mc/mc_record.hpp"

#include <xbt/asserts.h>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_api, mc, "Logging specific to MC Fasade APIs ");

namespace simgrid {
namespace mc {

void mc_api::initialize(char** argv)
{
  simgrid::mc::session = new simgrid::mc::Session([argv] {
    int i = 1;
    while (argv[i] != nullptr && argv[i][0] == '-')
      i++;
    xbt_assert(argv[i] != nullptr,
               "Unable to find a binary to exec on the command line. Did you only pass config flags?");
    execvp(argv[i], argv + i);
    xbt_die("The model-checked process failed to exec(): %s", strerror(errno));
  });
}

std::vector<simgrid::mc::ActorInformation>& mc_api::get_actors() const
{
  return mc_model_checker->get_remote_simulation().actors();
}

bool mc_api::actor_is_enabled(aid_t pid) const
{
  return session->actor_is_enabled(pid);
}

void mc_api::s_initialize() const
{
  session->initialize();
}

ModelChecker* mc_api::get_model_checker() const
{
  return mc_model_checker;
}

void mc_api::mc_inc_visited_states() const
{
  mc_model_checker->visited_states++;
}

void mc_api::mc_inc_executed_trans() const
{
  mc_model_checker->executed_transitions++;
}

unsigned long mc_api::mc_get_visited_states() const
{
  return mc_model_checker->visited_states;
}

unsigned long mc_api::mc_get_executed_trans() const
{
  return mc_model_checker->executed_transitions;
}

bool mc_api::mc_check_deadlock() const
{
  return mc_model_checker->checkDeadlock();
}

void mc_api::mc_show_deadlock() const
{
  MC_show_deadlock();
}

smx_actor_t mc_api::mc_smx_simcall_get_issuer(s_smx_simcall const* req) const
{
  return MC_smx_simcall_get_issuer(req);
}

void mc_api::mc_assert(bool notNull, const char* message) const
{
  if (notNull)
    xbt_assert(mc_model_checker == nullptr, message);
  else
    xbt_assert(mc_model_checker != nullptr, message);
}

bool mc_api::mc_is_null() const
{
  auto is_null = (mc_model_checker == nullptr) ? true : false;
  return is_null;
}

Checker* mc_api::mc_get_checker() const
{
  return mc_model_checker->getChecker();
}

RemoteSimulation& mc_api::mc_get_remote_simulation() const
{
  return mc_model_checker->get_remote_simulation();
}

void mc_api::handle_simcall(Transition const& transition) const
{
  mc_model_checker->handle_simcall(transition);
}

void mc_api::mc_wait_for_requests() const
{
  mc_model_checker->wait_for_requests();
}

void mc_api::mc_exit(int status) const
{
  mc_model_checker->exit(status);
}

std::string const& mc_api::mc_get_host_name(std::string const& hostname) const
{
  return mc_model_checker->get_host_name(hostname);
}

PageStore& mc_api::mc_page_store() const
{
  return mc_model_checker->page_store();
}

void mc_api::mc_dump_record_path() const
{
  simgrid::mc::dumpRecordPath();
}

smx_simcall_t mc_api::mc_state_choose_request(simgrid::mc::State* state) const
{
  return MC_state_choose_request(state);
}

bool mc_api::request_depend(smx_simcall_t req1, smx_simcall_t req2) const
{
  return simgrid::mc::request_depend(req1, req2);
}

std::string mc_api::request_to_string(smx_simcall_t req, int value, RequestType request_type) const
{
  return simgrid::mc::request_to_string(req, value, request_type).c_str();
}

std::string mc_api::request_get_dot_output(smx_simcall_t req, int value) const
{
  return simgrid::mc::request_get_dot_output(req, value);
}

const char* mc_api::simix_simcall_name(e_smx_simcall_t kind) const
{
  return SIMIX_simcall_name(kind);
}

bool mc_api::snapshot_equal(const Snapshot* s1, const Snapshot* s2) const
{
  return simgrid::mc::snapshot_equal(s1, s2);
}

void mc_api::s_close() const
{
  session->close();
}

void mc_api::s_restore_initial_state() const
{
  session->restore_initial_state();
}

void mc_api::execute(Transition const& transition)
{
  session->execute(transition);
}

void mc_api::s_log_state() const
{
  session->log_state();
}

} // namespace mc
} // namespace simgrid
