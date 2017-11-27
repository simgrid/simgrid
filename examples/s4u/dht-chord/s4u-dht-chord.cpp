/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "s4u-dht-chord.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_chord, "Messages specific for this s4u example");
simgrid::xbt::Extension<simgrid::s4u::Host, HostChord> HostChord::EXTENSION_ID;

int nb_bits  = 24;
int nb_keys  = 0;
int timeout  = 50;
int* powers2 = nullptr;

/* Global initialization of the Chord simulation. */
static void chord_init()
{
  // compute the powers of 2 once for all
  powers2 = new int[nb_bits];
  int pow = 1;
  for (int i = 0; i < nb_bits; i++) {
    powers2[i] = pow;
    pow        = pow << 1;
  }
  nb_keys = pow;
  XBT_DEBUG("Sets nb_keys to %d", nb_keys);

  HostChord::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostChord>();

  std::vector<simgrid::s4u::Host*> list;
  simgrid::s4u::Engine::getInstance()->getHostList(&list);
  for (auto const& host : list)
    host->extension_set(new HostChord(host));
}

static void chord_exit()
{
  delete[] powers2;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s [-nb_bits=n] [-timeout=t] platform_file deployment_file\n"
                       "\tExample: %s ../msg_platform.xml chord.xml\n",
             argv[0], argv[0]);
  char** options = &argv[1];
  while (not strncmp(options[0], "-", 1)) {
    unsigned int length = strlen("-nb_bits=");
    if (not strncmp(options[0], "-nb_bits=", length) && strlen(options[0]) > length) {
      nb_bits = xbt_str_parse_int(options[0] + length, "Invalid nb_bits parameter: %s");
      XBT_DEBUG("Set nb_bits to %d", nb_bits);
    } else {
      length = strlen("-timeout=");
      if (not strncmp(options[0], "-timeout=", length) && strlen(options[0]) > length) {
        timeout = xbt_str_parse_int(options[0] + length, "Invalid timeout parameter: %s");
        XBT_DEBUG("Set timeout to %d", timeout);
      } else {
        xbt_die("Invalid chord option '%s'", options[0]);
      }
    }
    options++;
  }

  e.loadPlatform(options[0]);

  chord_init();

  e.registerFunction<Node>("node");
  e.loadDeployment(options[1]);

  e.run();

  XBT_INFO("Simulated time: %g", e.getClock());

  chord_exit();

  return 0;
}
