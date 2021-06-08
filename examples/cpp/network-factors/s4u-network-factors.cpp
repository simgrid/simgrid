/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to build set customized communication factors
 *
 * It uses the interface provided by NetworkModelIntf to register 2 callbacks that
 * are called everytime a communication occurs.
 *
 * These factors are used to change the communication time depending on the message size
 * and destination.
 *
 * This example uses factors obtained by some experiments on dahu cluster in Grid'5000.
 * You must change the values according to the calibration of your enviroment.
 */

#include <map>
#include <simgrid/kernel/resource/NetworkModelIntf.hpp>
#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_network_factors, "Messages specific for this s4u example");

/* Factors used in this platform, for remote and local communications
 * Obtained from dahu cluster. Obs.: just an example, change the values according
 * to the calibration on your environment */
static const std::map<double, double> REMOTE_BW_FACTOR = {
    {0, 1.0000000000000002},         {8000, 1.0000000000000002},     {15798, 0.07435006650635523},
    {64000, 0.3163352696348148},     {6000000, 0.13003278960133288}, {42672591, 0.10354740223279707},
    {160097505, 0.40258935729656503}};
static const std::map<double, double> LOCAL_BW_FACTOR = {{0, 0.17591906192813994},
                                                         {16000, 0.12119203247138953},
                                                         {6000000, 0.07551057012803415},
                                                         {36900419, 0.04281516758309203},
                                                         {160097505, 0.17440518795992602}};

static const std::map<double, double> REMOTE_LAT_FACTOR = {{0, 0.0},
                                                           {8000, 1731.7102918851567},
                                                           {15798, 1441.073993161278},
                                                           {64000, 1761.4784830658123},
                                                           {6000000, 0.0},
                                                           {42672591, 0.0},
                                                           {160097505, 970913.4558162984}};
static const std::map<double, double> LOCAL_LAT_FACTOR  = {
    {0, 0.0}, {16000, 650.2212383180362}, {6000000, 0.0}, {36900419, 0.0}, {160097505, 1017885.3518765072}};

/* bandwidth and latency used on the platform */
constexpr static double BW_REMOTE = 12.5e9;
constexpr static double BW_LOCAL  = 25e9;
constexpr static double LATENCY   = .1e-6;

/*************************************************************************************************/
/** @brief Create a simple platform based on Dahu cluster */
static void load_platform()
{
  /**
   * Inspired on dahu cluster on Grenoble
   *     ________________
   *     |               |
   *     |     dahu      |
   *     |_______________|
   *     / /   | |    \ \
   *    / /    | |     \ \     <-- 12.5GBps links
   *   / /     | |      \ \
   * host1     ...      hostN
   */

  auto* root         = sg4::create_star_zone("dahu");
  std::string prefix = "dahu-";
  std::string suffix = ".grid5000.fr";

  for (int id = 0; id < 32; id++) {
    std::string hostname = prefix + std::to_string(id) + suffix;
    /* create host */
    const sg4::Host* host = root->create_host(hostname, 1)->set_core_count(32)->seal();
    /* create UP/DOWN link */
    sg4::Link* l_up   = root->create_link(hostname + "_up", BW_REMOTE)->set_latency(LATENCY)->seal();
    sg4::Link* l_down = root->create_link(hostname + "_down", BW_REMOTE)->set_latency(LATENCY)->seal();

    /* add link UP/DOWN for communications from the host */
    root->add_route(host->get_netpoint(), nullptr, nullptr, nullptr, std::vector<sg4::Link*>{l_up}, false);
    root->add_route(nullptr, host->get_netpoint(), nullptr, nullptr, std::vector<sg4::Link*>{l_down}, false);

    sg4::Link* loopback = root->create_link(hostname + "_loopback", BW_LOCAL)->set_latency(LATENCY)->seal();
    root->add_route(host->get_netpoint(), host->get_netpoint(), nullptr, nullptr, std::vector<sg4::Link*>{loopback});
  }

  root->seal();
}

/*************************************************************************************************/
/** @brief Auxiliary method to get factor for a message size */
static double get_factor_from_map(const std::map<double, double>& factors, double size)
{
  double factor = 1.0;
  for (auto const& fact : factors) {
    if (size < fact.first) {
      break;
    } else {
      factor = fact.second;
    }
  }
  return factor;
}

/**
 * @brief Callback to set latency factor for a communication
 *
 * Set different factors for local (loopback) and remote communications.
 * Function signature is defined by API
 *
 * @param size Message size
 * @param src Host origin
 * @param dst Host destination
 */
static double latency_factor_cb(double size, const sg4::Host* src, const sg4::Host* dst,
                                const std::vector<sg4::Link*>& /*links*/,
                                const std::unordered_set<sg4::NetZone*>& /*netzones*/)
{
  if (src->get_name() == dst->get_name()) {
    /* local communication factors */
    return get_factor_from_map(LOCAL_LAT_FACTOR, size);
  } else {
    return get_factor_from_map(REMOTE_LAT_FACTOR, size);
  }
}

