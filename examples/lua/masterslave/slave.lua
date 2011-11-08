function Slave(...)

  local my_mailbox = "slave " .. arg[1]
  simgrid.info("Hello from lua, I'm a poor slave with mbox: " .. my_mailbox)

  while true do

    local tk = simgrid.Task.recv(my_mailbox)
    if (simgrid.Task.name(tk) == "finalize") then
      simgrid.info("Slave '" .. my_mailbox .. "' got finalize msg")
      break
    end
    local task_name = simgrid.Task.name(tk) 
    simgrid.info("Slave '" .. my_mailbox.. "' processing " .. task_name)
    simgrid.Task.execute(tk)
    simgrid.info("Slave '" .. my_mailbox .. "': task " .. task_name .. " done")
  end

  simgrid.info("Slave '" .. my_mailbox .. "': I'm Done . See You !!")
end -- Slave

