/* Copyright (c) 2014-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/routing/AsClusterDragonfly.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf.hpp" // FIXME: move that back to the parsing area

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_cluster_dragonfly, surf_route_cluster, "Dragonfly Routing part of surf");



namespace simgrid {
namespace routing {

AsClusterDragonfly::AsClusterDragonfly(const char*name)
  : AsCluster(name) {
}

AsClusterDragonfly::~AsClusterDragonfly() {

  if(this->routers_!=nullptr){
    unsigned int i;
    for (i=0; i<this->numGroups_*this->numChassisPerGroup_*this->numBladesPerChassis_;i++)
        delete(routers_[i]);
    xbt_free(routers_);
  }
}

unsigned int *AsClusterDragonfly::rankId_to_coords(int rankId)
{

  //coords : group, chassis, blade, node
  unsigned int *coords = (unsigned int *) malloc(4 * sizeof(unsigned int));
  coords[0] = rankId/ (numChassisPerGroup_*numBladesPerChassis_*numNodesPerBlade_);
  rankId=rankId%(numChassisPerGroup_*numBladesPerChassis_*numNodesPerBlade_);
  coords[1] = rankId/ (numBladesPerChassis_*numNodesPerBlade_);
  rankId=rankId%(numBladesPerChassis_*numNodesPerBlade_);
  coords[2] = rankId/ numNodesPerBlade_;
  coords[3]=rankId%numNodesPerBlade_;
  
  return coords;
}

void AsClusterDragonfly::parse_specific_arguments(sg_platf_cluster_cbarg_t cluster) {
  std::vector<std::string> parameters;
  std::vector<std::string> tmp;
  boost::split(parameters, cluster->topo_parameters, boost::is_any_of(";"));

  // TODO : we have to check for zeros and negative numbers, or it might crash
  if (parameters.size() != 4){
    surf_parse_error("Dragonfly are defined by the number of groups, chassiss per groups, blades per chassis, nodes per blade");
  }

  // Blue network : number of groups, number of links between each group
  boost::split(tmp, parameters[0], boost::is_any_of(","));
  if(tmp.size() != 2) {
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");
  }

  this->numGroups_=xbt_str_parse_int(tmp[0].c_str(), "Invalid number of groups: %s");
  this->numLinksBlue_=xbt_str_parse_int(tmp[1].c_str(), "Invalid number of links for the blue level: %s");

 // Black network : number of chassiss/group, number of links between each router on the black network
  boost::split(tmp, parameters[1], boost::is_any_of(","));
  if(tmp.size() != 2) {
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");
  }

  this->numChassisPerGroup_=xbt_str_parse_int(tmp[0].c_str(), "Invalid number of groups: %s");
  this->numLinksBlack_=xbt_str_parse_int(tmp[1].c_str(), "Invalid number of links  for the black level: %s");


 // Green network : number of blades/chassis, number of links between each router on the green network
  boost::split(tmp, parameters[2], boost::is_any_of(","));
  if(tmp.size() != 2) {
    surf_parse_error("Dragonfly topologies are defined by 3 levels with 2 elements each, and one with one element");
  }

  this->numBladesPerChassis_=xbt_str_parse_int(tmp[0].c_str(), "Invalid number of groups: %s");
  this->numLinksGreen_=xbt_str_parse_int(tmp[1].c_str(), "Invalid number of links for the green level: %s");


  // The last part of topo_parameters should be the number of nodes per blade
  this->numNodesPerBlade_ = xbt_str_parse_int(parameters[3].c_str(), "Last parameter is not the amount of nodes per blade: %s");
  this->cluster_ = cluster;
}

/*
* Generate the cluster once every node is created
*/
void AsClusterDragonfly::seal(){
  if(this->numNodesPerBlade_ == 0) {
    return;
  }

  this->generateRouters();
  this->generateLinks();
}

DragonflyRouter::DragonflyRouter(int group, int chassis, int blade){
    this->group_=group;
    this->chassis_=chassis;
    this->blade_=blade;
}

