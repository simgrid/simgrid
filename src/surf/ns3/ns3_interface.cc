
#include "surf/ns3/ns3_interface.h"

#include "ns3/core-module.h"
#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"
#include "ns3/global-route-manager.h"

#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(interface_ns3, surf,
                                "Logging specific to the SURF network NS3 module");

//NodeContainer nodes;

void ns3_add_host(char * id)
{
	XBT_INFO("Interface ns3 add host");
//	Ptr<Node> a =  CreateObject<Node> (0);
//	nodes.Add(a);
}
