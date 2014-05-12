#include "surf_routing_cluster_fat_tree.hpp"
#include "xbt/lib.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <iostream>
#include <fstream>



AsClusterFatTree::AsClusterFatTree() : levels(0) {}

AsClusterFatTree::~AsClusterFatTree() {
  for (unsigned int i = 0 ; i < this->nodes.size() ; i++) {
    delete this->nodes[i];
  }
}

bool AsClusterFatTree::isInSubTree(FatTreeNode *root, FatTreeNode *node) {
  for (unsigned int i = 0 ; i < node->level ; i++) {
    if(root->label[i] != node->label[i]) {
      return false;
    }
  }
  
  for (unsigned int i = root->level + 1 ; i < this->levels ; i++) {
    if(root->label[i] != node->label[i]) {
      return false;
    }
  }
  return true;
}

void AsClusterFatTree::getRouteAndLatency(RoutingEdgePtr src,
                                          RoutingEdgePtr dst,
                                          sg_platf_route_cbarg_t into,
                                          double *latency) {
  FatTreeNode *source, *destination, *currentNode;
  std::vector<NetworkLink*> route;
  source = this->nodes.find(src->getId())->second;
  destination = this->nodes.find(dst->getId())->second;

 

  currentNode = source;

  // up part
  while (!isInSubTree(currentNode, destination)) {
    int d, k; // as in d-mod-k
    d = destination->position;

    for (unsigned int i = 0 ; i < currentNode->level ; i++) {
      d /= this->upperLevelNodesNumber[i];
    }
     k = this->upperLevelNodesNumber[currentNode->level] *
      this->lowerLevelNodesNumber[currentNode->level];
     d = d % k;
     route.push_back(currentNode->parents[d]->upLink);
     if(latency) {
       *latency += currentNode->parents[d]->upLink->getLatency();
     }
     currentNode = currentNode->parents[d]->upNode;
  }
  
  // Down part
  while(currentNode != destination) {
    for(unsigned int i = 0 ; i < currentNode->children.size() ; i++) {
      if(i % this->lowerLevelNodesNumber[currentNode->level] ==
         destination->label[currentNode->level]) {
        route.push_back(currentNode->children[i]->downLink);
        if(latency) {
          *latency += currentNode->children[i]->downLink->getLatency();
        }
        currentNode = currentNode->children[i]->downNode;
      }
    }
  }
  
  for (unsigned int i = 0 ; i < route.size() ; i++) {
    xbt_dynar_push_as(into->link_list, void*, route[i]);
  }

}

/* This function makes the assumption that parse_specific_arguments() and
 * addNodes() have already been called
 */
void AsClusterFatTree::create_links(sg_platf_cluster_cbarg_t cluster){
  if(this->levels == 0) {
    return;
  }
  this->generateSwitches();
  this->generateLabels();
  unsigned int k = 0;
  // Nodes are totally ordered, by level and then by position, in this->nodes
  for (unsigned int i = 0 ; i < this->levels ; i++) {
    for (unsigned int j = 0 ; j < this->nodesByLevel[i] ; j++) {
      if(i != this->levels - 1) {
        for(unsigned int l = 0 ; l < this->nodesByLevel[i+1] ; l++) {
          this->connectNodeToParents(cluster, this->nodes[k]);
        }
      }
      k++;
    }
  }
}

int AsClusterFatTree::connectNodeToParents(sg_platf_cluster_cbarg_t cluster,
                                           FatTreeNode *node) {
  FatTreeNode* currentParentNode;
  int connectionsNumber = 0;
  const int level = node->level;
  currentParentNode = this->nodes[this->getLevelPosition(level + 1)];
  for (unsigned int i = 0 ; i < this->nodesByLevel[level] ; i++ ) {
    if(this->areRelated(currentParentNode, node)) {
      for (unsigned int j = 0 ; j < this->lowerLevelPortsNumber[level + 1] ; j++) {
      this->addLink(cluster, currentParentNode, node->label[level + 1] +
                    j * this->lowerLevelNodesNumber[level + 1], node,
                    currentParentNode->label[level + 1] +
                    j * this->upperLevelNodesNumber[level + 1]);
      }
      connectionsNumber++;
    }
  }
  return connectionsNumber;
}


bool AsClusterFatTree::areRelated(FatTreeNode *parent, FatTreeNode *child) {
  if (parent->level != child->level + 1) {
    return false;
  }
  
  for (unsigned int i = 0 ; i < this->levels; i++) {
    if (parent->label[i] != child->label[i] && i != parent->level) {
      return false;
    }
  }
  return true;
}

