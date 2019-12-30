/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.hpp"
#include "xbt/graph.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_routing, instr, "Tracing platform hierarchy");

static std::vector<simgrid::instr::NetZoneContainer*> currentContainer; /* push and pop, used only in creation */

static const char* instr_node_name(xbt_node_t node)
{
  return static_cast<char*>(xbt_graph_node_get_data(node));
}

static container_t lowestCommonAncestor(const simgrid::instr::Container* a1, const simgrid::instr::Container* a2)
{
  // this is only an optimization (since most of a1 and a2 share the same parent)
  if (a1->father_ == a2->father_)
    return a1->father_;

  // create an array with all ancestors of a1
  std::vector<container_t> ancestors_a1;
  container_t p = a1->father_;
  while (p) {
    ancestors_a1.push_back(p);
    p = p->father_;
  }

  // create an array with all ancestors of a2
  std::vector<container_t> ancestors_a2;
  p = a2->father_;
  while (p) {
    ancestors_a2.push_back(p);
    p = p->father_;
  }

  // find the lowest ancestor
  p     = nullptr;
  int i = ancestors_a1.size() - 1;
  int j = ancestors_a2.size() - 1;
  while (i >= 0 && j >= 0) {
    container_t a1p = ancestors_a1.at(i);
    const simgrid::instr::Container* a2p = ancestors_a2.at(j);
    if (a1p == a2p) {
      p = a1p;
    } else {
      break;
    }
    i--;
    j--;
  }
  return p;
}

static void linkContainers(container_t src, container_t dst, std::set<std::string>* filter)
{
  // ignore loopback
  if (src->get_name() == "__loopback__" || dst->get_name() == "__loopback__") {
    XBT_DEBUG("  linkContainers: ignoring loopback link");
    return;
  }

  // find common father
  container_t father = lowestCommonAncestor(src, dst);
  if (not father) {
    xbt_die("common father unknown, this is a tracing problem");
  }

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
  std::string link_typename = father->type_->get_name() + "-" + src->type_->get_name() +
                              std::to_string(src->type_->get_id()) + "-" + dst->type_->get_name() +
                              std::to_string(dst->type_->get_id());
  simgrid::instr::LinkType* link = father->type_->by_name_or_create(link_typename, src->type_, dst->type_);
  link->set_calling_container(father);

  // register EDGE types for triva configuration
  trivaEdgeTypes.insert(link->get_name());

  // create the link
  static long long counter = 0;

  std::string key = std::to_string(counter);
  counter++;

  link->start_event(src, "topology", key);
  link->end_event(dst, "topology", key);

  XBT_DEBUG("  linkContainers %s <-> %s", src->get_cname(), dst->get_cname());
}

static void recursiveGraphExtraction(const simgrid::s4u::NetZone* netzone, container_t container,
                                     std::set<std::string>* filter)
{
  if (not TRACE_platform_topology()) {
    XBT_DEBUG("Graph extraction disabled by user.");
    return;
  }
  XBT_DEBUG("Graph extraction for NetZone = %s", netzone->get_cname());
  if (not netzone->get_children().empty()) {
    // bottom-up recursion
    for (auto const& nz_son : netzone->get_children()) {
      container_t child_container = container->children_.at(nz_son->get_name());
      recursiveGraphExtraction(nz_son, child_container, filter);
    }
  }

  xbt_graph_t graph                        = xbt_graph_new_graph(0, nullptr);
  std::map<std::string, xbt_node_t>* nodes = new std::map<std::string, xbt_node_t>();
  std::map<std::string, xbt_edge_t>* edges = new std::map<std::string, xbt_edge_t>();

  netzone->get_impl()->get_graph(graph, nodes, edges);
  for (auto elm : *edges) {
    const xbt_edge* edge = elm.second;
    linkContainers(simgrid::instr::Container::by_name(static_cast<const char*>(edge->src->data)),
                   simgrid::instr::Container::by_name(static_cast<const char*>(edge->dst->data)), filter);
  }
  delete nodes;
  delete edges;
  xbt_graph_free_graph(graph, xbt_free_f, xbt_free_f, nullptr);
}

/*
 * Callbacks
 */