/**
 * @brief Callback to set bandwidth factor for a communication
 *
 * Set different factors for local (loopback) and remote communications.
 * Function signature is defined by API
 *
 * @param size Message size
 * @param src Host origin
 * @param dst Host destination
 */
static double bandwidth_factor_cb(double size, const sg4::Host* src, const sg4::Host* dst,
                                  const std::vector<sg4::Link*>& /*links*/,
                                  const std::unordered_set<sg4::NetZone*>& /*netzones*/)
{
  if (src->get_name() == dst->get_name()) {
    /* local communication factors */
    return get_factor_from_map(LOCAL_BW_FACTOR, size);
  } else {
    return get_factor_from_map(REMOTE_BW_FACTOR, size);
  }
}

/*************************************************************************************************/
class Sender {
  std::vector<sg4::Host*> hosts_;
  double crosstraffic_ = 1.0;

public:
  explicit Sender(const std::vector<sg4::Host*>& hosts, bool crosstraffic) : hosts_{hosts}
  {
    if (crosstraffic)
      crosstraffic_ = 1.05; // add crosstraffic load if it is enabled
  }
  void operator()() const
  {
    const std::vector<double> msg_sizes = {64e3, 64e6, 64e9}; // 64KB, 64MB, 64GB

    for (double size : msg_sizes) {
      for (const auto* host : hosts_) {
        std::string msg;
        /* calculating the estimated communication time depending of message size and destination */
        if (host->get_name() == sg4::this_actor::get_host()->get_name()) {
          double lat_factor = get_factor_from_map(LOCAL_LAT_FACTOR, size);
          double bw_factor  = get_factor_from_map(LOCAL_BW_FACTOR, size);
          /* Account for crosstraffic on local communications
           * local communications use only a single link and crosstraffic impact on resource sharing
           * on remote communications, we don't see this effect since we have split-duplex links */
          double est_time =
              sg4::Engine::get_clock() + size / (BW_LOCAL * bw_factor / crosstraffic_) + LATENCY * lat_factor;

          msg = "Local communication: size=" + std::to_string(size) + ". Use bw_factor=" + std::to_string(bw_factor) +
                " lat_factor=" + std::to_string(lat_factor) + ". Estimated finished time=" + std::to_string(est_time);
        } else {
          double lat_factor = get_factor_from_map(REMOTE_LAT_FACTOR, size);
          double bw_factor  = get_factor_from_map(REMOTE_BW_FACTOR, size);
          double est_time   = sg4::Engine::get_clock() + (size / (BW_REMOTE * bw_factor)) + LATENCY * lat_factor * 2;
          msg = "Remote communication: size=" + std::to_string(size) + ". Use bw_factor=" + std::to_string(bw_factor) +
                " lat_factor=" + std::to_string(lat_factor) + ". Estimated finished time=" + std::to_string(est_time);
        }

        /* Create a communication representing the ongoing communication */
        auto mbox     = sg4::Mailbox::by_name(host->get_name());
        auto* payload = new std::string(msg);
        mbox->put(payload, static_cast<uint64_t>(size));
      }
    }

    XBT_INFO("Done dispatching all messages");
    /* sending message to stop receivers */
    for (const auto* host : hosts_) {
      auto mbox = sg4::Mailbox::by_name(host->get_name());
      mbox->put(new std::string("finalize"), 0);
    }
  }
};

/* Receiver actor: wait for messages on the mailbox identified by the hostname */
class Receiver {
public:
  void operator()() const
  {
    auto mbox = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());
    // Receiving the message was all we were supposed to do
    for (bool cont = true; cont;) {
      auto received = mbox->get_unique<std::string>();
      XBT_INFO("I got a '%s'.", received->c_str());
      cont = (*received != "finalize"); // If it's a finalize message, we're done
    }
  }
};

/*************************************************************************************************/
int main(int argc, char* argv[])
{
  bool crosstraffic = true;
  sg4::Engine e(&argc, argv);
  /* setting network model to default one */
  sg4::Engine::set_config("network/model:CM02");

  /* test with crosstraffic disabled */
  if (argc == 2 && std::string(argv[1]) == "disable_crosstraffic") {
    sg4::Engine::set_config("network/crosstraffic:0");
    crosstraffic = false;
  }

  /* create platform */
  load_platform();
  /* setting network factors callbacks */
  simgrid::kernel::resource::NetworkModelIntf* model = e.get_netzone_root()->get_network_model();
  model->set_lat_factor_cb(latency_factor_cb);
  model->set_bw_factor_cb(bandwidth_factor_cb);

  sg4::Host* host        = e.host_by_name("dahu-1.grid5000.fr");
  sg4::Host* host_remote = e.host_by_name("dahu-10.grid5000.fr");
  sg4::Actor::create(std::string("receiver-local"), host, Receiver());
  sg4::Actor::create(std::string("receiver-remote"), host_remote, Receiver());
  sg4::Actor::create(std::string("sender") + std::string(host->get_name()), host,
                     Sender({host, host_remote}, crosstraffic));

  /* runs the simulation */
  e.run();

  return 0;
}
