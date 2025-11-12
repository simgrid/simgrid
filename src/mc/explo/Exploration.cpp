/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/Exploration.hpp"
#include "simgrid/forward.h"
#include "src/mc/api/Strategy.hpp"
#include "src/mc/api/states/SleepSetState.hpp"
#include "src/mc/api/states/State.hpp"
#include "src/mc/explo/CriticalTransitionExplorer.hpp"
#include "src/mc/explo/odpor/Execution.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_environ.h"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/log.h"
#include "xbt/random.hpp"
#include "xbt/string.hpp"

#include <algorithm>
#include <memory>
#include <signal.h>
#include <sys/wait.h>
#include <utility>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_explo, mc, "Generic exploration algorithm of the model-checker");

namespace simgrid::mc {

static simgrid::config::Flag<std::string> cfg_dot_output_file{
    "model-check/dot-output", "Name of dot output file corresponding to graph state", ""};

Exploration* Exploration::instance_                         = nullptr; // singleton instance
std::unique_ptr<ExplorationStrategy> Exploration::strategy_ = std::make_unique<ExplorationStrategy>();

xbt::signal<void(State&, RemoteApp&)> Exploration::on_restore_state_signal;
xbt::signal<void(Transition*, RemoteApp&)> Exploration::on_transition_replay_signal;
xbt::signal<void(RemoteApp&)> Exploration::on_backtracking_signal;

xbt::signal<void(RemoteApp&)> Exploration::on_exploration_start_signal;
xbt::signal<void(State*, RemoteApp&)> Exploration::on_state_creation_signal;
xbt::signal<void(Transition*, RemoteApp&)> Exploration::on_transition_execute_signal;
xbt::signal<void(RemoteApp&)> Exploration::on_log_state_signal;

Exploration::Exploration(std::unique_ptr<RemoteApp> remote_app) : remote_app_(std::move(remote_app))
{
  // This awfull code should only be called when trying to create a critical transition explorer in the middle of
  // something else. It would be nice if someone smarter than me figured out a better way to do it.
  xbt_assert(instance_ != nullptr, "You shouldn't call this to create your first exploration");
  instance_               = this;
  is_looking_for_critical = true;
}

Exploration::Exploration()
{
  xbt_assert(instance_ == nullptr, "Cannot have more than one exploration instance");
  instance_ = this;

  time(&starting_time_);

  simgrid::xbt::random::set_mersenne_seed(_sg_mc_random_seed);

  if (not cfg_dot_output_file.get().empty()) {
    dot_output_ = fopen(cfg_dot_output_file.get().c_str(), "w");
    xbt_assert(dot_output_ != nullptr, "Error open dot output file: %s", strerror(errno));

    fprintf(dot_output_, "digraph graphname{\n fixedsize=true; rankdir=TB; ranksep=.25; edge [fontsize=12]; node "
                         "[fontsize=10, shape=circle,width=.5 ]; graph [fontsize=10];\n");
  }
}

Exploration::~Exploration()
{
  if (dot_output_ != nullptr)
    fclose(dot_output_);
  instance_ = nullptr;
}

void Exploration::dot_output(const char* fmt, ...)
{
  if (dot_output_ != nullptr) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(dot_output_, fmt, ap);
    va_end(ap);
    fflush(dot_output_);
  }
}

void Exploration::log_state()
{
  if (not cfg_dot_output_file.get().empty()) {
    dot_output("}\n");
    fclose(dot_output_);
  }
  if (getenv(MC_ENV_SYSTEM_STATISTICS)) {
    int ret = system("free");
    if (ret != 0)
      XBT_WARN("Call to system(free) did not return 0, but %d", ret);
  }
  if (_sg_mc_debug_optimality)
    odpor::MazurkiewiczTraces::log_data();
}
// Make our tests fully reproducible despite the subtle differences of strsignal() across archs
static const char* signal_name(int status)
{
  switch (WTERMSIG(status)) {
    case SIGABRT: // FreeBSD uses "Abort trap" as a strsignal for SIGABRT
      return "Aborted";
    case SIGSEGV: // MacOSX uses "Segmentation fault: 11" for SIGKILL
      return "Segmentation fault";
    default:
      return strsignal(WTERMSIG(status));
  }
}

std::vector<std::string> Exploration::get_textual_trace(int max_elements)
{
  std::vector<std::string> trace;
  for (auto const& transition : get_record_trace()) {
    auto const& call_location = transition->get_call_location();
    if (not call_location.empty())
      trace.push_back(xbt::string_printf("Actor %ld in %s ==> simcall: %s", transition->aid_, call_location.c_str(),
                                         transition->to_string().c_str()));
    else
      trace.push_back(xbt::string_printf("Actor %ld in simcall %s", transition->aid_, transition->to_string().c_str()));
    max_elements--;
    if (max_elements == 0)
      break;
  }
  return trace;
}

