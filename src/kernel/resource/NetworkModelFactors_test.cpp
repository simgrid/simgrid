/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"

#include "simgrid/s4u/Engine.hpp"
#include "src/internal_config.h" // HAVE_SMPI
#include "src/kernel/resource/NetworkModelFactors.hpp"
#include "src/simgrid/sg_config.hpp"

static double factor_cb(double, const simgrid::s4u::Host*, const simgrid::s4u::Host*,
                        const std::vector<simgrid::s4u::Link*>&, const std::unordered_set<simgrid::s4u::NetZone*>&)
{
  return 1.0;
}

TEST_CASE("kernel::resource::NetworkModelFactors: Factors invalid callbacks: exception", "")
{
  std::vector<std::string> models{"LV08", "CM02"};
#if HAVE_SMPI
  models.emplace_back("SMPI");
  models.emplace_back("IB");
#endif

  for (const auto& model : models) {
    _sg_cfg_init_status = 0; /* HACK: clear config global to be able to do set_config in UTs */
    simgrid::s4u::Engine e("test");
    simgrid::s4u::Engine::set_config("network/model:" + model);
    simgrid::s4u::create_full_zone("root");

    SECTION("Model: " + model)
    {
      const auto* zone = e.get_netzone_root();
      REQUIRE_THROWS_AS(zone->set_latency_factor_cb({}), std::invalid_argument);
      REQUIRE_THROWS_AS(zone->set_latency_factor_cb(nullptr), std::invalid_argument);
      REQUIRE_THROWS_AS(zone->set_bandwidth_factor_cb({}), std::invalid_argument);
      REQUIRE_THROWS_AS(zone->set_bandwidth_factor_cb(nullptr), std::invalid_argument);
    }
  }
}

TEST_CASE("kernel::resource::NetworkModelFactors: Invalid network/latency-factor and network/bandwidth-factor", "")
{
  std::vector<std::string> models{"LV08", "CM02"};
#if HAVE_SMPI
  models.emplace_back("SMPI");
  models.emplace_back("IB");
#endif

  for (const auto& model : models) {
    _sg_cfg_init_status = 0; /* HACK: clear config global to be able to do set_config in UTs */
    simgrid::s4u::Engine e("test");
    simgrid::s4u::Engine::set_config("network/model:" + model);
    simgrid::s4u::Engine::set_config("network/latency-factor:10");
    simgrid::s4u::Engine::set_config("network/bandwidth-factor:0.3");
    simgrid::s4u::create_full_zone("root");

    SECTION("Model: " + model)
    {
      const auto* zone = e.get_netzone_root();
      REQUIRE_THROWS_AS(zone->set_latency_factor_cb(factor_cb), std::invalid_argument);
      REQUIRE_THROWS_AS(zone->set_bandwidth_factor_cb(factor_cb), std::invalid_argument);
    }
  }
}
