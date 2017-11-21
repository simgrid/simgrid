/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstddef>
#include <algorithm>

#include <xbt/log.h>

#include "network_smpi.hpp"
#include "simgrid/sg_config.h"
#include "smpi_utils.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

std::vector<s_smpi_factor_t> smpi_bw_factor;
std::vector<s_smpi_factor_t> smpi_lat_factor;

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

  xbt_cfg_setdefault_double("network/weight-S", 8775);
}

namespace simgrid {
namespace surf {

NetworkSmpiModel::NetworkSmpiModel() : NetworkCm02Model()
{
}

NetworkSmpiModel::~NetworkSmpiModel() = default;

double NetworkSmpiModel::bandwidthFactor(double size)
{
  if (smpi_bw_factor.empty())
    smpi_bw_factor = parse_factor(xbt_cfg_get_string("smpi/bw-factor"));

  double current = 1.0;
  for (auto const& fact : smpi_bw_factor) {
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

  double current = 1.0;
  for (auto const& fact : smpi_lat_factor) {
    if (size <= fact.factor) {
      XBT_DEBUG("%f <= %zu return %f", size, fact.factor, current);
      return current;
    } else
      current = fact.values.front();
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