void AsClusterFatTree::generateSwitches() {
  this->nodesByLevel.resize(this->levels, 0);
  unsigned int nodesRequired = 0;

  // We take care of the number of nodes by level
  this->nodesByLevel[0] = 1;
  for (unsigned int i = 0 ; i < this->levels ; i++) {
    this->nodesByLevel[0] *= this->lowerLevelNodesNumber[i];
  }

     
  if(this->nodesByLevel[0] < this->nodes.size()) {
    surf_parse_error("There is not enough nodes to fit to the described topology."
                     " Please check your platform description (We need %d nodes, we only got %zu)",
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


  // If we have to many compute nodes, we ditch them
  if (this->nodesByLevel[0] > this->nodes.size()) {
    for (unsigned int i = this->nodesByLevel[0] ; i < this->nodes.size() ; i++) {
      delete this->nodes[i];
    }
  }

  // We create the switches
  int k = 0;
  for (unsigned int i = 0 ; i < this->levels ; i++) {
    for (unsigned int j = 0 ; j < this->nodesByLevel[i] ; j++) {
      FatTreeNode* newNode;
      newNode = new FatTreeNode(--k, i + 1, j);
      newNode->children.resize(this->lowerLevelNodesNumber[i] *
                               this->lowerLevelPortsNumber[i]);
      if (i != this->levels - 1) {
        newNode->parents.resize(this->upperLevelNodesNumber[i + 1]);
      }
      this->nodes.insert(std::make_pair(k,newNode));
    }
  }
}

void AsClusterFatTree::generateLabels() {
  // TODO : check if nodesByLevel and nodes are filled
  for (unsigned int i = 0 ; i < this->levels ; i++) {
    std::vector<int> maxLabel(this->nodesByLevel[i]);
    std::vector<int> currentLabel(this->nodesByLevel[i], 0);
    unsigned int k = 0;
    for (unsigned int j = 0 ; j < this->nodesByLevel[i] ; j++) {
      maxLabel[j] = j > i ?
        this->lowerLevelNodesNumber[i] : this->upperLevelNodesNumber[i];
    }
    
    for (unsigned int j = 0 ; j < this->nodesByLevel[i] ; j++) {
      this->nodes[k]->label.assign(currentLabel.begin(), currentLabel.end());

      int remainder = 0;
      
      do {
        int pos = currentLabel.size() - 1;
        remainder = ++currentLabel[pos] / maxLabel[pos];
        currentLabel[pos] = currentLabel[pos] % maxLabel[pos];
        --pos;
      }
      while(remainder != 0);
        k++;
    }
  }
}


int AsClusterFatTree::getLevelPosition(const unsigned  int level) {
  if (level > this->levels - 1) {
    // Well, that should never happen. Maybe should we throw instead.
    return -1;
  }
  int tempPosition = 0;

  for (unsigned int i = 0 ; i < level ; i++) {
    tempPosition += this->nodesByLevel[i];
  }
 return tempPosition;
}

void AsClusterFatTree::addComputeNodes(std::vector<int> const& id) {
  using std::make_pair;
  FatTreeNode* newNode;
  for (size_t  i = 0 ; i < id.size() ; i++) {
    newNode = new FatTreeNode(id[i], 0, i);
    newNode->parents.resize(this->upperLevelNodesNumber[0] * this->lowerLevelPortsNumber[i]);
    this->nodes.insert(make_pair(id[i],newNode));
  }
}

void AsClusterFatTree::addLink(sg_platf_cluster_cbarg_t cluster, 
                               FatTreeNode *parent, unsigned int parentPort,
                               FatTreeNode *child, unsigned int childPort) {
  FatTreeLink *newLink;
  newLink = new FatTreeLink(cluster, parent, child);

  parent->children[parentPort] = newLink;
  child->parents[childPort] = newLink;

  this->links.push_back(newLink);

  

}

void AsClusterFatTree::parse_specific_arguments(sg_platf_cluster_cbarg_t 
                                                cluster) {
  std::vector<string> parameters;
  std::vector<string> tmp;
  boost::split(parameters, cluster->topo_parameters, boost::is_any_of(";"));
 

  // TODO : we have to check for zeros and negative numbers, or it might crash
  if (parameters.size() != 4){
    surf_parse_error("Fat trees are defined by the levels number and 3 vectors" 
                     ", see the documentation for more informations");
    // Well, there's no doc, yet
  }

  // The first parts of topo_parameters should be the levels number
  this->levels = std::atoi(tmp[0].c_str()); // stoi() only in C++11...
  
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
}


void AsClusterFatTree::generateDotFile(const string& filename) const {
  ofstream file;
  /* Maybe should we get directly a char*, as open takes strings only beginning
   * with C++11...
   */
  file.open(filename.c_str(), ios::out | ios::trunc); 
  
  if(file.is_open()) {
    // That could also be greatly clarified with C++11
    std::vector<FatTreeLink*>::const_iterator iter;
    file << "graph AsClusterFatTree {\n";
    for (iter = this->links.begin() ; iter != this->links.end() ; iter++ ) {
      file << (*iter)->downNode->id
             << " -- "
           << (*iter)->upNode->id
             << ";\n";
    }
    file << "}";
    file.close();
  }
  else {
    std::cerr << "Unable to open file " << filename << std::endl;
    return;
  }
}

FatTreeNode::FatTreeNode(int id, int level, int position) : id(id),
                                                            level(level),
                                                            position(position){}

FatTreeLink::FatTreeLink(sg_platf_cluster_cbarg_t cluster, FatTreeNode *downNode,
                         FatTreeNode *upNode) : upNode(upNode),
                                                downNode(downNode) {
  static int uniqueId = 0;
  s_sg_platf_link_cbarg_t linkTemplate;
  linkTemplate.bandwidth = cluster->bw;
  linkTemplate.latency = cluster->lat;
  linkTemplate.state = SURF_RESOURCE_ON;
  linkTemplate.policy = cluster->sharing_policy; // Maybe should we do sthg with that ?


  NetworkLink* link;
  linkTemplate.id = bprintf("link_from_%d_to_%d_%d_UP", downNode->id, upNode->id, uniqueId);
  sg_platf_new_link(&linkTemplate);
  link = (NetworkLink*) xbt_lib_get_or_null(link_lib, linkTemplate.id, SURF_LINK_LEVEL);
  this->upLink = link; // check link?
  linkTemplate.id = bprintf("link_from_%d_to_%d_%d_DOWN", downNode->id, upNode->id, uniqueId);
  sg_platf_new_link(&linkTemplate);
  link = (NetworkLink*) xbt_lib_get_or_null(link_lib, linkTemplate.id, SURF_LINK_LEVEL);
  this->downLink = link; // check link ?

  uniqueId++;

}
