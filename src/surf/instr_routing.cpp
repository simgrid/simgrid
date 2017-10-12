/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/routing/NetZoneImpl.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "surf/surf.h"
#include "xbt/graph.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_routing, instr, "Tracing platform hierarchy");

static int platform_created = 0;            /* indicate whether the platform file has been traced */
static std::vector<container_t> currentContainer; /* push and pop, used only in creation */

static const char *instr_node_name (xbt_node_t node)
{
  void *data = xbt_graph_node_get_data(node);
  return static_cast<char*>(data);
}

static container_t lowestCommonAncestor (container_t a1, container_t a2)
{
  //this is only an optimization (since most of a1 and a2 share the same parent)
  if (a1->father_ == a2->father_)
    return a1->father_;

  //create an array with all ancestors of a1
  std::vector<container_t> ancestors_a1;
  container_t p = a1->father_;
  while (p){
    ancestors_a1.push_back(p);
    p = p->father_;
  }

  //create an array with all ancestors of a2
  std::vector<container_t> ancestors_a2;
  p = a2->father_;
  while (p){
    ancestors_a2.push_back(p);
    p = p->father_;
  }

  //find the lowest ancestor
  p = nullptr;
  int i = ancestors_a1.size() - 1;
  int j = ancestors_a2.size() - 1;
  while (i >= 0 && j >= 0){
    container_t a1p = ancestors_a1.at(i);
    container_t a2p = ancestors_a2.at(j);
    if (a1p == a2p){
      p = a1p;
    }else{
      break;
    }
    i--;
    j--;
  }
  return p;
}

static void linkContainers(container_t src, container_t dst, std::set<std::string>* filter)
{
  //ignore loopback
  if (src->name_ == "__loopback__" || dst->name_ == "__loopback__") {
    XBT_DEBUG ("  linkContainers: ignoring loopback link");
    return;
  }

  //find common father
  container_t father = lowestCommonAncestor (src, dst);
  if (not father) {
    xbt_die ("common father unknown, this is a tracing problem");
  }

  // check if we already register this pair (we only need one direction)
  std::string aux1 = src->name_ + dst->name_;
  std::string aux2 = dst->name_ + src->name_;
  if (filter->find(aux1) != filter->end()) {
    XBT_DEBUG("  linkContainers: already registered %s <-> %s (1)", src->name_.c_str(), dst->name_.c_str());
    return;
  }
  if (filter->find(aux2) != filter->end()) {
    XBT_DEBUG("  linkContainers: already registered %s <-> %s (2)", dst->name_.c_str(), src->name_.c_str());
    return;
  }

  // ok, not found, register it
  filter->insert(aux1);
  filter->insert(aux2);

  //declare type
  char link_typename[INSTR_DEFAULT_STR_SIZE];
  snprintf(link_typename, INSTR_DEFAULT_STR_SIZE, "%s-%s%s-%s%s", father->type_->getCname(), src->type_->getCname(),
           src->type_->getId(), dst->type_->getCname(), dst->type_->getId());
  simgrid::instr::Type* link_type = father->type_->getChildOrNull(link_typename);
  if (link_type == nullptr)
    link_type = simgrid::instr::Type::linkNew(link_typename, father->type_, src->type_, dst->type_);

  //register EDGE types for triva configuration
  trivaEdgeTypes.insert(link_type->getName());

  //create the link
  static long long counter = 0;

  char key[INSTR_DEFAULT_STR_SIZE];
  snprintf (key, INSTR_DEFAULT_STR_SIZE, "%lld", counter);
  counter++;

  new simgrid::instr::StartLinkEvent(SIMIX_get_clock(), father, link_type, src, "topology", key);
  new simgrid::instr::EndLinkEvent(SIMIX_get_clock(), father, link_type, dst, "topology", key);

  XBT_DEBUG("  linkContainers %s <-> %s", src->name_.c_str(), dst->name_.c_str());
}

static void recursiveGraphExtraction(simgrid::s4u::NetZone* netzone, container_t container,
                                     std::set<std::string>* filter)
{
  if (not TRACE_platform_topology()) {
    XBT_DEBUG("Graph extraction disabled by user.");
    return;
  }
  XBT_DEBUG("Graph extraction for NetZone = %s", netzone->getCname());
  if (not netzone->getChildren()->empty()) {
    //bottom-up recursion
    for (auto const& nz_son : *netzone->getChildren()) {
      container_t child_container = container->children_.at(nz_son->getCname());
      recursiveGraphExtraction(nz_son, child_container, filter);
    }
  }

  xbt_graph_t graph = xbt_graph_new_graph (0, nullptr);
  xbt_dict_t nodes = xbt_dict_new_homogeneous(nullptr);
  xbt_dict_t edges = xbt_dict_new_homogeneous(nullptr);
  xbt_edge_t edge = nullptr;

  xbt_dict_cursor_t cursor = nullptr;
  char *edge_name;

  static_cast<simgrid::kernel::routing::NetZoneImpl*>(netzone)->getGraph(graph, nodes, edges);
  xbt_dict_foreach(edges,cursor,edge_name,edge) {
    linkContainers(simgrid::instr::Container::byName(static_cast<const char*>(edge->src->data)),
                   simgrid::instr::Container::byName(static_cast<const char*>(edge->dst->data)), filter);
  }
  xbt_dict_free (&nodes);
  xbt_dict_free (&edges);
  xbt_graph_free_graph(graph, xbt_free_f, xbt_free_f, nullptr);
}

