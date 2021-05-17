/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "catch.hpp"

#include "simgrid/kernel/resource/NetworkModelIntf.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.hpp"

static double factor_cb(double, const simgrid::s4u::Host*, const simgrid::s4u::Host*,
                        const std::vector<simgrid::s4u::Link*>&, const std::unordered_set<simgrid::s4u::NetZone*>&)
{
  return 1.0;
}

TEST_CASE("kernel::resource::NetworkModelIntf: Factors invalid callbacks: exception", "")
{
  for (const auto& model : std::vector<std::string>{"LV08", "SMPI", "IB", "CM02"}) {
    _sg_cfg_init_status = 0; /* HACK: clear config global to be able to do set_config in UTs */
    simgrid::s4u::Engine e("test");
    e.set_config("network/model:" + model);
    simgrid::s4u::create_full_zone("root");

    SECTION("Model: " + model)
    {
      simgrid::kernel::resource::NetworkModelIntf* model = e.get_netzone_root()->get_network_model();
      REQUIRE_THROWS_AS(model->set_lat_factor_cb({}), std::invalid_argument);
      REQUIRE_THROWS_AS(model->set_lat_factor_cb(nullptr), std::invalid_argument);
      REQUIRE_THROWS_AS(model->set_bw_factor_cb({}), std::invalid_argument);
      REQUIRE_THROWS_AS(model->set_bw_factor_cb(nullptr), std::invalid_argument);
    }
  }
}

TEST_CASE("kernel::resource::NetworkModelIntf: Invalid network/latency-factor and network/bandwidth-factor", "")
{
  for (const auto& model : std::vector<std::string>{"LV08", "CM02"}) {
    _sg_cfg_init_status = 0; /* HACK: clear config global to be able to do set_config in UTs */
    simgrid::s4u::Engine e("test");
    e.set_config("network/model:" + model);
    e.set_config("network/latency-factor:10");
    e.set_config("network/bandwidth-factor:0.3");
    simgrid::s4u::create_full_zone("root");

    SECTION("Model: " + model)
    {
      simgrid::kernel::resource::NetworkModelIntf* model = e.get_netzone_root()->get_network_model();
      REQUIRE_THROWS_AS(model->set_lat_factor_cb(factor_cb), std::invalid_argument);
      REQUIRE_THROWS_AS(model->set_bw_factor_cb(factor_cb), std::invalid_argument);
    }
  }
}

TEST_CASE("kernel::resource::NetworkModelIntf: Invalid smpi/lat-factor and smpi/bw-factor", "")
{
  for (const auto& model : std::vector<std::string>{"SMPI", "IB"}) {
    _sg_cfg_init_status = 0; /* HACK: clear config global to be able to do set_config in UTs */
    simgrid::s4u::Engine e("test");
    e.set_config("network/model:" + model);
    e.set_config("smpi/lat-factor:65472:0.940694;15424:0.697866;9376:0.58729;5776:1.08739;3484:0.77493");
    e.set_config("smpi/bw-factor:65472:11.6436;15424:3.48845");
    simgrid::s4u::create_full_zone("root");

    SECTION("Model: " + model)
    {
      simgrid::kernel::resource::NetworkModelIntf* model = e.get_netzone_root()->get_network_model();
      REQUIRE_THROWS_AS(model->set_lat_factor_cb(factor_cb), std::invalid_argument);
      REQUIRE_THROWS_AS(model->set_bw_factor_cb(factor_cb), std::invalid_argument);
    }
  }
}
