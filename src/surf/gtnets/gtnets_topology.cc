
//SGNode, SGTopology: tmporary classes for GTNetS topology.

#include "gtnets_topology.h"

// 
//  SGNode
// 
//  TODO when merging a node, remember to add the existing "hosts".
SGNode::SGNode(int id, int hostid){
  ID_ = id;
  if (hostid >= 0)
    hosts_.push_back(hostid);
}

SGNode::~SGNode(){
  //TODO
}

void SGNode::add_link(SGLink* newlink){
  links_.push_back(newlink);
}


SGLink* SGNode::other_link(int linkid){
  for (int i = 0; i < links_.size(); i++){
    if (links_[i]->id() != linkid)
      return links_[i];
  }
  return NULL;
}

bool SGNode::has_link(SGLink* link){
  for (int i = 0; i < links_.size(); i++){
    //TODO can compare by object itself?
    if ((links_[i]->id()) == (link->id()))
      return true;
  }
  return false;
}

void SGNode::print_hosts(){
  cout << "hosts[";
  for (int i = 0; i < hosts_.size(); i++){
    cout << hosts_[i] << ",";
  }
  cout << "] ";
}

void SGNode::print_links(){
  cout << "links[";
  for (int i = 0; i < links_.size(); i++){
    cout << links_[i]->id() << ",";
  }
  cout << "]" << endl;
}

vector<SGLink*>& SGNode::links(){
  return links_;
}
vector<int>& SGNode::hosts(){
  return hosts_;
}

//
//  SGLink
//
SGLink::SGLink(int id, SGNode* left, SGNode* right)
  :ID_(id),
   left_node_(left),
   right_node_(right){}


SGLink::~SGLink(){

}

// add_left_linK: tow things.
// add the link to the corresponding node.
// add the corresponding node to the new link. 
//  (change from the tmp node to the correct node)
void SGLink::add_left_link(SGLink* newlink, int side){
  if (!left_node_){
    //if alllinks don't have a node, then copy it from
    //tmporary link.
    if (side == LEFTSIDE){
      left_node_ = newlink->left_node_;
    } else if (side == RIGHTSIDE){
      left_node_ = newlink->right_node_;
    } else {
      cout << "should not come here. side: " << side << endl;
    }
  } else {
    // if already exists, then add the new link to the existing node.
    left_node_->add_link(newlink);
  }

  if (side == LEFTSIDE){
    newlink->left_node_ = left_node_;
    //printf("link[%d] new left node: %d\n", link.ID, @left_node.ID)
  }else if (side == RIGHTSIDE){
    newlink->right_node_ = left_node_;
    //printf("link[%d] new left node: %d\n", link.ID, @left_node.ID)
  }else{
    cout << "should not come here. side: " << side << endl;
  }

}

void SGLink::add_right_link(SGLink* newlink, int side){
  if (!right_node_) {
    //if alllinks doesn't have a node, then copy it from
    //tmporary link.
    if (side == LEFTSIDE){
      right_node_ = newlink->left_node_;
    }else if (side == RIGHTSIDE){
      right_node_ = newlink->right_node_;
    }else{
      cout << "should not come here. side: " << side << endl;
    }
  }else{
    right_node_->add_link(newlink);
  }

  if (side == LEFTSIDE){
    newlink->left_node_ = right_node_;
    //printf("link[%d] new left node: %d\n", link.ID, @right_node.ID)
  }else if (side == RIGHTSIDE){
    newlink->right_node_ = right_node_;
    //printf("link[%d] new right node: %d\n", link.ID, @right_node.ID)
  }else{
    cout << "should not come here. side: " << side << endl;
  }
}

bool SGLink::is_inleft(SGLink* link){
  if (!left_node_)
    return false;
  else
    return left_node_->has_link(link);
}

bool SGLink::is_inright(SGLink* link){
  if (!right_node_)
    return false;
  else
    return right_node_->has_link(link);
}


//return the pointer for the left link.
SGLink* SGLink::left_link(){
  if (!left_node_){
    return NULL;
  }else{
    return left_node_->other_link(ID_);
  }
}

SGLink* SGLink::right_link(){
  if (!right_node_){
    return NULL;
  }else{
    return right_node_->other_link(ID_);
  }

}

SGNode* SGLink::left_node(){
  return left_node_;
}


SGNode* SGLink::right_node(){
  return right_node_;
}


void SGLink::print(){
  printf("link[%d]:\n", ID_);
  if (left_node_){
    printf("   left  node: %d ", left_node_->id());
    left_node_->print_hosts();
    left_node_->print_links();
  }
  
  if (right_node_){
    printf("   right node: %d ", right_node_->id());
    right_node_->print_hosts();
    right_node_->print_links();
  }
}


SGTopology::SGTopology(){
  nodeID_ = 0;
}

SGTopology::~SGTopology(){

}


// TODO: assume that all router-route 1-hop routes are set.
void SGTopology::add_link(int src, int dst, int* links, int nsize){
  if (nsize == 1){
    map<int, SGNode*>::iterator it;
    it = nodes_.find(src);
    //if not exists, add one.
    if (it == nodes_.end()){    
      nodes_[src] = new SGNode(src, src);
    }
    it = nodes_.find(dst);
    //if not exists, add one.
    if (it == nodes_.end()){    
      nodes_[dst] = new SGNode(dst, dst);
    }

    map<int, SGLink*>::iterator itl;
    itl = links_.find(links[0]);
    //if not exists, add one.
    if (itl == links_.end()){
      links_[links[0]] = new SGLink(links[0], nodes_[src], nodes_[dst]);
    }
  }
}