/*
 * Callbacks
 */
static void sg_instr_AS_begin(simgrid::s4u::NetZone& netzone)
{
  std::string id = netzone.getName();

  if (PJ_container_get_root() == nullptr){
    container_t root = new simgrid::instr::Container(id, simgrid::instr::INSTR_AS, nullptr);
    PJ_container_set_root (root);

    if (TRACE_smpi_is_enabled()) {
      simgrid::instr::Type* mpi = root->type_->getChildOrNull("MPI");
      if (mpi == nullptr){
        mpi = simgrid::instr::Type::containerNew("MPI", root->type_);
        if (not TRACE_smpi_is_grouped())
          simgrid::instr::Type::stateNew("MPI_STATE", mpi);
        simgrid::instr::Type::linkNew("MPI_LINK", PJ_type_get_root(), mpi, mpi);
      }
    }

    if (TRACE_needs_platform()){
      currentContainer.push_back(root);
    }
    return;
  }

  if (TRACE_needs_platform()){
    container_t father = currentContainer.back();
    container_t container = new simgrid::instr::Container(id, simgrid::instr::INSTR_AS, father);
    currentContainer.push_back(container);
  }
}

static void sg_instr_AS_end(simgrid::s4u::NetZone& /*netzone*/)
{
  if (TRACE_needs_platform()){
    currentContainer.pop_back();
  }
}

static void instr_routing_parse_start_link(simgrid::s4u::Link& link)
{
  if (currentContainer.empty()) // No ongoing parsing. Are you creating the loopback?
    return;
  container_t father = currentContainer.back();

  double bandwidth_value = link.bandwidth();
  double latency_value   = link.latency();

  container_t container = new simgrid::instr::Container(link.name(), simgrid::instr::INSTR_LINK, father);

  if ((TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) && (not TRACE_disable_link())) {
    simgrid::instr::Type* bandwidth = container->type_->getChildOrNull("bandwidth");
    if (bandwidth == nullptr)
      bandwidth                   = simgrid::instr::Type::variableNew("bandwidth", "", container->type_);
    simgrid::instr::Type* latency = container->type_->getChildOrNull("latency");
    if (latency == nullptr)
      latency = simgrid::instr::Type::variableNew("latency", "", container->type_);
    new simgrid::instr::SetVariableEvent(0, container, bandwidth, bandwidth_value);
    new simgrid::instr::SetVariableEvent(0, container, latency, latency_value);
  }
  if (TRACE_uncategorized()) {
    simgrid::instr::Type* bandwidth_used = container->type_->getChildOrNull("bandwidth_used");
    if (bandwidth_used == nullptr)
      simgrid::instr::Type::variableNew("bandwidth_used", "0.5 0.5 0.5", container->type_);
  }
}

