/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstddef>
#include <algorithm>

#include <xbt/log.h>

#include "network_smpi.hpp"
#include "simgrid/sg_config.h"
#include "smpi/smpi_utils.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

std::vector<s_smpi_factor_t> smpi_bw_factor;
std::vector<s_smpi_factor_t> smpi_lat_factor;

xbt_dict_t gap_lookup = nullptr;

/*********
 * Model *
 *********/

/************************************************************************/
/* New model based on LV08 and experimental results of MPI ping-pongs   */
/************************************************************************/
/* @Inproceedings{smpi_ipdps, */
/*  author={Pierre-Nicolas Clauss and Mark Stillwell and Stéphane Genaud and Frédéric Suter and Henri Casanova and Martin Quinson}, */
/*  title={Single Node On-Line Simulation of {MPI} Applications with SMPI}, */
/*  booktitle={25th IEEE International Parallel and Distributed Processing Symposium (IPDPS'11)}, */
/*  address={Anchorage (Alaska) USA}, */
/*  month=may, */
/*  year={2011} */
/*  } */
void surf_network_model_init_SMPI()
{
  if (surf_network_model)
    return;
  surf_network_model = new simgrid::surf::NetworkSmpiModel();
  all_existing_models->push_back(surf_network_model);

  xbt_cfg_setdefault_double("network/sender-gap", 10e-6);
  xbt_cfg_setdefault_double("network/weight-S", 8775);
}

namespace simgrid {
  namespace surf {

  NetworkSmpiModel::NetworkSmpiModel() : NetworkCm02Model()
  {
    haveGap_ = true;
    }

    NetworkSmpiModel::~NetworkSmpiModel()
    {
      xbt_dict_free(&gap_lookup);
    }

    void NetworkSmpiModel::gapAppend(double size, Link* link, NetworkAction *act)
    {
      const char *src = link->getName();
      xbt_fifo_t fifo;
      NetworkCm02Action *action= static_cast<NetworkCm02Action*>(act);

      if (sg_sender_gap > 0.0) {
        if (!gap_lookup) {
          gap_lookup = xbt_dict_new_homogeneous(nullptr);
        }
        fifo = (xbt_fifo_t) xbt_dict_get_or_null(gap_lookup, src);
        action->senderGap_ = 0.0;
        if (fifo && xbt_fifo_size(fifo) > 0) {
          /* Compute gap from last send */
          /*last_action =
          (surf_action_network_CM02_t)
          xbt_fifo_get_item_content(xbt_fifo_get_last_item(fifo));*/
          // bw = net_get_link_bandwidth(link);
          action->senderGap_ = sg_sender_gap;
          /*  max(sg_sender_gap,last_action->sender.size / bw);*/
          action->latency_ += action->senderGap_;
        }
        /* Append action as last send */
        /*action->sender.link_name = link->lmm_resource.generic_resource.name;
    fifo =
        (xbt_fifo_t) xbt_dict_get_or_null(gap_lookup,
                                          action->sender.link_name);
    if (!fifo) {
      fifo = xbt_fifo_new();
      xbt_dict_set(gap_lookup, action->sender.link_name, fifo, nullptr);
    }
    action->sender.fifo_item = xbt_fifo_push(fifo, action);*/
        action->senderSize_ = size;
      }
    }

    void NetworkSmpiModel::gapRemove(Action *lmm_action)
    {
      xbt_fifo_t fifo;
      size_t size;
      NetworkCm02Action *action = static_cast<NetworkCm02Action*>(lmm_action);

      if (sg_sender_gap > 0.0 && action->senderLinkName_
          && action->senderFifoItem_) {
        fifo =
            (xbt_fifo_t) xbt_dict_get_or_null(gap_lookup,
                action->senderLinkName_);
        xbt_fifo_remove_item(fifo, action->senderFifoItem_);
        size = xbt_fifo_size(fifo);
        if (size == 0) {
          xbt_fifo_free(fifo);
          xbt_dict_remove(gap_lookup, action->senderLinkName_);
          size = xbt_dict_length(gap_lookup);
          if (size == 0) {
            xbt_dict_free(&gap_lookup);
          }
        }
      }
    }

    double NetworkSmpiModel::bandwidthFactor(double size)
    {
      if (smpi_bw_factor.empty())
        smpi_bw_factor = parse_factor(xbt_cfg_get_string("smpi/bw-factor"));

      double current = 1.0;
      for (const auto& fact : smpi_bw_factor) {
        if (size <= fact.factor) {
          XBT_DEBUG("%f <= %zu return %f", size, fact.factor, current);
          return current;
        } else
          current = fact.values.front();
      }
      XBT_DEBUG("%f > %zu return %f", size, smpi_bw_factor.back().factor, current);

      return current;
    }

    double NetworkSmpiModel::latencyFactor(double size)
    {
      if (smpi_lat_factor.empty())
        smpi_lat_factor = parse_factor(xbt_cfg_get_string("smpi/lat-factor"));

      double current=1.0;
      for (auto fact: smpi_lat_factor) {
        if (size <= fact.factor) {
          XBT_DEBUG("%f <= %zu return %f", size, fact.factor, current);
          return current;
        }else
          current=fact.values.front();
      }
      XBT_DEBUG("%f > %zu return %f", size, smpi_lat_factor.back().factor, current);

      return current;
    }

    double NetworkSmpiModel::bandwidthConstraint(double rate, double bound, double size)
    {
      return rate < 0 ? bound : std::min(bound, rate * bandwidthFactor(size));
    }

    /************
     * Resource *
     ************/



    /**********
     * Action *
     **********/

  }
}
