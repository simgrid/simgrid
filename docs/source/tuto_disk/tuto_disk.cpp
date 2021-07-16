/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This tutorial presents how to faithful emulate disk operation in SimGrid
 */

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <random>
#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(disk_test, "Messages specific for this simulation");

/** @brief Calculates the bandwidth for disk doing async operations */
static void estimate_bw(const sg4::Disk* disk, int n, int n_flows, bool read)
{
  unsigned long long size = 100000;
  double cur_time         = sg4::Engine::get_clock();
  std::vector<sg4::IoPtr> activities;
  for (int i = 0; i < n_flows; i++) {
    sg4::IoPtr act;
    if (read)
      act = disk->read_async(size);
    else
      act = disk->write_async(size);

    activities.push_back(act);
  }

  for (const auto& act : activities)
    act->wait();

  double elapsed_time = sg4::Engine::get_clock() - cur_time;
  printf("%s,%s,%d,%d,%d,%lf\n", disk->get_cname(), read ? "read" : "write", n, n_flows, size, elapsed_time);
}

static void host()
{
  /* - Estimating bw for each disk and considering concurrent flows */
  for (int i = 0; i < 20; i++) {
    for (int flows = 1; flows <= 15; flows++) {
      for (auto* disk : sg4::Host::current()->get_disks()) {
        estimate_bw(disk, i, flows, true);
        estimate_bw(disk, i, flows, false);
      }
    }
  }
}

/*************************************************************************************************/
/** @brief Auxiliary class to generate noise in disk operations */
class DiskNoise {
  double bw_;
  std::vector<double> breaks_;
  std::vector<double> heights_;
  std::mt19937& gen_;

public:
  DiskNoise(double capacity, std::mt19937& gen, const std::vector<double>& b, const std::vector<double> h)
      : bw_(capacity), breaks_(b), heights_(h), gen_(gen)
  {
  }
  double operator()(sg_size_t /*size*/) const
  {
    std::piecewise_constant_distribution<double> d(breaks_.begin(), breaks_.end(), heights_.begin());
    auto value = d(gen_);
    return value / bw_;
  }
};

/** @brief Auxiliary method to get list of values from json in a vector */
static std::vector<double> get_list_from_json(const boost::property_tree::ptree& pt, const std::string& path)
{
  std::vector<double> v;
  for (const auto& it : pt.get_child(path)) {
    double value = it.second.get_value<double>();
    v.push_back(value * 1e6);
  }
  return v;
}
/*************************************************************************************************/
/**
 * @brief Non-linear resource callback for disks
 *
 * @param degradation Vector with effective read/write bandwidth
 * @param capacity Resource current capacity in SimGrid
 * @param n Number of activities sharing this resource
 */
static double disk_dynamic_sharing(const std::vector<double>& degradation, double capacity, int n)
{
  n--;
  if (n >= degradation.size())
    return capacity;
  return degradation[n];
}

/**
 * @brief Noise for I/O operations
 *
 * @param data Map with noise information
 * @param size I/O size in bytes
 * @param op I/O operation: read/write
 */
static double disk_variability(const std::unordered_map<sg4::Io::OpType, DiskNoise>& data, sg_size_t size,
                               sg4::Io::OpType op)
{
  auto it = data.find(op);
  if (it == data.end())
    return 1.0;
  double value = it->second(size);
  return value;
}

/** @brief Creates a disk */
static void create_disk(sg4::Host* host, std::mt19937& gen, const std::string& disk_name,
                        const boost::property_tree::ptree& pt)
{

  double read_bw                = pt.get_child("read_bw").begin()->second.get_value<double>() * 1e6;
  double write_bw               = pt.get_child("write_bw").begin()->second.get_value<double>() * 1e6;
  auto* disk                    = host->create_disk(disk_name, read_bw, write_bw);
  std::vector<double> read_deg  = get_list_from_json(pt, "degradation.read");
  std::vector<double> write_deg = get_list_from_json(pt, "degradation.write");

  /* get maximum possible disk speed for read/write disk constraint */
  double max_bw = std::max(*std::max_element(read_deg.begin(), read_deg.end()),
                           *std::max_element(write_deg.begin(), write_deg.end()));
  disk->set_readwrite_bandwidth(max_bw);

  disk->set_sharing_policy(sg4::Disk::Operation::READ, sg4::Disk::SharingPolicy::NONLINEAR,
                           std::bind(&disk_dynamic_sharing, read_deg, std::placeholders::_1, std::placeholders::_2));
  disk->set_sharing_policy(sg4::Disk::Operation::WRITE, sg4::Disk::SharingPolicy::NONLINEAR,
                           std::bind(&disk_dynamic_sharing, write_deg, std::placeholders::_1, std::placeholders::_2));
  /* this is the default behavior, expliciting only to make it clearer */
  disk->set_sharing_policy(sg4::Disk::Operation::READWRITE, sg4::Disk::SharingPolicy::LINEAR);

  /* configuring variability */
  DiskNoise read_noise(read_bw, gen, get_list_from_json(pt, "noise.read.breaks"),
                       get_list_from_json(pt, "noise.read.heights"));
  DiskNoise write_noise(write_bw, gen, get_list_from_json(pt, "noise.write.breaks"),
                        get_list_from_json(pt, "noise.write.heights"));

  std::unordered_map<sg4::Io::OpType, DiskNoise> noise{{sg4::Io::OpType::READ, read_noise},
                                                       {sg4::Io::OpType::WRITE, write_noise}};
  disk->set_factor_cb(std::bind(&disk_variability, noise, std::placeholders::_1, std::placeholders::_2));
  disk->seal();
}

/*************************************************************************************************/
int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);
  std::mt19937 gen(42);
  /* simple platform containing 1 host and 2 disk */
  auto* zone = sg4::create_full_zone("bob_zone");
  auto* bob  = zone->create_host("bob", 1e6);
  std::ifstream jsonFile("IO_noise.json");
  boost::property_tree::ptree pt;
  boost::property_tree::read_json(jsonFile, pt);
  for (const auto& it : pt.get_child("")) {
    create_disk(bob, gen, it.first, it.second);
  }
  zone->seal();

  sg4::Actor::create("", bob, host);

  printf("disk,op,n,flows,size,elapsed\n");

  e.run();

  return 0;
}
