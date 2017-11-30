/* Copyright (c) 2014-2017. The SimGrid Team.
*All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <utility>

#include "simgrid/sg_config.h"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/network_ib.hpp"
#include "src/surf/xml/platf.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

static void IB_create_host_callback(simgrid::s4u::Host& host){
  using simgrid::surf::NetworkIBModel;
  using simgrid::surf::IBNode;

  static int id=0;
  // pour t->id -> rajouter une nouvelle struct dans le dict, pour stocker les comms actives

  IBNode* act = new IBNode(id);

  id++;
  ((NetworkIBModel*)surf_network_model)->active_nodes.insert({host.getName(), act});
}

static void IB_action_state_changed_callback(simgrid::surf::NetworkAction* action)
{
  using simgrid::surf::NetworkIBModel;
  using simgrid::surf::IBNode;

  if (action->getState() != simgrid::surf::Action::State::done)
    return;
  std::pair<IBNode*,IBNode*> pair = ((NetworkIBModel*)surf_network_model)->active_comms[action];
  XBT_DEBUG("IB callback - action %p finished", action);

  ((NetworkIBModel*)surf_network_model)->updateIBfactors(action, pair.first, pair.second, 1);

  ((NetworkIBModel*)surf_network_model)->active_comms.erase(action);

}

static void IB_action_init_callback(simgrid::surf::NetworkAction* action, simgrid::s4u::Host* src,
                                    simgrid::s4u::Host* dst)
{
  simgrid::surf::NetworkIBModel* ibModel = (simgrid::surf::NetworkIBModel*)surf_network_model;
  simgrid::surf::IBNode* act_src;
  simgrid::surf::IBNode* act_dst;

  auto asrc = ibModel->active_nodes.find(src->getName());
  if (asrc != ibModel->active_nodes.end()) {
    act_src = asrc->second;
  } else {
    throw std::out_of_range(std::string("Could not find '") + src->getCname() + "' active comms !");
  }

  auto adst = ibModel->active_nodes.find(dst->getName());
  if (adst != ibModel->active_nodes.end()) {
    act_dst = adst->second;
  } else {
    throw std::out_of_range(std::string("Could not find '") + dst->getCname() + "' active comms !");
  }

  ibModel->active_comms[action]=std::make_pair(act_src, act_dst);

  ibModel->updateIBfactors(action, act_src, act_dst, 0);
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
  if (surf_network_model)
    return;

  surf_network_model = new simgrid::surf::NetworkIBModel();
  all_existing_models->push_back(surf_network_model);
  simgrid::s4u::Link::onCommunicationStateChange.connect(IB_action_state_changed_callback);
  simgrid::s4u::Link::onCommunicate.connect(IB_action_init_callback);
  simgrid::s4u::Host::onCreation.connect(IB_create_host_callback);
  xbt_cfg_setdefault_double("network/weight-S", 8775);

}

namespace simgrid {
namespace surf {

NetworkIBModel::NetworkIBModel() : NetworkSmpiModel()
{
  std::string IB_factors_string = xbt_cfg_get_string("smpi/IB-penalty-factors");
  std::vector<std::string> radical_elements;
  boost::split(radical_elements, IB_factors_string, boost::is_any_of(";"));

  surf_parse_assert(radical_elements.size() == 3, "smpi/IB-penalty-factors should be provided and contain 3 "
                                                  "elements, semi-colon separated. Example: 0.965;0.925;1.35");

  try {
    Be = std::stod(radical_elements.front());
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("First part of smpi/IB-penalty-factors is not numerical:") + ia.what());
  }

  try {
    Bs = std::stod(radical_elements.at(1));
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Second part of smpi/IB-penalty-factors is not numerical:") + ia.what());
  }

  try {
    ys = std::stod(radical_elements.back());
  } catch (std::invalid_argument& ia) {
    throw std::invalid_argument(std::string("Third part of smpi/IB-penalty-factors is not numerical:") + ia.what());
  }
}

NetworkIBModel::~NetworkIBModel()
{
  for (auto const& instance : active_nodes)
    delete instance.second;
}

void NetworkIBModel::computeIBfactors(IBNode* root)
{
  double num_comm_out    = static_cast<double>(root->ActiveCommsUp.size());
  double max_penalty_out = 0.0;
  // first, compute all outbound penalties to get their max
  for (std::vector<ActiveComm*>::iterator it = root->ActiveCommsUp.begin(); it != root->ActiveCommsUp.end(); ++it) {
    double my_penalty_out = 1.0;

    if (num_comm_out != 1) {
      if ((*it)->destination->nbActiveCommsDown > 2) // number of comms sent to the receiving node
        my_penalty_out = num_comm_out * Bs * ys;
      else
        my_penalty_out = num_comm_out * Bs;
    }

    max_penalty_out = std::max(max_penalty_out, my_penalty_out);
  }

  for (std::vector<ActiveComm*>::iterator it = root->ActiveCommsUp.begin(); it != root->ActiveCommsUp.end(); ++it) {
    // compute inbound penalty
    double my_penalty_in = 1.0;
    int nb_comms         = (*it)->destination->nbActiveCommsDown; // total number of incoming comms
    if (nb_comms != 1)
      my_penalty_in = ((*it)->destination->ActiveCommsDown)[root]        // number of comm sent to dest by root node
                      * Be * (*it)->destination->ActiveCommsDown.size(); // number of different nodes sending to dest

    double penalty = std::max(my_penalty_in, max_penalty_out);

    double rate_before_update = (*it)->action->getBound();
    // save initial rate of the action
    if ((*it)->init_rate == -1)
      (*it)->init_rate = rate_before_update;

    double penalized_bw = num_comm_out ? (*it)->init_rate / penalty : (*it)->init_rate;

    if (not double_equals(penalized_bw, rate_before_update, sg_surf_precision)) {
      XBT_DEBUG("%d->%d action %p penalty updated : bw now %f, before %f , initial rate %f", root->id,
                (*it)->destination->id, (*it)->action, penalized_bw, (*it)->action->getBound(), (*it)->init_rate);
      maxminSystem_->update_variable_bound((*it)->action->getVariable(), penalized_bw);
    } else {
      XBT_DEBUG("%d->%d action %p penalty not updated : bw %f, initial rate %f", root->id, (*it)->destination->id,
                (*it)->action, penalized_bw, (*it)->init_rate);
    }
  }
  XBT_DEBUG("Finished computing IB penalties");
}

void NetworkIBModel::updateIBfactors_rec(IBNode* root, std::vector<bool>& updatedlist)
{
  if (not updatedlist[root->id]) {
    XBT_DEBUG("IB - Updating rec %d", root->id);
    computeIBfactors(root);
    updatedlist[root->id] = true;
    for (std::vector<ActiveComm*>::iterator it = root->ActiveCommsUp.begin(); it != root->ActiveCommsUp.end(); ++it) {
      if (not updatedlist[(*it)->destination->id])
        updateIBfactors_rec((*it)->destination, updatedlist);
    }
    for (std::map<IBNode*, int>::iterator it = root->ActiveCommsDown.begin(); it != root->ActiveCommsDown.end(); ++it) {
      if (not updatedlist[it->first->id])
        updateIBfactors_rec(it->first, updatedlist);
    }
  }
}

void NetworkIBModel::updateIBfactors(NetworkAction* action, IBNode* from, IBNode* to, int remove)
{
  if (from == to) // disregard local comms (should use loopback)
    return;

  ActiveComm* comm = nullptr;
  if (remove) {
    if (to->ActiveCommsDown[from] == 1)
      to->ActiveCommsDown.erase(from);
    else
      to->ActiveCommsDown[from] -= 1;

    to->nbActiveCommsDown--;
    for (std::vector<ActiveComm*>::iterator it = from->ActiveCommsUp.begin(); it != from->ActiveCommsUp.end(); ++it) {
      if ((*it)->action == action) {
        comm = (*it);
        from->ActiveCommsUp.erase(it);
        break;
      }
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
  delete comm;
}
}
}
