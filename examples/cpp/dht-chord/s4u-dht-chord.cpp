/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "s4u-dht-chord.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_chord, "Messages specific for this s4u example");

int nb_bits  = 24;
int nb_keys  = 0;
int timeout  = 50;

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s [-nb_bits=n] [-timeout=t] platform_file deployment_file\n"
             "\tExample: %s ../platforms/cluster_backbone.xml ./s4u-dht-chord_d.xml\n",
             argv[0], argv[0]);
  char** options = &argv[1];
  while (not strncmp(options[0], "-", 1)) {
    unsigned int length = strlen("-nb_bits=");
    if (not strncmp(options[0], "-nb_bits=", length) && strlen(options[0]) > length) {
      nb_bits = static_cast<int>(xbt_str_parse_int(options[0] + length, "Invalid nb_bits parameter"));
      XBT_DEBUG("Set nb_bits to %d", nb_bits);
    } else {
      length = strlen("-timeout=");
      xbt_assert(strncmp(options[0], "-timeout=", length) == 0 && strlen(options[0]) > length,
                 "Invalid chord option '%s'", options[0]);
      timeout = static_cast<int>(xbt_str_parse_int(options[0] + length, "Invalid timeout parameter"));
      XBT_DEBUG("Set timeout to %d", timeout);
    }
    options++;
  }

  e.load_platform(options[0]);

  /* Global initialization of the Chord simulation. */
  nb_keys = 1U << nb_bits;
  XBT_DEBUG("Sets nb_keys to %d", nb_keys);

  e.register_actor<Node>("node");
  e.load_deployment(options[1]);

  e.run();

  XBT_INFO("Simulated time: %g", e.get_clock());
  return 0;
}
