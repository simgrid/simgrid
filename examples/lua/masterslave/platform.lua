 
  --create new routing model
  --simgrid.AS.new(AS_id,AS_mode)
  simgrid.AS.new{id="AS0",mode="Full"}; 
  --simgrid.host.new(host_id,power)
  simgrid.AS.addHost{AS="AS0",id="Tremblay",power=98095000};
  simgrid.AS.addHost{AS="AS0",id="Jupiter",power=76296000};
  simgrid.AS.addHost{AS="AS0",id="Fafard",power=76296000};
  simgrid.AS.addHost{AS="AS0",id="Ginette",power=48492000};
  simgrid.AS.addHost{AS="AS0",id="Bourassa",power=48492000};

    -- create Links
  for i=10,0,-1 do
    simgrid.AS.addLink{AS="AS0",id=i,bandwidth=252750+ i*768,latency=0.000270544+i*0.087};  
  end
  -- simgrid.route.new(src_id,des_id,links_nb,links_list)
   simgrid.AS.addRoute{AS="AS0",src="Tremblay",dest="Jupiter",links="1"};
   simgrid.AS.addRoute{AS="AS0",src="Tremblay",dest="Fafard",links="0,1,2,3,4,8"};
   simgrid.AS.addRoute{AS="AS0",src="Tremblay",dest="Ginette",links="3,4,5"};
   simgrid.AS.addRoute{AS="AS0",src="Tremblay",dest="Bourassa",links="0,1,3,2,4,6,7"};

   simgrid.AS.addRoute{AS="AS0",src="Jupiter",dest="Tremblay",links="1"};
   simgrid.AS.addRoute{AS="AS0",src="Jupiter",dest="Fafard",links="0,1,2,3,4,8,9"};
   simgrid.AS.addRoute{AS="AS0",src="Jupiter",dest="Ginette",links="3,4,5,9"};
   simgrid.AS.addRoute{AS="AS0",src="Jupiter",dest="Bourassa",links="0,1,2,3,4,6,7,9"};
 
   simgrid.AS.addRoute{AS="AS0",src="Fafard",dest="Tremblay",links="0,1,2,3,4,8"};
   simgrid.AS.addRoute{AS="AS0",src="Fafard",dest="Jupiter",links="0,1,2,3,4,8,9"};
   simgrid.AS.addRoute{AS="AS0",src="Fafard",dest="Ginette",links="0,1,2,5,8"};
   simgrid.AS.addRoute{AS="AS0",src="Fafard",dest="Bourassa",links="6,7,8"};
  
   simgrid.AS.addRoute{AS="AS0",src="Ginette",dest="Tremblay",links="3,4,5"};
   simgrid.AS.addRoute{AS="AS0",src="Ginette",dest="Jupiter",links="3,4,5,9"};
   simgrid.AS.addRoute{AS="AS0",src="Ginette",dest="Fafard",links="0,1,2,5,8"};
   simgrid.AS.addRoute{AS="AS0",src="Ginette",dest="Bourassa",links="0,1,2,5,6,7"};

   simgrid.AS.addRoute{AS="AS0",src="Bourassa",dest="Tremblay",links="0,1,3,2,4,6,7"};
   simgrid.AS.addRoute{AS="AS0",src="Bourassa",dest="Jupiter",links="0,1,2,3,4,6,7,9"};
   simgrid.AS.addRoute{AS="AS0",src="Bourassa",dest="Fafard",links="6,7,8"};
   simgrid.AS.addRoute{AS="AS0",src="Bourassa",dest="Ginette",links="0,1,2,5,6,7"};
  
   --Save Platform
   --simgrid.info("start registering platform");
   simgrid.msg_register_platform();
   --simgrid.info("platform registered");
