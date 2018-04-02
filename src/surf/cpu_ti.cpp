/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_ti.hpp"
#include "src/surf/trace_mgr.hpp"
#include "xbt/utility.hpp"
#include <algorithm>

#ifndef SURF_MODEL_CPUTI_H_
#define SURF_MODEL_CPUTI_H_

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_ti, surf_cpu, "Logging specific to the SURF CPU TRACE INTEGRATION module");

namespace simgrid {
namespace surf {

/*********
 * Trace *
 *********/

CpuTiTrace::CpuTiTrace(tmgr_trace_t speedTrace)
{
  double integral = 0;
  double time = 0;
  int i = 0;
  nb_points_      = speedTrace->event_list.size() + 1;
  time_points_    = new double[nb_points_];
  integral_       = new double[nb_points_];
  for (auto const& val : speedTrace->event_list) {
    time_points_[i] = time;
    integral_[i] = integral;
    integral += val.date_ * val.value_;
    time += val.date_;
    i++;
  }
  time_points_[i] = time;
  integral_[i] = integral;
}

CpuTiTrace::~CpuTiTrace()
{
  delete[] time_points_;
  delete [] integral_;
}

CpuTiTgmr::~CpuTiTgmr()
{
  if (trace_)
    delete trace_;
}

/**
* \brief Integrate trace
*
* Wrapper around surf_cpu_integrate_trace_simple() to get
* the cyclic effect.
*
* \param a      Begin of interval
* \param b      End of interval
* \return the integrate value. -1 if an error occurs.
*/
double CpuTiTgmr::integrate(double a, double b)
{
  int a_index;

  if ((a < 0.0) || (a > b)) {
    xbt_die("Error, invalid integration interval [%.2f,%.2f]. "
        "You probably have a task executing with negative computation amount. Check your code.", a, b);
  }
  if (fabs(a -b) < EPSILON)
    return 0.0;

  if (type_ == TRACE_FIXED) {
    return ((b - a) * value_);
  }

  if (fabs(ceil(a / last_time_) - a / last_time_) < EPSILON)
    a_index = 1 + static_cast<int>(ceil(a / last_time_));
  else
    a_index = static_cast<int>(ceil(a / last_time_));

  int b_index = static_cast<int>(floor(b / last_time_));

  if (a_index > b_index) {      /* Same chunk */
    return trace_->integrate_simple(a - (a_index - 1) * last_time_, b - (b_index)*last_time_);
  }

  double first_chunk  = trace_->integrate_simple(a - (a_index - 1) * last_time_, last_time_);
  double middle_chunk = (b_index - a_index) * total_;
  double last_chunk   = trace_->integrate_simple(0.0, b - (b_index)*last_time_);

  XBT_DEBUG("first_chunk=%.2f  middle_chunk=%.2f  last_chunk=%.2f\n", first_chunk, middle_chunk, last_chunk);

  return (first_chunk + middle_chunk + last_chunk);
}

/**
 * \brief Auxiliary function to compute the integral between a and b.
 *     It simply computes the integrals at point a and b and returns the difference between them.
 * \param a  Initial point
 * \param b  Final point
*/
double CpuTiTrace::integrate_simple(double a, double b)
{
  return integrate_simple_point(b) - integrate_simple_point(a);
}

/**
 * \brief Auxiliary function to compute the integral at point a.
 * \param a        point
 */
double CpuTiTrace::integrate_simple_point(double a)
{
  double integral = 0;
  double a_aux = a;
  int ind         = binary_search(time_points_, a, 0, nb_points_ - 1);
  integral += integral_[ind];

  XBT_DEBUG("a %f ind %d integral %f ind + 1 %f ind %f time +1 %f time %f", a, ind, integral, integral_[ind + 1],
            integral_[ind], time_points_[ind + 1], time_points_[ind]);
  double_update(&a_aux, time_points_[ind], sg_maxmin_precision * sg_surf_precision);
  if (a_aux > 0)
    integral +=
        ((integral_[ind + 1] - integral_[ind]) / (time_points_[ind + 1] - time_points_[ind])) * (a - time_points_[ind]);
  XBT_DEBUG("Integral a %f = %f", a, integral);

  return integral;
}

/**
* \brief Computes the time needed to execute "amount" on cpu.
*
* Here, amount can span multiple trace periods
*
* \param a        Initial time
* \param amount  Amount to be executed
* \return  End time
*/
double CpuTiTgmr::solve(double a, double amount)
{
  /* Fix very small negative numbers */
  if ((a < 0.0) && (a > -EPSILON)) {
    a = 0.0;
  }
  if ((amount < 0.0) && (amount > -EPSILON)) {
    amount = 0.0;
  }

  /* Sanity checks */
  if ((a < 0.0) || (amount < 0.0)) {
    XBT_CRITICAL ("Error, invalid parameters [a = %.2f, amount = %.2f]. "
        "You probably have a task executing with negative computation amount. Check your code.", a, amount);
    xbt_abort();
  }

  /* At this point, a and amount are positive */
  if (amount < EPSILON)
    return a;

  /* Is the trace fixed ? */
  if (type_ == TRACE_FIXED) {
    return (a + (amount / value_));
  }

  XBT_DEBUG("amount %f total %f", amount, total_);
  /* Reduce the problem to one where amount <= trace_total */
  int quotient = static_cast<int>(floor(amount / total_));
  double reduced_amount = (total_) * ((amount / total_) - floor(amount / total_));
  double reduced_a      = a - (last_time_) * static_cast<int>(floor(a / last_time_));

  XBT_DEBUG("Quotient: %d reduced_amount: %f reduced_a: %f", quotient, reduced_amount, reduced_a);

  /* Now solve for new_amount which is <= trace_total */
  double reduced_b;
  XBT_DEBUG("Solve integral: [%.2f, amount=%.2f]", reduced_a, reduced_amount);
  double amount_till_end = integrate(reduced_a, last_time_);

  if (amount_till_end > reduced_amount) {
    reduced_b = trace_->solve_simple(reduced_a, reduced_amount);
  } else {
    reduced_b = last_time_ + trace_->solve_simple(0.0, reduced_amount - amount_till_end);
  }

  /* Re-map to the original b and amount */
  return (last_time_) * static_cast<int>(floor(a / last_time_)) + (quotient * last_time_) + reduced_b;
}

/**
 * \brief Auxiliary function to solve integral.
 *  It returns the date when the requested amount of flops is available
 * \param a        Initial point
 * \param amount  Amount of flops
 * \return The date when amount is available.
*/
double CpuTiTrace::solve_simple(double a, double amount)
{
  double integral_a = integrate_simple_point(a);
  int ind           = binary_search(integral_, integral_a + amount, 0, nb_points_ - 1);
  double time       = time_points_[ind];
  time += (integral_a + amount - integral_[ind]) /
          ((integral_[ind + 1] - integral_[ind]) / (time_points_[ind + 1] - time_points_[ind]));

  return time;
}

/**
* \brief Auxiliary function to update the CPU speed scale.
*
*  This function uses the trace structure to return the speed scale at the determined time a.
* \param a        Time
* \return CPU speed scale
*/
double CpuTiTgmr::get_power_scale(double a)
{
  double reduced_a          = a - floor(a / last_time_) * last_time_;
  int point                 = trace_->binary_search(trace_->time_points_, reduced_a, 0, trace_->nb_points_ - 1);
  trace_mgr::DatedValue val = speed_trace_->event_list.at(point);
  return val.value_;
}

/**
* \brief Creates a new integration trace from a tmgr_trace_t
*
* \param  speedTrace    CPU availability trace
* \param  value          Percentage of CPU speed available (useful to fixed tracing)
* \return  Integration trace structure
*/
CpuTiTgmr::CpuTiTgmr(tmgr_trace_t speedTrace, double value) : speed_trace_(speedTrace)
{
  double total_time = 0.0;
  trace_ = 0;

/* no availability file, fixed trace */
  if (not speedTrace) {
    type_ = TRACE_FIXED;
    value_ = value;
    XBT_DEBUG("No availability trace. Constant value = %f", value);
    return;
  }

  /* only one point available, fixed trace */
  if (speedTrace->event_list.size() == 1) {
    trace_mgr::DatedValue val = speedTrace->event_list.front();
    type_ = TRACE_FIXED;
    value_                    = val.value_;
    return;
  }

  type_ = TRACE_DYNAMIC;

  /* count the total time of trace file */
  for (auto const& val : speedTrace->event_list)
    total_time += val.date_;

  trace_ = new CpuTiTrace(speedTrace);
  last_time_ = total_time;
  total_    = trace_->integrate_simple(0, total_time);

  XBT_DEBUG("Total integral %f, last_time %f ", total_, last_time_);
}

/**
 * \brief Binary search in array.
 *  It returns the first point of the interval in which "a" is.
 * \param array    Array
 * \param a        Value to search
 * \param low     Low bound to search in array
 * \param high    Upper bound to search in array
 * \return Index of point
*/
int CpuTiTrace::binary_search(double* array, double a, int low, int high)
{
  xbt_assert(low < high, "Wrong parameters: low (%d) should be smaller than high (%d)", low, high);

  do {
    int mid = low + (high - low) / 2;
    XBT_DEBUG("a %f low %d high %d mid %d value %f", a, low, high, mid, array[mid]);

    if (array[mid] > a)
      high = mid;
    else
      low = mid;
  }
  while (low < high - 1);

  return low;
}

}
}

