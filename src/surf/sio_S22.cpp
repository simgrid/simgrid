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
                                                           "fairbottleneck",
                                                           &simgrid::kernel::lmm::System::validate_solver);

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/
void surf_host_model_init_sio_S22()
{
  XBT_CINFO(xbt_cfg, "Switching to the S22 model to handle streaming I/Os.");
  xbt_assert(cfg_sio_solver != "maxmin", "Invalid configuration. Cannot use maxmin solver with streaming I/Os.");

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

  auto net_model = std::make_shared<NetworkS22Model>("Network_Sio", this, sys);
  auto engine    = EngineImpl::get_instance();
  engine->add_model(net_model);
  engine->get_netzone_root()->set_network_model(net_model);

  auto disk_model = std::make_shared<DiskS22Model>("Disk_Sio", this, sys);
  engine->add_model(disk_model);
  engine->get_netzone_root()->set_disk_model(disk_model);

  surf_cpu_model_init_Cas01();
}

DiskS22Model::DiskS22Model(const std::string& name, HostS22Model* hmodel, lmm::System* sys)
    : DiskModel(name), hostModel_(hmodel)
{
  set_maxmin_system(sys);
}

DiskS22Model::~DiskS22Model()
{
  set_maxmin_system(nullptr);
}

NetworkS22Model::NetworkS22Model(const std::string& name, HostS22Model* hmodel, lmm::System* sys)
    : NetworkModel(name), hostModel_(hmodel)
{
  set_maxmin_system(sys);
  loopback_.reset(create_link("__loopback__", {simgrid::config::get_value<double>("network/loopback-bw")}));
  loopback_->set_sharing_policy(s4u::Link::SharingPolicy::FATPIPE, {});
  loopback_->set_latency(simgrid::config::get_value<double>("network/loopback-lat"));
  loopback_->get_iface()->seal();
}

NetworkS22Model::~NetworkS22Model()
{
  set_maxmin_system(nullptr);
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

S22Action::S22Action(Model* model, s4u::Host* src_host, s4u::Disk* src_disk, s4u::Host* dst_host, s4u::Disk* dst_disk, double size)
    : DiskAction(model, 1.0, false)
    , src_host_(src_host)
    , src_disk_(src_disk)
    , dst_host_(dst_host)
    , dst_disk_(dst_disk)
    , size_(size)
{
  size_t disk_nb       = 0;
  size_t link_nb       = 0;
  double latency       = 0.0;
  this->set_last_update();

  if (src_disk_ != nullptr)
    disk_nb++;
  if (dst_disk_ != nullptr)
    disk_nb++;

  if (src_host_ != dst_host_ && size_ > 0) {
    std::unordered_set<const char*> affected_links;
    double lat = 0.0;
    std::vector<StandardLinkImpl*> route;
    src_host_->route_to(dst_host_, route, &lat);
    latency = std::max(latency, lat);

    for (auto const& link : route)
      affected_links.insert(link->get_cname());

    link_nb = affected_links.size();
  }

  XBT_DEBUG("Creating a stream io (%p) with %zu hosts and %zu unique links.", this, disk_nb, link_nb);
  latency_ = latency;

  set_variable(model->get_maxmin_system()->variable_new(this, 1.0, -1.0, disk_nb + link_nb));

  if (latency_ > 0)
    model->get_maxmin_system()->update_variable_penalty(get_variable(), 0.0);

  /* Expand it for the disks even if there is nothing to read/write, to make sure that it gets expended even if there is no
   * communication either */
  if (src_disk_ != nullptr)
    model->get_maxmin_system()->expand(src_disk_->get_impl()->get_constraint(), get_variable(), size, true);
  if (dst_disk_ != nullptr)
    model->get_maxmin_system()->expand(dst_disk_->get_impl()->get_constraint(), get_variable(), size, true);

  if (src_host_ != dst_host_) {
    std::vector<StandardLinkImpl*> route;
    src_host_->route_to(dst_host_, route, nullptr);
    for (auto const& link : route)
      model->get_maxmin_system()->expand(link->get_constraint(), this->get_variable(), size_);
  }

  if (link_nb + disk_nb == 0) {
    this->set_cost(1.0);
    this->set_remains(0.0);
  }

  /* finally calculate the initial bound value */
  update_bound();
}

S22Action* HostS22Model::io_stream(s4u::Host* src_host, s4u::Disk* src_disk, s4u::Host* dst_host, s4u::Disk* dst_disk,
                                   double size)
{
  return new S22Action(this, src_host, src_disk, dst_host, dst_disk, size);
}

Action* NetworkS22Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double /*rate*/)
{
  return hostModel_->io_stream(src, nullptr, dst, nullptr, size);
}

