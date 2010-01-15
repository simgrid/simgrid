
--Master Function
function master(...) 

print("Hello from lua, I'm the master")
for i,v in ipairs(arg) do
    print("Got "..v)
end

nb_task = arg[1];
comp_size = arg[2];
comm_size = arg[3];
slave_count = arg[4]


argc=#arg
print("Argc="..argc.." (should be 4)")

-- Dispatch the tasks

for i=1,nb_task do
  tk = Msg.Task.new("Task "..i,comp_size,comm_size);
  alias = "slave "..(i%slave_count);
  print("Master sending  '" .. Msg.Task.name(tk) .."' To '" .. alias .."'");
  Msg.Task.send(tk,alias); -- C user data set to NULL
  print("Master done sending '".. Msg.Task.name(tk) .."' To '" .. alias .."'");
end

--[[
Sending Finalize Message To Others
--]]

print("Master: All tasks have been dispatched. Let's tell everybody the computation is over.");
for i=0,slave_count-1 do
  alias = "slave "..i;
  print("Master: sending finalize to "..alias);
  Msg.Task.send(Msg.Task.new("finalize",0,0),alias);
end
print("Master: Everything's done.");


end

--Slave Function ---------------------------------------------------------
function slave(...)

my_mailbox="slave "..arg[1]
print("Hello from lua, I'm a poor slave with mbox: "..my_mailbox)


while true do
--  tk = Msg.Task.new("",0,0); --??
--  Msg.Task.recv2(tk,my_mailbox);
  tk = Msg.Task.recv(my_mailbox);
  
  tk_name = Msg.Task.name(tk)

  if (tk_name == "finalize") then
    print("Slave '" ..my_mailbox.."' got finalize msg");
    break
  end

  print("Slave '" ..my_mailbox.."' processing "..Msg.Task.name(tk))
  Msg.Task.execute(tk);

  print("Slave '" ..my_mailbox.."': task "..Msg.Task.name(tk) .. " done")
end -- while

print("Slave '" ..my_mailbox.."': I'm Done . See You !!");

end -- function ----------------------------------------------------------
--]]