/*********
 * Model *
 *********/

void surf_cpu_model_init_ti()
{
  xbt_assert(not surf_cpu_model_pm, "CPU model already initialized. This should not happen.");
  xbt_assert(not surf_cpu_model_vm, "CPU model already initialized. This should not happen.");

  surf_cpu_model_pm = new simgrid::surf::CpuTiModel();
  all_existing_models->push_back(surf_cpu_model_pm);

  surf_cpu_model_vm = new simgrid::surf::CpuTiModel();
  all_existing_models->push_back(surf_cpu_model_vm);
}

namespace simgrid {
namespace surf {

CpuTiModel::~CpuTiModel()
{
  surf_cpu_model_pm = nullptr;
}

Cpu *CpuTiModel::createCpu(simgrid::s4u::Host *host, std::vector<double>* speedPerPstate, int core)
{
  return new CpuTi(this, host, speedPerPstate, core);
}

double CpuTiModel::next_occuring_event(double now)
{
  double min_action_duration = -1;

  /* iterates over modified cpus to update share resources */
  for (auto it = std::begin(modified_cpus_); it != std::end(modified_cpus_);) {
    CpuTi& ti = *it;
    ++it; // increment iterator here since the following call to ti.updateActionsFinishTime() may invalidate it
    ti.update_actions_finish_time(now);
  }

  /* get the min next event if heap not empty */
  if (not get_action_heap().empty())
    min_action_duration = get_action_heap().top_date() - now;

  XBT_DEBUG("Share resources, min next event date: %f", min_action_duration);

  return min_action_duration;
}

void CpuTiModel::update_actions_state(double now, double /*delta*/)
{
  while (not get_action_heap().empty() && double_equals(get_action_heap().top_date(), now, sg_surf_precision)) {
    CpuTiAction* action = static_cast<CpuTiAction*>(get_action_heap().pop());
    XBT_DEBUG("Action %p: finish", action);
    action->finish(kernel::resource::Action::State::done);
    /* update remaining amount of all actions */
    action->cpu_->update_remaining_amount(surf_get_clock());
  }
}

/************
 * Resource *
 ************/
CpuTi::CpuTi(CpuTiModel *model, simgrid::s4u::Host *host, std::vector<double> *speedPerPstate, int core)
  : Cpu(model, host, speedPerPstate, core)
{
  xbt_assert(core == 1, "Multi-core not handled by this model yet");

  speed_.peak = speedPerPstate->front();
  XBT_DEBUG("CPU create: peak=%f", speed_.peak);

  speed_integrated_trace_ = new CpuTiTgmr(nullptr, 1 /*scale*/);
}

CpuTi::~CpuTi()
{
  set_modified(false);
  delete speed_integrated_trace_;
}
void CpuTi::set_speed_trace(tmgr_trace_t trace)
{
  if (speed_integrated_trace_)
    delete speed_integrated_trace_;

  speed_integrated_trace_ = new CpuTiTgmr(trace, speed_.scale);

  /* add a fake trace event if periodicity == 0 */
  if (trace && trace->event_list.size() > 1) {
    trace_mgr::DatedValue val = trace->event_list.back();
    if (val.date_ < 1e-12)
      speed_.event = future_evt_set->add_trace(new simgrid::trace_mgr::trace(), this);
  }
}

void CpuTi::apply_event(tmgr_trace_event_t event, double value)
{
  if (event == speed_.event) {
    tmgr_trace_t speedTrace;
    CpuTiTgmr *trace;

    XBT_DEBUG("Finish trace date: value %f", value);
    /* update remaining of actions and put in modified cpu list */
    update_remaining_amount(surf_get_clock());

    set_modified(true);

    speedTrace                = speed_integrated_trace_->speed_trace_;
    trace_mgr::DatedValue val = speedTrace->event_list.back();
    delete speed_integrated_trace_;
    speed_.scale = val.value_;

    trace = new CpuTiTgmr(TRACE_FIXED, val.value_);
    XBT_DEBUG("value %f", val.value_);

    speed_integrated_trace_ = trace;

    tmgr_trace_event_unref(&speed_.event);

  } else if (event == stateEvent_) {
    if (value > 0) {
      if (is_off())
        host_that_restart.push_back(getHost());
      turn_on();
    } else {
      turn_off();
      double date = surf_get_clock();

      /* put all action running on cpu to failed */
      for (CpuTiAction& action : action_set_) {
        if (action.get_state() == kernel::resource::Action::State::running ||
            action.get_state() == kernel::resource::Action::State::ready ||
            action.get_state() == kernel::resource::Action::State::not_in_the_system) {
          action.set_finish_time(date);
          action.set_state(kernel::resource::Action::State::failed);
          get_model()->get_action_heap().remove(&action);
        }
      }
    }
    tmgr_trace_event_unref(&stateEvent_);

  } else {
    xbt_die("Unknown event!\n");
  }
}

/** Update the actions that are running on this CPU (which was modified recently) */
void CpuTi::update_actions_finish_time(double now)
{
  /* update remaining amount of actions */
  update_remaining_amount(now);

  /* Compute the sum of priorities for the actions running on that CPU */
  sum_priority_ = 0.0;
  for (CpuTiAction const& action : action_set_) {
    /* action not running, skip it */
    if (action.get_state_set() != surf_cpu_model_pm->get_running_action_set())
      continue;

    /* bogus priority, skip it */
    if (action.get_priority() <= 0)
      continue;

    /* action suspended, skip it */
    if (action.suspended_ != kernel::resource::Action::SuspendStates::not_suspended)
      continue;

    sum_priority_ += 1.0 / action.get_priority();
  }

  for (CpuTiAction& action : action_set_) {
    double min_finish = -1;
    /* action not running, skip it */
    if (action.get_state_set() != surf_cpu_model_pm->get_running_action_set())
      continue;

    /* verify if the action is really running on cpu */
    if (action.suspended_ == kernel::resource::Action::SuspendStates::not_suspended && action.get_priority() > 0) {
      /* total area needed to finish the action. Used in trace integration */
      double total_area = (action.get_remains() * sum_priority_ * action.get_priority()) / speed_.peak;

      action.set_finish_time(speed_integrated_trace_->solve(now, total_area));
      /* verify which event will happen before (max_duration or finish time) */
      if (action.get_max_duration() > NO_MAX_DURATION &&
          action.get_start_time() + action.get_max_duration() < action.get_finish_time())
        min_finish = action.get_start_time() + action.get_max_duration();
      else
        min_finish = action.get_finish_time();
    } else {
      /* put the max duration time on heap */
      if (action.get_max_duration() > NO_MAX_DURATION)
        min_finish = action.get_start_time() + action.get_max_duration();
    }
    /* add in action heap */
    if (min_finish > NO_MAX_DURATION)
      get_model()->get_action_heap().update(&action, min_finish, kernel::resource::ActionHeap::Type::unset);
    else
      get_model()->get_action_heap().remove(&action);

    XBT_DEBUG("Update finish time: Cpu(%s) Action: %p, Start Time: %f Finish Time: %f Max duration %f", get_cname(),
              &action, action.get_start_time(), action.get_finish_time(), action.get_max_duration());
  }
  /* remove from modified cpu */
  set_modified(false);
}

bool CpuTi::is_used()
{
  return not action_set_.empty();
}

double CpuTi::get_available_speed()
{
  speed_.scale = speed_integrated_trace_->get_power_scale(surf_get_clock());
  return Cpu::get_available_speed();
}

/** @brief Update the remaining amount of actions */
void CpuTi::update_remaining_amount(double now)
{

  /* already updated */
  if (last_update_ >= now)
    return;

  /* compute the integration area */
  double area_total = speed_integrated_trace_->integrate(last_update_, now) * speed_.peak;
  XBT_DEBUG("Flops total: %f, Last update %f", area_total, last_update_);
  for (CpuTiAction& action : action_set_) {
    /* action not running, skip it */
    if (action.get_state_set() != get_model()->get_running_action_set())
      continue;

    /* bogus priority, skip it */
    if (action.get_priority() <= 0)
      continue;

    /* action suspended, skip it */
    if (action.suspended_ != kernel::resource::Action::SuspendStates::not_suspended)
      continue;

    /* action don't need update */
    if (action.get_start_time() >= now)
      continue;

    /* skip action that are finishing now */
    if (action.get_finish_time() >= 0 && action.get_finish_time() <= now)
      continue;

    /* update remaining */
    action.update_remains(area_total / (sum_priority_ * action.get_priority()));
    XBT_DEBUG("Update remaining action(%p) remaining %f", &action, action.get_remains_no_update());
  }
  last_update_ = now;
}

CpuAction *CpuTi::execution_start(double size)
{
  XBT_IN("(%s,%g)", get_cname(), size);
  CpuTiAction* action = new CpuTiAction(static_cast<CpuTiModel*>(get_model()), size, is_off(), this);

  action_set_.push_back(*action);

  XBT_OUT();
  return action;
}


CpuAction *CpuTi::sleep(double duration)
{
  if (duration > 0)
    duration = std::max(duration, sg_surf_precision);

  XBT_IN("(%s,%g)", get_cname(), duration);
  CpuTiAction* action = new CpuTiAction(static_cast<CpuTiModel*>(get_model()), 1.0, is_off(), this);

  action->set_max_duration(duration);
  action->suspended_ = kernel::resource::Action::SuspendStates::sleeping;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention is used to speed up update_resource_state */
    simgrid::xbt::intrusive_erase(*action->get_state_set(), *action);
    action->state_set_ = &static_cast<CpuTiModel*>(get_model())->runningActionSetThatDoesNotNeedBeingChecked_;
    action->get_state_set()->push_back(*action);
  }