DiskImpl* DiskS22Model::create_disk(const std::string& name, double read_bandwidth, double write_bandwidth)
{
  return (new DiskS22(name, read_bandwidth, write_bandwidth))->set_model(this);
}

StandardLinkImpl* NetworkS22Model::create_link(const std::string& name, const std::vector<double>& bandwidths)
{
  xbt_assert(bandwidths.size() == 1, "Non WIFI link must have only 1 bandwidth.");
  auto link = new LinkS22(name, bandwidths[0], get_maxmin_system());
  link->set_model(this);
  return link;
}

StandardLinkImpl* NetworkS22Model::create_wifi_link(const std::string& name, const std::vector<double>& bandwidths)
{
  THROW_UNIMPLEMENTED;
}

/************
 * Resource *
 ************/
DiskAction* DiskS22::io_start(sg_size_t size, s4u::Io::OpType type)
{
  return static_cast<S22Action*>(io_start(size, type));
}

void DiskS22::apply_event(kernel::profile::Event* triggered, double value)
{
  /* Find out which of my iterators was triggered, and react accordingly */
  if (triggered == get_read_event()) {
    set_read_bandwidth(value);
    unref_read_event();
  } else if (triggered == get_write_event()) {
    set_write_bandwidth(value);
    unref_write_event();
  } else if (triggered == get_state_event()) {
    if (value > 0)
      turn_on();
    else
      turn_off();
    unref_state_event();
  } else {
    xbt_die("Unknown event!\n");
  }
}

LinkS22::LinkS22(const std::string& name, double bandwidth, lmm::System* system) : StandardLinkImpl(name)
{
  this->set_constraint(system->constraint_new(this, bandwidth));
  bandwidth_.peak = bandwidth;
}

void LinkS22::apply_event(profile::Event* triggered, double value)
{
  XBT_DEBUG("Updating link %s (%p) with value=%f", get_cname(), this, value);
  if (triggered == bandwidth_.event) {
    set_bandwidth(value);
    tmgr_trace_event_unref(&bandwidth_.event);

  } else if (triggered == latency_.event) {
    set_latency(value);
    tmgr_trace_event_unref(&latency_.event);

  } else if (triggered == get_state_event()) {
    if (value > 0)
      turn_on();
    else
      turn_off();
    unref_state_event();
  } else {
    xbt_die("Unknown event ! \n");
  }
}

void LinkS22::set_bandwidth(double value)
{
  bandwidth_.peak = value;
  StandardLinkImpl::on_bandwidth_change();

  get_model()->get_maxmin_system()->update_constraint_bound(get_constraint(), bandwidth_.peak * bandwidth_.scale);
}

void LinkS22::set_latency(double value)
{
  latency_check(value);
  const lmm::Element* elem = nullptr;

  latency_.peak = value;
  while (const auto* var = get_constraint()->get_variable(&elem)) {
    const auto* action = static_cast<S22Action*>(var->get_id());
    action->update_bound();
  }
}

/**********
 * Action *
 **********/
double S22Action::calculate_io_read_bound() const
{
  double io_read_bound = std::numeric_limits<double>::max();

  if (src_disk_ == nullptr)
    return io_read_bound;

  if (size_ > 0)
    io_read_bound = std::min(io_read_bound, src_disk_->get_read_bandwidth() / size_);

  return io_read_bound;
}

double S22Action::calculate_io_write_bound() const
{
  double io_write_bound = std::numeric_limits<double>::max();

  if (dst_disk_ == nullptr)
    return io_write_bound;

  if (size_ > 0)
    io_write_bound = std::min(io_write_bound, dst_disk_->get_write_bandwidth() / size_);

  return io_write_bound;
}

double S22Action::calculate_network_bound() const
{
  double lat = 0.0;
  double lat_bound   = std::numeric_limits<double>::max();

  if (src_host_ == dst_host_)
    return lat_bound;

  if (size_ <= 0){
    std::vector<StandardLinkImpl*> route;
    src_host_->route_to(dst_host_, route, &lat);
  }

  if (lat > 0)
    lat_bound = NetworkModel::cfg_tcp_gamma / (2.0 * lat);

  return lat_bound;
}

void S22Action::update_bound() const
{
  double bound = std::min(std::min(calculate_io_read_bound(), calculate_io_write_bound()),
                          calculate_network_bound());

  XBT_DEBUG("action (%p) : bound = %g", this, bound);

  /* latency has been paid (or no latency), we can set the appropriate bound for network limit */
  if ((bound < std::numeric_limits<double>::max()) && (latency_ <= 0.0)) {
    if (rate_ < 0)
      get_model()->get_maxmin_system()->update_variable_bound(get_variable(), bound);
    else
      get_model()->get_maxmin_system()->update_variable_bound(get_variable(), std::min(rate_, bound));
  }
}

} // namespace simgrid::kernel::resource
