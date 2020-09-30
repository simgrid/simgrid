/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/network_ib.hpp"
#include "simgrid/sg_config.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/xml/platf.hpp"
#include "surf/surf.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

static void IB_create_host_callback(simgrid::s4u::Host const& host)
{
  using simgrid::kernel::resource::IBNode;
  using simgrid::kernel::resource::NetworkIBModel;

  static int id=0;

  ((NetworkIBModel*)surf_network_model)->active_nodes.emplace(host.get_name(), IBNode(id));
  id++;
}

static void IB_action_state_changed_callback(simgrid::kernel::resource::NetworkAction& action,
                                             simgrid::kernel::resource::Action::State /*previous*/)
{
  using simgrid::kernel::resource::IBNode;
  using simgrid::kernel::resource::NetworkIBModel;

  if (action.get_state() != simgrid::kernel::resource::Action::State::FINISHED)
    return;
  std::pair<IBNode*, IBNode*> pair = ((NetworkIBModel*)surf_network_model)->active_comms[&action];
  XBT_DEBUG("IB callback - action %p finished", &action);

  ((NetworkIBModel*)surf_network_model)->updateIBfactors(&action, pair.first, pair.second, 1);

  ((NetworkIBModel*)surf_network_model)->active_comms.erase(&action);
}

static void IB_action_init_callback(simgrid::kernel::resource::NetworkAction& action)
{
  simgrid::kernel::resource::NetworkIBModel* ibModel = (simgrid::kernel::resource::NetworkIBModel*)surf_network_model;
  simgrid::kernel::resource::IBNode* act_src         = &ibModel->active_nodes.at(action.get_src().get_name());
  simgrid::kernel::resource::IBNode* act_dst         = &ibModel->active_nodes.at(action.get_dst().get_name());

  ibModel->active_comms[&action] = std::make_pair(act_src, act_dst);
  ibModel->updateIBfactors(&action, act_src, act_dst, 0);
}

/*********
 * Model *
 *********/

/************************************************************************/
/* New model based on MPI contention model for Infiniband platforms */
/************************************************************************/
/* @Inproceedings{mescal_vienne_phd, */
/*  author={Jérôme Vienne}, */
/*  title={prédiction de performances d’applications de calcul haute performance sur réseau Infiniband}, */
/*  address={Grenoble FRANCE}, */
/*  month=june, */
/*  year={2010} */
/*  } */
void surf_network_model_init_IB()
{
  xbt_assert(surf_network_model == nullptr, "Cannot set the network model twice");

  surf_network_model = new simgrid::kernel::resource::NetworkIBModel();
  simgrid::s4u::Link::on_communication_state_change.connect(IB_action_state_changed_callback);
  simgrid::s4u::Link::on_communicate.connect(IB_action_init_callback);
  simgrid::s4u::Host::on_creation.connect(IB_create_host_callback);
  simgrid::config::set_default<double>("network/weight-S", 8775);
}

namespace simgrid {
namespace kernel {
namespace resource {

NetworkIBModel::NetworkIBModel() : NetworkSmpiModel()
{
  /* Do not add this into all_existing_models: our ancestor already does so */

  std::string IB_factors_string = config::get_value<std::string>("smpi/IB-penalty-factors");
  std::vector<std::string> radical_elements;
  boost::split(radical_elements, IB_factors_string, boost::is_any_of(";"));

  surf_parse_assert(radical_elements.size() == 3, "smpi/IB-penalty-factors should be provided and contain 3 "
                                                  "elements, semi-colon separated. Example: 0.965;0.925;1.35");

  try {
    Be = std::stod(radical_elements.front());
  } catch (const std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("First part of smpi/IB-penalty-factors is not numerical:") + ia.what());
  }

  try {
    Bs = std::stod(radical_elements.at(1));
  } catch (const std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Second part of smpi/IB-penalty-factors is not numerical:") + ia.what());
  }

