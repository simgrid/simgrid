#include "surf_routing_cluster_fat_tree.hpp"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iostream>
#include <fstream>


AsClusterFatTree::AsClusterFatTree() : levels(0) {}

AsClusterFatTree::~AsClusterFatTree() {
  for (int i = 0 ; i < this->nodes.size() ; i++) {
    delete this->nodes[i];
  }
}

void AsClusterFatTree::getRouteAndLatency(RoutingEdgePtr src,
                                          RoutingEdgePtr dst,
                                          sg_platf_route_cbarg_t into,
                                          double *latency) const{
  // TODO
}

/* This function makes the assumption that parse_specific_arguments() and
 * addNodes() have already been called
 */
void AsClusterFatTree::create_links(sg_platf_cluster_cbarg_t cluster) {

  if(this->levels == 0) {
    return;
  }
  std::vector<int> nodesByLevel(this->levels); 
  int nodesRequired = 0;


    for (int i = 0 ; i < this->levels ; i++) {
      int nodesInThisLevel = 1;
      
      for (int j = 0 ;  j < i ; j++) {
        nodesInThisLevel *= this->upperLevelNodesNumber[j];
      }
      
      for (int j = i+1 ; j < this->levels ; j++) {
        nodesInThisLevel *= this->lowerLevelNodesNumber[j];
      }

      nodesByLevel[i] = nodesInThisLevel;
      nodesRequired += nodesInThisLevel;
    }
   
    if(nodesRequired > this->nodes.size()) {
      surf_parse_error("There is not enough nodes to fit to the described topology. Please check your platform description (We need %d nodes, we only got %lu)", nodesRequired, this->nodes.size());
      return;
    }

    // Nodes are totally ordered, by level and then by position, in this->nodes
    int k = 0;
    for (int i = 0 ; i < this->levels ; i++) {
      for (int j = 0 ; j < nodesByLevel[i] ; j++) {
        this->nodes[k]->level = i;
        this->nodes[k]->position = j;

        if (i == 0) {
          
        }
        else if (i == this->levels - 1) {
          
        }
        else {
          
        }
      }
    }
}


void AsClusterFatTree::addNodes(std::vector<int> const& id) {
  for (int i = 0 ; i < id.size() ; i++) {
    this->nodes.push_back(new FatTreeNode(id[i]));
  }
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


void AsClusterFatTree::generateDotFile(string filename) {
  ofstream file;
  /* Maybe should we get directly a char*, as open takes strings only beginning
   * with c++11...
   */
  file.open(filename.c_str(), ios::out | ios::trunc); 
  
  if(file.is_open()) {
    /* TODO : Iterate through a map takes 10 chars with c++11, 100 with c++98.
     * All I have to do is write it down...
     */

    // file << "graph AsClusterFatTree {\n";
    // for (std::map<std::pair<int,int>, FatTreeLink*>::iterator link = this->links.begin() ; link != this->links.end() ; link++ ) {
    //   for (int j = 0 ; j < link->ports ; j++) {
    //   file << this->links[i]->source.id 
    //        << " -- " this->links[i]->destination.id
    //        << ";\n";
    //   }
    // }
    // file << "}";
    // file.close();
  }
  else {
    std::cerr << "Unable to open file " << filename << std::endl;
    return;
  }
}

FatTreeNode::FatTreeNode(int id, int level, int position) : id(id),
                                                            level(level),
                                                            position(position){}
