/* Copyright (c) 2014-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetPoint.hpp>

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/resource/HostImpl.hpp"
#include "src/kernel/resource/models/network_ib.hpp"
#include "src/simgrid/math_utils.h"
#include "src/simgrid/module.hpp"
#include "src/simgrid/sg_config.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_network);

/****************************************************************/
/* Model based on MPI contention model for Infiniband platforms */
/****************************************************************/
/* @Inproceedings{mescal_vienne_phd, */
/*  author={Jérôme Vienne}, */
/*  title={prédiction de performances d’applications de calcul haute performance sur réseau Infiniband}, */
/*  address={Grenoble FRANCE}, */
/*  month=june, */
/*  year={2010} */
/*  } */
SIMGRID_REGISTER_NETWORK_MODEL(
    IB,
    "Realistic network model specifically tailored for HPC settings, with Infiniband contention model as described in "
    "http://mescal.imag.fr/membres/jean-marc.vincent/index.html/PhD/Vienne.pdf",
    []() {
      using simgrid::kernel::resource::NetworkIBModel;

      auto net_model = std::make_shared<NetworkIBModel>("Network_IB");
      auto* engine   = simgrid::kernel::EngineImpl::get_instance();
      engine->add_model(net_model);
      engine->get_netzone_root()->set_network_model(net_model);

      simgrid::s4u::Link::on_communication_state_change_cb(NetworkIBModel::IB_action_state_changed_callback);
      simgrid::s4u::Comm::on_start_cb(NetworkIBModel::IB_comm_start_callback);
      simgrid::s4u::Host::on_creation_cb(NetworkIBModel::IB_create_host_callback);
      simgrid::config::set_default<double>("network/weight-S", 8775);
    });

namespace simgrid::kernel::resource {

void NetworkIBModel::IB_create_host_callback(s4u::Host const& host)
{
  static int id = 0;
  auto* ibModel = static_cast<NetworkIBModel*>(host.get_netpoint()->get_englobing_zone()->get_network_model().get());
  ibModel->active_nodes.try_emplace(host.get_name(), id);
  id++;
}

void NetworkIBModel::IB_action_state_changed_callback(NetworkAction& action, Action::State /*previous*/)
{
  if (action.get_state() != Action::State::FINISHED)
    return;
  auto* ibModel   = static_cast<NetworkIBModel*>(action.get_model());
  auto [src, dst] = ibModel->active_comms[&action];

  XBT_DEBUG("IB callback - action %p finished", &action);
  ibModel->update_IB_factors(&action, src, dst, true);
  ibModel->active_comms.erase(&action);
}

void NetworkIBModel::IB_comm_start_callback(const s4u::Comm& comm)
{
  auto* action  = static_cast<NetworkAction*>(static_cast<activity::CommImpl*>(comm.get_impl())->model_action_);
  auto* ibModel = static_cast<NetworkIBModel*>(action->get_model());
  auto* act_src = &ibModel->active_nodes.at(action->get_src().get_name());
  auto* act_dst = &ibModel->active_nodes.at(action->get_dst().get_name());

  ibModel->active_comms[action] = std::make_pair(act_src, act_dst);
  ibModel->update_IB_factors(action, act_src, act_dst, false);
}

NetworkIBModel::NetworkIBModel(const std::string& name) : NetworkCm02Model(name)
{
  std::string IB_factors_string = config::get_value<std::string>("smpi/IB-penalty-factors");
  std::vector<std::string> radical_elements;
  boost::split(radical_elements, IB_factors_string, boost::is_any_of(";"));

  xbt_assert(radical_elements.size() == 3, "smpi/IB-penalty-factors should be provided and contain 3 "
                                           "elements, semi-colon separated. Example: 0.965;0.925;1.35");

  try {
    Be_ = std::stod(radical_elements.front());
  } catch (const std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("First part of smpi/IB-penalty-factors is not numerical:") + ia.what());
  }

  try {
    Bs_ = std::stod(radical_elements.at(1));
  } catch (const std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Second part of smpi/IB-penalty-factors is not numerical:") + ia.what());
  }

