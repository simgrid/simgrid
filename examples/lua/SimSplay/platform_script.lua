require "simgrid"

  simgrid.AS.new{id="AS0",mode="Full"}; 

  simgrid.AS.addHost{AS="AS0",id="Tremblay",power=98095000};
  simgrid.AS.addHost{AS="AS0",id="Jupiter",power=76296000};
  simgrid.AS.addHost{AS="AS0",id="Fafard",power=76296000};

  simgrid.host.setProperty{host="Tremblay",prop_id="ip",prop_value="199.23.98.3"};
  simgrid.host.setProperty{host="Tremblay",prop_id="port",prop_value="65"};
  simgrid.host.setProperty{host="Jupiter",prop_id="ip",prop_value="199.23.98.4"};
  simgrid.host.setProperty{host="Jupiter",prop_id="port",prop_value="83"};
  simgrid.host.setProperty{host="Fafard",prop_id="ip",prop_value="199.23.98.5"};
  simgrid.host.setProperty{host="Fafard",prop_id="port",prop_value="76"};
    -- create Links
  for i=10,0,-1 do
    simgrid.AS.addLink{AS="AS0",id=i,bandwidth=252750+ i*768,latency=0.000270544+i*0.087};   
  end
  -- simgrid.route.new(src_id,des_id,links_nb,links_list)
   simgrid.AS.addRoute("AS0","Tremblay","Jupiter",{"1"});
   simgrid.AS.addRoute("AS0","Tremblay","Fafard",{"0","1","2","3","4","8"});

   simgrid.AS.addRoute("AS0","Jupiter","Tremblay",{"1"});
   simgrid.AS.addRoute("AS0","Jupiter","Fafard",{"0","1","2","3","4","8","9"});
 
   simgrid.AS.addRoute("AS0","Fafard","Tremblay",{"0","1","2","3","4","8"});
   simgrid.AS.addRoute("AS0","Fafard","Jupiter",{"0","1","2","3","4","8","9"});
  
  
   --Save Platform
   simgrid.msg_register_platform();

  --Set Application
   simgrid.host.set_function{host="Tremblay",fct="SPLAYschool",args=""};
   simgrid.host.set_function{host="Fafard",fct="SPLAYschool",args=""};
   simgrid.host.set_function{host="Jupiter",fct="SPLAYschool",args=""};
   
  --Save Application 
   simgrid.msg_register_application(); 


