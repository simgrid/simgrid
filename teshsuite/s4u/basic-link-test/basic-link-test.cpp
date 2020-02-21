/* Copyright (c) 2008-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(basic_link_test, s4u, "basic link test");

int main(int argc, char** argv)
{
  char user_data[] = "some user_data";

  /* initialization of SD */
  simgrid::s4u::Engine e(&argc, argv);

  /* creation of the environment */
  e.load_platform(argv[1]);

  std::vector<simgrid::s4u::Link*> links = simgrid::s4u::Engine::get_instance()->get_all_links();
  XBT_INFO("Link count: %zu", links.size());

  std::sort(links.begin(), links.end(), [](const simgrid::s4u::Link* a, const simgrid::s4u::Link* b) {
    return strcmp(sg_link_name(a), sg_link_name(b)) < 0;
  });

  for (const auto& l : links) {
    XBT_INFO("%s: latency = %.5f, bandwidth = %f", l->get_cname(), l->get_latency(), l->get_bandwidth());
    l->set_data(user_data);
    xbt_assert(!strcmp(user_data, static_cast<const char*>(l->get_data())), "User data was corrupted.");
  }

  return 0;
}
