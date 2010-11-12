require "simgrid"

  simgrid.AS.new{id="AS0",mode="Full"};

  simgrid.Host.new{id="Tremblay",power=98095000};
  simgrid.Host.new{id="Jupiter",power=76296000};
  simgrid.Host.new{id="Fafard",power=76296000};
  simgrid.Host.new{id="Ginette",power=48492000};
  simgrid.Host.new{id="Bourassa",power=48492000};

    -- create Links
  for i=10,0,-1 do
    simgrid.Link.new{id=i,bandwidth=252750+ i*768,latency=0.000270544+i*0.087};   
  end
  -- simgrid.Route.new(src_id,des_id,links_nb,links_list)
   simgrid.Route.new("Tremblay","Jupiter",{"1"});
   simgrid.Route.new("Tremblay","Fafard",{"0","1","2","3","4","8"});
   simgrid.Route.new("Tremblay","Ginette",{"3","4","5"});
   simgrid.Route.new("Tremblay","Bourassa",{"0","1","3","2","4","6","7"});

   simgrid.Route.new("Jupiter","Tremblay",{"1"});
   simgrid.Route.new("Jupiter","Fafard",{"0","1","2","3","4","8","9"});
   simgrid.Route.new("Jupiter","Ginette",{"3","4","5","9"});
   simgrid.Route.new("Jupiter","Bourassa",{"0","1","2","3","4","6","7","9"});
 
   simgrid.Route.new("Fafard","Tremblay",{"0","1","2","3","4","8"});
   simgrid.Route.new("Fafard","Jupiter",{"0","1","2","3","4","8","9"});
   simgrid.Route.new("Fafard","Ginette",{"0","1","2","5","8"});
   simgrid.Route.new("Fafard","Bourassa",{"6","7","8"});
  
   simgrid.Route.new("Ginette","Tremblay",{"3","4","5"});
   simgrid.Route.new("Ginette","Jupiter",{"3","4","5","9"});
   simgrid.Route.new("Ginette","Fafard",{"0","1","2","5","8"});
   simgrid.Route.new("Ginette","Bourassa",{"0","1","2","5","6","7"});

   simgrid.Route.new("Bourassa","Tremblay",{"0","1","3","2","4","6","7"});
   simgrid.Route.new("Bourassa","Jupiter",{"0","1","2","3","4","6","7","9"});
   simgrid.Route.new("Bourassa","Fafard",{"6","7","8"});
   simgrid.Route.new("Bourassa","Ginette",{"0","1","2","5","6","7"});
  
   --Save Platform
   simgrid.gras_register_platform();

  --Set Application
   simgrid.Host.setFunction{host="Tremblay",fct="server",args="4000"};
   simgrid.Host.setFunction{host="Fafard",fct="client",args="Tremblay,4000"};

  --Save Application 
   simgrid.gras_register_application(); 


