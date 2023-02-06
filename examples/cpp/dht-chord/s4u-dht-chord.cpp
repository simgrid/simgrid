/* Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "s4u-dht-chord.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_chord, "Messages specific for this s4u example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s [-nb_bits=n] [-timeout=t] platform_file deployment_file\n"
             "\tExample: %s ../platforms/cluster_backbone.xml ./s4u-dht-chord_d.xml\n",
             argv[0], argv[0]);
  std::string platform_file(argv[argc - 2]);
  std::string deployment_file(argv[argc - 1]);
  int nb_bits = 24;
  int timeout = 50;
  for (const auto& option : std::vector<std::string>(argv + 1, argv + argc - 2)) {
    if (option.rfind("-nb_bits=", 0) == 0) {
      nb_bits = std::stoi(option.substr(option.find('=') + 1));
      XBT_DEBUG("Set nb_bits to %d", nb_bits);
    } else if (option.rfind("-timeout=", 0) == 0) {
      timeout = std::stoi(option.substr(option.find('=') + 1));
      XBT_DEBUG("Set timeout to %d", timeout);
    } else {
      xbt_die("Invalid chord option '%s'", option.c_str());
    }
  }
  int nb_keys = 1U << nb_bits;
  XBT_DEBUG("Sets nb_keys to %d", nb_keys);

  e.load_platform(platform_file);

  /* Global initialization of the Chord simulation. */
  Node::set_parameters(nb_bits, nb_keys, timeout);

  e.register_actor<Node>("node");
  e.load_deployment(deployment_file);

  e.run();

  XBT_INFO("Simulated time: %g", simgrid::s4u::Engine::get_clock());
  return 0;
}
