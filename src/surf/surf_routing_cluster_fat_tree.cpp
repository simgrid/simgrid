#include <cstdlib>

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <iostream>

#include "surf_routing_cluster_fat_tree.hpp"
#include "xbt/lib.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_route_fat_tree, surf, "Routing for fat trees");

AS_t model_fat_tree_cluster_create(void)
{
  return new simgrid::surf::AsClusterFatTree();
}

namespace simgrid {
namespace surf {

AsClusterFatTree::AsClusterFatTree() : levels(0) {
  XBT_DEBUG("Creating a new fat tree.");
}

AsClusterFatTree::~AsClusterFatTree() {
  for (unsigned int i = 0 ; i < this->nodes.size() ; i++) {
    delete this->nodes[i];
  }
  for (unsigned int i = 0 ; i < this->links.size() ; i++) {
    delete this->links[i];
  }
}

bool AsClusterFatTree::isInSubTree(FatTreeNode *root, FatTreeNode *node) {
  XBT_DEBUG("Is %d(%u,%u) in the sub tree of %d(%u,%u) ?", node->id,
            node->level, node->position, root->id, root->level, root->position);
  if (root->level <= node->level) {
    return false;
  }
  for (unsigned int i = 0 ; i < node->level ; i++) {
    if(root->label[i] != node->label[i]) {
      return false;
    }
  }
  
  for (unsigned int i = root->level ; i < this->levels ; i++) {
    if(root->label[i] != node->label[i]) {
      return false;
    }
  }
  return true;
}

void AsClusterFatTree::getRouteAndLatency(NetCard *src,
                                          NetCard *dst,
                                          sg_platf_route_cbarg_t into,
                                          double *latency) {
  FatTreeNode *source, *destination, *currentNode;

  std::map<int, FatTreeNode*>::const_iterator tempIter;
  
if (dst->getRcType() == SURF_NETWORK_ELEMENT_ROUTER || src->getRcType() == SURF_NETWORK_ELEMENT_ROUTER) return;

  /* Let's find the source and the destination in our internal structure */
  tempIter = this->computeNodes.find(src->getId());

  // xbt_die -> assert
  if (tempIter == this->computeNodes.end()) {
    xbt_die("Could not find the source %s [%d] in the fat tree", src->getName(),
            src->getId());
  }
  source = tempIter->second;
  tempIter = this->computeNodes.find(dst->getId());
  if (tempIter == this->computeNodes.end()) {
    xbt_die("Could not find the destination %s [%d] in the fat tree",
            dst->getName(), dst->getId());
  }


  destination = tempIter->second;
  
  XBT_VERB("Get route and latency from '%s' [%d] to '%s' [%d] in a fat tree",
            src->getName(), src->getId(), dst->getName(), dst->getId());

  /* In case destination is the source, and there is a loopback, let's get
     through it instead of going up to a switch*/
  if(source->id == destination->id && this->p_has_loopback) {
    xbt_dynar_push_as(into->link_list, void*, source->loopback);
    if(latency) {
      *latency += source->loopback->getLatency();
    }
    return;
  }

  currentNode = source;

  // up part
  while (!isInSubTree(currentNode, destination)) {
    int d, k; // as in d-mod-k
    d = destination->position;

    for (unsigned int i = 0 ; i < currentNode->level ; i++) {
      d /= this->upperLevelNodesNumber[i];
    }
    k = this->upperLevelNodesNumber[currentNode->level];
    d = d % k;
    xbt_dynar_push_as(into->link_list, void*,currentNode->parents[d]->upLink);

    if(latency) {
      *latency += currentNode->parents[d]->upLink->getLatency();
    }

    if (this->p_has_limiter) {
      xbt_dynar_push_as(into->link_list, void*,currentNode->limiterLink);
    }
    currentNode = currentNode->parents[d]->upNode;
  }

  XBT_DEBUG("%d(%u,%u) is in the sub tree of %d(%u,%u).", destination->id,
            destination->level, destination->position, currentNode->id,
            currentNode->level, currentNode->position);

  // Down part
  while(currentNode != destination) {
    for(unsigned int i = 0 ; i < currentNode->children.size() ; i++) {
      if(i % this->lowerLevelNodesNumber[currentNode->level - 1] ==
         destination->label[currentNode->level - 1]) {
        xbt_dynar_push_as(into->link_list, void*,currentNode->children[i]->downLink);
        if(latency) {
          *latency += currentNode->children[i]->downLink->getLatency();
        }
        currentNode = currentNode->children[i]->downNode;
        if (this->p_has_limiter) {
          xbt_dynar_push_as(into->link_list, void*,currentNode->limiterLink);
        }
        XBT_DEBUG("%d(%u,%u) is accessible through %d(%u,%u)", destination->id,
                  destination->level, destination->position, currentNode->id,
                  currentNode->level, currentNode->position);
      }
    }
  }
}

/* This function makes the assumption that parse_specific_arguments() and
 * addNodes() have already been called
 */
void AsClusterFatTree::create_links(){
  if(this->levels == 0) {
    return;
  }
  this->generateSwitches();


  if(XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
    std::stringstream msgBuffer;

    msgBuffer << "We are creating a fat tree of " << this->levels << " levels "
              << "with " << this->nodesByLevel[0] << " processing nodes";
    for (unsigned int i = 1 ; i <= this->levels ; i++) {
      msgBuffer << ", " << this->nodesByLevel[i] << " switches at level " << i;
    }
    XBT_DEBUG("%s", msgBuffer.str().c_str());
    msgBuffer.str("");
    msgBuffer << "Nodes are : ";

    for (unsigned int i = 0 ;  i < this->nodes.size() ; i++) {
      msgBuffer << this->nodes[i]->id << "(" << this->nodes[i]->level << ","
                << this->nodes[i]->position << ") ";
    }
    XBT_DEBUG("%s", msgBuffer.str().c_str());
  }


  this->generateLabels();

  unsigned int k = 0;
  // Nodes are totally ordered, by level and then by position, in this->nodes
  for (unsigned int i = 0 ; i < this->levels ; i++) {
    for (unsigned int j = 0 ; j < this->nodesByLevel[i] ; j++) {
        this->connectNodeToParents(this->nodes[k]);
        k++;
    }
  }
  
  if(XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
    std::stringstream msgBuffer;
    msgBuffer << "Links are : ";
    for (unsigned int i = 0 ; i < this->links.size() ; i++) {
      msgBuffer << "(" << this->links[i]->upNode->id << ","
                << this->links[i]->downNode->id << ") ";
    }
    XBT_DEBUG("%s", msgBuffer.str().c_str());
  }


}

int AsClusterFatTree::connectNodeToParents(FatTreeNode *node) {
  std::vector<FatTreeNode*>::iterator currentParentNode = this->nodes.begin();
  int connectionsNumber = 0;
  const int level = node->level;
  XBT_DEBUG("We are connecting node %d(%u,%u) to his parents.",
            node->id, node->level, node->position);
  currentParentNode += this->getLevelPosition(level + 1);
  for (unsigned int i = 0 ; i < this->nodesByLevel[level + 1] ; i++ ) {
    if(this->areRelated(*currentParentNode, node)) {
      XBT_DEBUG("%d(%u,%u) and %d(%u,%u) are related,"
                " with %u links between them.", node->id,
                node->level, node->position, (*currentParentNode)->id,
                (*currentParentNode)->level, (*currentParentNode)->position, this->lowerLevelPortsNumber[level]);
      for (unsigned int j = 0 ; j < this->lowerLevelPortsNumber[level] ; j++) {
      this->addLink(*currentParentNode, node->label[level] +
                    j * this->lowerLevelNodesNumber[level], node,
                    (*currentParentNode)->label[level] +
                    j * this->upperLevelNodesNumber[level]);
      }
      connectionsNumber++;
    }
    ++currentParentNode;
  }
  return connectionsNumber;
}


bool AsClusterFatTree::areRelated(FatTreeNode *parent, FatTreeNode *child) {
  std::stringstream msgBuffer;

  if(XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug)) {
    msgBuffer << "Are " << child->id << "(" << child->level << ","
              << child->position << ") <";

    for (unsigned int i = 0 ; i < this->levels ; i++) {
      msgBuffer << child->label[i] << ",";
    }
    msgBuffer << ">";
    
    msgBuffer << " and " << parent->id << "(" << parent->level
              << "," << parent->position << ") <";
    for (unsigned int i = 0 ; i < this->levels ; i++) {
      msgBuffer << parent->label[i] << ",";
    }
    msgBuffer << ">";
    msgBuffer << " related ? ";
    XBT_DEBUG("%s", msgBuffer.str().c_str());
    
  }
  if (parent->level != child->level + 1) {
    return false;
  }
  