static void sg_instr_new_host(simgrid::s4u::Host& host)
{
  container_t father = currentContainer.back();
  container_t container = new simgrid::instr::Container(host.getCname(), simgrid::instr::INSTR_HOST, father);

  if ((TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) && (not TRACE_disable_speed())) {
    simgrid::instr::Type* speed = container->type_->getChildOrNull("power");
    if (speed == nullptr){
      speed = simgrid::instr::Type::variableNew("power", "", container->type_);
    }

    double current_speed_state = host.getSpeed();
    new simgrid::instr::SetVariableEvent(0, container, speed, current_speed_state);
  }
  if (TRACE_uncategorized()){
    simgrid::instr::Type* speed_used = container->type_->getChildOrNull("power_used");
    if (speed_used == nullptr){
      simgrid::instr::Type::variableNew("power_used", "0.5 0.5 0.5", container->type_);
    }
  }

  if (TRACE_smpi_is_enabled() && TRACE_smpi_is_grouped()){
    simgrid::instr::Type* mpi = container->type_->getChildOrNull("MPI");
    if (mpi == nullptr){
      mpi = simgrid::instr::Type::containerNew("MPI", container->type_);
      simgrid::instr::Type::stateNew("MPI_STATE", mpi);
    }
  }

  if (TRACE_msg_process_is_enabled()) {
    simgrid::instr::Type* msg_process = container->type_->getChildOrNull("MSG_PROCESS");
    if (msg_process == nullptr){
      msg_process                 = simgrid::instr::Type::containerNew("MSG_PROCESS", container->type_);
      simgrid::instr::Type* state = simgrid::instr::Type::stateNew("MSG_PROCESS_STATE", msg_process);
      simgrid::instr::Value::byNameOrCreate("suspend", "1 0 1", state);
      simgrid::instr::Value::byNameOrCreate("sleep", "1 1 0", state);
      simgrid::instr::Value::byNameOrCreate("receive", "1 0 0", state);
      simgrid::instr::Value::byNameOrCreate("send", "0 0 1", state);
      simgrid::instr::Value::byNameOrCreate("task_execute", "0 1 1", state);
      simgrid::instr::Type::linkNew("MSG_PROCESS_LINK", PJ_type_get_root(), msg_process, msg_process);
      simgrid::instr::Type::linkNew("MSG_PROCESS_TASK_LINK", PJ_type_get_root(), msg_process, msg_process);
    }
  }

  if (TRACE_msg_vm_is_enabled()) {
    simgrid::instr::Type* msg_vm = container->type_->getChildOrNull("MSG_VM");
    if (msg_vm == nullptr){
      msg_vm                      = simgrid::instr::Type::containerNew("MSG_VM", container->type_);
      simgrid::instr::Type* state = simgrid::instr::Type::stateNew("MSG_VM_STATE", msg_vm);
      simgrid::instr::Value::byNameOrCreate("suspend", "1 0 1", state);
      simgrid::instr::Value::byNameOrCreate("sleep", "1 1 0", state);
      simgrid::instr::Value::byNameOrCreate("receive", "1 0 0", state);
      simgrid::instr::Value::byNameOrCreate("send", "0 0 1", state);
      simgrid::instr::Value::byNameOrCreate("task_execute", "0 1 1", state);
      simgrid::instr::Type::linkNew("MSG_VM_LINK", PJ_type_get_root(), msg_vm, msg_vm);
      simgrid::instr::Type::linkNew("MSG_VM_PROCESS_LINK", PJ_type_get_root(), msg_vm, msg_vm);
    }
  }
}

static void sg_instr_new_router(simgrid::kernel::routing::NetPoint * netpoint)
{
  if (not netpoint->isRouter())
    return;
  if (TRACE_is_enabled() && TRACE_needs_platform()) {
    container_t father = currentContainer.back();
    new simgrid::instr::Container(netpoint->cname(), simgrid::instr::INSTR_ROUTER, father);
  }
}

static void instr_routing_parse_end_platform ()
{
  currentContainer.clear();
  std::set<std::string>* filter = new std::set<std::string>;
  XBT_DEBUG ("Starting graph extraction.");
  recursiveGraphExtraction(simgrid::s4u::Engine::getInstance()->getNetRoot(), PJ_container_get_root(), filter);
  XBT_DEBUG ("Graph extraction finished.");
  delete filter;
  platform_created = 1;
  TRACE_paje_dump_buffer(1);
}

void instr_routing_define_callbacks ()
{
  // always need the callbacks to ASes (we need only the root AS), to create the rootContainer and the rootType properly
  if (not TRACE_is_enabled())
    return;
  if (TRACE_needs_platform()) {
    simgrid::s4u::Link::onCreation.connect(instr_routing_parse_start_link);
    simgrid::s4u::onPlatformCreated.connect(instr_routing_parse_end_platform);
    simgrid::s4u::Host::onCreation.connect(sg_instr_new_host);
  }
  simgrid::s4u::NetZone::onCreation.connect(sg_instr_AS_begin);
  simgrid::s4u::NetZone::onSeal.connect(sg_instr_AS_end);
  simgrid::kernel::routing::NetPoint::onCreation.connect(&sg_instr_new_router);
}
/*
 * user categories support
 */
static void recursiveNewVariableType(const char* new_typename, const char* color, simgrid::instr::Type* root)
{

  if (root->getName() == "HOST") {
    char tnstr[INSTR_DEFAULT_STR_SIZE];
    snprintf (tnstr, INSTR_DEFAULT_STR_SIZE, "p%s", new_typename);
    simgrid::instr::Type::variableNew(tnstr, color == nullptr ? "" : color, root);
  }
  if (root->getName() == "MSG_VM") {
    char tnstr[INSTR_DEFAULT_STR_SIZE];
    snprintf (tnstr, INSTR_DEFAULT_STR_SIZE, "p%s", new_typename);
    simgrid::instr::Type::variableNew(tnstr, color == nullptr ? "" : color, root);
  }
  if (root->getName() == "LINK") {
    char tnstr[INSTR_DEFAULT_STR_SIZE];
    snprintf (tnstr, INSTR_DEFAULT_STR_SIZE, "b%s", new_typename);
    simgrid::instr::Type::variableNew(tnstr, color == nullptr ? "" : color, root);
  }
  for (auto elm : root->children_) {
    recursiveNewVariableType(new_typename, color == nullptr ? "" : color, elm.second);
  }
}

