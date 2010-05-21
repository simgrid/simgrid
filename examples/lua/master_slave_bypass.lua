--Master Function
function Master(...) 

simgrid.info("Hello from lua, I'm the master")
for i,v in ipairs(arg) do
    simgrid.info("Got "..v)
end

nb_task = arg[1];
comp_size = arg[2];
comm_size = arg[3];
slave_count = arg[4]


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

end -- function ----------------------------------------------------------
--]]

require "simgrid"
  
  --first we save the host number ( since we'll first save them into a C table )
  simgrid.Host.setNumber(5);
  --simgrid.Host.new(host_id,power)
  simgrid.Host.new("Tremblay",98095000);
  simgrid.Host.new("Jupiter",76296000);
  simgrid.Host.new("Fafard",76296000);
  simgrid.Host.new("Ginette",48492000);
  simgrid.Host.new("Bourassa",48492000);

    -- set Number of links
  simgrid.Link.setNumber(12);
    -- create Links
  for i=0,11 do
    simgrid.Link.new(i,252750+ i*768,0.000270544+i*0.087);    -- let's create link !! with crazy values ;)
  end

  -- set number of route ( 5 hosts >> 5*5 = 25 )
  simgrid.Route.setNumber(20);

  -- simgrid.Route.new(src_id,des_id,links_nb,links_list)
  simgrid.Route.new("Tremblay","Jupiter",1,{"1"});
  simgrid.Route.new("Tremblay","Fafard",6,{"0","1","2","3","4","8"});
  simgrid.Route.new("Tremblay","Ginette",3,{"3","4","5"});
  simgrid.Route.new("Tremblay","Bourassa",7,{"0","1","3","2","4","6","7"});

   simgrid.Route.new("Jupiter","Tremblay",1,{"1"});
   simgrid.Route.new("Jupiter","Fafard",7,{"0","1","2","3","4","8","9"});
   simgrid.Route.new("Jupiter","Ginette",4,{"3","4","5","9"});
   simgrid.Route.new("Jupiter","Bourassa",8,{"0","1","2","3","4","6","7","9"});
 
   simgrid.Route.new("Fafard","Tremblay",6,{"0","1","2","3","4","8"});
   simgrid.Route.new("Fafard","Jupiter",7,{"0","1","2","3","4","8","9"});
   simgrid.Route.new("Fafard","Ginette",5,{"0","1","2","5","8"});
   simgrid.Route.new("Fafard","Bourassa",3,{"6","7","8"});
  
   simgrid.Route.new("Ginette","Tremblay",3,{"3","4","5"});
   simgrid.Route.new("Ginette","Jupiter",4,{"3","4","5","9"});
   simgrid.Route.new("Ginette","Fafard",5,{"0","1","2","5","8"});
   simgrid.Route.new("Ginette","Bourassa",6,{"0","1","2","5","6","7"});

   simgrid.Route.new("Bourassa","Tremblay",7,{"0","1","3","2","4","6","7"});
   simgrid.Route.new("Bourassa","Jupiter",8,{"0","1","2","3","4","6","7","9"});
   simgrid.Route.new("Bourassa","Fafard",3,{"6","7","8"});
   simgrid.Route.new("Bourassa","Ginette",6,{"0","1","2","5","6","7"});
  
   --Save Platform
   simgrid.register_platform();
  
  --Set Application

  --simgrid.Host.setFunction(host_id,function_name,args_nb,args_list)
   simgrid.Host.setFunction("Tremblay","Master",4,{"20","550000000","1000000","4"});
   simgrid.Host.setFunction("Bourassa","Slave",1,{"0"});
   simgrid.Host.setFunction("Jupiter","Slave",1,{"1"});
   simgrid.Host.setFunction("Fafard","Slave",1,{"2"});
   simgrid.Host.setFunction("Ginette","Slave",1,{"3"});
   
  --Save Application 
   simgrid.register_application();

-- Run The Application

   simgrid.run()
   simgrid.info("Simulation's over.See you.")
   simgrid.clean()