/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_smpi.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.hpp"
#include "smpi_utils.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_network);

/*********
 * Model *
 *********/

/************************************************************************/
/* New model based on LV08 and experimental results of MPI ping-pongs   */
/************************************************************************/
/* @Inproceedings{smpi_ipdps, */
/*  author={Pierre-Nicolas Clauss and Mark Stillwell and Stéphane Genaud and Frédéric Suter and Henri Casanova and
 * Martin Quinson}, */
/*  title={Single Node On-Line Simulation of {MPI} Applications with SMPI}, */
/*  booktitle={25th IEEE International Parallel and Distributed Processing Symposium (IPDPS'11)}, */
/*  address={Anchorage (Alaska) USA}, */
/*  month=may, */
/*  year={2011} */
/*  } */
void surf_network_model_init_SMPI()
{
  auto net_model = std::make_shared<simgrid::kernel::resource::NetworkSmpiModel>("Network_SMPI");
  simgrid::kernel::EngineImpl::get_instance()->add_model(net_model);
  simgrid::s4u::Engine::get_instance()->get_netzone_root()->get_impl()->set_network_model(net_model);

  simgrid::config::set_default<double>("network/weight-S", 8775);
}

namespace simgrid {
namespace kernel {
namespace resource {

void NetworkSmpiModel::check_lat_factor_cb()
{
  if (not simgrid::config::is_default("smpi/lat-factor")) {
    throw std::invalid_argument(
        "NetworkModelIntf: Cannot mix network/latency-factor and callback configuration. Choose only one of them.");
  }
}

void NetworkSmpiModel::check_bw_factor_cb()
{
  if (not simgrid::config::is_default("smpi/bw-factor")) {
    throw std::invalid_argument(
        "NetworkModelIntf: Cannot mix network/bandwidth-factor and callback configuration. Choose only one of them.");
  }
}

double NetworkSmpiModel::get_bandwidth_factor(double size)
{
  static std::vector<s_smpi_factor_t> smpi_bw_factor;
  if (smpi_bw_factor.empty())
    smpi_bw_factor = smpi::utils::parse_factor(config::get_value<std::string>("smpi/bw-factor"));

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

double NetworkSmpiModel::get_latency_factor(double size)
{
  static std::vector<s_smpi_factor_t> smpi_lat_factor;
  if (smpi_lat_factor.empty())
    smpi_lat_factor = smpi::utils::parse_factor(config::get_value<std::string>("smpi/lat-factor"));

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
} // namespace resource
} // namespace kernel
} // namespace simgrid
