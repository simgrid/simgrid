/* Copyright (c) 2013-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Engine.hpp>

#include "simgrid/sg_config.hpp"
#include "smpi_utils.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/surf/network_smpi.hpp"

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
  auto* engine   = simgrid::kernel::EngineImpl::get_instance();
  engine->add_model(net_model);
  engine->get_netzone_root()->set_network_model(net_model);

  simgrid::config::set_default<double>("network/weight-S", 8775);
  simgrid::config::set_default<std::string>("network/bandwidth-factor",
                                            "65472:0.940694;15424:0.697866;9376:0.58729;5776:1.08739;3484:0.77493;"
                                            "1426:0.608902;732:0.341987;257:0.338112;0:0.812084");
  simgrid::config::set_default<std::string>("network/latency-factor",
                                            "65472:11.6436;15424:3.48845;9376:2.59299;5776:2.18796;3484:1.88101;"
                                            "1426:1.61075;732:1.9503;257:1.95341;0:2.01467");
}

namespace simgrid::kernel::resource {

void NetworkSmpiModel::check_lat_factor_cb()
{
  if (not simgrid::config::is_default("network/latency-factor")) {
    throw std::invalid_argument(
        "NetworkModelIntf: Cannot mix network/latency-factor and callback configuration. Choose only one of them.");
  }
}

void NetworkSmpiModel::check_bw_factor_cb()
{
  if (not simgrid::config::is_default("network/bandwidth-factor")) {
    throw std::invalid_argument(
        "NetworkModelIntf: Cannot mix network/bandwidth-factor and callback configuration. Choose only one of them.");
  }
}

} // namespace simgrid::kernel::resource
