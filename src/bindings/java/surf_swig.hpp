#include <cstdio>
#include <iostream>
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/maxmin_private.hpp"

typedef xbt_dynar_t NetworkLinkDynar;

double getClock();

void clean();

NetworkLinkDynar getRoute(char *srcName, char *dstName);

class Plugin {
public:
 virtual ~Plugin() {
   std::cout << "Plugin::~Plugin()" << std:: endl;
 }

 void activateCpuCreatedCallback();
 virtual void cpuCreatedCallback(Cpu *cpu) {}

 void activateCpuDestructedCallback();
 virtual void cpuDestructedCallback(Cpu *cpu) {}

 void activateCpuStateChangedCallback();
 virtual void cpuStateChangedCallback(Cpu *cpu, e_surf_resource_state_t, e_surf_resource_state_t) {}

 void activateCpuActionStateChangedCallback();
 virtual void cpuActionStateChangedCallback(CpuAction *action, e_surf_action_state_t, e_surf_action_state_t) {}


 void activateNetworkLinkCreatedCallback();
 virtual void networkLinkCreatedCallback(NetworkLink *link) {}

 void activateNetworkLinkDestructedCallback();
 virtual void networkLinkDestructedCallback(NetworkLink *link) {}

 void activateNetworkLinkStateChangedCallback();
 virtual void networkLinkStateChangedCallback(NetworkLink *link, e_surf_resource_state_t, e_surf_resource_state_t) {}

 void activateNetworkActionStateChangedCallback();
 virtual void networkActionStateChangedCallback(NetworkAction *action, e_surf_action_state_t old, e_surf_action_state_t cur) {}

 void activateNetworkCommunicateCallback();
 virtual void networkCommunicateCallback(NetworkAction *action, RoutingEdge *src, RoutingEdge *dst, double size, double rate) {}
};
