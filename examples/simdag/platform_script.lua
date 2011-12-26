require "simgrid"

  simgrid.AS.new{id="AS0",mode="Full"};

  simgrid.host.new{id="Tremblay",power=98095000};
  simgrid.host.new{id="Jupiter",power=76296000};
  simgrid.host.new{id="Fafard",power=76296000};
  simgrid.host.new{id="Ginette",power=48492000};
  simgrid.host.new{id="Bourassa",power=48492000};

    -- create Links
  for i=0,11 do
    simgrid.link.new{id=i,bandwidth=252750+ i*768,latency=0.000270544+i*0.087};   
  end
  -- simgrid.route.new(src_id,des_id,links_nb,links_list)
   simgrid.route.new("Tremblay","Jupiter",{"1"});
   simgrid.route.new("Tremblay","Fafard",{"0","1","2","3","4","8"});
   simgrid.route.new("Tremblay","Ginette",{"3","4","5"});
   simgrid.route.new("Tremblay","Bourassa",{"0","1","3","2","4","6","7"});

   simgrid.route.new("Jupiter","Tremblay",{"1"});
   simgrid.route.new("Jupiter","Fafard",{"0","1","2","3","4","8","9"});
   simgrid.route.new("Jupiter","Ginette",{"3","4","5","9"});
   simgrid.route.new("Jupiter","Bourassa",{"0","1","2","3","4","6","7","9"});
 
   simgrid.route.new("Fafard","Tremblay",{"0","1","2","3","4","8"});
   simgrid.route.new("Fafard","Jupiter",{"0","1","2","3","4","8","9"});
   simgrid.route.new("Fafard","Ginette",{"0","1","2","5","8"});
   simgrid.route.new("Fafard","Bourassa",{"6","7","8"});
  
   simgrid.route.new("Ginette","Tremblay",{"3","4","5"});
   simgrid.route.new("Ginette","Jupiter",{"3","4","5","9"});
   simgrid.route.new("Ginette","Fafard",{"0","1","2","5","8"});
   simgrid.route.new("Ginette","Bourassa",{"0","1","2","5","6","7"});

   simgrid.route.new("Bourassa","Tremblay",{"0","1","3","2","4","6","7"});
   simgrid.route.new("Bourassa","Jupiter",{"0","1","2","3","4","6","7","9"});
   simgrid.route.new("Bourassa","Fafard",{"6","7","8"});
   simgrid.route.new("Bourassa","Ginette",{"0","1","2","5","6","7"});
  
   --Save Platform
   simgrid.sd_register_platform();


