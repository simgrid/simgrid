-- See routing_cluster.xml for description.
--

require "simgrid"
simgrid.engine.open()
simgrid.engine.AS_open{id="AS0",mode="Full"}
  simgrid.engine.AS_open{id="my_cluster1",mode="Cluster"}
    simgrid.engine.router_new{id="router1"}

    simgrid.engine.host_new{id="host1",speed="1Gf"};
    simgrid.engine.link_new{id="l1_UP",bandwidth="125MBps",lat="100us"};
    simgrid.engine.link_new{id="l1_DOWN",bandwidth="125MBps",lat="100us"};
    simgrid.engine.host_link_new{id="host1",up="l1_UP",down="l1_DOWN"};

    simgrid.engine.host_new{id="host2",speed="1Gf"};
    simgrid.engine.link_new{id="l2",bandwidth="125MBps",lat="100us",sharing_policy="SPLITDUPLEX"};
    simgrid.engine.host_link_new{id="host2",up="l2_UP",down="l2_DOWN"};

    simgrid.engine.host_new{id="host3",speed="1Gf"};
    simgrid.engine.link_new{id="l3",bandwidth="125MBps",lat="100us"};
    simgrid.engine.host_link_new{id="host3",up="l3",down="l3"};

    simgrid.engine.backbone_new{id="backbone1",bandwidth="2.25GBps",lat="500us"};

  simgrid.engine.AS_seal()
  simgrid.engine.AS_open{id="my_cluster2",mode="Cluster"}
    simgrid.engine.router_new{id="router2"}

    simgrid.engine.host_new{id="host4",speed="1Gf"};
    simgrid.engine.link_new{id="l4_UP",bandwidth="125MBps",lat="100us"};
    simgrid.engine.link_new{id="l4_DOWN",bandwidth="125MBps",lat="100us"};
    simgrid.engine.host_link_new{id="host4",up="l4_UP",down="l4_DOWN"};

    simgrid.engine.host_new{id="host5",speed="1Gf"};
    simgrid.engine.link_new{id="l5",bandwidth="125MBps",lat="100us",sharing_policy="SPLITDUPLEX"};
    simgrid.engine.host_link_new{id="host5",up="l5_UP",down="l5_DOWN"};

    simgrid.engine.host_new{id="host6",speed="1Gf"};
    simgrid.engine.link_new{id="l6",bandwidth="125MBps",lat="100us"};
    simgrid.engine.host_link_new{id="host6",up="l6",down="l6"};

    simgrid.engine.backbone_new{id="backbone2",bandwidth="2.25GBps",lat="500us"}

  simgrid.engine.AS_seal()
  simgrid.engine.link_new{id="link1-2",bandwidth="2.25GBps",lat="500us"};

  simgrid.engine.ASroute_new{src="my_cluster1", dst="my_cluster2",
                            gw_src="router1", gw_dst="router2", links="link1-2"}

simgrid.engine.AS_seal()
simgrid.engine.close()
