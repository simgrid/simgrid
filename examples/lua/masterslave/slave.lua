function Slave(...)

  local my_mailbox = "slave " .. arg[1]
  simgrid.info("Hello from lua, I'm a poor slave with mbox: " .. my_mailbox)

  while true do

    local tk = simgrid.task.recv(my_mailbox)
    if (simgrid.task.name(tk) == "finalize") then
      simgrid.info("Slave '" .. my_mailbox .. "' got finalize msg")
      break
    end
    local task_name = simgrid.task.name(tk) 
    simgrid.info("Slave '" .. my_mailbox.. "' processing " .. task_name)
    simgrid.task.execute(tk)
    simgrid.info("Slave '" .. my_mailbox .. "': task " .. task_name .. " done")
  end

  simgrid.info("Slave '" .. my_mailbox .. "': I'm Done . See You !!")
end -- Slave