DragonflyRouter::~DragonflyRouter(){
  if(this->myNodes_!=nullptr)
    xbt_free(myNodes_);
  if(this->greenLinks_!=nullptr)
    xbt_free(greenLinks_);
  if(this->blackLinks_!=nullptr)
    xbt_free(blackLinks_);
  if(this->blueLinks_!=nullptr)
    xbt_free(blueLinks_);
}


void AsClusterDragonfly::generateRouters() {

unsigned int i, j, k;

this->routers_=(DragonflyRouter**)xbt_malloc0(this->numGroups_*this->numChassisPerGroup_*this->numBladesPerChassis_*sizeof(DragonflyRouter*));

for(i=0;i<this->numGroups_;i++){
  for(j=0;j<this->numChassisPerGroup_;j++){
    for(k=0;k<this->numBladesPerChassis_;k++){
      DragonflyRouter* router = new DragonflyRouter(i,j,k);
      this->routers_[i*this->numChassisPerGroup_*this->numBladesPerChassis_+j*this->numBladesPerChassis_+k]=router;
    }
  }
}

}


void AsClusterDragonfly::createLink(char* id, int numlinks, Link** linkup, Link** linkdown){
  *linkup=nullptr;
  *linkdown=nullptr;
  s_sg_platf_link_cbarg_t linkTemplate;
  memset(&linkTemplate, 0, sizeof(linkTemplate));
  linkTemplate.bandwidth = this->cluster_->bw * numlinks;
  linkTemplate.latency = this->cluster_->lat;
  linkTemplate.policy = this->cluster_->sharing_policy; // sthg to do with that ?
  linkTemplate.id = id;
  sg_platf_new_link(&linkTemplate);
  XBT_DEBUG("Generating link %s", id);
  Link* link;
  std::string tmpID;
  if (this->cluster_->sharing_policy == SURF_LINK_FULLDUPLEX) {
    tmpID = std::string(linkTemplate.id) + "_UP";
    link =  Link::byName(tmpID.c_str());
    *linkup = link; // check link?
    tmpID = std::string(linkTemplate.id) + "_DOWN";
    link = Link::byName(tmpID.c_str());
    *linkdown = link; // check link ?
  }
  else {
    link = Link::byName(linkTemplate.id);
    *linkup = link;
    *linkdown = link;
  }

  free((void*)linkTemplate.id);
}


