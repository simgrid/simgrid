function Slave(...)

  if #arg ~= 1 then
    error("Wrong number of arguments (got " .. #arg .. ", expected 1: slave_id)")
  end

  local my_mailbox = "slave " .. arg[1]
  simgrid.info("Hello from lua, I'm a poor slave with mailbox: " .. my_mailbox)

  while true do

    local task = simgrid.task.recv(my_mailbox)
    local task_name = task:get_name()
    if (task_name == "finalize") then
      simgrid.info("Got finalize message")
      break
    end
    simgrid.info("Received task '" .. task_name .. "' on mailbox '" .. my_mailbox .. "'")
    task:execute()
    simgrid.info("Task '" .. task_name .. "' is done")
  end

  simgrid.info("I'm done. See you!")
end -- end_of_slave

