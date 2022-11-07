/* Copyright (c) 2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <xbt/config.hpp>

#include "simgrid/config.h"
#include "src/kernel/EngineImpl.hpp"
#if SIMGRID_HAVE_EIGEN3
#include "src/kernel/lmm/bmf.hpp"
#endif
#include "src/kernel/resource/profile/Event.hpp"
#include "src/surf/sio_S22.hpp"

#include <unordered_set>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_host);
XBT_LOG_EXTERNAL_CATEGORY(xbt_cfg);

/***********
 * Options *
 ***********/
 static simgrid::config::Flag<std::string> cfg_sio_solver("host/sio_solver",
                                                           "Set linear equations solver used by sio model",
                                                           "maxmin",
                                                           &simgrid::kernel::lmm::System::validate_solver);

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/
void surf_host_model_init_sio_S22()
{
  XBT_CINFO(xbt_cfg, "Switching to the S22 model to handle streaming I/Os.");
  simgrid::config::set_default<bool>("network/crosstraffic", true);
  auto* system    = simgrid::kernel::lmm::System::build(cfg_sio_solver.get(), true /* selective update */);
  auto host_model = std::make_shared<simgrid::kernel::resource::HostS22Model>("Host_Sio", system);
  auto* engine    = simgrid::kernel::EngineImpl::get_instance();
  engine->add_model(host_model);
  engine->get_netzone_root()->set_host_model(host_model);
}