void AsClusterDragonfly::generateLinks() {

  unsigned int i, j, k, l;
  static int uniqueId = 0;
  char* id = nullptr;
  Link* linkup, *linkdown;

  unsigned int numRouters = this->numGroups_*this->numChassisPerGroup_*this->numBladesPerChassis_;

  if (this->cluster_->sharing_policy == SURF_LINK_FULLDUPLEX)
    numLinksperLink_=2;


  //Links from routers to their local nodes.
  for(i=0; i<numRouters;i++){
  //allocate structures
    this->routers_[i]->myNodes_=(Link**)xbt_malloc0(numLinksperLink_*this->numNodesPerBlade_*sizeof(Link*));
    this->routers_[i]->greenLinks_=(Link**)xbt_malloc0(this->numBladesPerChassis_*sizeof(Link*));
    this->routers_[i]->blackLinks_=(Link**)xbt_malloc0(this->numChassisPerGroup_*sizeof(Link*));

    for(j=0; j< numLinksperLink_*this->numNodesPerBlade_; j+=numLinksperLink_){
      id = bprintf("local_link_from_router_%d_to_node_%d_%d", i, j/numLinksperLink_, uniqueId);
      this->createLink(id, 1, &linkup, &linkdown);
      if (this->cluster_->sharing_policy == SURF_LINK_FULLDUPLEX) {
        this->routers_[i]->myNodes_[j] = linkup; 
        this->routers_[i]->myNodes_[j+1] = linkdown; 
      }
      else {
        this->routers_[i]->myNodes_[j] = linkup;
      }
      uniqueId++;
    }
  }

  //Green links from routers to same chassis routers - alltoall
  for(i=0; i<this->numGroups_*this->numChassisPerGroup_;i++){
    for(j=0; j<this->numBladesPerChassis_;j++){
      for(k=j+1;k<this->numBladesPerChassis_;k++){
        id = bprintf("green_link_in_chassis_%d_between_routers_%d_and_%d_%d", i%numChassisPerGroup_, j, k, uniqueId);
        this->createLink(id, this->numLinksGreen_, &linkup, &linkdown);
        this->routers_[i*numBladesPerChassis_+j]->greenLinks_[k] = linkup;
        this->routers_[i*numBladesPerChassis_+k]->greenLinks_[j] = linkdown; 
        uniqueId++;
      }
    }
  }

  //Black links from routers to same group routers - alltoall
  for(i=0; i<this->numGroups_;i++){
    for(j=0; j<this->numChassisPerGroup_;j++){
      for(k=j+1;k<this->numChassisPerGroup_;k++){
        for(l=0;l<this->numBladesPerChassis_;l++){
          id = bprintf("black_link_in_group_%d_between_chassis_%d_and_%d_blade_%d_%d", i, j, k,l, uniqueId);
          this->createLink(id, this->numLinksBlack_,&linkup, &linkdown);
          this->routers_[i*numBladesPerChassis_*numChassisPerGroup_+j*numBladesPerChassis_+l]->blackLinks_[k] = linkup;
          this->routers_[i*numBladesPerChassis_*numChassisPerGroup_+k*numBladesPerChassis_+l]->blackLinks_[j] = linkdown; 
          uniqueId++;
        }
      }
    }
  }


  //Blue links betweeen groups - Not all routers involved, only one per group is linked to others. Let's say router n of each group is linked to group n. 
//FIXME: in reality blue links may be attached to several different routers
  for(i=0; i<this->numGroups_;i++){
    for(j=i+1; j<this->numGroups_;j++){
      unsigned int routernumi=i*numBladesPerChassis_*numChassisPerGroup_+j;
      unsigned int routernumj=j*numBladesPerChassis_*numChassisPerGroup_+i;
      this->routers_[routernumi]->blueLinks_=(Link**)xbt_malloc0(sizeof(Link*));
      this->routers_[routernumj]->blueLinks_=(Link**)xbt_malloc0(sizeof(Link*));
        id = bprintf("blue_link_between_group_%d_and_%d_routers_%d_and_%d_%d", i, j, routernumi,routernumj, uniqueId);
        this->createLink(id, this->numLinksBlue_, &linkup, &linkdown);
        this->routers_[routernumi]->blueLinks_[0] = linkup;
        this->routers_[routernumj]->blueLinks_[0] = linkdown; 
        uniqueId++;
    }
  }
}


