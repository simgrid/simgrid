/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>
#include <utility>

#include "network_ib.hpp"

#include "src/surf/HostImpl.hpp"
#include "simgrid/sg_config.h"
#include "maxmin_private.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

static void IB_create_host_callback(simgrid::s4u::Host& host){
  using namespace simgrid::surf;

  static int id=0;
  // pour t->id -> rajouter une nouvelle struct dans le dict, pour stocker les comms actives
  if(((NetworkIBModel*)surf_network_model)->active_nodes==NULL)
    ((NetworkIBModel*)surf_network_model)->active_nodes=xbt_dict_new();

  IBNode* act = new IBNode(id);

  id++;
  xbt_dict_set(((NetworkIBModel*)surf_network_model)->active_nodes,
      host.name().c_str(), act, NULL);

}

static void IB_action_state_changed_callback(
    simgrid::surf::NetworkAction *action,
    e_surf_action_state_t statein, e_surf_action_state_t stateout)
{
  using namespace simgrid::surf;
  if(statein!=SURF_ACTION_RUNNING|| stateout!=SURF_ACTION_DONE)
    return;
  std::pair<IBNode*,IBNode*> pair = ((NetworkIBModel*)surf_network_model)->active_comms[action];
  XBT_DEBUG("IB callback - action %p finished", action);

  ((NetworkIBModel*)surf_network_model)->updateIBfactors(action, pair.first, pair.second, 1);

  ((NetworkIBModel*)surf_network_model)->active_comms.erase(action);

}


