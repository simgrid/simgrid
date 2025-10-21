/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/VirtualMachine.hpp>
#include <xbt/graph.h>

#include "src/instr/instr_paje_containers.hpp"
#include "src/instr/instr_paje_types.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/kernel/resource/NetworkModel.hpp"
#include "xbt/log.h"

#include <fstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_routing, instr, "Tracing platform hierarchy");

std::string instr_pid(simgrid::s4u::Actor const& proc)
{
  return proc.get_name() + "-" + std::to_string(proc.get_pid());
}

static simgrid::instr::Container* lowestCommonAncestor(const simgrid::instr::Container* a1,
                                                       const simgrid::instr::Container* a2)
{
  // this is only an optimization (since most of a1 and a2 share the same parent)
  if (a1->get_parent() == a2->get_parent())
    return a1->get_parent();

  // create an array with all ancestors of a1
  std::vector<simgrid::instr::Container*> ancestors_a1;
  for (auto* p = a1->get_parent(); p != nullptr; p = p->get_parent())
    ancestors_a1.push_back(p);

  // create an array with all ancestors of a2
  std::vector<simgrid::instr::Container*> ancestors_a2;
  for (auto* p = a2->get_parent(); p != nullptr; p = p->get_parent())
    ancestors_a2.push_back(p);

  // find the lowest ancestor
  simgrid::instr::Container* p = nullptr;
  int i = static_cast<int>(ancestors_a1.size()) - 1;
  int j = static_cast<int>(ancestors_a2.size()) - 1;
  while (i >= 0 && j >= 0) {
    simgrid::instr::Container* a1p       = ancestors_a1.at(i);
    if (a1p != ancestors_a2.at(j))
      break;
    p = a1p;
    i--;
    j--;
  }
  return p;
}

static void linkContainers(simgrid::instr::Container* src, simgrid::instr::Container* dst,
                           std::set<std::string, std::less<>>* filter)
{
  // ignore loopback
  if (src->get_name() == "__loopback__" || dst->get_name() == "__loopback__") {
    XBT_DEBUG("  linkContainers: ignoring loopback link");
    return;
  }

  // find common parent
  simgrid::instr::Container* parent = lowestCommonAncestor(src, dst);
  xbt_assert(parent, "common parent unknown, this is a tracing problem");

  // check if we already register this pair (we only need one direction)
  std::string aux1 = src->get_name() + dst->get_name();
  std::string aux2 = dst->get_name() + src->get_name();
  if (filter->find(aux1) != filter->end()) {
    XBT_DEBUG("  linkContainers: already registered %s <-> %s (1)", src->get_cname(), dst->get_cname());
    return;
  }
  if (filter->find(aux2) != filter->end()) {
    XBT_DEBUG("  linkContainers: already registered %s <-> %s (2)", dst->get_cname(), src->get_cname());
    return;
  }

  // ok, not found, register it
  filter->insert(aux1);
  filter->insert(aux2);

  // declare type
  std::string link_typename = parent->get_type()->get_name() + "-" + src->get_type()->get_name() +
                              std::to_string(src->get_type()->get_id()) + "-" + dst->get_type()->get_name() +
                              std::to_string(dst->get_type()->get_id());
  simgrid::instr::LinkType* link =
      parent->get_type()->by_name_or_create(link_typename, src->get_type(), dst->get_type());
  link->set_calling_container(parent);

  // create the link
  static long long counter = 0;

  std::string key = std::to_string(counter);
  counter++;

  link->start_event(src, "topology", key);
  link->end_event(dst, "topology", key);

  XBT_DEBUG("  linkContainers %s <-> %s", src->get_cname(), dst->get_cname());
}

