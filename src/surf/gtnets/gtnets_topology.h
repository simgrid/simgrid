

#ifndef _GTNETS_TOPOLOGY_H
#define _GTNETS_TOPOLOGY_H

#include <map>
#include <vector>
#include <iostream>

#define LEFTSIDE 0
#define RIGHTSIDE 1

using namespace std;

class SGLink;

class SGNode{

 public:
  SGNode(int id, int hostid);
  ~SGNode();

  void add_link(SGLink*);

  //get other link than the link with the given id.
  //Note it's only for the case the node has two links.
  SGLink* other_link(int);

  bool has_link(SGLink*); //TODO can do const SGLink*?
  void print_links();
  void print_hosts();
  
  vector<SGLink*>& links();
  vector<int>& hosts();
  int id(){return ID_;};

 private:
  int ID_;
  bool ishost_;
  vector<int> hosts_; //simgrid hosts
  vector<SGLink*> links_;
};

class SGLink{

 public:
  SGLink(int id, SGNode* left, SGNode* right);
  ~SGLink();

  //for a temporary link set, that is, a link has at most two neibours.
  SGLink* left_link();
  SGLink* right_link();


  SGNode* left_node();
  SGNode* right_node();

  bool is_inleft(SGLink*);
  bool is_inright(SGLink*);

  void add_left_link(SGLink*, int side);
  void add_right_link(SGLink*, int side);

  int id(){return ID_;};

  void print();

 private:
  int ID_;
  SGNode* left_node_;
  SGNode* right_node_;

};


class SGTopology{
 public:
  SGTopology();
  ~SGTopology();

  void add_link(int src, int dst, int* links, int nsize);

  void create_tmplink(int src, int dst, int* links, int nsize);

  void merge_link(SGLink*);

  void add_tmplink_to_links(map<int, SGLink*> tmplink); //???

  void print_topology();
  
  void create_gtnets_topology();

  map<int, SGLink*>& get_links();

 private:
  int nodeID_;
  map<int, SGLink*> links_;
  map<int, SGNode*> nodes_;
};

#endif
