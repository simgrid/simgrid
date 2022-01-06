/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to simulate a non-linear resource sharing for disk
 * operations.
 *
 * It is inspired on the paper
 * "Adding Storage Simulation Capacities to the SimGridToolkit: Concepts, Models, and API"
 * Available at : https://hal.inria.fr/hal-01197128/document
 *
 * It shows how to simulate concurrent operations degrading overall performance of IO
 * operations (specifically the effects presented in Fig. 8 of the paper).
 */

#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(disk_test, "Messages specific for this simulation");

/** @brief Calculates the bandwidth for disk doing async operations */
static void estimate_bw(const sg4::Disk* disk, int n_flows, bool read)
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
  double estimated_bw = static_cast<double>(size * n_flows) / elapsed_time;
  XBT_INFO("Disk: %s, concurrent %s: %d, estimated bandwidth: %lf", disk->get_cname(), read ? "read" : "write", n_flows,
           estimated_bw);
}

static void host()
{
  /* - Estimating bw for each disk and considering concurrent flows */
  for (int n = 1; n < 15; n += 2) {
    for (const auto* disk : sg4::Host::current()->get_disks()) {
      estimate_bw(disk, n, true);
      estimate_bw(disk, n, false);
    }
  }
}

/**
 * @brief Non-linear resource callback for SSD disks
 *
 * In this case, we have measurements for some resource sharing and directly use them to return the
 * correct value
 * @param disk Disk on which the operation is happening (defined by the user through the std::bind)
 * @param op read or write operation (defined by the user through the std::bind)
 * @param capacity Resource current capacity in SimGrid
 * @param n Number of activities sharing this resource
 */
static double ssd_dynamic_sharing(const sg4::Disk* /*disk*/, const std::string& op, double capacity, int n)
{
  /* measurements for SSD disks */
  using DiskCapacity                                                   = std::unordered_map<int, double>;
  static const std::unordered_map<std::string, DiskCapacity> SSD_SPEED = {{"write", {{1, 131.}}},
                                                                          {"read",
                                                                           {{1, 152.},
                                                                            {2, 161.},
                                                                            {3, 184.},
                                                                            {4, 197.},
                                                                            {5, 207.},
                                                                            {6, 215.},
                                                                            {7, 220.},
                                                                            {8, 224.},
                                                                            {9, 227.},
                                                                            {10, 231.},
                                                                            {11, 233.},
                                                                            {12, 235.},
                                                                            {13, 237.},
                                                                            {14, 238.},
                                                                            {15, 239.}}}};

  const auto& data = SSD_SPEED.at(op);
  /* no special bandwidth for this disk sharing N flows, just returns maximal capacity */
  if (data.find(n) != data.end())
    capacity = data.at(n);

  return capacity;
}

/**
 * @brief Non-linear resource callback for SATA disks
 *
 * In this case, the degradation for read operations is linear and we have a formula that represents it.
 *
 * @param disk Disk on which the operation is happening (defined by the user through the std::bind)
 * @param capacity Resource current capacity in SimGrid
 * @param n Number of activities sharing this resource
 */
static double sata_dynamic_sharing(const sg4::Disk* /*disk*/, double /*capacity*/, int n)
{
  return 68.3 - 1.7 * n;
}

/** @brief Creates an SSD disk, setting the appropriate callback for non-linear resource sharing */
static void create_ssd_disk(sg4::Host* host, const std::string& disk_name)
{
  auto* disk = host->create_disk(disk_name, "240MBps", "170MBps");
  disk->set_sharing_policy(sg4::Disk::Operation::READ, sg4::Disk::SharingPolicy::NONLINEAR,
                           std::bind(&ssd_dynamic_sharing, disk, "read", std::placeholders::_1, std::placeholders::_2));
  disk->set_sharing_policy(
      sg4::Disk::Operation::WRITE, sg4::Disk::SharingPolicy::NONLINEAR,
      std::bind(&ssd_dynamic_sharing, disk, "write", std::placeholders::_1, std::placeholders::_2));
  disk->set_sharing_policy(sg4::Disk::Operation::READWRITE, sg4::Disk::SharingPolicy::LINEAR);
}

/** @brief Same for a SATA disk, only read operation follows a non-linear resource sharing */
static void create_sata_disk(sg4::Host* host, const std::string& disk_name)
{
  auto* disk = host->create_disk(disk_name, "68MBps", "50MBps");
  disk->set_sharing_policy(sg4::Disk::Operation::READ, sg4::Disk::SharingPolicy::NONLINEAR,
                           std::bind(&sata_dynamic_sharing, disk, std::placeholders::_1, std::placeholders::_2));
  /* this is the default behavior, expliciting only to make it clearer */
  disk->set_sharing_policy(sg4::Disk::Operation::WRITE, sg4::Disk::SharingPolicy::LINEAR);
  disk->set_sharing_policy(sg4::Disk::Operation::READWRITE, sg4::Disk::SharingPolicy::LINEAR);
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);
  /* simple platform containing 1 host and 2 disk */
  auto* zone = sg4::create_full_zone("bob_zone");
  auto* bob  = zone->create_host("bob", 1e6);
  create_ssd_disk(bob, "Edel (SSD)");
  create_sata_disk(bob, "Griffon (SATA II)");
  zone->seal();

  simgrid::s4u::Actor::create("", bob, host);

  e.run();
  XBT_INFO("Simulated time: %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