  try {
    ys = std::stod(radical_elements.back());
  } catch (const std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Third part of smpi/IB-penalty-factors is not numerical:") + ia.what());
  }
}

void NetworkIBModel::computeIBfactors(IBNode* root) const
{
  double num_comm_out    = root->ActiveCommsUp.size();
  double max_penalty_out = 0.0;
  // first, compute all outbound penalties to get their max
  for (ActiveComm const* comm : root->ActiveCommsUp) {
    double my_penalty_out = 1.0;

    if (num_comm_out != 1) {
      if (comm->destination->nbActiveCommsDown > 2) // number of comms sent to the receiving node
        my_penalty_out = num_comm_out * Bs * ys;
      else
        my_penalty_out = num_comm_out * Bs;
    }

    max_penalty_out = std::max(max_penalty_out, my_penalty_out);
  }

  for (ActiveComm* comm : root->ActiveCommsUp) {
    // compute inbound penalty
    double my_penalty_in = 1.0;
    int nb_comms         = comm->destination->nbActiveCommsDown; // total number of incoming comms
    if (nb_comms != 1)
      my_penalty_in = (comm->destination->ActiveCommsDown)[root]        // number of comm sent to dest by root node
                      * Be * comm->destination->ActiveCommsDown.size(); // number of different nodes sending to dest

    double penalty = std::max(my_penalty_in, max_penalty_out);

    double rate_before_update = comm->action->get_bound();
    // save initial rate of the action
    if (comm->init_rate == -1)
      comm->init_rate = rate_before_update;

    double penalized_bw = num_comm_out ? comm->init_rate / penalty : comm->init_rate;

    if (not double_equals(penalized_bw, rate_before_update, sg_surf_precision)) {
      XBT_DEBUG("%d->%d action %p penalty updated : bw now %f, before %f , initial rate %f", root->id,
                comm->destination->id, comm->action, penalized_bw, comm->action->get_bound(), comm->init_rate);
      get_maxmin_system()->update_variable_bound(comm->action->get_variable(), penalized_bw);
    } else {
      XBT_DEBUG("%d->%d action %p penalty not updated : bw %f, initial rate %f", root->id, comm->destination->id,
                comm->action, penalized_bw, comm->init_rate);
    }
  }
  XBT_DEBUG("Finished computing IB penalties");
}

void NetworkIBModel::updateIBfactors_rec(IBNode* root, std::vector<bool>& updatedlist) const
{
  if (not updatedlist[root->id]) {
    XBT_DEBUG("IB - Updating rec %d", root->id);
    computeIBfactors(root);
    updatedlist[root->id] = true;
    for (ActiveComm const* comm : root->ActiveCommsUp) {
      if (not updatedlist[comm->destination->id])
        updateIBfactors_rec(comm->destination, updatedlist);
    }
    for (std::map<IBNode*, int>::value_type const& comm : root->ActiveCommsDown) {
      if (not updatedlist[comm.first->id])
        updateIBfactors_rec(comm.first, updatedlist);
    }
  }
}

void NetworkIBModel::updateIBfactors(NetworkAction* action, IBNode* from, IBNode* to, int remove) const
{
  if (from == to) // disregard local comms (should use loopback)
    return;

  if (remove) {
    if (to->ActiveCommsDown[from] == 1)
      to->ActiveCommsDown.erase(from);
    else
      to->ActiveCommsDown[from] -= 1;

    to->nbActiveCommsDown--;
    std::vector<ActiveComm*>::iterator it =
        std::find_if(begin(from->ActiveCommsUp), end(from->ActiveCommsUp),
                     [action](const ActiveComm* comm) { return comm->action == action; });
    if (it != std::end(from->ActiveCommsUp)) {
      delete *it;
      from->ActiveCommsUp.erase(it);
    }
    action->unref();
  } else {
    action->ref();
    ActiveComm* comm  = new ActiveComm();
    comm->action      = action;
    comm->destination = to;
    from->ActiveCommsUp.push_back(comm);

    to->ActiveCommsDown[from] += 1;
    to->nbActiveCommsDown++;
  }
  XBT_DEBUG("IB - Updating %d", from->id);
  std::vector<bool> updated(active_nodes.size(), false);
  updateIBfactors_rec(from, updated);
  XBT_DEBUG("IB - Finished updating %d", from->id);
}
} // namespace resource
} // namespace kernel
} // namespace simgrid