static void recursiveGraphExtraction(const simgrid::s4u::NetZone* netzone, const simgrid::instr::Container* container,
                                     std::set<std::string, std::less<>>* filter)
{
  if (not TRACE_platform_topology()) {
    XBT_DEBUG("Graph extraction disabled by user.");
    return;
  }
  XBT_DEBUG("Graph extraction for NetZone = %s", netzone->get_cname());

  // bottom-up recursion
  for (auto const& nz_son : netzone->get_children()) {
    const simgrid::instr::Container* child_container = container->get_child_by_name(nz_son->get_name());
    recursiveGraphExtraction(nz_son, child_container, filter);
  }

  auto* graph = xbt_graph_new_graph(0, nullptr);
  std::map<std::string, xbt_node_t, std::less<>> nodes;
  std::map<std::string, xbt_edge_t, std::less<>> edges;

  netzone->get_impl()->get_graph(graph, &nodes, &edges);
  for (auto const& [_, edge] : edges) {
    linkContainers(simgrid::instr::Container::by_name(static_cast<const char*>(edge->src->data)),
                   simgrid::instr::Container::by_name(static_cast<const char*>(edge->dst->data)), filter);
  }
  xbt_graph_free_graph(graph, xbt_free_f, xbt_free_f, nullptr);
}

/*
 * user categories support
 */
static void recursiveNewVariableType(const std::string& new_typename, const std::string& color,
                                     simgrid::instr::Type* root)
{
  if (root->get_name() == "HOST" || root->get_name() == "VM")
    root->by_name_or_create("p" + new_typename, color);

  if (root->get_name() == "LINK")
    root->by_name_or_create("b" + new_typename, color);

  for (auto const& [_, child] : root->get_children()) {
    recursiveNewVariableType(new_typename, color, child.get());
  }
}

void instr_new_variable_type(const std::string& new_typename, const std::string& color)
{
  recursiveNewVariableType(new_typename, color, simgrid::instr::Container::get_root()->get_type());
}

static void recursiveNewUserVariableType(const std::string& parent_type, const std::string& new_typename,
                                         const std::string& color, simgrid::instr::Type* root)
{
  if (root->get_name() == parent_type) {
    root->by_name_or_create(new_typename, color);
  }
  for (auto const& [_, child] : root->get_children())
    recursiveNewUserVariableType(parent_type, new_typename, color, child.get());
}

void instr_new_user_variable_type(const std::string& parent_type, const std::string& new_typename,
                                  const std::string& color)
{
  recursiveNewUserVariableType(parent_type, new_typename, color, simgrid::instr::Container::get_root()->get_type());
}

static void recursiveNewUserStateType(const std::string& parent_type, const std::string& new_typename,
                                      simgrid::instr::Type* root)
{
  if (root->get_name() == parent_type)
    root->by_name_or_create<simgrid::instr::StateType>(new_typename);

  for (auto const& [_, child] : root->get_children())
    recursiveNewUserStateType(parent_type, new_typename, child.get());
}

void instr_new_user_state_type(const std::string& parent_type, const std::string& new_typename)
{
  recursiveNewUserStateType(parent_type, new_typename, simgrid::instr::Container::get_root()->get_type());
}

static void recursiveNewValueForUserStateType(const std::string& type_name, const char* val, const std::string& color,
                                              simgrid::instr::Type* root)
{
  if (root->get_name() == type_name)
    static_cast<simgrid::instr::StateType*>(root)->add_entity_value(val, color);

  for (auto const& [_, child] : root->get_children())
    recursiveNewValueForUserStateType(type_name, val, color, child.get());
}

void instr_new_value_for_user_state_type(const std::string& type_name, const char* value, const std::string& color)
{
  recursiveNewValueForUserStateType(type_name, value, color, simgrid::instr::Container::get_root()->get_type());
}