//create a temporary link set. (one route)
// not used now. should clean up...
void SGTopology::create_tmplink(int src, int dst, int* links, int nsize){
  map<int, SGLink*> tmplinks;

  SGNode* n1;
  SGNode* n2;

  int cur = -1;
  int nex;

  for (int i = 0; i < nsize; i++){
    if (i == 0) {
      nodeID_++;  //new node
      n1   = new SGNode(nodeID_, src);
    }else 
      n1   = n2; //current
    
    cur = links[i];
    
    if (i == (nsize-1)){
      nodeID_++;
      n2  = new SGNode(nodeID_, dst);
    }else{
      nex = links[i+1];
      nodeID_++;  //new node
      n2  = new SGNode(nodeID_, -1);
    }

    tmplinks[cur] = new SGLink(cur, n1, n2);
    if (n1) n1->add_link(tmplinks[cur]);
    if (n2) n2->add_link(tmplinks[cur]);
  }

  for (int i = 0; i < nsize; i++){
    //tmplinks[links[i]]->print();
    merge_link(tmplinks[links[i]]);
  }
  add_tmplink_to_links(tmplinks);
}

void SGTopology::merge_link(SGLink* link){
  map<int, SGLink*>::iterator iter;
  iter = links_.find(link->id());
  if (iter == links_.end()){
    //if the link has not been added to the topology.links_, then
    //printf("link %d doesn't exist in alllinks. No action.\n", link->id());
    return;
  }else{
    int ncommon = 0;
    int comleft = -1;  //if tmplink.left == @alllinks.link.left, 0, otherwise, 1.
    int comright = -1; //if tmplink.right == @alllinks.link.left, 0, otherwise, 1.
    //since link is from tmplinks, each link has at most two neibours.

    if (link->left_link()){
      //printf("common neibor: left_first: %d\n", link->left_link()->id());
      if (links_[link->id()]->is_inleft(link->left_link())){
	comleft = 0;
	ncommon += 1;
      }
      if (links_[link->id()]->is_inright(link->left_link())){
	comleft = 1;
	ncommon += 1;
      }
    }

    if (link->right_link()){
      //printf("common neibor: right_first: %d\n", link->right_link()->id());
      if (links_[link->id()]->is_inleft(link->right_link())){
	comright = 0;
	ncommon += 1;
      }
      if (links_[link->id()]->is_inright(link->right_link())){
	comright = 1;
	ncommon += 1;
      }
    }

    //printf("common: %d, comright: %d, comleft: %d\n",ncommon, comright, comleft);

    if (ncommon == 0){
      //merge link.n1 with @alllink.n1, link.n2 with @alllink.n2
      if (link->left_link())
	links_[link->id()]->add_left_link(link->left_link(), RIGHTSIDE);

      if (link->right_link())
	links_[link->id()]->add_right_link(link->right_link(), LEFTSIDE);
      
    }else if (ncommon == 1){
      printf("ncommon %d\n", links_[link->id()]->id());
      //left --> right
      if ((comleft == -1) && (comright == 0)){
	if (link->left_link())
	  links_[link->id()]->add_right_link(link->left_link(), RIGHTSIDE);
      }else if ((comleft == -1) && (comright == 1)){
	//left --> left
	if (link->left_link())
	  links_[link->id()]->add_left_link(link->left_link(), RIGHTSIDE);
      }else if ((comright == -1) && (comleft == 0)){
	//right --> right
	if (link->right_link())
	  links_[link->id()]->add_right_link(link->right_link(), LEFTSIDE);
      }else if ((comright == -1) && (comleft == 1)){
	//right --> left
	if (link->right_link())
	  links_[link->id()]->add_left_link(link->right_link(), LEFTSIDE);
      }else{
	fprintf(stderr, "should not come here\n");
      }

    }else if (ncommon == 2){
      //no change

    }else{
      fprintf(stderr, "error, common links are more than 2. %d\n", ncommon);
    }
  }
}

void SGTopology::add_tmplink_to_links(map<int, SGLink*> tmplink){
  map<int, SGLink*>::iterator iter;

  for (iter = tmplink.begin(); iter != tmplink.end(); iter++){
    map<int, SGLink*>::iterator alliter = links_.find(iter->first);
    if (alliter == links_.end()){
      // cout << "tmplink " << iter->first << " not found in links_, adding."
      // << endl;
      links_[iter->first] = iter->second;
    }
    //    cout << "tmplink: " << iter->second->id() << endl;
  }
							       
}

void SGTopology::print_topology(){
  map<int, SGLink*>::iterator iter;
  for (iter = links_.begin(); iter != links_.end(); iter++){
    iter->second->print();
  }
}

void SGTopology::create_gtnets_topology(){
  map<int, SGLink*>::iterator it;
  for (it = links_.begin(); it != links_.end(); it++){
    
  }
}

map<int, SGLink*>& SGTopology::get_links(){
  return links_;
}