  action_set_.push_back(*action);

  XBT_OUT();
  return action;
}

void CpuTi::set_modified(bool modified)
{
  CpuTiList& modifiedCpu = static_cast<CpuTiModel*>(get_model())->modified_cpus_;
  if (modified) {
    if (not cpu_ti_hook.is_linked()) {
      modifiedCpu.push_back(*this);
    }
  } else {
    if (cpu_ti_hook.is_linked())
      simgrid::xbt::intrusive_erase(modifiedCpu, *this);
  }
}

/**********
 * Action *
 **********/

CpuTiAction::CpuTiAction(CpuTiModel *model_, double cost, bool failed, CpuTi *cpu)
 : CpuAction(model_, cost, failed)
 , cpu_(cpu)
{
  cpu_->set_modified(true);
}
CpuTiAction::~CpuTiAction()
{
  /* remove from action_set */
  if (action_ti_hook.is_linked())
    simgrid::xbt::intrusive_erase(cpu_->action_set_, *this);
  /* remove from heap */
  get_model()->get_action_heap().remove(this);
  cpu_->set_modified(true);
}

void CpuTiAction::set_state(Action::State state)
{
  CpuAction::set_state(state);
  cpu_->set_modified(true);
}

void CpuTiAction::cancel()
{
  this->set_state(Action::State::failed);
  get_model()->get_action_heap().remove(this);
  cpu_->set_modified(true);
}

void CpuTiAction::suspend()
{
  XBT_IN("(%p)", this);
  if (suspended_ != Action::SuspendStates::sleeping) {
    suspended_ = Action::SuspendStates::suspended;
    get_model()->get_action_heap().remove(this);
    cpu_->set_modified(true);
  }
  XBT_OUT();
}

void CpuTiAction::resume()
{
  XBT_IN("(%p)", this);
  if (suspended_ != Action::SuspendStates::sleeping) {
    suspended_ = Action::SuspendStates::not_suspended;
    cpu_->set_modified(true);
  }
  XBT_OUT();
}

void CpuTiAction::set_max_duration(double duration)
{
  double min_finish;

  XBT_IN("(%p,%g)", this, duration);

  Action::set_max_duration(duration);

  if (duration >= 0)
    min_finish = (get_start_time() + get_max_duration()) < get_finish_time() ? (get_start_time() + get_max_duration())
                                                                             : get_finish_time();
  else
    min_finish = get_finish_time();

  /* add in action heap */
  get_model()->get_action_heap().update(this, min_finish, kernel::resource::ActionHeap::Type::unset);

  XBT_OUT();
}

void CpuTiAction::set_priority(double priority)
{
  XBT_IN("(%p,%g)", this, priority);
  set_priority_no_update(priority);
  cpu_->set_modified(true);
  XBT_OUT();
}

double CpuTiAction::get_remains()
{
  XBT_IN("(%p)", this);
  cpu_->update_remaining_amount(surf_get_clock());
  XBT_OUT();
  return get_remains_no_update();
}

}
}

#endif /* SURF_MODEL_CPUTI_H_ */
