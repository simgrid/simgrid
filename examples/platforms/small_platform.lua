-- Copyright (c) 2011, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

  require("simgrid")
  simgrid.platf.open();
  simgrid.platf.AS_open{id="AS0",mode="Full"};

  simgrid.platf.host_new{AS="AS0",id="Tremblay",power=98095000};
  simgrid.platf.host_new{AS="AS0",id="Jupiter",power=76296000};
  simgrid.platf.host_new{AS="AS0",id="Fafard",power=76296000};
  simgrid.platf.host_new{AS="AS0",id="Ginette",power=48492000};
  simgrid.platf.host_new{AS="AS0",id="Bourassa",power=48492000};

  -- create Links
  simgrid.platf.link_new{AS="AS0",id=0,bandwidth=41279125,latency=0.000059904};
  simgrid.platf.link_new{AS="AS0",id=1,bandwidth=34285625,latency=0.000514433};
  simgrid.platf.link_new{AS="AS0",id=2,bandwidth=118682500,latency=0.000136931};
  simgrid.platf.link_new{AS="AS0",id=3,bandwidth=34285625,latency=0.000514433};
  simgrid.platf.link_new{AS="AS0",id=4,bandwidth=10099625,latency=0.00047978};
  simgrid.platf.link_new{AS="AS0",id=5,bandwidth=27946250,latency=0.000278066};
  simgrid.platf.link_new{AS="AS0",id=6,bandwidth=41279125,latency=0.000059904};
  simgrid.platf.link_new{AS="AS0",id=7,bandwidth=11618875,latency=0.00018998};
  simgrid.platf.link_new{AS="AS0",id=8,bandwidth=8158000,latency=0.000270544};
  simgrid.platf.link_new{AS="AS0",id=9,bandwidth=7209750,latency=0.001461517};
  simgrid.platf.link_new{AS="AS0",id="loopback",bandwidth=498000000,latency=0.000015,policy="FATPIPE"};

  for i=1,5,1 do
    simgrid.platf.route_new{AS="AS0",src=simgrid.host.name(simgrid.host.at(i)),dest=simgrid.host.name(simgrid.host.at(i)),links="loopback"};
  end

  simgrid.platf.route_new{AS="AS0",src="Tremblay",dest="Jupiter",links="9"};
  simgrid.platf.route_new{AS="AS0",src="Tremblay",dest="Fafard",links="4,3,2,0,1,8"};
  simgrid.platf.route_new{AS="AS0",src="Tremblay",dest="Ginette",links="4,3,5"};
  simgrid.platf.route_new{AS="AS0",src="Tremblay",dest="Bourassa",links="4,3,2,0,1,6,7"};

  --simgrid.platf.route_new{AS="AS0",src="Jupiter",dest="Tremblay",links="1"};
  simgrid.platf.route_new{AS="AS0",src="Jupiter",dest="Fafard",links="9,4,3,2,0,1,8"};
  simgrid.platf.route_new{AS="AS0",src="Jupiter",dest="Ginette",links="9,4,3,5"};
  simgrid.platf.route_new{AS="AS0",src="Jupiter",dest="Bourassa",links="9,4,3,2,0,1,6,7"};

  --simgrid.platf.route_new{AS="AS0",src="Fafard",dest="Tremblay",links="0,1,2,3,4,8"};
  --simgrid.platf.route_new{AS="AS0",src="Fafard",dest="Jupiter",links="0,1,2,3,4,8,9"};
  simgrid.platf.route_new{AS="AS0",src="Fafard",dest="Ginette",links="8,1,0,2,5"};
  simgrid.platf.route_new{AS="AS0",src="Fafard",dest="Bourassa",links="8,6,7"};

  --simgrid.platf.route_new{AS="AS0",src="Ginette",dest="Tremblay",links="3,4,5"};
  --simgrid.platf.route_new{AS="AS0",src="Ginette",dest="Jupiter",links="3,4,5,9"};
  --simgrid.platf.route_new{AS="AS0",src="Ginette",dest="Fafard",links="0,1,2,5,8"};
  simgrid.platf.route_new{AS="AS0",src="Ginette",dest="Bourassa",links="5,2,0,1,6,7"};

  --simgrid.platf.route_new{AS="AS0",src="Bourassa",dest="Tremblay",links="0,1,3,2,4,6,7"};
  --simgrid.platf.route_new{AS="AS0",src="Bourassa",dest="Jupiter",links="0,1,2,3,4,6,7,9"};
  --simgrid.platf.route_new{AS="AS0",src="Bourassa",dest="Fafard",links="6,7,8"};
  --simgrid.platf.route_new{AS="AS0",src="Bourassa",dest="Ginette",links="0,1,2,5,6,7"};

  simgrid.platf.AS_close();

  simgrid.platf.close();

  --Save Platform
  --simgrid.info("start registering platform");
  --simgrid.msg_register_platform();
  --simgrid.info("platform registered");
