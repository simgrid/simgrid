//Kayo Fujiwara 1/8/2007
#include "gtnets_simulator.h"
#include "gtnets_topology.h"
#include <map>

void static tcp_sent_callback(int id, double completion_time);

// Constructor.
// TODO: check the default values.
GTSim::GTSim(){
  int wsize = 20000;
  is_topology_ = 0;
  nflow_ = 0;
  sim_ = new Simulator();
  topo_ = new SGTopology();

  // Set default values.
  TCP::DefaultAdvWin(wsize);
  TCP::DefaultSegSize(1000);
  TCP::DefaultTxBuffer(128000);
  TCP::DefaultRxBuffer(128000);

}
GTSim::~GTSim(){

  map<int, Linkp2p*>::iterator it;
  for (it = gtnets_links_.begin(); it != gtnets_links_.end(); it++){
    delete it->second;
  }
  map<int, SGLink*>::iterator it2;
  for (it2 = tmp_links_.begin(); it2 != tmp_links_.end(); it2++){
    delete it2->second;
  }
  map<int, Node*>::iterator it3;
  for (it3 = gtnets_nodes_.begin(); it3 != gtnets_nodes_.end(); it3++){
    delete it3->second;
  }
  map<int, TCPServer*>::iterator it4;
  for (it4 = gtnets_servers_.begin(); it4 != gtnets_servers_.end(); it4++){
    delete it4->second;
  }
  map<int, TCPSend*>::iterator it5;
  for (it5 = gtnets_clients_.begin(); it5 != gtnets_clients_.end(); it5++){
    delete it5->second;
  }
  //TODO delete other objects!
}


int GTSim::add_link(int id, double bandwidth, double latency){
  if (gtnets_links_.find(id) != gtnets_links_.end()){
    fprintf(stderr, "can't add link %d. already exists.\n", id);
    return -1;
  }
  gtnets_links_[id] = new Linkp2p(bandwidth, latency);
  return 0;
}

int GTSim::add_route(int src, int dst, int* links, int nlink){
  topo_->add_link(src, dst, links, nlink);
  //  topo_->create_tmplink(src, dst, links, nlink);
  return 0;
}

void GTSim::create_gtnets_topology(){
  //TODO: add manual routing
  //TODO: is this addressing fine?
  static unsigned int address = IPAddr("192.168.0.1");
  tmp_links_ = topo_->get_links();
  map<int, SGLink*>::iterator it;

  //By now, all links has two nodes.
  //Connect left and right with the link.
  for (it = tmp_links_.begin(); it != tmp_links_.end(); it++){
    it->second->print();
    //both nodes must be not null. TODO check it!
    int linkid = it->second->id();
    int leftid = it->second->left_node()->id();
    int rightid = it->second->right_node()->id();

    cout << "linkid: " << linkid << endl;
    cout << "leftid: " << leftid << endl;
    cout << "rightid: " << rightid << endl;


    map<int, Node*>::iterator nodeit = gtnets_nodes_.find(leftid);
    // if the nodes don't exist, add one.
    if (nodeit == gtnets_nodes_.end()){
      gtnets_nodes_[leftid] = new Node();
      gtnets_nodes_[leftid]->SetIPAddr(address++);
      //add host-node relationships.
      vector<int> tmphosts = it->second->left_node()->hosts();

      for (int i = 0; i < tmphosts.size(); i++){
	gtnets_hosts_[tmphosts[i]] = leftid;
	cout << "host: " << tmphosts[i] << " node: " << leftid << endl;
      }

    }
    nodeit = gtnets_nodes_.find(rightid);
    if (nodeit == gtnets_nodes_.end()){//new entry
      gtnets_nodes_[rightid] = new Node();
      gtnets_nodes_[rightid]->SetIPAddr(address++);

      //add host-node relationships.
      vector<int> tmphosts = it->second->right_node()->hosts();

      for (int i = 0; i < tmphosts.size(); i++){
	gtnets_hosts_[tmphosts[i]] = rightid;
	cout << "host: " << tmphosts[i] << " node: " << rightid << endl;
      }
    }

    gtnets_nodes_[leftid]->
      AddDuplexLink(gtnets_nodes_[rightid], *gtnets_links_[linkid]);

  }
}

int GTSim::create_flow(int src, int dst, long datasize, void* metadata){
  if (is_topology_ == 0){
    create_gtnets_topology();
    is_topology_ = 1;
  }
  //TODO: what if more than two flows?
  //TODO: check if src and dst exists.
  //TODO: should use "flowID" to name servers and clients.
  gtnets_servers_[nflow_] = (TCPServer*)gtnets_nodes_[gtnets_hosts_[dst]]->
    AddApplication(TCPServer(TCPReno()));
  gtnets_servers_[nflow_]->BindAndListen(80);

  gtnets_clients_[nflow_] = (TCPSend*)gtnets_nodes_[gtnets_hosts_[src]]->
    AddApplication(TCPSend(nflow_, gtnets_nodes_[gtnets_hosts_[dst]]->GetIPAddr(), 
			   80, Constant(datasize), TCPReno()));
  gtnets_clients_[nflow_]->SetSendCallBack(tcp_sent_callback);
  gtnets_clients_[nflow_]->Start(0);
  nflow_++;

  return 0;
}

Time_t GTSim::get_time_to_next_flow_completion(){
  int status;
  Time_t t1;
  int pfds[2];
  
  pipe(pfds);
  
  t1 = 0;
  if (fork() != 0){
    read(pfds[0], &t1, sizeof(Time_t));
    waitpid(-1, &status, 0);      
  }else{
    Time_t t;
    t = sim_->RunUntilNextCompletion();
    write(pfds[1], (const void*)&t, sizeof(Time_t));
    exit(0);
  }
  return t1;
}

int GTSim::run_until_next_flow_completion(void ***metadata, int *number_of_flows){
  Time_t t1 = sim_->RunUntilNextCompletion();
  sim_->Run(t1);
  //TODO set metadata and number of flows.
  return 0;
}

int GTSim::run(double delta){
  sim_->Run(delta);
  return 0;
}

// Clean up.
int GTSim::finalize(){
  //TODO
  is_topology_ = 0;
  delete sim_;
  delete topo_;
  return 0;
}

void static tcp_sent_callback(int id, double completion_time){
  // Schedule the flow complete event.
  SimulatorEvent* e =
    new SimulatorEvent(SimulatorEvent::FLOW_COMPLETE);
  Simulator::instance->Schedule(e, 0, Simulator::instance);

  //TODO: set metadata
  printf("In tcp_sent_callback: flow id: %d, time: %f\n", id, completion_time);
}