  for (unsigned int i = 0 ; i < this->levels; i++) {
    if (parent->label[i] != child->label[i] && i + 1 != parent->level) {
      return false;
    }
  }
  return true;
}

void AsClusterFatTree::generateSwitches() {
  XBT_DEBUG("Generating switches.");
  this->nodesByLevel.resize(this->levels + 1, 0);
  unsigned int nodesRequired = 0;

  // We take care of the number of nodes by level
  this->nodesByLevel[0] = 1;
  for (unsigned int i = 0 ; i < this->levels ; i++) {
    this->nodesByLevel[0] *= this->lowerLevelNodesNumber[i];
  }

     
  if(this->nodesByLevel[0] != this->nodes.size()) {
    surf_parse_error("The number of provided nodes does not fit with the wanted topology."
                     " Please check your platform description (We need %d nodes, we got %zu)",
                     this->nodesByLevel[0], this->nodes.size());
    return;
  }

  
  for (unsigned int i = 0 ; i < this->levels ; i++) {
    int nodesInThisLevel = 1;
      
    for (unsigned int j = 0 ;  j <= i ; j++) {
      nodesInThisLevel *= this->upperLevelNodesNumber[j];
    }
      
    for (unsigned int j = i+1 ; j < this->levels ; j++) {
      nodesInThisLevel *= this->lowerLevelNodesNumber[j];
    }

    this->nodesByLevel[i+1] = nodesInThisLevel;
    nodesRequired += nodesInThisLevel;
  }


  // We create the switches
  int k = 0;
  for (unsigned int i = 0 ; i < this->levels ; i++) {
    for (unsigned int j = 0 ; j < this->nodesByLevel[i + 1] ; j++) {
      FatTreeNode* newNode;
      newNode = new FatTreeNode(this->cluster, --k, i + 1, j);
      XBT_DEBUG("We create the switch %d(%d,%d)", newNode->id, newNode->level,
                newNode->position);
      newNode->children.resize(this->lowerLevelNodesNumber[i] *
                               this->lowerLevelPortsNumber[i]);
      if (i != this->levels - 1) {
        newNode->parents.resize(this->upperLevelNodesNumber[i + 1] *
                                this->lowerLevelPortsNumber[i + 1]);
      }
      newNode->label.resize(this->levels);
      this->nodes.push_back(newNode);
    }
  }
}

void AsClusterFatTree::generateLabels() {
  XBT_DEBUG("Generating labels.");
  // TODO : check if nodesByLevel and nodes are filled
  std::vector<int> maxLabel(this->levels);
  std::vector<int> currentLabel(this->levels);
  unsigned int k = 0;
  for (unsigned int i = 0 ; i <= this->levels ; i++) {
    currentLabel.assign(this->levels, 0);
    for (unsigned int j = 0 ; j < this->levels ; j++) {
      maxLabel[j] = j + 1 > i ?
        this->lowerLevelNodesNumber[j] : this->upperLevelNodesNumber[j];
    }
    
    for (unsigned int j = 0 ; j < this->nodesByLevel[i] ; j++) {

      if(XBT_LOG_ISENABLED(surf_route_fat_tree, xbt_log_priority_debug )) {
        std::stringstream msgBuffer;

        msgBuffer << "Assigning label <";
        for (unsigned int l = 0 ; l < this->levels ; l++) {
          msgBuffer << currentLabel[l] << ",";
        }
        msgBuffer << "> to " << k << " (" << i << "," << j <<")";
        
        XBT_DEBUG("%s", msgBuffer.str().c_str());
      }
      this->nodes[k]->label.assign(currentLabel.begin(), currentLabel.end());

      bool remainder = true;
      
      unsigned int pos = 0;
      do {
        std::stringstream msgBuffer;

        ++currentLabel[pos];
        if (currentLabel[pos] >= maxLabel[pos]) {
          currentLabel[pos] = 0;
          remainder = true;
        }
        else {
          remainder = false;
        }
        if (!remainder) {
          pos = 0;
        }
        else {
          ++pos;
        }
      }
      while(remainder && pos < this->levels);
      k++;
    }
  }
}


int AsClusterFatTree::getLevelPosition(const unsigned  int level) {
  if (level > this->levels) {
    // Well, that should never happen. Maybe should we throw instead.
    return -1;
  }
  int tempPosition = 0;

  for (unsigned int i = 0 ; i < level ; i++) {
    tempPosition += this->nodesByLevel[i];
  }
 return tempPosition;
}

void AsClusterFatTree::addProcessingNode(int id) {
  using std::make_pair;
  static int position = 0;
  FatTreeNode* newNode;
  newNode = new FatTreeNode(this->cluster, id, 0, position++);
  newNode->parents.resize(this->upperLevelNodesNumber[0] *
                          this->lowerLevelPortsNumber[0]);
  newNode->label.resize(this->levels);
  this->computeNodes.insert(make_pair(id,newNode));
  this->nodes.push_back(newNode);
}

void AsClusterFatTree::addLink(FatTreeNode *parent, unsigned int parentPort,
                               FatTreeNode *child, unsigned int childPort) {
  FatTreeLink *newLink;
  newLink = new FatTreeLink(this->cluster, child, parent);
  XBT_DEBUG("Creating a link between the parent (%d,%d,%u)"
            " and the child (%d,%d,%u)", parent->level, parent->position,
            parentPort, child->level, child->position, childPort);
  parent->children[parentPort] = newLink;
  child->parents[childPort] = newLink;

  this->links.push_back(newLink);

  

}

void AsClusterFatTree::parse_specific_arguments(sg_platf_cluster_cbarg_t 
                                                cluster) {
  std::vector<std::string> parameters;
  std::vector<std::string> tmp;
  boost::split(parameters, cluster->topo_parameters, boost::is_any_of(";"));
 

  // TODO : we have to check for zeros and negative numbers, or it might crash
  if (parameters.size() != 4){
    surf_parse_error("Fat trees are defined by the levels number and 3 vectors" 
                     ", see the documentation for more informations");
  }

  // The first parts of topo_parameters should be the levels number
  this->levels = std::atoi(parameters[0].c_str()); // stoi() only in C++11...
  
  // Then, a l-sized vector standing for the childs number by level
  boost::split(tmp, parameters[1], boost::is_any_of(","));
  if(tmp.size() != this->levels) {
    surf_parse_error("Fat trees are defined by the levels number and 3 vectors" 
                     ", see the documentation for more informations"); 
  }
  for(size_t i = 0 ; i < tmp.size() ; i++){
    this->lowerLevelNodesNumber.push_back(std::atoi(tmp[i].c_str())); 
  }
  
  // Then, a l-sized vector standing for the parents number by level
  boost::split(tmp, parameters[2], boost::is_any_of(","));
  if(tmp.size() != this->levels) {
    surf_parse_error("Fat trees are defined by the levels number and 3 vectors" 
                     ", see the documentation for more informations"); 
  }
  for(size_t i = 0 ; i < tmp.size() ; i++){
    this->upperLevelNodesNumber.push_back(std::atoi(tmp[i].c_str())); 
  }
  
  // Finally, a l-sized vector standing for the ports number with the lower level
  boost::split(tmp, parameters[3], boost::is_any_of(","));
  if(tmp.size() != this->levels) {
    surf_parse_error("Fat trees are defined by the levels number and 3 vectors" 
                     ", see the documentation for more informations"); 
    
  }
  for(size_t i = 0 ; i < tmp.size() ; i++){
    this->lowerLevelPortsNumber.push_back(std::atoi(tmp[i].c_str())); 
  }
  this->cluster = cluster;
}


void AsClusterFatTree::generateDotFile(const std::string& filename) const {
  std::ofstream file;
  /* Maybe should we get directly a char*, as open takes strings only beginning
   * with C++11...
   */
  file.open(filename.c_str(), std::ios::out | std::ios::trunc); 
  
  if(file.is_open()) {
    file << "graph AsClusterFatTree {\n";
    for (unsigned int i = 0 ; i < this->nodes.size() ; i++) {
      file << this->nodes[i]->id;
      if(this->nodes[i]->id < 0) {
        file << " [shape=circle];\n";
      }
      else {
        file << " [shape=hexagon];\n";
      }
    }

    for (unsigned int i = 0 ; i < this->links.size() ; i++ ) {
      file << this->links[i]->downNode->id
             << " -- "
           << this->links[i]->upNode->id
             << ";\n";
    }
    file << "}";
    file.close();
  }
  else {
    XBT_DEBUG("Unable to open file %s", filename.c_str());
    return;
  }
}

FatTreeNode::FatTreeNode(sg_platf_cluster_cbarg_t cluster, int id, int level,
                         int position) : id(id), level(level),
                                         position(position) {
  s_sg_platf_link_cbarg_t linkTemplate = SG_PLATF_LINK_INITIALIZER;
  if(cluster->limiter_link) {
    memset(&linkTemplate, 0, sizeof(linkTemplate));
    linkTemplate.bandwidth = cluster->limiter_link;
    linkTemplate.latency = 0;
    linkTemplate.state = SURF_RESOURCE_ON;
    linkTemplate.policy = SURF_LINK_SHARED;
    linkTemplate.id = bprintf("limiter_%d", id);
    sg_platf_new_link(&linkTemplate);
    this->limiterLink = Link::byName(linkTemplate.id);
    free((void*)linkTemplate.id);
  }
  if(cluster->loopback_bw || cluster->loopback_lat) {
    memset(&linkTemplate, 0, sizeof(linkTemplate));
    linkTemplate.bandwidth = cluster->loopback_bw;
    linkTemplate.latency = cluster->loopback_lat;
    linkTemplate.state = SURF_RESOURCE_ON;
    linkTemplate.policy = SURF_LINK_FATPIPE;
    linkTemplate.id = bprintf("loopback_%d", id);
    sg_platf_new_link(&linkTemplate);
    this->loopback = Link::byName(linkTemplate.id);
    free((void*)linkTemplate.id);
  }  
}

FatTreeLink::FatTreeLink(sg_platf_cluster_cbarg_t cluster,
                         FatTreeNode *downNode,
                         FatTreeNode *upNode) : upNode(upNode),
                                                downNode(downNode) {
  static int uniqueId = 0;
  s_sg_platf_link_cbarg_t linkTemplate = SG_PLATF_LINK_INITIALIZER;
  memset(&linkTemplate, 0, sizeof(linkTemplate));
  linkTemplate.bandwidth = cluster->bw;
  linkTemplate.latency = cluster->lat;
  linkTemplate.state = SURF_RESOURCE_ON;
  linkTemplate.policy = cluster->sharing_policy; // sthg to do with that ?
  linkTemplate.id = bprintf("link_from_%d_to_%d_%d", downNode->id, upNode->id,
                            uniqueId);
  sg_platf_new_link(&linkTemplate);
  Link* link;
  std::string tmpID;
  if (cluster->sharing_policy == SURF_LINK_FULLDUPLEX) {
    tmpID = std::string(linkTemplate.id) + "_UP";
    link =  Link::byName(tmpID.c_str());
    this->upLink = link; // check link?
    tmpID = std::string(linkTemplate.id) + "_DOWN";
    link = Link::byName(tmpID.c_str());
    this->downLink = link; // check link ?
  }
  else {
    link = Link::byName(linkTemplate.id);
    this->upLink = link;
    this->downLink = link;
  }
  uniqueId++;
  free((void*)linkTemplate.id);
}

}
}
