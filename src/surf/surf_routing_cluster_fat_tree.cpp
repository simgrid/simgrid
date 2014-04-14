#include "surf_routing_cluster_fat_tree.hpp"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>


AsClusterFatTree::AsClusterFatTree() : levels(0) {}



void AsClusterFatTree::getRouteAndLatency(RoutingEdgePtr src,
                                          RoutingEdgePtr dst,
                                          sg_platf_route_cbarg_t into,
                                          double *latency) {
  // TODO
}

void AsClusterFatTree::create_links() {
  if(this->levels == 0) {
    return;
  }
  
  //  for (unsigned int i = 0 ; i < this->levels)

}



void AsClusterFatTree::parse_specific_arguments(sg_platf_cluster_cbarg_t 
                                                cluster) {
  std::vector<string> parameters;
  std::vector<string> tmp;
  boost::split(parameters, cluster->topo_parameters, boost::is_any_of(";"));
  
  if (parameters.size() != 4){
    surf_parse_error("Fat trees are defined by the levels number et 3 vectors," 
                     " see the documentation for more informations");
    // Well, there's no doc, yet
  }

  // The first parts of topo_parameters should be the levels number
  this->levels = std::atoi(tmp[0].c_str());

  // Then, a l-sized vector standing for the childs number by level
  boost::split(tmp, parameters[1], boost::is_any_of(","));
  if(tmp.size() != this->levels) {
    surf_parse_error("Fat trees are defined by the levels number et 3 vectors," 
                     " see the documentation for more informations"); 
  }
  for(unsigned int i = 0 ; i < tmp.size() ; i++){
    this->lowerLevelNodesNumber.push_back(std::atoi(tmp[i].c_str())); 
  }
  
  // Then, a l-sized vector standing for the parents number by level
   boost::split(tmp, parameters[2], boost::is_any_of(","));
  if(tmp.size() != this->levels) {
    surf_parse_error("Fat trees are defined by the levels number et 3 vectors," 
                     " see the documentation for more informations"); 
  }
  for(unsigned int i = 0 ; i < tmp.size() ; i++){
    this->upperLevelNodesNumber.push_back(std::atoi(tmp[i].c_str())); 
  }
  
  // Finally, a l-sized vector standing for the ports number with the lower level
  boost::split(tmp, parameters[3], boost::is_any_of(","));
  if(tmp.size() != this->levels) {
    surf_parse_error("Fat trees are defined by the levels number et 3 vectors," 
                     " see the documentation for more informations"); 
    
  }
  for(unsigned int i = 0 ; i < tmp.size() ; i++){
    this->lowerLevelPortsNumber.push_back(std::atoi(tmp[i].c_str())); 
  }
}
  