  try {
    ys_ = std::stod(radical_elements.back());
  } catch (const std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Third part of smpi/IB-penalty-factors is not numerical:") + ia.what());
  }
}

void NetworkIBModel::compute_IB_factors(IBNode* root) const
{
  size_t num_comm_out    = root->active_comms_up_.size();
  double max_penalty_out = 0.0;
  // first, compute all outbound penalties to get their max
  for (ActiveComm const* comm : root->active_comms_up_) {
    double my_penalty_out = 1.0;

    if (num_comm_out != 1) {
      if (comm->destination->nb_active_comms_down_ > 2) // number of comms sent to the receiving node
        my_penalty_out = num_comm_out * Bs_ * ys_;
      else
        my_penalty_out = num_comm_out * Bs_;
    }

    max_penalty_out = std::max(max_penalty_out, my_penalty_out);
  }

  for (ActiveComm* comm : root->active_comms_up_) {
    // compute inbound penalty
    double my_penalty_in = 1.0;
    if (comm->destination->nb_active_comms_down_ != 1)                      // total number of incoming comms
      my_penalty_in = (comm->destination->active_comms_down_)[root]         // number of comm sent to dest by root node
                      * Be_ * comm->destination->active_comms_down_.size(); // number of different nodes sending to dest

    double penalty = std::max(my_penalty_in, max_penalty_out);

    double rate_before_update = comm->action->get_bound();
    // save initial rate of the action
    if (comm->init_rate == -1)
      comm->init_rate = rate_before_update;

    double penalized_bw = num_comm_out ? comm->init_rate / penalty : comm->init_rate;

    if (not double_equals(penalized_bw, rate_before_update, sg_precision_timing)) {
      XBT_DEBUG("%d->%d action %p penalty updated : bw now %f, before %f , initial rate %f", root->id_,
                comm->destination->id_, comm->action, penalized_bw, comm->action->get_bound(), comm->init_rate);
      get_maxmin_system()->update_variable_bound(comm->action->get_variable(), penalized_bw);
    } else {
      XBT_DEBUG("%d->%d action %p penalty not updated : bw %f, initial rate %f", root->id_, comm->destination->id_,
                comm->action, penalized_bw, comm->init_rate);
    }
  }
  XBT_DEBUG("Finished computing IB penalties");
}

void NetworkIBModel::update_IB_factors_rec(IBNode* root, std::vector<bool>& updatedlist) const
{
  if (not updatedlist[root->id_]) {
    XBT_DEBUG("IB - Updating rec %d", root->id_);
    compute_IB_factors(root);
    updatedlist[root->id_] = true;
    for (ActiveComm const* comm : root->active_comms_up_) {
      if (not updatedlist[comm->destination->id_])
        update_IB_factors_rec(comm->destination, updatedlist);
    }
    for (auto const& [comm, _] : root->active_comms_down_) {
      if (not updatedlist[comm->id_])
        update_IB_factors_rec(comm, updatedlist);
    }
  }
}

void NetworkIBModel::update_IB_factors(NetworkAction* action, IBNode* from, IBNode* to, bool remove) const
{
  if (from == to) // disregard local comms (should use loopback)
    return;

  if (remove) {
    if (to->active_comms_down_[from] == 1)
      to->active_comms_down_.erase(from);
    else
      to->active_comms_down_[from] -= 1;

    to->nb_active_comms_down_--;
    if (auto it = std::find_if(begin(from->active_comms_up_), end(from->active_comms_up_),
                               [action](const ActiveComm* comm) { return comm->action == action; });
        it != std::end(from->active_comms_up_)) {
      delete *it;
      from->active_comms_up_.erase(it);
    }
    action->unref();
  } else {
    action->ref();
    auto* comm        = new ActiveComm();
    comm->action      = action;
    comm->destination = to;
    from->active_comms_up_.push_back(comm);

    to->active_comms_down_[from] += 1;
    to->nb_active_comms_down_++;
  }
  XBT_DEBUG("IB - Updating %d", from->id_);
  std::vector<bool> updated(active_nodes.size(), false);
  update_IB_factors_rec(from, updated);
  XBT_DEBUG("IB - Finished updating %d", from->id_);
}
} // namespace simgrid::kernel::resource