namespace simgrid::instr {

/** @brief Creates a file with the topology of the platform file used for the simulator.
 *
 *  The graph topology will have the following properties: all hosts, links and routers of the platform file are mapped
 *  to graph nodes; routes are mapped to edges. The platform's zones are not represented in the output.
 */
void platform_graph_export_graphviz(const std::string& output_filename)
{
  auto* g     = xbt_graph_new_graph(0, nullptr);
  std::map<std::string, xbt_node_t, std::less<>> nodes;
  std::map<std::string, xbt_edge_t, std::less<>> edges;
  s4u::Engine::get_instance()->get_netzone_root()->extract_xbt_graph(g, &nodes, &edges);

  std::ofstream fs;
  fs.open(output_filename, std::ofstream::out);
  xbt_assert(not fs.fail(), "Failed to open %s", output_filename.c_str());

  if (g->directed)
    fs << "digraph test {\n";
  else
    fs << "graph test {\n";

  fs << "  graph [overlap=scale]\n";

  fs << "  node [shape=box, style=filled]\n";
  fs << "  node [width=.3, height=.3, style=filled, color=skyblue]\n\n";

  for (auto const& [node, _] : nodes)
    fs << "  \"" << node << "\";\n";

  for (auto const& [_, edge] : edges) {
    const char* src_s = static_cast<char*>(edge->src->data);
    const char* dst_s = static_cast<char*>(edge->dst->data);
    if (g->directed)
      fs << "  \"" << src_s << "\" -> \"" << dst_s << "\";\n";
    else
      fs << "  \"" << src_s << "\" -- \"" << dst_s << "\";\n";
  }
  fs << "}\n";
  fs.close();

  xbt_graph_free_graph(g, xbt_free_f, xbt_free_f, nullptr);
}

void platform_graph_export_csv(const std::string& output_filename)
{
  auto* g         = xbt_graph_new_graph(0, nullptr);
  std::map<std::string, xbt_node_t, std::less<>> nodes;
  std::map<std::string, xbt_edge_t, std::less<>> edges;
  s4u::Engine::get_instance()->get_netzone_root()->extract_xbt_graph(g, &nodes, &edges);

  std::ofstream fs;
  fs.open(output_filename, std::ofstream::out);
  xbt_assert(not fs.fail(), "Failed to open %s", output_filename.c_str());

  fs << "src,dst" << std::endl;
  for (auto const& [_, edge] : edges) {
    const char* src_s = static_cast<char*>(edge->src->data);
    const char* dst_s = static_cast<char*>(edge->dst->data);
    fs << src_s << "," << dst_s << "\n";
  }
  fs.close();
  xbt_graph_free_graph(g, xbt_free_f, xbt_free_f, nullptr);
}

/* Callbacks */
static std::vector<NetZoneContainer*> currentContainer; /* push and pop, used only in creation */
static void on_netzone_creation(s4u::NetZone const& netzone)
{
  std::string id = netzone.get_name();
  if (Container::get_root() == nullptr) {
    auto* root = new NetZoneContainer(id, 0, nullptr);
    xbt_assert(Container::get_root() == root);

    if (TRACE_needs_platform())
      currentContainer.push_back(root);

    return;
  }

  // This static is ugly, but I fail to make it work otherwise
  static bool inited = false;
  if (TRACE_smpi_is_enabled() && not inited) {
    inited = true;

    auto root = Container::get_root();
    auto* mpi = root->get_type()->by_name_or_create<ContainerType>("MPI");
    if (not TRACE_smpi_is_grouped())
      mpi->by_name_or_create<StateType>("MPI_STATE");
    root->get_type()->by_name_or_create("MPI_LINK", mpi, mpi);
    root->get_type()->by_name_or_create("MIGRATE_LINK", mpi, mpi);
    mpi->by_name_or_create<StateType>("MIGRATE_STATE");
  }

  if (TRACE_needs_platform()) {
    auto level      = static_cast<unsigned>(currentContainer.size());
    auto* container = new NetZoneContainer(id, level, currentContainer.back());
    currentContainer.push_back(container);
  }
}

static void on_link_creation(s4u::Link const& link)
{
  if (link.get_name() == "__loopback__") // Don't trace the loopback
    return;

  auto* container = new Container(link.get_name(), "LINK", Container::by_name(link.get_englobing_zone()->get_name()));

  if ((TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) && (not TRACE_disable_link())) {
    VariableType* bandwidth = container->get_type()->by_name_or_create("bandwidth", "");
    bandwidth->set_calling_container(container);
    VariableType* latency = container->get_type()->by_name_or_create("latency", "");
    latency->set_calling_container(container);
  }

  if (TRACE_uncategorized()) {
    container->get_type()->by_name_or_create("bandwidth_used", "0.5 0.5 0.5");
  }
}

static void on_host_creation(s4u::Host const& host)
{
  if (Container::by_name_or_null(host.get_name())) // This host already exists, do nothing
    return;

  Container* container  = new HostContainer(host);
  const Container* root = Container::get_root();

  if ((TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) && (not TRACE_disable_speed())) {
    VariableType* speed = container->get_type()->by_name_or_create("speed", "");
    speed->set_calling_container(container);
    speed->set_event(0, host.get_speed());

    VariableType* cores = container->get_type()->by_name_or_create("core_count", "");
    cores->set_calling_container(container);
    cores->set_event(0, host.get_core_count());
  }

  if (TRACE_uncategorized())
    container->get_type()->by_name_or_create("speed_used", "0.5 0.5 0.5");

  if (TRACE_smpi_is_enabled() && TRACE_smpi_is_grouped()) {
    auto* mpi = container->get_type()->by_name_or_create<ContainerType>("MPI");
    mpi->by_name_or_create<StateType>("MPI_STATE");
    root->get_type()->by_name_or_create("MIGRATE_LINK", mpi, mpi);
    mpi->by_name_or_create<StateType>("MIGRATE_STATE");
  }

   if (TRACE_actor_is_enabled()) {
    auto* host_type = container->get_type();
    auto* state     = host_type->by_name_or_create<StateType>("HOST_STATE");
    state->set_calling_container(container);
    state->add_entity_value("receive", "1 0 0");
    state->add_entity_value("send", "0 0 1");
    state->add_entity_value("execute", "0 1 1");
  }
}

static void on_action_state_change(kernel::resource::Action const& action,
                                   kernel::resource::Action::State /* previous */)
{
  auto n = static_cast<unsigned>(action.get_variable()->get_number_of_constraint());

  for (unsigned i = 0; i < n; i++) {
    double value = action.get_rate() * action.get_variable()->get_constraint_weight(i);
    /* Beware of composite actions: ptasks put links and cpus together. Extra pb: we cannot dynamic_cast from void* */
    kernel::resource::Resource* resource = action.get_variable()->get_constraint(i)->get_id();
    if (const auto* cpu = dynamic_cast<kernel::resource::CpuImpl*>(resource))
      resource_set_utilization("HOST", "speed_used", cpu->get_cname(), action.get_category(), value,
                               action.get_last_update(), simgrid_get_clock() - action.get_last_update());

    else if (const auto* link = dynamic_cast<kernel::resource::StandardLinkImpl*>(resource))
      resource_set_utilization("LINK", "bandwidth_used", link->get_cname(), action.get_category(), value,
                               action.get_last_update(), simgrid_get_clock() - action.get_last_update());
  }
}

static void on_activity_suspend_resume(s4u::Activity const& activity)
{
  on_action_state_change(*activity.get_impl()->model_action_, /*ignored*/ kernel::resource::Action::State::STARTED);
}

static void on_simulation_start()
{
  std::set<std::string, std::less<>> filter;
  XBT_DEBUG("Starting graph extraction.");
  recursiveGraphExtraction(s4u::Engine::get_instance()->get_netzone_root(), Container::get_root(), &filter);
  XBT_DEBUG("Graph extraction finished.");
  if ((TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) && (not TRACE_disable_link())) {
   auto links = simgrid::s4u::Engine::get_instance()->get_all_links();
   for (const auto& link : links) {
     if (link->get_name() == "__loopback__")
      continue; 
     auto container = Container::by_name(link->get_name());
     container->get_type()->by_name_or_create("bandwidth", "")->set_event(0, link->get_bandwidth());
     container->get_type()->by_name_or_create("latency", "")->set_event(0, link->get_latency());
    }
  }
  dump_buffer(true);
}

static void on_actor_creation(s4u::Actor const& actor)
{
  const Container* root      = Container::get_root();
  Container* container       = Container::by_name(actor.get_host()->get_name());
  std::string container_name = instr_pid(actor);

  container->create_child(container_name, "ACTOR");
  auto* actor_type = container->get_type()->by_name_or_create<ContainerType>("ACTOR");
  auto* state      = actor_type->by_name_or_create<StateType>("ACTOR_STATE");
  state->add_entity_value("suspend", "1 0 1");
  state->add_entity_value("sleep", "1 1 0");
  state->add_entity_value("receive", "1 0 0");
  state->add_entity_value("send", "0 0 1");
  state->add_entity_value("execute", "0 1 1");
  root->get_type()->by_name_or_create("ACTOR_LINK", actor_type, actor_type);

  actor.on_exit([container_name](bool failed) {
    if (failed)
      // kill means that this actor no longer exists, let's destroy it
      Container::by_name(container_name)->remove_from_parent();
  });
}

static void on_actor_host_change(s4u::Actor const& actor, s4u::Host const& /*previous_location*/)
{
  static long long int counter = 0;
  Container* container         = Container::by_name(instr_pid(actor));
  LinkType* link               = Container::get_root()->get_link("ACTOR_LINK");

  // start link
  link->start_event(container, "M", std::to_string(counter));
  // destroy existing container of this process
  container->remove_from_parent();
  // create new container on the new_host location
  Container::by_name(actor.get_host()->get_name())->create_child(instr_pid(actor), "ACTOR");
  // end link
  link->end_event(Container::by_name(instr_pid(actor)), "M", std::to_string(counter));
  counter++;
}

static void on_vm_creation(s4u::Host const& host)
{
  const Container* container = new HostContainer(host);
  const Container* root      = Container::get_root();
  auto* vm                   = container->get_type()->by_name_or_create<ContainerType>("VM");
  auto* state                = vm->by_name_or_create<StateType>("VM_STATE");
  state->add_entity_value("suspend", "1 0 1");
  state->add_entity_value("sleep", "1 1 0");
  state->add_entity_value("receive", "1 0 0");
  state->add_entity_value("send", "0 0 1");
  state->add_entity_value("execute", "0 1 1");
  root->get_type()->by_name_or_create("VM_LINK", vm, vm);
  root->get_type()->by_name_or_create("VM_ACTOR_LINK", vm, vm);
}

void define_callbacks()
{
  // always need the callbacks to zones (we need only the root zone), to create the rootContainer and the rootType
  // properly
  if (TRACE_needs_platform()) {
    s4u::Engine::on_simulation_start_cb(on_simulation_start);
    s4u::Host::on_creation_cb(on_host_creation);
    s4u::Host::on_speed_change_cb([](s4u::Host const& host) {
      Container::by_name(host.get_name())
          ->get_variable("speed")
          ->set_event(simgrid_get_clock(), host.get_core_count() * host.get_available_speed());
    });
    s4u::Link::on_creation_cb(on_link_creation);
    s4u::Link::on_bandwidth_change_cb([](s4u::Link const& link) {
      Container::by_name(link.get_name())
          ->get_variable("bandwidth")
          ->set_event(
              simgrid_get_clock(),
              static_cast<kernel::resource::NetworkModel*>(link.get_impl()->get_model())->get_bandwidth_factor() *
                  link.get_bandwidth());
    });
    kernel::routing::NetPoint::on_creation.connect([](kernel::routing::NetPoint const& netpoint) {
      if (netpoint.is_router())
        new RouterContainer(netpoint);
    });
  }

  s4u::NetZone::on_creation_cb(on_netzone_creation);
  // The root netzone is created earlier when we are parsing an XML file, so handle it now
  for (auto nz : s4u::Engine::get_instance()->get_all_netzones())
    on_netzone_creation(*nz);

  s4u::Host::on_exec_state_change_cb(on_action_state_change);
  s4u::Link::on_communication_state_change_cb(on_action_state_change);
  s4u::Exec::on_suspend_cb(on_activity_suspend_resume);
  s4u::Exec::on_resume_cb(on_activity_suspend_resume);

  if (TRACE_actor_is_enabled()) {
    s4u::Actor::on_creation_cb(on_actor_creation);
    s4u::Actor::on_destruction_cb([](s4u::Actor const& actor) {
      if (auto* container = Container::by_name_or_null(instr_pid(actor)))
        container->remove_from_parent();
    });
    s4u::Actor::on_suspend_cb([](s4u::Actor const& actor) {
      Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->push_event("suspend");
    });
    s4u::Actor::on_resume_cb(
        [](s4u::Actor const& actor) { Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->pop_event(); });
    s4u::Actor::on_sleep_cb([](s4u::Actor const& actor) {
      Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->push_event("sleep");
    });
    s4u::Actor::on_wake_up_cb(
        [](s4u::Actor const& actor) { Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->pop_event(); });

    s4u::Exec::on_start_cb([](s4u::Exec const& e) {
      std::string pid = instr_pid(*s4u::Actor::self());
      if (pid == "-0") //Exec is launched directly by Maestro, use the host as container
        Container::by_name(e.get_host()->get_name())->get_state("HOST_STATE")->push_event("execute");
      else
        Container::by_name(pid)->get_state("ACTOR_STATE")->push_event("execute");
    });

    s4u::Exec::on_completion_cb([](const s4u::Exec& e) {
      std::string pid = instr_pid(*s4u::Actor::self());
      if (pid == "-0") //Exec is launched directly by Maestro, use the host as container
        Container::by_name(e.get_host()->get_name())->get_state("HOST_STATE")->pop_event();
      else
        Container::by_name(pid)->get_state("ACTOR_STATE")->pop_event();
    });

    s4u::Comm::on_completion_cb([](const s4u::Comm& c) {
      if (c.get_sender()) {
        Container::by_name(instr_pid(*c.get_sender()))->get_state("ACTOR_STATE")->pop_event();
        Container::by_name(instr_pid(*c.get_receiver()))->get_state("ACTOR_STATE")->pop_event();
      } else if(c.get_source()) {
        Container::by_name(c.get_source()->get_name())->get_state("HOST_STATE")->pop_event();
        Container::by_name(c.get_destination()->get_name())->get_state("HOST_STATE")->pop_event();
      }
    });
    s4u::Comm::on_start_cb([](s4u::Comm const& c) {
      std::string pid = instr_pid(*s4u::Actor::self());
      if (pid == "-0") { //Comm is launched directly by Maestro, use the host as container
        Container::by_name(c.get_source()->get_name())->get_state("HOST_STATE")->push_event("start");
        Container::by_name(c.get_destination()->get_name())->get_state("HOST_STATE")->push_event("start");
      }
    });
    s4u::Comm::on_send_cb([](s4u::Comm const&) {
      Container::by_name(instr_pid(*s4u::Actor::self()))->get_state("ACTOR_STATE")->push_event("send");
    });
    s4u::Comm::on_recv_cb([](s4u::Comm const&) {
      Container::by_name(instr_pid(*s4u::Actor::self()))->get_state("ACTOR_STATE")->push_event("receive");
    });
    s4u::Actor::on_host_change_cb(on_actor_host_change);
  }

  if (TRACE_smpi_is_enabled() && TRACE_smpi_is_computing()) {
    s4u::Exec::on_start_cb([](s4u::Exec const& exec) {
      Container::by_name("rank-" + std::to_string(s4u::Actor::self()->get_pid()))
          ->get_state("MPI_STATE")
          ->push_event("computing", new CpuTIData("compute", exec.get_cost()));
    });
    s4u::Exec::on_completion_cb([](const s4u::Exec&) {
      Container::by_name("rank-" + std::to_string(s4u::Actor::self()->get_pid()))->get_state("MPI_STATE")->pop_event();
    });
  }

  if (TRACE_vm_is_enabled()) {
    s4u::Host::on_creation_cb(on_vm_creation);
    s4u::VirtualMachine::on_start_cb([](s4u::VirtualMachine const& vm) {
      Container::by_name(vm.get_name())->get_state("VM_STATE")->push_event("start");
    });
    s4u::VirtualMachine::on_started_cb(
        [](s4u::VirtualMachine const& vm) { Container::by_name(vm.get_name())->get_state("VM_STATE")->pop_event(); });
    s4u::VirtualMachine::on_suspend_cb([](s4u::VirtualMachine const& vm) {
      Container::by_name(vm.get_name())->get_state("VM_STATE")->push_event("suspend");
    });
    s4u::VirtualMachine::on_resume_cb(
        [](s4u::VirtualMachine const& vm) { Container::by_name(vm.get_name())->get_state("VM_STATE")->pop_event(); });
    s4u::Host::on_destruction_cb(
        [](s4u::Host const& host) { Container::by_name(host.get_name())->remove_from_parent(); });
  }
}
} // namespace simgrid::instr