static void instr_netzone_on_creation(simgrid::s4u::NetZone const& netzone)
{
  std::string id = netzone.get_name();
  if (simgrid::instr::Container::get_root() == nullptr) {
    simgrid::instr::NetZoneContainer* root = new simgrid::instr::NetZoneContainer(id, 0, nullptr);
    xbt_assert(simgrid::instr::Container::get_root() == root);

    if (TRACE_smpi_is_enabled()) {
      simgrid::instr::ContainerType* mpi = root->type_->by_name_or_create<simgrid::instr::ContainerType>("MPI");
      if (not TRACE_smpi_is_grouped())
        mpi->by_name_or_create<simgrid::instr::StateType>("MPI_STATE");
      root->type_->by_name_or_create("MPI_LINK", mpi, mpi);
      // TODO See if we can move this to the LoadBalancer plugin
      root->type_->by_name_or_create("MIGRATE_LINK", mpi, mpi);
      mpi->by_name_or_create<simgrid::instr::StateType>("MIGRATE_STATE");
    }

    if (TRACE_needs_platform()) {
      currentContainer.push_back(root);
    }
    return;
  }

  if (TRACE_needs_platform()) {
    simgrid::instr::NetZoneContainer* container =
        new simgrid::instr::NetZoneContainer(id, currentContainer.size(), currentContainer.back());
    currentContainer.push_back(container);
  }
}

static void instr_link_on_creation(simgrid::s4u::Link const& link)
{
  if (currentContainer.empty()) // No ongoing parsing. Are you creating the loopback?
    return;

  container_t container = new simgrid::instr::Container(link.get_name(), "LINK", currentContainer.back());

  if ((TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) && (not TRACE_disable_link())) {
    simgrid::instr::VariableType* bandwidth = container->type_->by_name_or_create("bandwidth", "");
    bandwidth->set_calling_container(container);
    bandwidth->set_event(0, link.get_bandwidth());
    simgrid::instr::VariableType* latency = container->type_->by_name_or_create("latency", "");
    latency->set_calling_container(container);
    latency->set_event(0, link.get_latency());
  }
  if (TRACE_uncategorized()) {
    container->type_->by_name_or_create("bandwidth_used", "0.5 0.5 0.5");
  }
}

static void instr_host_on_creation(simgrid::s4u::Host const& host)
{
  simgrid::instr::Container* container  = new simgrid::instr::HostContainer(host, currentContainer.back());
  const simgrid::instr::Container* root = simgrid::instr::Container::get_root();

  if ((TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) && (not TRACE_disable_speed())) {
    simgrid::instr::VariableType* speed = container->type_->by_name_or_create("speed", "");
    speed->set_calling_container(container);
    speed->set_event(0, host.get_speed());

    simgrid::instr::VariableType* cores = container->type_->by_name_or_create("core_count", "");
    cores->set_calling_container(container);
    cores->set_event(0, host.get_core_count());
  }

  if (TRACE_uncategorized())
    container->type_->by_name_or_create("speed_used", "0.5 0.5 0.5");

  if (TRACE_smpi_is_enabled() && TRACE_smpi_is_grouped()) {
    simgrid::instr::ContainerType* mpi = container->type_->by_name_or_create<simgrid::instr::ContainerType>("MPI");
    mpi->by_name_or_create<simgrid::instr::StateType>("MPI_STATE");
    // TODO See if we can move this to the LoadBalancer plugin
    root->type_->by_name_or_create("MIGRATE_LINK", mpi, mpi);
    mpi->by_name_or_create<simgrid::instr::StateType>("MIGRATE_STATE");
  }
}

static void instr_host_on_speed_change(simgrid::s4u::Host const& host)
{
  simgrid::instr::Container::by_name(host.get_name())
      ->get_variable("speed")
      ->set_event(surf_get_clock(), host.get_core_count() * host.get_available_speed());
}

static void instr_action_on_state_change(simgrid::kernel::resource::Action const& action,
                                         simgrid::kernel::resource::Action::State /* previous */)
{
  int n = action.get_variable()->get_number_of_constraint();

  for (int i = 0; i < n; i++) {
    double value = action.get_variable()->get_value() * action.get_variable()->get_constraint_weight(i);
    /* Beware of composite actions: ptasks put links and cpus together. Extra pb: we cannot dynamic_cast from void* */
    simgrid::kernel::resource::Resource* resource = action.get_variable()->get_constraint(i)->get_id();
    const simgrid::kernel::resource::Cpu* cpu     = dynamic_cast<simgrid::kernel::resource::Cpu*>(resource);

    if (cpu != nullptr)
      TRACE_surf_resource_set_utilization("HOST", "speed_used", cpu->get_cname(), action.get_category(), value,
                                          action.get_last_update(), SIMIX_get_clock() - action.get_last_update());

    const simgrid::kernel::resource::LinkImpl* link = dynamic_cast<simgrid::kernel::resource::LinkImpl*>(resource);

    if (link != nullptr)
      TRACE_surf_resource_set_utilization("LINK", "bandwidth_used", link->get_cname(), action.get_category(), value,
                                          action.get_last_update(), SIMIX_get_clock() - action.get_last_update());
  }
}