static void IB_action_init_callback(
    simgrid::surf::NetworkAction *action, simgrid::surf::NetCard *src, simgrid::surf::NetCard *dst,
    double size, double rate)
{
  using namespace simgrid::surf;
  if(((NetworkIBModel*)surf_network_model)->active_nodes==NULL)
    xbt_die("IB comm added, without any node connected !");

  IBNode* act_src= (IBNode*) xbt_dict_get_or_null(((NetworkIBModel*)surf_network_model)->active_nodes, src->name());
  if(act_src==NULL)
    xbt_die("could not find src node active comms !");
  //act_src->rate=rate;

  IBNode* act_dst= (IBNode*) xbt_dict_get_or_null(((NetworkIBModel*)surf_network_model)->active_nodes, dst->name());
  if(act_dst==NULL)
    xbt_die("could not find dst node active comms !");  
  // act_dst->rate=rate;

  ((NetworkIBModel*)surf_network_model)->active_comms[action]=std::make_pair(act_src, act_dst);
  //post the action in the second dist, to retrieve in the other callback
  XBT_DEBUG("IB callback - action %p init", action);

  ((NetworkIBModel*)surf_network_model)->updateIBfactors(action, act_src, act_dst, 0);

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
void surf_network_model_init_IB(void)
{
  using simgrid::surf::networkActionStateChangedCallbacks;
  using simgrid::surf::networkCommunicateCallbacks;

  if (surf_network_model)
    return;

  simgrid::surf::on_link.connect(netlink_parse_init);
  surf_network_model = new simgrid::surf::NetworkIBModel();
  xbt_dynar_push(all_existing_models, &surf_network_model);
  networkActionStateChangedCallbacks.connect(IB_action_state_changed_callback);
  networkCommunicateCallbacks.connect(IB_action_init_callback);
  simgrid::s4u::Host::onCreation.connect(IB_create_host_callback);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S", 8775);

}

#include "src/surf/xml/platf.hpp" // FIXME: move that back to the parsing area

namespace simgrid {
  namespace surf {

    NetworkIBModel::NetworkIBModel()
    : NetworkSmpiModel() {
      m_haveGap=false;
      active_nodes=NULL;

      const char* IB_factors_string=sg_cfg_get_string("smpi/IB_penalty_factors");
      xbt_dynar_t radical_elements = xbt_str_split(IB_factors_string, ";");

      surf_parse_assert(xbt_dynar_length(radical_elements)==3,
          "smpi/IB_penalty_factors should be provided and contain 3 elements, semi-colon separated : for example 0.965;0.925;1.35");

      Be = xbt_str_parse_double(xbt_dynar_get_as(radical_elements, 0, char *), "First part of smpi/IB_penalty_factors is not numerical: %s");
      Bs = xbt_str_parse_double(xbt_dynar_get_as(radical_elements, 1, char *), "Second part of smpi/IB_penalty_factors is not numerical: %s");
      ys = xbt_str_parse_double(xbt_dynar_get_as(radical_elements, 2, char *), "Third part of smpi/IB_penalty_factors is not numerical: %s");

      xbt_dynar_free(&radical_elements);
    }

    NetworkIBModel::~NetworkIBModel()
    {
      xbt_dict_cursor_t cursor = NULL;
      IBNode* instance = NULL;
      char *name = NULL;
      xbt_dict_foreach(active_nodes, cursor, name, instance)
      delete instance;
      xbt_dict_free(&active_nodes);
    }

    void NetworkIBModel::computeIBfactors(IBNode *root) {
      double penalized_bw=0.0;
      double num_comm_out = (double) root->ActiveCommsUp.size();
      double max_penalty_out=0.0;
      //first, compute all outbound penalties to get their max
      for (std::vector<ActiveComm*>::iterator it= root->ActiveCommsUp.begin(); it != root->ActiveCommsUp.end(); ++it) {
        double my_penalty_out = 1.0;

        if(num_comm_out!=1){
          if((*it)->destination->nbActiveCommsDown > 2)//number of comms sent to the receiving node
            my_penalty_out = num_comm_out * Bs * ys;
          else
            my_penalty_out = num_comm_out * Bs;
        }

        max_penalty_out = std::max(max_penalty_out,my_penalty_out);
      }

      for (std::vector<ActiveComm*>::iterator it= root->ActiveCommsUp.begin(); it != root->ActiveCommsUp.end(); ++it) {

        //compute inbound penalty
        double my_penalty_in = 1.0;
        int nb_comms = (*it)->destination->nbActiveCommsDown;//total number of incoming comms
        if(nb_comms!=1)
          my_penalty_in = ((*it)->destination->ActiveCommsDown)[root] //number of comm sent to dest by root node
                                                                * Be
                                                                * (*it)->destination->ActiveCommsDown.size();//number of different nodes sending to dest

        double penalty = std::max(my_penalty_in,max_penalty_out);

        double rate_before_update = (*it)->action->getBound();
        //save initial rate of the action
        if((*it)->init_rate==-1)
          (*it)->init_rate= rate_before_update;

        penalized_bw= ! num_comm_out ? (*it)->init_rate : (*it)->init_rate /penalty;

        if (!double_equals(penalized_bw, rate_before_update, sg_surf_precision)){
          XBT_DEBUG("%d->%d action %p penalty updated : bw now %f, before %f , initial rate %f", root->id,(*it)->destination->id,(*it)->action,penalized_bw, (*it)->action->getBound(), (*it)->init_rate );
          lmm_update_variable_bound(maxminSystem_, (*it)->action->getVariable(), penalized_bw);
        }else{
          XBT_DEBUG("%d->%d action %p penalty not updated : bw %f, initial rate %f", root->id,(*it)->destination->id,(*it)->action,penalized_bw, (*it)->init_rate );
        }

      }
      XBT_DEBUG("Finished computing IB penalties");
    }

    void NetworkIBModel::updateIBfactors_rec(IBNode *root, bool* updatedlist) {
      if(updatedlist[root->id]==0){
        XBT_DEBUG("IB - Updating rec %d", root->id);
        computeIBfactors(root);
        updatedlist[root->id]=1;
        for (std::vector<ActiveComm*>::iterator it= root->ActiveCommsUp.begin(); it != root->ActiveCommsUp.end(); ++it) {
          if(updatedlist[(*it)->destination->id]!=1)
            updateIBfactors_rec((*it)->destination, updatedlist);
        }
        for (std::map<IBNode*, int>::iterator it= root->ActiveCommsDown.begin(); it != root->ActiveCommsDown.end(); ++it) {
          if(updatedlist[it->first->id]!=1)
            updateIBfactors_rec(it->first, updatedlist);
        }
      }
    }


    void NetworkIBModel::updateIBfactors(NetworkAction *action, IBNode *from, IBNode * to, int remove) {
      if (from == to)//disregard local comms (should use loopback)
        return;

      bool* updated=(bool*)xbt_malloc0(xbt_dict_size(active_nodes)*sizeof(bool));
      ActiveComm* comm=NULL;
      if(remove){
        if(to->ActiveCommsDown[from]==1)
          to->ActiveCommsDown.erase(from);
        else
          to->ActiveCommsDown[from]-=1;

        to->nbActiveCommsDown--;
        for (std::vector<ActiveComm*>::iterator it= from->ActiveCommsUp.begin();
            it != from->ActiveCommsUp.end(); ++it) {
          if((*it)->action==action){
            comm=(*it);
            from->ActiveCommsUp.erase(it);
            break;
          }
        }
        action->unref();

      }else{
        action->ref();
        ActiveComm* comm=new ActiveComm();
        comm->action=action;
        comm->destination=to;
        from->ActiveCommsUp.push_back(comm);

        to->ActiveCommsDown[from]+=1;
        to->nbActiveCommsDown++;
      }
      XBT_DEBUG("IB - Updating %d", from->id);
      updateIBfactors_rec(from, updated);
      XBT_DEBUG("IB - Finished updating %d", from->id);
      if(comm)
        delete comm;
      xbt_free(updated);
    }

  }
}
