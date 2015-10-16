-- Copyright (c) 2011-2012, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

function Slave(...)

  if select("#", ...) ~= 1 then
    error("Wrong number of arguments (got " .. #arg .. ", expected 1: slave_id)")
  end

  local my_mailbox = "slave " .. select(1, ...)
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