namespace simgrid::kernel::resource {

HostS22Model::HostS22Model(const std::string& name, lmm::System* sys) : HostModel(name)
{
  set_maxmin_system(sys);
  surf_network_model_init_LegrandVelho();
  surf_cpu_model_init_Cas01();
  surf_disk_model_init_S19();
}

double HostS22Model::next_occurring_event(double now)
{
  double min = HostModel::next_occurring_event_full(now);
  for (Action const& action : *get_started_action_set()) {
    const auto& net_action = static_cast<const S22Action&>(action);
    if (net_action.get_latency() > 0 && (min < 0 || net_action.get_latency() < min)) {
      min = net_action.get_latency();
      XBT_DEBUG("Updating min with %p (start %f): %f", &net_action, net_action.get_start_time(), min);
    }
  }
  XBT_DEBUG("min value: %f", min);

  return min;
}

void HostS22Model::update_actions_state(double /*now*/, double delta)
{
  for (auto it = std::begin(*get_started_action_set()); it != std::end(*get_started_action_set());) {
    auto& action = static_cast<S22Action&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    if (action.get_latency() > 0) {
      if (action.get_latency() > delta) {
        action.update_latency(delta, sg_surf_precision);
      } else {
        action.set_latency(0.0);
      }
      if ((action.get_latency() <= 0.0) && (action.is_suspended() == 0)) {
        action.update_bound();
        get_maxmin_system()->update_variable_penalty(action.get_variable(), 1.0);
        action.set_last_update();
      }
    }
    XBT_DEBUG("Action (%p) : remains (%g) updated by %g.", &action, action.get_remains(), action.get_rate() * delta);
    action.update_remains(action.get_rate() * delta);
    action.update_max_duration(delta);

    XBT_DEBUG("Action (%p) : remains (%g).", &action, action.get_remains());

    /* In the next if cascade, the action can be finished either because:
     *  - The amount of remaining work reached 0
     *  - The max duration was reached
     * If it's not done, it may have failed.
     */

    if (((action.get_remains() <= 0) && (action.get_variable()->get_penalty() > 0)) ||
        ((action.get_max_duration() != NO_MAX_DURATION) && (action.get_max_duration() <= 0))) {
      action.finish(Action::State::FINISHED);
      continue;
    }

    /* Need to check that none of the model has failed */
    int i                               = 0;
    const lmm::Constraint* cnst         = action.get_variable()->get_constraint(i);
    while (cnst != nullptr) {
      i++;
      if (not cnst->get_id()->is_on()) {
        XBT_DEBUG("Action (%p) Failed!!", &action);
        action.finish(Action::State::FAILED);
        break;
      }
      cnst = action.get_variable()->get_constraint(i);
    }
  }
}


DiskAction* HostS22Model::io_stream(s4u::Host* src_host, DiskImpl* src_disk, s4u::Host* dst_host, DiskImpl* dst_disk,
                                   double size)
{
  return new S22Action(this, src_host, src_disk, dst_host, dst_disk, size);
}

Action* HostS22Model::execute_thread(const s4u::Host* host, double flops_amount, int thread_count)
{
  /* Create a single action whose cost is thread_count * flops_amount and that requests thread_count cores. */
  return host->get_cpu()->execution_start(thread_count * flops_amount, thread_count, -1);
}

/**********
 * Action *
 **********/
void S22Action::update_bound() const
{
  double bound = std::numeric_limits<double>::max();
  if (src_disk_)
    bound = std::min(bound, src_disk_->get_read_bandwidth());
  if (dst_disk_)
    bound = std::min(bound, dst_disk_->get_write_bandwidth());
  if (src_host_ != dst_host_) {
    double lat       = 0.0;
    std::vector<StandardLinkImpl*> route;
    src_host_->route_to(dst_host_, route, &lat);
    if (lat > 0)
      bound = std::min(bound,NetworkModel::cfg_tcp_gamma / (2.0 * lat));
  }

  XBT_DEBUG("action (%p) : bound = %g", this, bound);

  /* latency has been paid (or no latency), we can set the appropriate bound for network limit */
  if ((bound < std::numeric_limits<double>::max()) && (latency_ <= 0.0))
    get_model()->get_maxmin_system()->update_variable_bound(get_variable(), bound);
 }

S22Action::S22Action(Model* model, s4u::Host* src_host, DiskImpl* src_disk, s4u::Host* dst_host, DiskImpl* dst_disk, double size)
    : DiskAction(model, size, false)
    , src_host_(src_host)
    , src_disk_(src_disk)
    , dst_host_(dst_host)
    , dst_disk_(dst_disk)
    , size_(size)
{
  this->set_last_update();

  size_t disk_nb       = 0;
  if (src_disk_ != nullptr)
    disk_nb++;
  if (dst_disk_ != nullptr)
    disk_nb++;

  /* there should always be a route between src_host and dst_host (loopback_ for self communication at least) */
  std::vector<StandardLinkImpl*> route;
  src_host_->route_to(dst_host_, route, &latency_);
  size_t link_nb = route.size();

  XBT_DEBUG("Creating a stream io (%p) with %zu disk(s) and %zu unique link(s).", this, disk_nb, link_nb);

  set_variable(model->get_maxmin_system()->variable_new(this, 1.0, -1.0, 3 * disk_nb + link_nb));

  if (latency_ > 0)
    model->get_maxmin_system()->update_variable_penalty(get_variable(), 0.0);

  /* Expand it for the disks even if there is nothing to read/write, to make sure that it gets expended even if there is no
   * communication either */
  if (src_disk_ != nullptr){
    model->get_maxmin_system()->expand(src_disk_->get_constraint(), get_variable(), 1);
    model->get_maxmin_system()->expand(src_disk->get_read_constraint(), get_variable(), 1);
  }
  if (dst_disk_ != nullptr){
    model->get_maxmin_system()->expand(dst_disk_->get_constraint(), get_variable(), 1);
    model->get_maxmin_system()->expand(dst_disk_->get_write_constraint(), get_variable(), 1);
  }

  for (auto const& link : route)
    model->get_maxmin_system()->expand(link->get_constraint(), get_variable(), 1);

  if (size <= 0) {
    this->set_cost(1.0);
    this->set_remains(0.0);
  }

  /* finally calculate the initial bound value */
  update_bound();
}
} // namespace simgrid::kernel::resource
