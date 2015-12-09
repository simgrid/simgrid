/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdio>
#include <iostream>
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/maxmin_private.hpp"

typedef xbt_dynar_t LinkDynar;
typedef simgrid::surf::ActionList *ActionArrayPtr;

double getClock();

void clean();

simgrid::surf::CpuModel *getCpuModel();
void setCpuModel(simgrid::surf::CpuModel *cpuModel);

void setCpu(char *name, simgrid::surf::Cpu *cpu);

LinkDynar getRoute(char *srcName, char *dstName);

class Plugin {
public:
 virtual ~Plugin() {
   std::cout << "Plugin::~Plugin()" << std:: endl;
 }

 void activateCpuCreatedCallback();
 virtual void cpuCreatedCallback(simgrid::surf::Cpu *cpu) {}

 void activateCpuDestructedCallback();
 virtual void cpuDestructedCallback(simgrid::surf::Cpu *cpu) {}

 void activateCpuStateChangedCallback();
 virtual void cpuStateChangedCallback(simgrid::surf::Cpu *cpu, e_surf_resource_state_t, e_surf_resource_state_t) {}

 void activateCpuActionStateChangedCallback();
 virtual void cpuActionStateChangedCallback(simgrid::surf::CpuAction *action, e_surf_action_state_t, e_surf_action_state_t) {}


 void activateLinkCreatedCallback();
 virtual void networkLinkCreatedCallback(simgrid::surf::Link *link) {}

 void activateLinkDestructedCallback();
 virtual void networkLinkDestructedCallback(simgrid::surf::Link *link) {}

 void activateLinkStateChangedCallback();
 virtual void networkLinkStateChangedCallback(simgrid::surf::Link *link, e_surf_resource_state_t, e_surf_resource_state_t) {}

 void activateNetworkActionStateChangedCallback();
 virtual void networkActionStateChangedCallback(simgrid::surf::NetworkAction *action, e_surf_action_state_t old, e_surf_action_state_t cur) {}

 void activateNetworkCommunicateCallback();
 virtual void networkCommunicateCallback(simgrid::surf::NetworkAction *action, simgrid::surf::RoutingEdge *src, simgrid::surf::RoutingEdge *dst, double size, double rate) {}
};
