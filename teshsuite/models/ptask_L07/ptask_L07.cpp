/* Copyright g(c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(ptask_L07_tes, "[usage] ptask_LO7 <platform-file>");

namespace sg4 = simgrid::s4u;

/* We need a separate actor so that it can sleep after each test */
static void main_dispatcher()
{
  const sg4::Engine* e = sg4::Engine::get_instance();
  double start_time;
  double end_time;
  std::vector<sg4::Host*> hosts = e->get_all_hosts();

  XBT_INFO("TEST: Create and run a sequential execution.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("Have to compute 1 flop on a 1 flop/s host.");
  XBT_INFO("Should be done in exactly one second.");
  start_time = sg4::Engine::get_clock();
  sg4::Exec::init()->set_flops_amount(1)->set_host(hosts[0])->wait();
  end_time = sg4::Engine::get_clock();
  XBT_INFO("Actual result: computing 1 flop at 1 flop/s takes %.2f seconds.", end_time - start_time);
  XBT_INFO("\n");

  sg4::this_actor::sleep_for(5);

  XBT_INFO("TEST: Create and run two concurrent sequential executions.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("Have to compute 2 x 1 flop on a 1 flop/s host.");
  XBT_INFO("Should be done in exactly 2 seconds because of sharing.");
  start_time      = sg4::Engine::get_clock();
  sg4::ExecPtr e1 = sg4::Exec::init()->set_flops_amount(1)->set_host(hosts[0])->start();
  sg4::ExecPtr e2 = sg4::Exec::init()->set_flops_amount(1)->set_host(hosts[0])->start();
  e1->wait();
  e2->wait();
  end_time = sg4::Engine::get_clock();
  XBT_INFO("Actual result: computing 2x1 flop at 1 flop/s takes %.2f seconds.", end_time - start_time);
  XBT_INFO("\n");

  sg4::this_actor::sleep_for(5);

  XBT_INFO("TEST: Create and run a parallel execution on 2 homogeneous hosts.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("Have to compute 2 flops across two hosts running at 1 flop/s.");
  XBT_INFO("Should be done in exactly one second.");
  start_time = sg4::Engine::get_clock();
  sg4::Exec::init()->set_flops_amounts(std::vector<double>({1.0, 1.0}))
                   ->set_hosts(std::vector<sg4::Host*>({hosts[0], hosts[1]}))
                   ->wait();
  end_time = sg4::Engine::get_clock();
  XBT_INFO("Actual result: computing 2 flops on 2 hosts at 1 flop/s takes %.2f seconds.", end_time - start_time);
  XBT_INFO("\n");

  sg4::this_actor::sleep_for(5);

  XBT_INFO("TEST: Create and run a parallel execution on 2 heterogeneous hosts.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("Have to compute 2 flops across two hosts, one running at 1 flop/s and one at 2 flop/s.");
  XBT_INFO("Should be done in exactly one second.");
  start_time = sg4::Engine::get_clock();
  sg4::Exec::init()->set_flops_amounts(std::vector<double>({1.0, 1.0}))
                   ->set_hosts(std::vector<sg4::Host*>({hosts[0], hosts[5]}))
                   ->wait();
  end_time = sg4::Engine::get_clock();
  XBT_INFO("Actual result: computing 2 flops on 2 heterogeneous hosts takes %.2f seconds.", end_time - start_time);
  XBT_INFO("\n");

  sg4::this_actor::sleep_for(5);

  XBT_INFO("TEST: Latency test between hosts connected by a shared link.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("Have to send 1B from one host to another at 1Bps with a latency of 500ms.");
  XBT_INFO("Should be done in 1.5 seconds (500ms latency + 1s transfert).");
  start_time = sg4::Engine::get_clock();
  sg4::Comm::sendto_async(hosts[0], hosts[4], 1.0)->wait();
  end_time = sg4::Engine::get_clock();
  XBT_INFO("Actual result: sending 1 byte on a shared link at 1Bps + 500ms takes %.2f seconds.", end_time - start_time);
  XBT_INFO("\n");

  sg4::this_actor::sleep_for(5);

  XBT_INFO("TEST: Latency test between hosts connected by a fatpipe link.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("Have to send 1B from one host to another at 1Bps with a latency of 500ms.");
  XBT_INFO("Should be done in 1.5 seconds (500ms latency + 1s transfert).");
  start_time = sg4::Engine::get_clock();
  sg4::Comm::sendto_async(hosts[0], hosts[5], 1.0)->wait();
  end_time = sg4::Engine::get_clock();
  XBT_INFO("Actual result: sending 1 byte on a fatpipe link at 1Bps + 500ms takes %.2f seconds.", end_time - start_time);
  XBT_INFO("\n");

  sg4::this_actor::sleep_for(5);

  XBT_INFO("TEST: Latency test between hosts connected by a 3-link route.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("Have to send 1B from one host to another at 1Bps with a latency of 2 x 500ms + 1s.");
  XBT_INFO("Should be done in 3 seconds (2 x 500ms + 1s latency + 1s transfert).");
  start_time = sg4::Engine::get_clock();
  sg4::Comm::sendto_async(hosts[0], hosts[1], 1.0)->wait();
  end_time = sg4::Engine::get_clock();
  XBT_INFO("Actual result: sending 1 byte on a 3-link route at 1Bps + 2,500ms takes %.2f seconds.", end_time - start_time);
  XBT_INFO("\n");

  sg4::this_actor::sleep_for(5);

  XBT_INFO("TEST: Latency test between hosts connected by a link with large latency.");
  XBT_INFO("------------------------------------------------------------");
  XBT_INFO("Have to send 1B from one host to another on a link at 2Bps with a latency of 2 x 1024^2s.");
  XBT_INFO("This latency is half the default TCP window size (4MiB). This limits the bandwidth to 1B");
  XBT_INFO("Should be done in 2 x 1024^2s + 1 seconds (large latency + 1s transfert).");
  start_time = sg4::Engine::get_clock();
  sg4::Comm::sendto_async(hosts[0], hosts[6], 1.0)->wait();
  end_time = sg4::Engine::get_clock();
  XBT_INFO("Actual result: sending 1 byte on a large latency link takes %.2f seconds.", end_time - start_time);
  XBT_INFO("\n");

  sg4::this_actor::sleep_for(5);

   XBT_INFO("TEST: Latency test between hosts connected by a shared link with 2 comms in same direction.");
   XBT_INFO("------------------------------------------------------------");
   XBT_INFO("Have to send 2 x 1B from one host to another at 1Bps with a latency of 500ms.");
   XBT_INFO("Should be done in 2.5 seconds (500ms latency + 2s transfert).");
   start_time      = sg4::Engine::get_clock();
   sg4::CommPtr c1 = sg4::Comm::sendto_async(hosts[0], hosts[4], 1.0);
   sg4::CommPtr c2 = sg4::Comm::sendto_async(hosts[0], hosts[4], 1.0);
   c1->wait();
   c2->wait();
   end_time = sg4::Engine::get_clock();
   XBT_INFO("Actual result: sending 2x1 bytes on a shared link at 1Bps + 500ms takes %.2f seconds.", end_time - start_time);
   XBT_INFO("\n");

   sg4::this_actor::sleep_for(5);

   XBT_INFO("TEST: Latency test between hosts connected by a fatpipe link with 2 comms in same direction.");
   XBT_INFO("------------------------------------------------------------");
   XBT_INFO("Have to send 2 x 1B from one host to another at 1Bps with a latency of 500ms.");
   XBT_INFO("Should be done in 1.5 seconds (500ms latency + 1s transfert).");
   start_time = sg4::Engine::get_clock();
   c1 = sg4::Comm::sendto_async(hosts[0], hosts[5], 1.0);
   c2 = sg4::Comm::sendto_async(hosts[0], hosts[5], 1.0);
   c1->wait();
   c2->wait();
   end_time = sg4::Engine::get_clock();
   XBT_INFO("Actual result: sending 2x1 bytes on a fatpipe link at 1Bps + 500ms takes %.2f seconds.", end_time - start_time);
   XBT_INFO("\n");

   sg4::this_actor::sleep_for(5);

   XBT_INFO("TEST: Latency test between hosts connected by a 3-link route with 2 comms in same direction.");
   XBT_INFO("------------------------------------------------------------");
   XBT_INFO("Have to send 2 x 1B from one host to another at 1Bps with a latency of 2 x 500ms + 1s.");
   XBT_INFO("Should be done in 4 seconds (2 x 500ms + 1s latency + 2s transfert).");
   start_time = sg4::Engine::get_clock();
   c1 = sg4::Comm::sendto_async(hosts[0], hosts[1], 1.0);
   c2 = sg4::Comm::sendto_async(hosts[0], hosts[1], 1.0);
   c1->wait();
   c2->wait();
   end_time = sg4::Engine::get_clock();
   XBT_INFO("Actual result: sending 2x1 bytes on a 3-link route at 1Bps + 2,500ms takes %.2f seconds.",
            end_time - start_time);
   XBT_INFO("\n");

   sg4::this_actor::sleep_for(5);

   XBT_INFO("TEST: Latency test between hosts connected by a shared link with 2 comms in opposite direction.");
   XBT_INFO("------------------------------------------------------------");
   XBT_INFO("Have to send 1B between two hosts in each direction at 1Bps with a latency of 500ms.");
   XBT_INFO("Should be done in 2.5 seconds (500ms latency + 2s transfert).");
   start_time = sg4::Engine::get_clock();
   c1 = sg4::Comm::sendto_async(hosts[0], hosts[4], 1.0);
   c2 = sg4::Comm::sendto_async(hosts[4], hosts[0], 1.0);
   c1->wait();
   c2->wait();
   end_time = sg4::Engine::get_clock();
   XBT_INFO("Actual result: sending 1 byte in both directions on a shared link at 1Bps + 500ms takes %.2f seconds.",
            end_time - start_time);
   XBT_INFO("\n");

   sg4::this_actor::sleep_for(5);

   XBT_INFO("TEST: Latency test between hosts connected by a fatpipe link with 2 comms in opposite direction.");
   XBT_INFO("------------------------------------------------------------");
   XBT_INFO("Have to send 1B between two hosts in each direction at 1Bps with a latency of 500ms.");
   XBT_INFO("Should be done in 1.5 seconds (500ms latency + 1s transfert).");
   start_time = sg4::Engine::get_clock();
   c1 = sg4::Comm::sendto_async(hosts[0], hosts[5], 1.0);
   c2 = sg4::Comm::sendto_async(hosts[5], hosts[0], 1.0);
   c1->wait();
   c2->wait();
   end_time = sg4::Engine::get_clock();
   XBT_INFO("Actual result: sending 1 byte in both directions on a fatpipe link at 1Bps + 500ms takes %.2f seconds.",
            end_time - start_time);
   XBT_INFO("\n");

   sg4::this_actor::sleep_for(5);

   XBT_INFO("TEST: Latency test between hosts connected by a 3-link route with 2 comms in opposite direction.");
   XBT_INFO("------------------------------------------------------------");
   XBT_INFO("Have to send 1B between two hosts in each direction at 1Bps with a latency of 2 x 500ms + 1s.");
   XBT_INFO("Should be done in 4 seconds (2 x 500ms + 1s latency + 2s transfert).");
   start_time = sg4::Engine::get_clock();
   c1 = sg4::Comm::sendto_async(hosts[0], hosts[1], 1.0);
   c2 = sg4::Comm::sendto_async(hosts[1], hosts[0], 1.0);
   c1->wait();
   c2->wait();
   end_time = sg4::Engine::get_clock();
   XBT_INFO("Actual result: sending 1 byte in both directions on a 3-link route at 1Bps + 2,500ms takes %.2f seconds.",
             end_time - start_time);
   XBT_INFO("\n");

   sg4::this_actor::sleep_for(5);

   XBT_INFO("TEST: 4-host parallel communication with independent transfers.");
   XBT_INFO("------------------------------------------------------------");
   XBT_INFO("'cpu0' sends 1B to 'cpu1' and 'cpu2' sends 1B to 'cpu3'. The only shared link is the fatpipe switch.");
   XBT_INFO("Should be done in 3 seconds (2 x 500ms + 1s latency + 1s transfert).");
   start_time = sg4::Engine::get_clock();
   sg4::Exec::init()->set_bytes_amounts(std::vector<double>({0.0, 1.0, 0.0, 0.0,
                                                             0.0, 0.0, 0.0, 0.0,
                                                             0.0, 0.0, 0.0, 1.0,
                                                             0.0, 0.0, 0.0, 0.0 }))
                    ->set_hosts(std::vector<sg4::Host*>({hosts[0], hosts[1], hosts[2], hosts[3]}))
                    ->wait();
   end_time = sg4::Engine::get_clock();
   XBT_INFO("Actual result: sending 2 x 1 byte in a parallel communication without interference takes %.2f seconds.",
             end_time - start_time);
   XBT_INFO("\n");

   sg4::this_actor::sleep_for(5);

   XBT_INFO("TEST: 4-host parallel communication with scatter pattern.");
   XBT_INFO("------------------------------------------------------------");
   XBT_INFO("'cpu0' sends 1B to 'cpu1', 2B to 'cpu2' and 3B to 'cpu3'.");
   XBT_INFO("Should be done in 8 seconds: 2 x 500ms + 1s of initial latency and :");
   XBT_INFO(" - For 3 seconds, three flows share a link to transfer 3 x 1B. 'cpu1' received its payload");
   XBT_INFO(" - For 2 seconds, two lows share a link to transfer 1 x 1B. 'cpu2' received is payload");
   XBT_INFO(" - For 1 second, one flow has the full bandwidth to transfer 1B. 'cpu3' received is payload");

   start_time = sg4::Engine::get_clock();
   sg4::Exec::init()->set_bytes_amounts(std::vector<double>({0.0, 1.0, 2.0, 3.0,
                                                             0.0, 0.0, 0.0, 0.0,
                                                             0.0, 0.0, 0.0, 0.0,
                                                             0.0, 0.0, 0.0, 0.0 }))
                    ->set_hosts(std::vector<sg4::Host*>({hosts[0], hosts[1], hosts[2], hosts[3]}))
                    ->wait();
   end_time = sg4::Engine::get_clock();
   XBT_INFO("Actual result: scattering an increasing number of bytes to 3 hosts takes %.2f seconds.",
             end_time - start_time);
   XBT_INFO("\n");

   sg4::this_actor::sleep_for(5);

   XBT_INFO("TEST: 4-host parallel communication with all-to-all pattern.");
   XBT_INFO("------------------------------------------------------------");
   XBT_INFO("Each host sends 1B to every other hosts.");
   XBT_INFO("Should be done in 8 seconds: 2 x 500ms + 1s of initial latency and 6 seconds for transfer");
   XBT_INFO("Each SHARED link is traversed by 6 flows (3 in and 3 out). ");
   XBT_INFO("Each 1B transfer thus takes 6 seconds on a 1Bps link");

   start_time = sg4::Engine::get_clock();
   sg4::Exec::init()->set_bytes_amounts(std::vector<double>({0.0, 1.0, 1.0, 1.0,
                                                             1.0, 0.0, 1.0, 1.0,
                                                             1.0, 1.0, 0.0, 1.0,
                                                             1.0, 1.0, 1.0, 0.0 }))
                    ->set_hosts(std::vector<sg4::Host*>({hosts[0], hosts[1], hosts[2], hosts[3]}))
                    ->wait();
   end_time = sg4::Engine::get_clock();
   XBT_INFO("Actual result: 1-byte all-too-all in a parallel communication takes %.2f seconds.",
             end_time - start_time);
   XBT_INFO("\n");
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine engine(&argc, argv);
  engine.load_platform(argv[1]);
  simgrid::s4u::Actor::create("dispatcher", simgrid::s4u::Host::by_name("cpu0"), main_dispatcher);
  engine.run();

  return 0;
}

