/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// TODO: also test the properties attached to links

#include <algorithm>
#include <simgrid/s4u.hpp>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Property test");
namespace sg4 = simgrid::s4u;

static void test_host(const std::string& hostname)
{
  sg4::Host* thehost                                            = sg4::Host::by_name(hostname);
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
  for (const std::string& key : keys)
    XBT_INFO("  Host property: '%s' -> '%s'", key.c_str(), hostprops->at(key).c_str());

  XBT_INFO("== Try to get a host property that does not exist");
  value = thehost->get_property(noexist);
  xbt_assert(not value, "The key exists (it's not supposed to)");

  XBT_INFO("== Try to get a host property that does exist");
  value = thehost->get_property(exist);
  xbt_assert(value, "\tProperty %s is undefined (where it should)", exist);
  xbt_assert(strcmp(value, "180") == 0, "\tValue of property %s is defined to %s (where it should be 180)", exist,
             value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  XBT_INFO("== Trying to modify a host property");
  thehost->set_property(exist, "250");

  /* Test if we have changed the value */
  value = thehost->get_property(exist);
  xbt_assert(value, "Property %s is undefined (where it should)", exist);
  xbt_assert(strcmp(value, "250") == 0, "Value of property %s is defined to %s (where it should be 250)", exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  /* Restore the value for the next test */
  thehost->set_property(exist, "180");

  const auto* thezone = thehost->get_englobing_zone();
  XBT_INFO("== Print the properties of the zone '%s' that contains '%s'", thezone->get_cname(), hostname.c_str());
  const std::unordered_map<std::string, std::string>* zoneprops = thezone->get_properties();
  keys.clear();
  for (auto const& kv : *zoneprops)
    keys.push_back(kv.first);
  std::sort(keys.begin(), keys.end());
  for (const std::string& key : keys)
    XBT_INFO("  Zone property: '%s' -> '%s'", key.c_str(), zoneprops->at(key).c_str());
}

static void alice()
{
  /* Dump what we have on the current host */
  test_host("host1");
}

static void carole()
{
  /* Dump what we have on a remote host */
  sg4::this_actor::sleep_for(1); // Wait for alice to be done with its experiment
  test_host("host1");
}

static void david()
{
  /* Dump what we have on a remote host */
  sg4::this_actor::sleep_for(2); // Wait for alice and carole to be done with its experiment
  test_host("node-0.simgrid.org");
}

static void bob()
{
  /* this host also tests the properties of the AS*/
  const sg4::NetZone* root = sg4::Engine::get_instance()->get_netzone_root();
  XBT_INFO("== Print the properties of the root zone");
  XBT_INFO("   Zone property: filename -> %s", root->get_property("filename"));
  XBT_INFO("   Zone property: date -> %s", root->get_property("date"));
  XBT_INFO("   Zone property: author -> %s", root->get_property("author"));

  /* Get the property list of current bob actor */
  const std::unordered_map<std::string, std::string>* props = sg4::Actor::self()->get_properties();
  const char* noexist = "UnknownProcessProp";
  XBT_ATTRIB_UNUSED const char* value;

  XBT_INFO("== Print the properties of the actor");
  for (const auto& kv : *props)
    XBT_INFO("   Actor property: %s -> %s", kv.first.c_str(), kv.second.c_str());

  XBT_INFO("== Try to get an actor property that does not exist");

  value = sg4::Actor::self()->get_property(noexist);
  xbt_assert(not value, "The property is defined (it should not)");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  auto* host1 = e.host_by_name("host1");
  auto* host2 = e.host_by_name("host2");

  size_t totalHosts = e.get_host_count();

  XBT_INFO("There are %zu hosts in the environment", totalHosts);
  std::vector<sg4::Host*> hosts = e.get_all_hosts();
  for (sg4::Host const* host : hosts)
    XBT_INFO("Host '%s' runs at %.0f flops/s", host->get_cname(), host->get_speed());

  sg4::Actor::create("alice", host1, alice);
  sg4::Actor::create("bob", host1, bob)->set_property("SomeProp", "SomeValue");
  sg4::Actor::create("carole", host2, carole);
  sg4::Actor::create("david", host2, david);

  e.run();

  return 0;
}