void instr_new_variable_type (const char *new_typename, const char *color)
{
  recursiveNewVariableType (new_typename, color, PJ_type_get_root());
}

static void recursiveNewUserVariableType(const char* father_type, const char* new_typename, const char* color,
                                         simgrid::instr::Type* root)
{
  if (root->getName() == father_type) {
    simgrid::instr::Type::variableNew(new_typename, color == nullptr ? "" : color, root);
  }
  for (auto elm : root->children_)
    recursiveNewUserVariableType(father_type, new_typename, color, elm.second);
}

void instr_new_user_variable_type  (const char *father_type, const char *new_typename, const char *color)
{
  recursiveNewUserVariableType (father_type, new_typename, color, PJ_type_get_root());
}

static void recursiveNewUserStateType(const char* father_type, const char* new_typename, simgrid::instr::Type* root)
{
  if (root->getName() == father_type) {
    simgrid::instr::Type::stateNew(new_typename, root);
  }
  for (auto elm : root->children_)
    recursiveNewUserStateType(father_type, new_typename, elm.second);
}

void instr_new_user_state_type (const char *father_type, const char *new_typename)
{
  recursiveNewUserStateType (father_type, new_typename, PJ_type_get_root());
}

static void recursiveNewValueForUserStateType(const char* type_name, const char* val, const char* color,
                                              simgrid::instr::Type* root)
{
  if (root->getName() == type_name)
    simgrid::instr::Value::byNameOrCreate(val, color, root);

  for (auto elm : root->children_)
    recursiveNewValueForUserStateType(type_name, val, color, elm.second);
}

void instr_new_value_for_user_state_type (const char *type_name, const char *value, const char *color)
{
  recursiveNewValueForUserStateType (type_name, value, color, PJ_type_get_root());
}

int instr_platform_traced ()
{
  return platform_created;
}

#define GRAPHICATOR_SUPPORT_FUNCTIONS

static void recursiveXBTGraphExtraction(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges, sg_netzone_t netzone,
                                        container_t container)
{
  if (not netzone->getChildren()->empty()) {
    //bottom-up recursion
    for (auto const& netzone_child : *netzone->getChildren()) {
      container_t child_container = container->children_.at(netzone_child->getCname());
      recursiveXBTGraphExtraction(graph, nodes, edges, netzone_child, child_container);
    }
  }

  static_cast<simgrid::kernel::routing::NetZoneImpl*>(netzone)->getGraph(graph, nodes, edges);
}

xbt_graph_t instr_routing_platform_graph ()
{
  xbt_graph_t ret = xbt_graph_new_graph (0, nullptr);
  xbt_dict_t nodes = xbt_dict_new_homogeneous(nullptr);
  xbt_dict_t edges = xbt_dict_new_homogeneous(nullptr);
  recursiveXBTGraphExtraction(ret, nodes, edges, simgrid::s4u::Engine::getInstance()->getNetRoot(),
                              PJ_container_get_root());
  xbt_dict_free (&nodes);
  xbt_dict_free (&edges);
  return ret;
}

void instr_routing_platform_graph_export_graphviz (xbt_graph_t g, const char *filename)
{
  unsigned int cursor = 0;
  xbt_node_t node = nullptr;
  xbt_edge_t edge = nullptr;

  FILE *file = fopen(filename, "w");
  xbt_assert(file, "Failed to open %s \n", filename);

  if (g->directed)
    fprintf(file, "digraph test {\n");
  else
    fprintf(file, "graph test {\n");

  fprintf(file, "  graph [overlap=scale]\n");

  fprintf(file, "  node [shape=box, style=filled]\n");
  fprintf(file, "  node [width=.3, height=.3, style=filled, color=skyblue]\n\n");

  xbt_dynar_foreach(g->nodes, cursor, node) {
    fprintf(file, "  \"%s\";\n", instr_node_name(node));
  }
  xbt_dynar_foreach(g->edges, cursor, edge) {
    const char *src_s = instr_node_name (edge->src);
    const char *dst_s = instr_node_name (edge->dst);
    if (g->directed)
      fprintf(file, "  \"%s\" -> \"%s\";\n", src_s, dst_s);
    else
      fprintf(file, "  \"%s\" -- \"%s\";\n", src_s, dst_s);
  }
  fprintf(file, "}\n");
  fclose(file);
}
