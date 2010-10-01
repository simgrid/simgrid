--Master Function
function Master(...) 

simgrid.info("Hello from lua, I'm the master")
for i,v in ipairs(arg) do
    simgrid.info("Got "..v)
end

nb_task = arg[1];
comp_size = arg[2];
comm_size = arg[3];
slave_count = arg[4];

if (#arg ~= 4) then
    error("Argc should be 4");
end
simgrid.info("Argc="..(#arg).." (should be 4)")

-- Dispatch the tasks

for i=1,nb_task do
  tk = simgrid.Task.new("Task "..i,comp_size,comm_size);
  alias = "slave "..(i%slave_count);
  simgrid.info("Master sending  '" .. simgrid.Task.name(tk) .."' To '" .. alias .."'");
  simgrid.Task.send(tk,alias); -- C user data set to NULL
  simgrid.info("Master done sending '".. simgrid.Task.name(tk) .."' To '" .. alias .."'");
end

-- Sending Finalize Message To Others

simgrid.info("Master: All tasks have been dispatched. Let's tell everybody the computation is over.");
for i=0,slave_count-1 do
  alias = "slave "..i;
  simgrid.info("Master: sending finalize to "..alias);
  finalize = simgrid.Task.new("finalize",comp_size,comm_size);
  simgrid.Task.send(finalize,alias)
end
  simgrid.info("Master: Everything's done.");
end

--end_of_master

-- Slave Function ---------------------------------------------------------
function Slave(...)

local my_mailbox="slave "..arg[1]
simgrid.info("Hello from lua, I'm a poor slave with mbox: "..my_mailbox)

while true do

  local tk = simgrid.Task.recv(my_mailbox);
  if (simgrid.Task.name(tk) == "finalize") then
    simgrid.info("Slave '" ..my_mailbox.."' got finalize msg");
    break
  end
  --local tk_name = simgrid.Task.name(tk) 
  simgrid.info("Slave '" ..my_mailbox.."' processing "..simgrid.Task.name(tk))
  simgrid.Task.execute(tk)
  simgrid.info("Slave '" ..my_mailbox.."': task "..simgrid.Task.name(tk) .. " done")
end -- while

simgrid.info("Slave '" ..my_mailbox.."': I'm Done . See You !!");

end 
-- end_of_slave

-- Simulation Core ----------------------------------------------------------
--]]

require "simgrid"
 
  --create new routing model
  --simgrid.AS.new(AS_id,AS_mode)
  simgrid.AS.new{id="AS0",mode="Full"}; 
  --simgrid.Host.new(host_id,power)
  simgrid.Host.new{id="Tremblay",power=98095000};
  simgrid.Host.new{id="Jupiter",power=76296000};
  simgrid.Host.new{id="Fafard",power=76296000};
  simgrid.Host.new{id="Ginette",power=48492000};
  simgrid.Host.new{id="Bourassa",power=48492000};

    -- create Links
  for i=0,11 do
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
   simgrid.info("start registering platform");
   simgrid.msg_register_platform();
   simgrid.info("platform registered");
  
  --Set Application
   simgrid.Host.setFunction("Tremblay","Master",{"20","550000000","1000000","4"});
   simgrid.Host.setFunction("Bourassa","Slave",{"0"});
   simgrid.Host.setFunction("Jupiter","Slave",{"1"});
   simgrid.Host.setFunction("Fafard","Slave",{"2"});
   simgrid.Host.setFunction("Ginette","Slave",{"3"});
   
  --Save Application 
   simgrid.msg_register_application();

  --Run The Application
   simgrid.run()
   simgrid.info("Simulation's over.See you.")
   simgrid.clean()

