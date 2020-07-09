/**
 * Test the wifi energy plugin
 * Desactivate cross-factor to get round values
 * Launch with: ./test test_platform_2STA.xml --cfg=plugin:link_energy_wifi --cfg=network/crosstraffic:0
 */

#include "simgrid/plugins/energy.h"
#include "simgrid/s4u/Activity.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Link.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "xbt/log.h"

//#include <exception>
//#include <iostream>
//#include <random>
//#include <sstream>
//#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(test_wifi, "Wifi energy demo");

static void sender()
{
  // start sending after 5 seconds
  simgrid::s4u::this_actor::sleep_until(5);

  std::string mbName = "MailBoxRCV";
  simgrid::s4u::Mailbox *dst = simgrid::s4u::Mailbox::by_name(mbName);

  int size = 6750000;

  XBT_INFO("SENDING 1 msg of size %d to %s", size, mbName.c_str());
  static char message[] = "message";
  dst->put(message, size);
  XBT_INFO("finished sending");
}

static void receiver()
{
  std::string mbName = "MailBoxRCV";
  XBT_INFO("RECEIVING on mb %s", mbName.c_str());
  simgrid::s4u::Mailbox *myBox = simgrid::s4u::Mailbox::by_name(mbName);
  myBox->get();

  XBT_INFO("received all messages");
}

int main(int argc, char** argv)
{
  // engine
  simgrid::s4u::Engine engine(&argc, argv);
  XBT_INFO("Activating the SimGrid link energy plugin");
  sg_wifi_energy_plugin_init();
  engine.load_platform(argv[1]);

  // setup WiFi bandwidths
  auto* l = simgrid::s4u::Link::by_name("AP1");
  l->set_host_wifi_rate(simgrid::s4u::Host::by_name("Station 1"), 0);
  l->set_host_wifi_rate(simgrid::s4u::Host::by_name("Station 2"), 0);

  // create the two actors for the test
  simgrid::s4u::Actor::create("act0", simgrid::s4u::Host::by_name("Station 1"), sender);
  simgrid::s4u::Actor::create("act1", simgrid::s4u::Host::by_name("Station 2"), receiver);

  engine.run();

  return 0;
}
