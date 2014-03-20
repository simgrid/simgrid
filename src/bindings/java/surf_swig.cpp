#include <boost/lambda/bind.hpp>
#include "src/surf/surf_interface.hpp"
#include "surf_swig.hpp"

double getClock() {
  surf_get_clock();
}

void Plugin::activateCpuCreatedCallback(){
  surf_callback_connect(cpuCreatedCallbacks, boost::bind(&Plugin::cpuCreatedCallback, this, _1));
}

void Plugin::activateCpuDestructedCallback(){
  surf_callback_connect(cpuDestructedCallbacks, boost::bind(&Plugin::cpuDestructedCallback, this, _1));
}

void Plugin::activateCpuStateChangedCallback(){
  surf_callback_connect(cpuStateChangedCallbacks, boost::bind(&Plugin::cpuStateChangedCallback, this, _1));
}

void Plugin::activateCpuActionStateChangedCallback(){
  surf_callback_connect(cpuActionStateChangedCallbacks, boost::bind(&Plugin::cpuActionStateChangedCallback, this, _1));
}


void Plugin::activateNetworkLinkCreatedCallback(){
  surf_callback_connect(networkLinkCreatedCallbacks, boost::bind(&Plugin::networkLinkCreatedCallback, this, _1));
}

void Plugin::activateNetworkLinkDestructedCallback(){
  surf_callback_connect(networkLinkDestructedCallbacks, boost::bind(&Plugin::networkLinkDestructedCallback, this, _1));
}

void Plugin::activateNetworkLinkStateChangedCallback(){
  surf_callback_connect(networkLinkStateChangedCallbacks, boost::bind(&Plugin::networkLinkStateChangedCallback, this, _1));
}

void Plugin::activateNetworkActionStateChangedCallback(){
  surf_callback_connect(networkActionStateChangedCallbacks, boost::bind(&Plugin::networkActionStateChangedCallback, this, _1));
}