void AsClusterDragonfly::getRouteAndLatency(NetCard * src, NetCard * dst, sg_platf_route_cbarg_t route, double *latency) {

  //Minimal routing version.
  // TODO : non-minimal random one, and adaptive ?

  if (dst->isRouter() || src->isRouter())
    return;

  XBT_VERB("dragonfly_get_route_and_latency from '%s'[%d] to '%s'[%d]",
          src->name(), src->id(), dst->name(), dst->id());

  if ((src->id() == dst->id()) && hasLoopback_) {
     s_surf_parsing_link_up_down_t info = xbt_dynar_get_as(privateLinks_, src->id() * linkCountPerNode_, s_surf_parsing_link_up_down_t);

     route->link_list->push_back(info.linkUp);
     if (latency)
       *latency += info.linkUp->getLatency();
     return;
  }

  unsigned int *myCoords, *targetCoords;
  myCoords = rankId_to_coords(src->id());
  targetCoords = rankId_to_coords(dst->id());
  XBT_DEBUG("src : %u group, %u chassis, %u blade, %u node", myCoords[0], myCoords[1], myCoords[2], myCoords[3]);
  XBT_DEBUG("dst : %u group, %u chassis, %u blade, %u node", targetCoords[0], targetCoords[1], targetCoords[2], targetCoords[3]);

  DragonflyRouter* myRouter = routers_[myCoords[0]*(numChassisPerGroup_*numBladesPerChassis_)+myCoords[1] * numBladesPerChassis_+myCoords[2]];
  DragonflyRouter* targetRouter = routers_[targetCoords[0]*(numChassisPerGroup_*numBladesPerChassis_)+targetCoords[1] *numBladesPerChassis_ +targetCoords[2]];
  DragonflyRouter* currentRouter=myRouter;

  //node->router local link
  route->link_list->push_back(myRouter->myNodes_[myCoords[3]*numLinksperLink_]);
  if(latency) {
    *latency += myRouter->myNodes_[myCoords[3]*numLinksperLink_]->getLatency();
  }

  if (hasLimiter_) {    // limiter for sender
    s_surf_parsing_link_up_down_t info;
    info = xbt_dynar_get_as(privateLinks_, src->id() * linkCountPerNode_ + hasLoopback_, s_surf_parsing_link_up_down_t);
    route->link_list->push_back(info.linkUp);
  }

  if(targetRouter!=myRouter){

    //are we on a different group ?
    if(targetRouter->group_ != currentRouter->group_){
      //go to the router of our group connected to this one.
      if(currentRouter->blade_!=targetCoords[0]){
        //go to the nth router in our chassis
        route->link_list->push_back(currentRouter->greenLinks_[targetCoords[0]]);
        if(latency) {
          *latency += currentRouter->greenLinks_[targetCoords[0]]->getLatency();
        }
        currentRouter=routers_[myCoords[0]*(numChassisPerGroup_*numBladesPerChassis_)+myCoords[1] * numBladesPerChassis_+targetCoords[0]];
      }

      if(currentRouter->chassis_!=0){
        //go to the first chassis of our group
        route->link_list->push_back(currentRouter->blackLinks_[0]);
        if(latency) {
          *latency += currentRouter->blackLinks_[0]->getLatency();
        }
        currentRouter=routers_[myCoords[0]*(numChassisPerGroup_*numBladesPerChassis_)+targetCoords[0]];
      }

      //go to destination group - the only optical hop 
      route->link_list->push_back(currentRouter->blueLinks_[0]);
      if(latency) {
        *latency += currentRouter->blueLinks_[0]->getLatency();
      }
      currentRouter=routers_[targetCoords[0]*(numChassisPerGroup_*numBladesPerChassis_)+myCoords[0]];
    }

    
    //same group, but same blade ?
    if(targetRouter->blade_ != currentRouter->blade_){
      route->link_list->push_back(currentRouter->greenLinks_[targetCoords[2]]);
      if(latency) {
        *latency += currentRouter->greenLinks_[targetCoords[2]]->getLatency();
      }
      currentRouter=routers_[targetCoords[0]*(numChassisPerGroup_*numBladesPerChassis_)+targetCoords[2]];
    }

    //same blade, but same chassis ?
    if(targetRouter->chassis_ != currentRouter->chassis_){
      route->link_list->push_back(currentRouter->blackLinks_[targetCoords[1]]);
      if(latency) {
        *latency += currentRouter->blackLinks_[targetCoords[1]]->getLatency();
      }
      currentRouter=routers_[targetCoords[0]*(numChassisPerGroup_*numBladesPerChassis_)+targetCoords[1]*numBladesPerChassis_+targetCoords[2]];
    }
  }

  if (hasLimiter_) {    // limiter for receiver
    s_surf_parsing_link_up_down_t info;
    info = xbt_dynar_get_as(privateLinks_, dst->id() * linkCountPerNode_ + hasLoopback_, s_surf_parsing_link_up_down_t);
    route->link_list->push_back(info.linkUp);
  }

  //router->node local link
  route->link_list->push_back(targetRouter->myNodes_[targetCoords[3]*numLinksperLink_+numLinksperLink_-1]);
  if(latency) {
    *latency += targetRouter->myNodes_[targetCoords[3]*numLinksperLink_+numLinksperLink_-1]->getLatency();
  }

  xbt_free(myCoords);
  xbt_free(targetCoords);

  
}
  }
}