static void instr_link_on_bandwidth_change(simgrid::s4u::Link const& link)
{
  simgrid::instr::Container::by_name(link.get_name())
      ->get_variable("bandwidth")
      ->set_event(surf_get_clock(), sg_bandwidth_factor * link.get_bandwidth());
}

static void instr_netpoint_on_creation(simgrid::kernel::routing::NetPoint const& netpoint)
{
  if (netpoint.is_router())
    new simgrid::instr::RouterContainer(netpoint.get_name(), currentContainer.back());
}

static void instr_on_platform_created()
{
  currentContainer.clear();
  std::set<std::string>* filter = new std::set<std::string>();
  XBT_DEBUG("Starting graph extraction.");
  recursiveGraphExtraction(simgrid::s4u::Engine::get_instance()->get_netzone_root(),
                           simgrid::instr::Container::get_root(), filter);
  XBT_DEBUG("Graph extraction finished.");
  delete filter;
  TRACE_paje_dump_buffer(true);
}

static void instr_actor_on_creation(simgrid::s4u::Actor const& actor)
{
  const simgrid::instr::Container* root = simgrid::instr::Container::get_root();
  simgrid::instr::Container* container  = simgrid::instr::Container::by_name(actor.get_host()->get_name());

  container->create_child(instr_pid(actor), "ACTOR");
  simgrid::instr::ContainerType* actor_type =
      container->type_->by_name_or_create<simgrid::instr::ContainerType>("ACTOR");
  simgrid::instr::StateType* state = actor_type->by_name_or_create<simgrid::instr::StateType>("ACTOR_STATE");
  state->add_entity_value("suspend", "1 0 1");
  state->add_entity_value("sleep", "1 1 0");
  state->add_entity_value("receive", "1 0 0");
  state->add_entity_value("send", "0 0 1");
  state->add_entity_value("execute", "0 1 1");
  root->type_->by_name_or_create("ACTOR_LINK", actor_type, actor_type);
  root->type_->by_name_or_create("ACTOR_TASK_LINK", actor_type, actor_type);

  std::string container_name = instr_pid(actor);
  actor.on_exit([container_name](bool failed) {
    if (failed)
      // kill means that this actor no longer exists, let's destroy it
      simgrid::instr::Container::by_name(container_name)->remove_from_parent();
  });
}

static void instr_actor_on_host_change(simgrid::s4u::Actor const& actor,
                                       simgrid::s4u::Host const& /*previous_location*/)
{
  static long long int counter = 0;
  container_t container = simgrid::instr::Container::by_name(instr_pid(actor));
  simgrid::instr::LinkType* link = simgrid::instr::Container::get_root()->get_link("ACTOR_LINK");

  // start link
  link->start_event(container, "M", std::to_string(counter));
  // destroy existing container of this process
  container->remove_from_parent();
  // create new container on the new_host location
  simgrid::instr::Container::by_name(actor.get_host()->get_name())->create_child(instr_pid(actor), "ACTOR");
  // end link
  link->end_event(simgrid::instr::Container::by_name(instr_pid(actor)), "M", std::to_string(counter));
  counter++;
}

static void instr_vm_on_creation(simgrid::s4u::Host const& host)
{
  const simgrid::instr::Container* container = new simgrid::instr::HostContainer(host, currentContainer.back());
  const simgrid::instr::Container* root      = simgrid::instr::Container::get_root();
  simgrid::instr::ContainerType* vm = container->type_->by_name_or_create<simgrid::instr::ContainerType>("VM");
  simgrid::instr::StateType* state  = vm->by_name_or_create<simgrid::instr::StateType>("VM_STATE");
  state->add_entity_value("suspend", "1 0 1");
  state->add_entity_value("sleep", "1 1 0");
  state->add_entity_value("receive", "1 0 0");
  state->add_entity_value("send", "0 0 1");
  state->add_entity_value("execute", "0 1 1");
  root->type_->by_name_or_create("VM_LINK", vm, vm);
  root->type_->by_name_or_create("VM_ACTOR_LINK", vm, vm);
}

