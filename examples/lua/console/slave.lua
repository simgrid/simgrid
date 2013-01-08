

-- Slave Function ---------------------------------------------------------
function Slave(...)

  if #arg ~= 1 then
    error("Wrong number of arguments (got " .. #arg .. ", expected 1: slave_id)")
  end

  local my_mailbox = "slave " .. arg[1]
  simgrid.info("Hello from lua, I'm a poor slave with mbox: " .. my_mailbox)

  while true do

    local task = simgrid.task.recv(my_mailbox);
    --print(task)
    local task_name = task:get_name()
    if (task:get_name() == "finalize") then
      simgrid.info("Slave '" .. my_mailbox .. "' got finalize msg");
      break
    end
    --local tk_name = simgrid.task.get_name(tk) 
    simgrid.info("Slave '" .. my_mailbox .. "' processing " .. task:get_name())
    simgrid.task.execute(task)
    simgrid.info("Slave '"  .. my_mailbox .. "': task " .. task:get_name() .. " done")
  end -- while

  simgrid.info("Slave '" .. my_mailbox .. "': I'm Done . See You !!");

end 
-- end_of_slave
