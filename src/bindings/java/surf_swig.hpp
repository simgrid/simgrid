#include <cstdio>
#include <iostream>
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/maxmin_private.hpp"

double getClock();

class Plugin {
public:
 virtual ~Plugin() { 
   std::cout << "Plugin::~Plugin()" << std:: endl;
 }

 void exit() {
   surf_exit();
 }

 void activateCpuCreatedCallback(); 
 virtual void cpuCreatedCallback(Cpu *cpu) {}

 void activateCpuDestructedCallback();
 virtual void cpuDestructedCallback(Cpu *cpu) {}
 
 void activateCpuStateChangedCallback();
 virtual void cpuStateChangedCallback(Cpu *cpu) {}

 void activateCpuActionStateChangedCallback();
 virtual void cpuActionStateChangedCallback(CpuAction *action) {}


 void activateNetworkLinkCreatedCallback();
 virtual void networkLinkCreatedCallback(NetworkLink *link) {}

 void activateNetworkLinkDestructedCallback();
 virtual void networkLinkDestructedCallback(NetworkLink *link) {}

 void activateNetworkLinkStateChangedCallback();
 virtual void networkLinkStateChangedCallback(NetworkLink *link) {}

 void activateNetworkActionStateChangedCallback();
 virtual void networkActionStateChangedCallback(NetworkAction *action) {}

};