void instr_define_callbacks()
{
  // always need the callbacks to zones (we need only the root zone), to create the rootContainer and the rootType
  // properly
  if (TRACE_needs_platform()) {
    simgrid::s4u::Engine::on_platform_created.connect(instr_on_platform_created);
    simgrid::s4u::Host::on_creation.connect(instr_host_on_creation);
    simgrid::s4u::Host::on_speed_change.connect(instr_host_on_speed_change);
    simgrid::s4u::Link::on_creation.connect(instr_link_on_creation);
    simgrid::s4u::Link::on_bandwidth_change.connect(instr_link_on_bandwidth_change);
    simgrid::s4u::NetZone::on_seal.connect(
        [](simgrid::s4u::NetZone const& /*netzone*/) { currentContainer.pop_back(); });
    simgrid::kernel::routing::NetPoint::on_creation.connect(instr_netpoint_on_creation);
  }
  simgrid::s4u::NetZone::on_creation.connect(instr_netzone_on_creation);

  simgrid::kernel::resource::CpuAction::on_state_change.connect(instr_action_on_state_change);
  simgrid::s4u::Link::on_communication_state_change.connect(instr_action_on_state_change);

  if (TRACE_actor_is_enabled()) {
    simgrid::s4u::Actor::on_creation.connect(instr_actor_on_creation);
    simgrid::s4u::Actor::on_destruction.connect([](simgrid::s4u::Actor const& actor) {
      auto container = simgrid::instr::Container::by_name_or_null(instr_pid(actor));
      if (container != nullptr)
        container->remove_from_parent();
    });
    simgrid::s4u::Actor::on_suspend.connect([](simgrid::s4u::Actor const& actor) {
      simgrid::instr::Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->push_event("suspend");
    });
    simgrid::s4u::Actor::on_resume.connect([](simgrid::s4u::Actor const& actor) {
      simgrid::instr::Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->pop_event();
    });
    simgrid::s4u::Actor::on_sleep.connect([](simgrid::s4u::Actor const& actor) {
      simgrid::instr::Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->push_event("sleep");
    });
    simgrid::s4u::Actor::on_wake_up.connect([](simgrid::s4u::Actor const& actor) {
      simgrid::instr::Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->pop_event();
    });
    simgrid::s4u::Exec::on_start.connect([](simgrid::s4u::Actor const& actor, simgrid::s4u::Exec const&) {
      simgrid::instr::Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->push_event("execute");
    });
    simgrid::s4u::Exec::on_completion.connect([](simgrid::s4u::Actor const& actor, simgrid::s4u::Exec const&) {
      simgrid::instr::Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->pop_event();
    });
    simgrid::s4u::Comm::on_sender_start.connect([](simgrid::s4u::Actor const& actor) {
      simgrid::instr::Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->push_event("send");
    });
    simgrid::s4u::Comm::on_receiver_start.connect([](simgrid::s4u::Actor const& actor) {
      simgrid::instr::Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->push_event("receive");
    });
    simgrid::s4u::Comm::on_completion.connect([](simgrid::s4u::Actor const& actor) {
      simgrid::instr::Container::by_name(instr_pid(actor))->get_state("ACTOR_STATE")->pop_event();
    });
    simgrid::s4u::Actor::on_host_change.connect(instr_actor_on_host_change);
  }

  if (TRACE_vm_is_enabled()) {
    simgrid::s4u::Host::on_creation.connect(instr_vm_on_creation);
    simgrid::s4u::VirtualMachine::on_start.connect([](simgrid::s4u::VirtualMachine const& vm) {
      simgrid::instr::Container::by_name(vm.get_name())->get_state("VM_STATE")->push_event("start");
    });
    simgrid::s4u::VirtualMachine::on_started.connect([](simgrid::s4u::VirtualMachine const& vm) {
      simgrid::instr::Container::by_name(vm.get_name())->get_state("VM_STATE")->pop_event();
    });
    simgrid::s4u::VirtualMachine::on_suspend.connect([](simgrid::s4u::VirtualMachine const& vm) {
      simgrid::instr::Container::by_name(vm.get_name())->get_state("VM_STATE")->push_event("suspend");
    });
    simgrid::s4u::VirtualMachine::on_resume.connect([](simgrid::s4u::VirtualMachine const& vm) {
      simgrid::instr::Container::by_name(vm.get_name())->get_state("VM_STATE")->pop_event();
    });
    simgrid::s4u::Host::on_destruction.connect([](simgrid::s4u::Host const& host) {
      simgrid::instr::Container::by_name(host.get_name())->remove_from_parent();
    });
  }
}
/*
 * user categories support
 */
static void recursiveNewVariableType(const std::string& new_typename, const std::string& color,
                                     simgrid::instr::Type* root)
{
  if (root->get_name() == "HOST" || root->get_name() == "VM")
    root->by_name_or_create(std::string("p") + new_typename, color);

  if (root->get_name() == "LINK")
    root->by_name_or_create(std::string("b") + new_typename, color);

  for (auto const& elm : root->children_) {
    recursiveNewVariableType(new_typename, color, elm.second.get());
  }
}

