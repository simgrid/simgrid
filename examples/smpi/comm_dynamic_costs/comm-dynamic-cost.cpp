/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
#include <smpi/smpi.h>
namespace sg4 = simgrid::s4u;

/**
 * @brief User's callback to set dynamic cost for MPI operations
 *
 * Note that src host can be a nullptr for some collective operations.
 * This is due the creation of an special requests that aggregates sub-requests
 * in collective operations. But it doesn't happen in this example
 *
 * @param op MPI Operation (set by user at cb registering below)
 * @param size Message size (set by simgrid)
 * @param src Source host (set by simgrid)
 * @param dst Source host (set by simgrid)
 */
static double smpi_cost_cb(SmpiOperation op, size_t /*size*/, const sg4::Host* src, const sg4::Host* dst)
{
  /* some dummy cost that depends on the operation and host */
  static std::unordered_map<std::string, double> read_cost  = {{"Tremblay", 1}, {"Jupiter", 2}};
  static std::unordered_map<std::string, double> write_cost = {{"Tremblay", 5}, {"Jupiter", 10}};

  if (op == SmpiOperation::RECV)
    return read_cost[src->get_name()];

  return write_cost[dst->get_name()];
}

/*
 * Creates a platform for examples/smpi/simple-execute/simple-execute.c MPI program
 *
 * Sets specific cost for MPI_Send and MPI_Recv operations
 */
extern "C" void load_platform(const sg4::Engine& e);
void load_platform(const sg4::Engine& /*e*/)
{
  /* create a simple 2 host platform inspired from small_platform.xml */
  auto* root = sg4::create_full_zone("zone0");

  const sg4::Host* tremblay = root->create_host("Tremblay", "98.095Mf")->seal();
  const sg4::Host* jupiter  = root->create_host("Jupiter", "76.296Mf")->seal();

  const sg4::Link* link9 = root->create_split_duplex_link("9", "7.20975MBps")->set_latency("1.461517ms")->seal();

  root->add_route(tremblay->get_netpoint(), jupiter->get_netpoint(), nullptr, nullptr,
                  {{link9, sg4::LinkInRoute::Direction::UP}}, true);
  root->seal();

  /* set cost callback for MPI_Send and MPI_Recv */
  smpi_register_op_cost_callback(SmpiOperation::SEND,
                                 std::bind(&smpi_cost_cb, SmpiOperation::SEND, std::placeholders::_1,
                                           std::placeholders::_2, std::placeholders::_3));

  smpi_register_op_cost_callback(SmpiOperation::RECV,
                                 std::bind(&smpi_cost_cb, SmpiOperation::RECV, std::placeholders::_1,
                                           std::placeholders::_2, std::placeholders::_3));
}
