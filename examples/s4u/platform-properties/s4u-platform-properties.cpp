/* Copyright (c) 2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// TODO: also test the properties attached to links

#include <simgrid/s4u.hpp>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Property test");

static void test_host(std::string hostname)
{
  simgrid::s4u::Host* thehost = simgrid::s4u::Host::by_name(hostname);
  std::map<std::string, std::string>* props = thehost->getProperties();
  const char* noexist = "Unknown";
  const char* exist   = "Hdd";
  const char* value;

  XBT_INFO("== Print the properties of the host '%s'", hostname.c_str());
  for (const auto& kv : *props)
    XBT_INFO("  Host property: '%s' -> '%s'", kv.first.c_str(), kv.second.c_str());

  XBT_INFO("== Try to get a host property that does not exist");
  value = thehost->getProperty(noexist);
  xbt_assert(not value, "The key exists (it's not supposed to)");

  XBT_INFO("== Try to get a host property that does exist");
  value = thehost->getProperty(exist);
  xbt_assert(value, "\tProperty %s is undefined (where it should)", exist);
  xbt_assert(!strcmp(value, "180"), "\tValue of property %s is defined to %s (where it should be 180)", exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  XBT_INFO("== Trying to modify a host property");
  thehost->setProperty(exist, "250");

  /* Test if we have changed the value */
  value = thehost->getProperty(exist);
  xbt_assert(value, "Property %s is undefined (where it should)", exist);
  xbt_assert(!strcmp(value, "250"), "Value of property %s is defined to %s (where it should be 250)", exist, value);
  XBT_INFO("   Property: %s old value: %s", exist, value);

  /* Restore the value for the next test */
  thehost->setProperty(exist, "180");
}

static int alice(int argc, char* argv[])
{
  /* Dump what we have on the current host */
  test_host("host1");
  return 0;
}

static int carole(int argc, char* argv[])
{
  /* Dump what we have on a remote host */
  simgrid::s4u::this_actor::sleep_for(1); // Wait for alice to be done with its experiment
  test_host("host1");
  return 0;
}

static int david(int argc, char* argv[])
{
  /* Dump what we have on a remote host */
  simgrid::s4u::this_actor::sleep_for(2); // Wait for alice and carole to be done with its experiment
  test_host("node-0.acme.org");
  return 0;
}

static int bob(int argc, char* argv[])
{
  /* this host also tests the properties of the AS*/
  simgrid::s4u::NetZone* root = simgrid::s4u::Engine::getInstance()->getNetRoot();
  XBT_INFO("== Print the properties of the zone");
  XBT_INFO("   Zone property: filename -> %s", root->getProperty("filename"));
  XBT_INFO("   Zone property: date -> %s", root->getProperty("date"));
  XBT_INFO("   Zone property: author -> %s", root->getProperty("author"));

  /* Get the property list of current bob process */
  std::map<std::string, std::string>* props = simgrid::s4u::Actor::self()->getProperties();
  const char* noexist = "UnknownProcessProp";
  XBT_ATTRIB_UNUSED const char* value;

  XBT_INFO("== Print the properties of the actor");
  for (const auto& kv : *props)
    XBT_INFO("   Actor property: %s -> %s", kv.first.c_str(), kv.second.c_str());

  XBT_INFO("== Try to get an actor property that does not exist");

  value = simgrid::s4u::Actor::self()->getProperty(noexist);
  xbt_assert(not value, "The property is defined (it shouldnt)");
  return 0;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);

  e.registerFunction("alice", alice);
  e.registerFunction("bob", bob);
  e.registerFunction("carole", carole);
  e.registerFunction("david", david);

  size_t totalHosts = sg_host_count();

  XBT_INFO("There are %zu hosts in the environment", totalHosts);
  simgrid::s4u::Host** hosts = sg_host_list();
  for (unsigned int i = 0; i < totalHosts; i++)
    XBT_INFO("Host '%s' runs at %.0f flops/s", hosts[i]->getCname(), hosts[i]->getSpeed());
  xbt_free(hosts);

  e.loadDeployment(argv[2]);
  e.run();

  return 0;
}