void instr_new_variable_type(const std::string& new_typename, const std::string& color)
{
  recursiveNewVariableType(new_typename, color, simgrid::instr::Container::get_root()->type_);
}

static void recursiveNewUserVariableType(const std::string& father_type, const std::string& new_typename,
                                         const std::string& color, simgrid::instr::Type* root)
{
  if (root->get_name() == father_type) {
    root->by_name_or_create(new_typename, color);
  }
  for (auto const& elm : root->children_)
    recursiveNewUserVariableType(father_type, new_typename, color, elm.second.get());
}

void instr_new_user_variable_type(const std::string& father_type, const std::string& new_typename,
                                  const std::string& color)
{
  recursiveNewUserVariableType(father_type, new_typename, color, simgrid::instr::Container::get_root()->type_);
}

static void recursiveNewUserStateType(const std::string& father_type, const std::string& new_typename,
                                      simgrid::instr::Type* root)
{
  if (root->get_name() == father_type)
    root->by_name_or_create<simgrid::instr::StateType>(new_typename);

  for (auto const& elm : root->children_)
    recursiveNewUserStateType(father_type, new_typename, elm.second.get());
}

void instr_new_user_state_type(const std::string& father_type, const std::string& new_typename)
{
  recursiveNewUserStateType(father_type, new_typename, simgrid::instr::Container::get_root()->type_);
}

static void recursiveNewValueForUserStateType(const std::string& type_name, const char* val, const std::string& color,
                                              simgrid::instr::Type* root)
{
  if (root->get_name() == type_name)
    static_cast<simgrid::instr::StateType*>(root)->add_entity_value(val, color);

  for (auto const& elm : root->children_)
    recursiveNewValueForUserStateType(type_name, val, color, elm.second.get());
}

void instr_new_value_for_user_state_type(const std::string& type_name, const char* value, const std::string& color)
{
  recursiveNewValueForUserStateType(type_name, value, color, simgrid::instr::Container::get_root()->type_);
}

#define GRAPHICATOR_SUPPORT_FUNCTIONS

static void recursiveXBTGraphExtraction(xbt_graph_t graph, std::map<std::string, xbt_node_t>* nodes,
                                        std::map<std::string, xbt_edge_t>* edges, const_sg_netzone_t netzone,
                                        container_t container)
{
  if (not netzone->get_children().empty()) {
    // bottom-up recursion
    for (auto const& netzone_child : netzone->get_children()) {
      container_t child_container = container->children_.at(netzone_child->get_name());
      recursiveXBTGraphExtraction(graph, nodes, edges, netzone_child, child_container);
    }
  }

  netzone->get_impl()->get_graph(graph, nodes, edges);
}

xbt_graph_t instr_routing_platform_graph()
{
  xbt_graph_t ret                          = xbt_graph_new_graph(0, nullptr);
  std::map<std::string, xbt_node_t>* nodes = new std::map<std::string, xbt_node_t>();
  std::map<std::string, xbt_edge_t>* edges = new std::map<std::string, xbt_edge_t>();
  recursiveXBTGraphExtraction(ret, nodes, edges, simgrid::s4u::Engine::get_instance()->get_netzone_root(),
                              simgrid::instr::Container::get_root());
  delete nodes;
  delete edges;
  return ret;
}

void instr_routing_platform_graph_export_graphviz(const s_xbt_graph_t* g, const char* filename)
{
  unsigned int cursor = 0;
  xbt_node_t node     = nullptr;
  xbt_edge_t edge     = nullptr;

  FILE* file = fopen(filename, "w");
  xbt_assert(file, "Failed to open %s \n", filename);

  if (g->directed)
    fprintf(file, "digraph test {\n");
  else
    fprintf(file, "graph test {\n");

  fprintf(file, "  graph [overlap=scale]\n");

  fprintf(file, "  node [shape=box, style=filled]\n");
  fprintf(file, "  node [width=.3, height=.3, style=filled, color=skyblue]\n\n");

  xbt_dynar_foreach (g->nodes, cursor, node) {
    fprintf(file, "  \"%s\";\n", instr_node_name(node));
  }
  xbt_dynar_foreach (g->edges, cursor, edge) {
    const char* src_s = instr_node_name(edge->src);
    const char* dst_s = instr_node_name(edge->dst);
    if (g->directed)
      fprintf(file, "  \"%s\" -> \"%s\";\n", src_s, dst_s);
    else
      fprintf(file, "  \"%s\" -- \"%s\";\n", src_s, dst_s);
  }
  fprintf(file, "}\n");
  fclose(file);
}
