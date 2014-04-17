#include "surf_routing_cluster_fat_tree.hpp"

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

void AsClusterFatTree::getRouteAndLatency(RoutingEdgePtr src,
                                          RoutingEdgePtr dst,
                                          sg_platf_route_cbarg_t into,
                                          double *latency) {
  // TODO
}

/* This function makes the assumption that parse_specific_arguments() and
 * addNodes() have already been called
 */
void AsClusterFatTree::create_links(sg_platf_cluster_cbarg_t cluster) {

  if(this->levels == 0) {
    return;
  }
  this->nodesByLevel.resize(this->levels, 0);
  unsigned int nodesRequired = 0;


    for (unsigned int i = 0 ; i < this->levels ; i++) {
      int nodesInThisLevel = 1;
      
      for (unsigned int j = 0 ;  j < i ; j++) {
        nodesInThisLevel *= this->upperLevelNodesNumber[j];
      }
      
      for (unsigned int j = i+1 ; j < this->levels ; j++) {
        nodesInThisLevel *= this->lowerLevelNodesNumber[j];
      }

      this->nodesByLevel[i] = nodesInThisLevel;
      nodesRequired += nodesInThisLevel;
    }
   
    if(nodesRequired > this->nodes.size()) {
      surf_parse_error("There is not enough nodes to fit to the described topology."
                       " Please check your platform description (We need %d nodes, we only got %lu)",
                       nodesRequired, this->nodes.size());
      return;
    }

    // Nodes are totally ordered, by level and then by position, in this->nodes
    int k = 0;
    for (unsigned int i = 0 ; i < this->levels ; i++) {
      for (unsigned int j = 0 ; j < this->nodesByLevel[i] ; j++) {
        this->nodes[k]->level = i;
        this->nodes[k]->position = j;
      }
    }

    
}

void AsClusterFatTree::getLevelPosition(const unsigned  int level, int &position, int &size) {
  if (level > this->levels - 1) {
    position = -1;
    size =  -1;
    return;
  }
  int tempPosition = 0;

  for (unsigned int i = 0 ; i < level ; i++) {
    tempPosition += this->nodesByLevel[i];
  }
  position = tempPosition;
  size = this->nodesByLevel[level];
}

void AsClusterFatTree::addNodes(std::vector<int> const& id) {
  for (unsigned int  i = 0 ; i < id.size() ; i++) {
    this->nodes.push_back(new FatTreeNode(id[i]));
  }
}

void AsClusterFatTree::addLink(FatTreeNode *parent, FatTreeNode *child) {
  using std::make_pair;
  if (parent->children.size() == this->nodesByLevel[parent->level] ||
      child->parents.size()   == this->nodesByLevel[child->level]) {
    /* NB : This case should never happen, if this private function is not misused,
     * so should we keep this test, keep it only for debug, throw an exception
     * or get rid of it ? In all cases, anytime we get in there, code should be
     * fixed
     */
    xbt_die("I've been asked to create a link that could not possibly exist");
    return;
  }

  parent->children.push_back(child);
  child->parents.push_back(parent);

  // FatTreeLink *newLink;

  // newLink = new FatTreeLink(parent, child, this->lowerLevelPortsNumber[parent->level]);
  // this->links.insert(make_pair(make_pair(parent->id, child->id), newLink));

  

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
  for(unsigned int i = 0 ; i < tmp.size() ; i++){
    this->lowerLevelNodesNumber.push_back(std::atoi(tmp[i].c_str())); 
  }
  
  // Then, a l-sized vector standing for the parents number by level
  boost::split(tmp, parameters[2], boost::is_any_of(","));
  if(tmp.size() != this->levels) {
    surf_parse_error("Fat trees are defined by the levels number and 3 vectors" 
                     ", see the documentation for more informations"); 
  }
  for(unsigned int i = 0 ; i < tmp.size() ; i++){
    this->upperLevelNodesNumber.push_back(std::atoi(tmp[i].c_str())); 
  }
  
  // Finally, a l-sized vector standing for the ports number with the lower level
  boost::split(tmp, parameters[3], boost::is_any_of(","));
  if(tmp.size() != this->levels) {
    surf_parse_error("Fat trees are defined by the levels number and 3 vectors" 
                     ", see the documentation for more informations"); 
    
  }
  for(unsigned int i = 0 ; i < tmp.size() ; i++){
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
    // std::map<std::pair<int,int>,FatTreeLink*>::iterator iter;
    // file << "graph AsClusterFatTree {\n";
    // for (iter = this->links.begin() ; iter != this->links.end() ; iter++ ) {
    //   for (int j = 0 ; j < iter->second->ports ; j++) {
    //     file << iter->second->source->id 
    //          << " -- " 
    //          << iter->second->destination->id
    //          << ";\n";
    //   }
    //}
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

// FatTreeLink::FatTreeLink(FatTreeNode *source, FatTreeNode *destination,
//                          int ports) : source(source), destination(destination),
//                                       ports(ports) {
  
// }
