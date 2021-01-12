/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// TODO: also test the properties attached to links

#include <algorithm>
#include <simgrid/s4u.hpp>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Property test");

static void test_host(const std::string& hostname)
{
  simgrid::s4u::Host* thehost = simgrid::s4u::Host::by_name(hostname);
  const std::unordered_map<std::string, std::string>* hostprops = thehost->get_properties();
  const char* noexist = "Unknown";
  const char* exist   = "Hdd";
  const char* value;

  XBT_INFO("== Print the properties of the host '%s'", hostname.c_str());
  // Sort the properties before displaying them, so that the tests are perfectly reproducible
  std::vector<std::string> keys;
  for (auto const& kv : *hostprops)
    keys.push_back(kv.first);
  std::sort(keys.begin(), keys.end());
  for (std::string key : keys)
    XBT_INFO("  Host property: '%s' -> '%s'", key.c_str(), hostprops->at(key).c_str());

  XBT_INFO("== Try to get a host property that does not exist");
  value = thehost->get_property(noexist);
  xbt_assert(not value, "The key exists (it's not supposed to)");

  XBT_INFO("== Try to get a host property that does exist");
  value = thehost->get_property(exist);
  xbt_assert(value, "\tProperty %s is undefined (where it should)", exist);
  xbt_assert(!strcmp(value, "180"), "\tValue of property %s is defined to %s (where it should be 180)", exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  XBT_INFO("== Trying to modify a host property");
  thehost->set_property(exist, "250");

  /* Test if we have changed the value */
  value = thehost->get_property(exist);
  xbt_assert(value, "Property %s is undefined (where it should)", exist);
  xbt_assert(!strcmp(value, "250"), "Value of property %s is defined to %s (where it should be 250)", exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  /* Restore the value for the next test */
  thehost->set_property(exist, "180");

  auto thezone = thehost->get_englobing_zone();
  XBT_INFO("== Print the properties of the zone '%s' that contains '%s'", thezone->get_cname(), hostname.c_str());
  const std::unordered_map<std::string, std::string>* zoneprops = thezone->get_properties();
  keys.clear();
  for (auto const& kv : *zoneprops)
    keys.push_back(kv.first);
  std::sort(keys.begin(), keys.end());
  for (std::string key : keys)
    XBT_INFO("  Zone property: '%s' -> '%s'", key.c_str(), zoneprops->at(key).c_str());
}

static void alice(std::vector<std::string> /*args*/)
{
  /* Dump what we have on the current host */
  test_host("host1");
}

static void carole(std::vector<std::string> /*args*/)
{
  /* Dump what we have on a remote host */
  simgrid::s4u::this_actor::sleep_for(1); // Wait for alice to be done with its experiment
  test_host("host1");
}

static void david(std::vector<std::string> /*args*/)
{
  /* Dump what we have on a remote host */
  simgrid::s4u::this_actor::sleep_for(2); // Wait for alice and carole to be done with its experiment
  test_host("node-0.simgrid.org");
}

static void bob(std::vector<std::string> /*args*/)
{
  /* this host also tests the properties of the AS*/
  const simgrid::s4u::NetZone* root = simgrid::s4u::Engine::get_instance()->get_netzone_root();
  XBT_INFO("== Print the properties of the root zone");
  XBT_INFO("   Zone property: filename -> %s", root->get_property("filename"));
  XBT_INFO("   Zone property: date -> %s", root->get_property("date"));
  XBT_INFO("   Zone property: author -> %s", root->get_property("author"));

  /* Get the property list of current bob actor */
  const std::unordered_map<std::string, std::string>* props = simgrid::s4u::Actor::self()->get_properties();
  const char* noexist = "UnknownProcessProp";
  XBT_ATTRIB_UNUSED const char* value;

  XBT_INFO("== Print the properties of the actor");
  for (const auto& kv : *props)
    XBT_INFO("   Actor property: %s -> %s", kv.first.c_str(), kv.second.c_str());

  XBT_INFO("== Try to get an actor property that does not exist");

  value = simgrid::s4u::Actor::self()->get_property(noexist);
  xbt_assert(not value, "The property is defined (it should not)");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  e.register_function("alice", alice);
  e.register_function("bob", bob);
  e.register_function("carole", carole);
  e.register_function("david", david);

  size_t totalHosts = e.get_host_count();

  XBT_INFO("There are %zu hosts in the environment", totalHosts);
  std::vector<simgrid::s4u::Host*> hosts = e.get_all_hosts();
  for (simgrid::s4u::Host const* host : hosts)
    XBT_INFO("Host '%s' runs at %.0f flops/s", host->get_cname(), host->get_speed());

  e.load_deployment(argv[2]);
  e.run();

  return 0;
}