void Exploration::run_critical_exploration_on_need(ExitStatus error)
{
  if (_sg_mc_max_errors == 0 && _sg_mc_search_critical_transition && not is_looking_for_critical) {
    is_looking_for_critical = true;
    stack_t stack           = get_stack();
    CriticalTransitionExplorer explorer(std::move(remote_app_), get_model_checking_reduction(), &stack);

    // This will be executed after the first (and only) critical exploration:
    // we raise the same error, so the checker can return the correct failure code in the end
    throw McError(error);
  }
}

XBT_ATTRIB_NORETURN void Exploration::report_crash(int status)
{
  if (is_looking_for_critical)
    // already looking for critical
    throw McWarning(ExitStatus::PROGRAM_CRASH);
  else {
    XBT_INFO("**************************");
    XBT_INFO("** CRASH IN THE PROGRAM **");
    XBT_INFO("**************************");
    if (WIFSIGNALED(status))
      XBT_INFO("From signal: %s", signal_name(status));
    else if (WIFEXITED(status))
      XBT_INFO("From exit: %i", WEXITSTATUS(status));
    if (not xbt_log_no_loc)
      XBT_INFO("%s core dump was generated by the system.", WCOREDUMP(status) ? "A" : "No");

    XBT_INFO("Counter-example execution trace:");
    for (auto const& s : get_textual_trace())
      XBT_INFO("  %s", s.c_str());
    XBT_INFO("You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with "
             "--cfg=model-check/replay:'%s'",
             get_record_trace().to_string().c_str());
    log_state();
  }

  errors_++;

  // This is used to let the opportunity to the exploration to do things about the app crashing
  // and only then launch the critical exploration by itself
  if (_sg_mc_max_errors == 0 && _sg_mc_search_critical_transition)
    throw McWarning(ExitStatus::PROGRAM_CRASH);

  if (_sg_mc_max_errors >= 0 && errors_ > _sg_mc_max_errors)
    throw McError(ExitStatus::PROGRAM_CRASH);

  throw McWarning(ExitStatus::PROGRAM_CRASH);
}

XBT_ATTRIB_NORETURN void Exploration::report_assertion_failure()
{
  if (is_looking_for_critical)
    // already looking for critical
    throw McWarning(ExitStatus::SAFETY);
  else {
    XBT_INFO("**************************");
    XBT_INFO("*** PROPERTY NOT VALID ***");
    XBT_INFO("**************************");
    XBT_INFO("Counter-example execution trace:");
    for (auto const& s : get_textual_trace())
      XBT_INFO("  %s", s.c_str());
    XBT_INFO("You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with "
             "--cfg=model-check/replay:'%s'",
             get_record_trace().to_string().c_str());
    log_state();
  }

  // FIXME: this shouldn't be called errors, but smthing like "errors_nb_"
  errors_++;

  // This is used to let the opportunity to the exploration to do things about the app crashing
  // and only then launch the critical exploration by itself
  if (_sg_mc_max_errors == 0 && _sg_mc_search_critical_transition)
    throw McWarning(ExitStatus::SAFETY);

  if (_sg_mc_max_errors >= 0 && errors_ > _sg_mc_max_errors)
    throw McError(ExitStatus::SAFETY);

  throw McWarning(ExitStatus::SAFETY);
}
void Exploration::check_deadlock()
{
  if (get_remote_app().check_deadlock()) {

    // If we are looking for the critical transition, finding errors is normal
    // keep going, we want a correct trace!
    if (is_looking_for_critical)
      return;

    errors_++;
    run_critical_exploration_on_need(ExitStatus::DEADLOCK);

    if (_sg_mc_max_errors >= 0 && errors_ > _sg_mc_max_errors) {
      throw McError(ExitStatus::DEADLOCK);
    }
  }

  if (soft_timouted()) {
    XBT_INFO("Soft timeout after %d seconds. Gracefully exiting.", _sg_mc_soft_timeout.get());
    get_remote_app().finalize_app(true);
    exit(0);
  }
}

void Exploration::report_correct_execution(State* last_state)
{
  XBT_DEBUG("A correct execution has been reported !");
  last_state->register_as_correct();
  if (is_looking_for_critical)
    throw McError(ExitStatus::SUCCESS);
}

