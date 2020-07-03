/**
  * Test the wifi energy plugin 
  * Desactivate cross-factor to get round values
  * Launche with: ./test test_platform_2STA.xml --cfg=plugin:link_energy_wifi --cfg=network/crosstraffic:0 
  */

#include "simgrid/s4u.hpp"
#include "xbt/log.h"
#include "simgrid/msg.h"
#include "simgrid/s4u/Activity.hpp"
#include "simgrid/plugins/energy.h"
#include "src/surf/network_wifi.hpp"

#include <exception>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(experience, "Wifi exp");
static void sender();
static void receiver();

int main(int argc, char **argv)
{
  // engine
  simgrid::s4u::Engine engine(&argc, argv);
  XBT_INFO("Activating the SimGrid link energy plugin");
  sg_wifi_energy_plugin_init();
  engine.load_platform(argv[1]);

  // setup WiFi bandwidths
  simgrid::kernel::resource::NetworkWifiLink *l = (simgrid::kernel::resource::NetworkWifiLink *)simgrid::s4u::Link::by_name(
                                                      "AP1")
                                                      ->get_impl();
  l->set_host_rate(simgrid::s4u::Host::by_name("Station 1"), 0);
  l->set_host_rate(simgrid::s4u::Host::by_name("Station 2"), 0);

  // create the two actors for the test
  simgrid::s4u::Actor::create("act0", simgrid::s4u::Host::by_name("Station 1"), sender);
  simgrid::s4u::Actor::create("act1", simgrid::s4u::Host::by_name("Station 2"), receiver);

  engine.run();

  return 0;
}

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
