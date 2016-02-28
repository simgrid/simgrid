/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstddef>

#include <algorithm>

#include <xbt/log.h>

#include "network_smpi.hpp"
#include "simgrid/sg_config.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

xbt_dynar_t smpi_bw_factor = NULL;
xbt_dynar_t smpi_lat_factor = NULL;

typedef struct s_smpi_factor *smpi_factor_t;
typedef struct s_smpi_factor { // FIXME: s_smpi_factor_multival (defined in smpi_base) should be used instead to dedupplicate this code
  long factor;
  double value;
} s_smpi_factor_t;

xbt_dict_t gap_lookup = NULL;

static int factor_cmp(const void *pa, const void *pb)
{
  return (((s_smpi_factor_t*)pa)->factor > ((s_smpi_factor_t*)pb)->factor) ? 1 :
      (((s_smpi_factor_t*)pa)->factor < ((s_smpi_factor_t*)pb)->factor) ? -1 : 0;
}

#include "src/surf/xml/platf.hpp" // FIXME: move that back to the parsing area
static xbt_dynar_t parse_factor(const char *smpi_coef_string)
{
  char *value = NULL;
  unsigned int iter = 0;
  s_smpi_factor_t fact;
  xbt_dynar_t smpi_factor, radical_elements, radical_elements2 = NULL;

  smpi_factor = xbt_dynar_new(sizeof(s_smpi_factor_t), NULL);
  radical_elements = xbt_str_split(smpi_coef_string, ";");
  xbt_dynar_foreach(radical_elements, iter, value) {

    radical_elements2 = xbt_str_split(value, ":");
    surf_parse_assert(xbt_dynar_length(radical_elements2) == 2,
        "Malformed radical '%s' for smpi factor. I was expecting something like 'a:b'", value);

    char *errmsg = bprintf("Invalid factor in chunk #%d: %%s", iter+1);
    fact.factor = xbt_str_parse_int(xbt_dynar_get_as(radical_elements2, 0, char *), errmsg);
    xbt_free(errmsg);
    fact.value = xbt_str_parse_double(xbt_dynar_get_as(radical_elements2, 1, char *), errmsg);
    errmsg = bprintf("Invalid factor value in chunk #%d: %%s", iter+1);
    xbt_free(errmsg);

    xbt_dynar_push_as(smpi_factor, s_smpi_factor_t, fact);
    XBT_DEBUG("smpi_factor:\t%ld : %f", fact.factor, fact.value);
    xbt_dynar_free(&radical_elements2);
  }
  xbt_dynar_free(&radical_elements);
  xbt_dynar_sort(smpi_factor, &factor_cmp);
  xbt_dynar_foreach(smpi_factor, iter, fact) {
    XBT_DEBUG("ordered smpi_factor:\t%ld : %f", fact.factor, fact.value);

  }
  return smpi_factor;
}

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
void surf_network_model_init_SMPI(void)
{

  if (surf_network_model)
    return;
  simgrid::surf::on_link.connect(netlink_parse_init);
  surf_network_model = new simgrid::surf::NetworkSmpiModel();
  xbt_dynar_push(all_existing_models, &surf_network_model);

  xbt_cfg_setdefault_double(_sg_cfg_set, "network/sender_gap", 10e-6);
  xbt_cfg_setdefault_double(_sg_cfg_set, "network/weight_S", 8775);
}

namespace simgrid {
  namespace surf {

    NetworkSmpiModel::NetworkSmpiModel()
    : NetworkCm02Model() {
      m_haveGap=true;
    }

    NetworkSmpiModel::~NetworkSmpiModel(){
      xbt_dict_free(&gap_lookup);
      xbt_dynar_free(&smpi_bw_factor);
      xbt_dynar_free(&smpi_lat_factor);
    }

    void NetworkSmpiModel::gapAppend(double size, Link* link, NetworkAction *act)
    {
      const char *src = link->getName();
      xbt_fifo_t fifo;
      NetworkCm02Action *action= static_cast<NetworkCm02Action*>(act);

      if (sg_sender_gap > 0.0) {
        if (!gap_lookup) {
          gap_lookup = xbt_dict_new_homogeneous(NULL);
        }
        fifo = (xbt_fifo_t) xbt_dict_get_or_null(gap_lookup, src);
        action->m_senderGap = 0.0;
        if (fifo && xbt_fifo_size(fifo) > 0) {
          /* Compute gap from last send */
          /*last_action =
          (surf_action_network_CM02_t)
          xbt_fifo_get_item_content(xbt_fifo_get_last_item(fifo));*/
          // bw = net_get_link_bandwidth(link);
          action->m_senderGap = sg_sender_gap;
          /*  max(sg_sender_gap,last_action->sender.size / bw);*/
          action->m_latency += action->m_senderGap;
        }
        /* Append action as last send */
        /*action->sender.link_name = link->lmm_resource.generic_resource.name;
    fifo =
        (xbt_fifo_t) xbt_dict_get_or_null(gap_lookup,
                                          action->sender.link_name);
    if (!fifo) {
      fifo = xbt_fifo_new();
      xbt_dict_set(gap_lookup, action->sender.link_name, fifo, NULL);
    }
    action->sender.fifo_item = xbt_fifo_push(fifo, action);*/
        action->m_senderSize = size;
      }
    }

    void NetworkSmpiModel::gapRemove(Action *lmm_action)
    {
      xbt_fifo_t fifo;
      size_t size;
      NetworkCm02Action *action = static_cast<NetworkCm02Action*>(lmm_action);

      if (sg_sender_gap > 0.0 && action->p_senderLinkName
          && action->p_senderFifoItem) {
        fifo =
            (xbt_fifo_t) xbt_dict_get_or_null(gap_lookup,
                action->p_senderLinkName);
        xbt_fifo_remove_item(fifo, action->p_senderFifoItem);
        size = xbt_fifo_size(fifo);
        if (size == 0) {
          xbt_fifo_free(fifo);
          xbt_dict_remove(gap_lookup, action->p_senderLinkName);
          size = xbt_dict_length(gap_lookup);
          if (size == 0) {
            xbt_dict_free(&gap_lookup);
          }
        }
      }
    }

    double NetworkSmpiModel::bandwidthFactor(double size)
    {
      if (!smpi_bw_factor)
        smpi_bw_factor =
            parse_factor(sg_cfg_get_string("smpi/bw_factor"));

      unsigned int iter = 0;
      s_smpi_factor_t fact;
      double current=1.0;
      xbt_dynar_foreach(smpi_bw_factor, iter, fact) {
        if (size <= fact.factor) {
          XBT_DEBUG("%f <= %ld return %f", size, fact.factor, current);
          return current;
        }else
          current=fact.value;
      }
      XBT_DEBUG("%f > %ld return %f", size, fact.factor, current);

      return current;
    }

    double NetworkSmpiModel::latencyFactor(double size)
    {
      if (!smpi_lat_factor)
        smpi_lat_factor =
            parse_factor(sg_cfg_get_string("smpi/lat_factor"));

      unsigned int iter = 0;
      s_smpi_factor_t fact;
      double current=1.0;
      xbt_dynar_foreach(smpi_lat_factor, iter, fact) {
        if (size <= fact.factor) {
          XBT_DEBUG("%f <= %ld return %f", size, fact.factor, current);
          return current;
        }else
          current=fact.value;
      }
      XBT_DEBUG("%f > %ld return %f", size, fact.factor, current);

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
