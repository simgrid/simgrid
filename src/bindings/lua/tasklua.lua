function getn(t)
  if type(t.n) == "number" then return t.n end
  local max=0
  for i,_ in to do
    if type(i) == "number" and i>max then max=i end
  end
  t.n = max
  return max
end

--Master Function
function master(...) 

print("Hello from lua, I'm the master")
for i,v in ipairs(arg) do
    print("Got "..v)
end
t_tasks={}   --tasks table
t_slaves={}  --slaves table

nb_task = arg[1];
comp_size = arg[2];
comm_size = arg[3];

-- Let's Create the tasks to dispatch
for i=1,nb_task do
  t_tasks[i] = Task.new("Task "..i,comp_size,comm_size); --data set to NULL
end


-- Process Organisation

argc=getn(arg)
print("Argc="..argc.." (should be 8)")

slave_count = getn(arg) -3;

for i=4,argc do
slv_name = arg[i];
t_slaves[i - 3] = Host.new(slv_name);
-- do not do directly that : t_slaves[i - 4] = Host.new(argv[i]); 
end




-- Print List Of Tasks / Slaves
--[[
for  i=1,nb_task do
todo = Task.name(t_tasks[i]);
print(i % slave_count+1);
slv = Host.name(t_slaves[i % slave_count+1]);
print ( "Sending : " .. todo .. " To : " .. slv);
end
]]--



for i=1,nb_task do
tk_name = Task.name(t_tasks[i]);
ht_name = Host.name(t_slaves[i]);
print("Sending  " .. tk_name .." To " .. ht_name);
Task.send(t_task[i],"slave "..(i%slave_count));
print("Sent");
end

end

--Slave Function ---------------------------------------------------------
function slave(...)

my_mailbox="slave "..arg[1]
print("Hello from lua, I'm a poor slave with mbox: "..my_mailbox)

while true do
  tk = Task.recv(my_mailbox);
  print("Got something");
  --testing res !!!!
  tk_name = Task.name(tk)

  if (tk_name == "finalize") then
    Task.destroy(tk);
   end

  print("Processing "..Task.name(tk))
  Task.execute(tk);

  print(Task.name(tk) .. "Done")
  Task.destroy(tk);
end -- while

print("I'm Done . See You !!");

end -- function ----------------------------------------------------------
--]]
