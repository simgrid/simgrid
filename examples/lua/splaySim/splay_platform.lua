require "simgrid"

  simgrid.AS.new{id="AS0",mode="Full"}; 

  -- create 5 Splayd (Hosts)
  for i=0,5,1 do
	simgrid.AS.addHost{AS="AS0",id="Splayd_"..i,power= 7000000+i*1000000}
  end

    -- create Links
  for i=10,0,-1 do
    simgrid.AS.addLink{AS="AS0",id=i,bandwidth=252750+ i*768,latency=0.000270544+i*0.087};   
  end
  -- simgrid.route.new(src_id,des_id,links_nb,links_list)
   simgrid.AS.addRoute("AS0","Splayd_1","Splayd_2",{"1"});
   simgrid.AS.addRoute("AS0","Splayd_1","Splayd_3",{"0","1","2","3","4","8"});
   simgrid.AS.addRoute("AS0","Splayd_1","Splayd_4",{"3","4","5"});
   simgrid.AS.addRoute("AS0","Splayd_1","Splayd_5",{"0","1","3","2","4","6","7"});

   simgrid.AS.addRoute("AS0","Splayd_2","Splayd_1",{"1"});
   simgrid.AS.addRoute("AS0","Splayd_2","Splayd_3",{"0","1","2","3","4","8","9"});
   simgrid.AS.addRoute("AS0","Splayd_2","Splayd_4",{"3","4","5","9"});
   simgrid.AS.addRoute("AS0","Splayd_2","Splayd_5",{"0","1","2","3","4","6","7","9"});
 
   simgrid.AS.addRoute("AS0","Splayd_3","Splayd_1",{"0","1","2","3","4","8"});
   simgrid.AS.addRoute("AS0","Splayd_3","Splayd_2",{"0","1","2","3","4","8","9"});
   simgrid.AS.addRoute("AS0","Splayd_3","Splayd_4",{"0","1","2","5","8"});
   simgrid.AS.addRoute("AS0","Splayd_3","Splayd_5",{"6","7","8"});
  
   simgrid.AS.addRoute("AS0","Splayd_4","Splayd_1",{"3","4","5"});
   simgrid.AS.addRoute("AS0","Splayd_4","Splayd_2",{"3","4","5","9"});
   simgrid.AS.addRoute("AS0","Splayd_4","Splayd_3",{"0","1","2","5","8"});
   simgrid.AS.addRoute("AS0","Splayd_4","Splayd_5",{"0","1","2","5","6","7"});

   simgrid.AS.addRoute("AS0","Splayd_5","Splayd_1",{"0","1","3","2","4","6","7"});
   simgrid.AS.addRoute("AS0","Splayd_5","Splayd_2",{"0","1","2","3","4","6","7","9"});
   simgrid.AS.addRoute("AS0","Splayd_5","Splayd_3",{"6","7","8"});
   simgrid.AS.addRoute("AS0","Splayd_5","Splayd_4",{"0","1","2","5","6","7"});
  
   --Save Platform
   simgrid.msg_register_platform();