void Exploration::report_data_race(const McDataRace& e)
{
  XBT_INFO("Found a datarace at location %p between actor %ld and %ld after the follwing execution:", e.location_,
           e.first_mem_op_.first, e.second_mem_op_.first);
  for (auto const& frame : Exploration::get_instance()->get_textual_trace())
    XBT_INFO("  %s", frame.c_str());
  XBT_INFO("You can debug the problem (and see the whole details) by rerunning out of simgrid-mc with "
           "--cfg=model-check/replay:'%s'",
           Exploration::get_instance()->get_record_trace().to_string().c_str());
  Exploration::get_instance()->log_state();
  get_remote_app().finalize_app(true);
}

bool Exploration::empty()
{
  std::vector<std::optional<simgrid::mc::ActorState>> actors;
  get_remote_app().get_actors_status(actors);
  return std::none_of(actors.begin(), actors.end(),
                      [](std::optional<simgrid::mc::ActorState> kv) { return kv.has_value() && kv->is_enabled(); });
}

bool Exploration::soft_timouted() const
{
  if (_sg_mc_soft_timeout < 0)
    return false;

  return time(nullptr) - starting_time_ > _sg_mc_soft_timeout;
}

void Exploration::backtrack_remote_app_to_state(RemoteApp& remote_app, State* target_state, bool finalize_app)
{
  on_backtracking_signal(remote_app);
  XBT_DEBUG("Backtracking to state #%ld", target_state->get_num());

  std::deque<Transition*> replayed_transitions;
  std::deque<std::pair<aid_t, int>> recipe;
  std::deque<std::pair<aid_t, int>> recipe_needing_actor_status;
  std::deque<StatePtr> state_needing_actor_status;
  auto* state       = target_state;
  State* root_state = nullptr;
  for (; state != nullptr && not state->has_state_factory(); state = state->get_parent_state()) {
    if (state->get_transition_in() != nullptr) { // The root has no transition_in
      if (not state->has_been_initialized()) {
        recipe_needing_actor_status.push_front(
            {state->get_transition_in()->aid_, state->get_transition_in()->times_considered_});
        state_needing_actor_status.push_front(state);
      } else {
        replayed_transitions.push_front(state->get_transition_in().get());
        recipe.push_front({state->get_transition_in()->aid_, state->get_transition_in()->times_considered_});
      }
    } else
      root_state = state;
  }

  if (state == nullptr) { /* restart from the root */
    remote_app.restore_checker_side(nullptr, finalize_app);
    on_restore_state_signal(*root_state, remote_app);
  } else { /* Found an intermediate restart point */
    remote_app.restore_checker_side(state->get_state_factory(), finalize_app);
    on_restore_state_signal(*state, remote_app);
  }

  XBT_DEBUG("Sending sequence for a replay (without actor_status): %s",
            std::accumulate(recipe.begin(), recipe.end(), std::string(), [](std::string a, auto b) {
              return std::move(a) + ';' + '<' + std::to_string(b.first) + '/' + std::to_string(b.second) + '>';
            }).c_str());
  XBT_DEBUG("... (with actor_status): %s",
            std::accumulate(recipe_needing_actor_status.begin(), recipe_needing_actor_status.end(), std::string(),
                            [](std::string a, auto b) {
                              return std::move(a) + ';' + '<' + std::to_string(b.first) + '/' +
                                     std::to_string(b.second) + '>';
                            })
                .c_str());

  remote_app.replay_sequence(recipe, recipe_needing_actor_status);

  XBT_DEBUG("Need to initialize %lu states (%lu in the replay actor_status side)", state_needing_actor_status.size(),
            recipe_needing_actor_status.size());

  // The semantic of the set_one_way is:
  //  Please model checker, just listen to the app for now, we already sent the request for you

  // Indeed, the Application is sending informations to update:
  //  - the incoming transition for all the state not yet visited
  //  - the actor status of the same states

  remote_app.set_one_way(true);
  for (auto& state : state_needing_actor_status) {
    state->update_incoming_transition_with_remote_app(remote_app, state->get_transition_in()->aid_,
                                                      state->get_transition_in()->times_considered_);
    state->initialize(remote_app);
  }
  remote_app.set_one_way(false);

  // not thread-safe counts
  visited_states_count_ += recipe.size();
  backtrack_count_++;
  Transition::replayed_transitions_ += recipe.size();

  for (auto& transition : replayed_transitions)
    on_transition_replay_signal(transition, remote_app);

  // If we initialized something and it has not yet been given a possible thing to explore, go one way
  if (state_needing_actor_status.size() != 0 and target_state->next_transition() == -1)
    static_cast<SleepSetState*>(target_state)->add_arbitrary_transition(remote_app);

  return;
}

}; // namespace simgrid::mc
